#include "adapters/diagnostics/LspClient.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileInfo>
#include <QJsonDocument>
#include <QThread>
#include <QUrl>

#include <cstring>

namespace ide::adapters::diagnostics {
using namespace ide::services::interfaces;

namespace {
QString locationUriToPath(const QString& uri) {
    return QUrl(uri).toLocalFile();
}

QString markdownOrPlain(const QJsonValue& value) {
    if (value.isString()) {
        return value.toString();
    }

    if (value.isObject()) {
        const QJsonObject obj = value.toObject();
        const QString valueText = obj.value("value").toString();
        if (!valueText.isEmpty()) {
            return valueText;
        }
        return obj.value("language").toString();
    }

    if (value.isArray()) {
        QStringList parts;
        for (const auto& item : value.toArray()) {
            const QString chunk = markdownOrPlain(item);
            if (!chunk.isEmpty()) {
                parts.push_back(chunk);
            }
        }
        return parts.join("\n");
    }

    return {};
}
} // namespace

LspClient::LspClient(QString program, QStringList args, QObject* parent)
    : QObject(parent)
    , m_program(std::move(program))
    , m_args(std::move(args)) {
    QObject::connect(&m_process, &QProcess::readyReadStandardOutput, this, &LspClient::onReadyReadStandardOutput);
    QObject::connect(&m_process, &QProcess::readyReadStandardError, this, &LspClient::onReadyReadStandardError);
    QObject::connect(&m_process, &QProcess::started, this, [this]() {
        sendInitialize();
        emit serverStatusChanged(statusLine());
    });
    QObject::connect(&m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, [this](int, QProcess::ExitStatus) {
        m_initialized = false;
        m_openDocuments.clear();
        m_pendingDocumentOpensByUri.clear();
        emit serverStatusChanged(statusLine());
    });
}

bool LspClient::ensureStarted() {
    if (m_program.isEmpty()) {
        return false;
    }

    if (m_process.state() == QProcess::Running) {
        return true;
    }

    m_process.start(m_program, m_args);
    emit serverStatusChanged(statusLine());
    return (m_process.state() != QProcess::NotRunning);
}

bool LspClient::isRunning() const {
    return m_process.state() == QProcess::Running;
}

QString LspClient::statusLine() const {
    if (m_program.isEmpty()) {
        return "disabled";
    }
    if (m_process.state() != QProcess::Running) {
        return QString("configured: %1 (not started)").arg(m_program);
    }
    if (!m_initialized) {
        return QString("%1 starting...").arg(m_program);
    }
    return QString("%1 running").arg(m_program);
}

int LspClient::publishDocument(const QString& filePath, const QString& text) {
    if (!ensureStarted()) {
        return -1;
    }

    const QString uri = uriForPath(filePath);
    const int nextVersion = m_documentVersions.value(uri, 0) + 1;
    m_documentVersions.insert(uri, nextVersion);

    if (!m_initialized) {
        m_pendingDocumentOpensByUri.insert(uri, PendingDocumentOpen{filePath, text});
        return nextVersion;
    }

    const bool justOpened = ensureDocumentOpened(filePath, text, nextVersion);
    if (justOpened) {
        return nextVersion;
    }

    sendNotification("textDocument/didChange", QJsonObject{
        {"textDocument", QJsonObject{{"uri", uri}, {"version", nextVersion}}},
        {"contentChanges", QJsonArray{QJsonObject{{"text", text}}}}
    });
    return nextVersion;
}

std::vector<Diagnostic> LspClient::latestDiagnostics(const QString& filePath) const {
    return m_diagnosticsByUri.value(uriForPath(filePath));
}

int LspClient::latestDiagnosticsVersion(const QString& filePath) const {
    return m_diagnosticVersionsByUri.value(uriForPath(filePath), -1);
}

std::vector<Diagnostic> LspClient::publishAndCollect(const QString& filePath, const QString& text, int waitMs) {
    publishDocument(filePath, text);

    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < waitMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::msleep(5);
    }

    return latestDiagnostics(filePath);
}

std::vector<CompletionItem> LspClient::requestCompletions(const QString& filePath,
                                                          const QString& text,
                                                          const EditorPosition& position,
                                                          int waitMs) {
    if (!ensureStarted()) {
        return {};
    }
    waitUntilInitialized();
    if (!m_initialized) {
        return {};
    }

    publishDocument(filePath, text);
    const int requestId = sendRequest("textDocument/completion", makeTextDocumentPositionParams(filePath, text, position));
    return parseCompletionItems(waitForResponse(requestId, waitMs).value("result"));
}

HoverInfo LspClient::requestHover(const QString& filePath,
                                  const QString& text,
                                  const EditorPosition& position,
                                  int waitMs) {
    if (!ensureStarted()) {
        return {};
    }
    waitUntilInitialized();
    if (!m_initialized) {
        return {};
    }

    publishDocument(filePath, text);
    const int requestId = sendRequest("textDocument/hover", makeTextDocumentPositionParams(filePath, text, position));
    return parseHover(waitForResponse(requestId, waitMs).value("result"));
}

std::optional<DefinitionLocation> LspClient::requestDefinition(const QString& filePath,
                                                               const QString& text,
                                                               const EditorPosition& position,
                                                               int waitMs) {
    if (!ensureStarted()) {
        return std::nullopt;
    }
    waitUntilInitialized();
    if (!m_initialized) {
        return std::nullopt;
    }

    publishDocument(filePath, text);
    const int requestId = sendRequest("textDocument/definition", makeTextDocumentPositionParams(filePath, text, position));
    return parseDefinition(waitForResponse(requestId, waitMs).value("result"));
}

int LspClient::requestCompletionsAsync(const QString& filePath,
                                       const QString& text,
                                       const EditorPosition& position) {
    if (!ensureStarted()) {
        return -1;
    }
    if (!m_initialized) {
        publishDocument(filePath, text);
        return -1;
    }
    publishDocument(filePath, text);
    const QString normalizedPath = QFileInfo(filePath).absoluteFilePath();
    const int requestId = sendRequest("textDocument/completion", makeTextDocumentPositionParams(filePath, text, position));
    m_pendingCompletionByRequestId.insert(requestId, normalizedPath);
    return requestId;
}

int LspClient::requestHoverAsync(const QString& filePath,
                                 const QString& text,
                                 const EditorPosition& position) {
    if (!ensureStarted()) {
        return -1;
    }
    if (!m_initialized) {
        publishDocument(filePath, text);
        return -1;
    }
    publishDocument(filePath, text);
    const QString normalizedPath = QFileInfo(filePath).absoluteFilePath();
    const int requestId = sendRequest("textDocument/hover", makeTextDocumentPositionParams(filePath, text, position));
    m_pendingHoverByRequestId.insert(requestId, normalizedPath);
    return requestId;
}

int LspClient::requestDefinitionAsync(const QString& filePath,
                                      const QString& text,
                                      const EditorPosition& position) {
    if (!ensureStarted()) {
        return -1;
    }
    if (!m_initialized) {
        publishDocument(filePath, text);
        return -1;
    }
    publishDocument(filePath, text);
    const QString normalizedPath = QFileInfo(filePath).absoluteFilePath();
    const int requestId = sendRequest("textDocument/definition", makeTextDocumentPositionParams(filePath, text, position));
    m_pendingDefinitionByRequestId.insert(requestId, normalizedPath);
    return requestId;
}

void LspClient::onReadyReadStandardOutput() {
    m_stdoutBuffer.append(m_process.readAllStandardOutput());
    parseBufferedMessages();
}

void LspClient::onReadyReadStandardError() {
    m_stderrBuffer.append(m_process.readAllStandardError());
}

int LspClient::sendRequest(const QString& method, const QJsonObject& params) {
    const int id = m_nextId++;
    sendMessage(QJsonObject{
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", method},
        {"params", params}
    });
    return id;
}

void LspClient::sendNotification(const QString& method, const QJsonObject& params) {
    sendMessage(QJsonObject{
        {"jsonrpc", "2.0"},
        {"method", method},
        {"params", params}
    });
}

void LspClient::sendMessage(const QJsonObject& message) {
    const QByteArray payload = QJsonDocument(message).toJson(QJsonDocument::Compact);
    const QByteArray header = "Content-Length: " + QByteArray::number(payload.size()) + "\r\n\r\n";
    m_process.write(header);
    m_process.write(payload);
    m_process.waitForBytesWritten(250);
}

void LspClient::parseBufferedMessages() {
    while (true) {
        const int headerEnd = m_stdoutBuffer.indexOf("\r\n\r\n");
        if (headerEnd < 0) {
            return;
        }

        const QByteArray headers = m_stdoutBuffer.left(headerEnd);
        int contentLength = -1;
        for (const QByteArray& line : headers.split('\n')) {
            const QByteArray trimmed = line.trimmed();
            if (trimmed.startsWith("Content-Length:")) {
                contentLength = trimmed.mid(strlen("Content-Length:")).trimmed().toInt();
            }
        }

        if (contentLength < 0) {
            m_stdoutBuffer.remove(0, headerEnd + 4);
            continue;
        }

        const int totalSize = headerEnd + 4 + contentLength;
        if (m_stdoutBuffer.size() < totalSize) {
            return;
        }

        const QByteArray payload = m_stdoutBuffer.mid(headerEnd + 4, contentLength);
        m_stdoutBuffer.remove(0, totalSize);

        const QJsonDocument json = QJsonDocument::fromJson(payload);
        if (json.isObject()) {
            handleMessage(json.object());
        }
    }
}

void LspClient::handleMessage(const QJsonObject& message) {
    const QString method = message.value("method").toString();
    if (method == "textDocument/publishDiagnostics") {
        const QJsonObject params = message.value("params").toObject();
        const QString uri = params.value("uri").toString();
        const QString filePath = locationUriToPath(uri);
        const int version = params.contains("version") ? params.value("version").toInt(-1) : -1;
        m_diagnosticsByUri.insert(uri, parseDiagnostics(params.value("diagnostics").toArray(), filePath, QFileInfo(m_program).baseName()));
        m_diagnosticVersionsByUri.insert(uri, version);
        emit diagnosticsPublished(filePath, version, QFileInfo(m_program).baseName());
        return;
    }

    if (message.contains("id")) {
        const int id = message.value("id").toInt();
        if (m_pendingCompletionByRequestId.contains(id)) {
            const QString filePath = m_pendingCompletionByRequestId.take(id);
            emit completionsReady(id, filePath, parseCompletionItems(message.value("result")));
            return;
        }
        if (m_pendingHoverByRequestId.contains(id)) {
            const QString filePath = m_pendingHoverByRequestId.take(id);
            emit hoverReady(id, filePath, parseHover(message.value("result")));
            return;
        }
        if (m_pendingDefinitionByRequestId.contains(id)) {
            const QString filePath = m_pendingDefinitionByRequestId.take(id);
            const auto definition = parseDefinition(message.value("result"));
            if (definition.has_value()) {
                emit definitionReady(id, filePath, true, definition.value());
            } else {
                emit definitionReady(id, filePath, false, DefinitionLocation{});
            }
            return;
        }
        m_pendingResponses.insert(id, message);
        if (!m_initialized && !message.contains("error")) {
            m_initialized = true;
            sendNotification("initialized", QJsonObject{});
            flushPendingDocumentOpens();
            emit serverStatusChanged(statusLine());
        }
    }
}

void LspClient::sendInitialize() {
    sendRequest("initialize", QJsonObject{
        {"processId", static_cast<int>(QCoreApplication::applicationPid())},
        {"rootUri", QUrl::fromLocalFile(QDir::currentPath()).toString()},
        {"capabilities", QJsonObject{
            {"textDocument", QJsonObject{
                {"publishDiagnostics", QJsonObject{{"relatedInformation", true}}},
                {"completion", QJsonObject{{"completionItem", QJsonObject{{"snippetSupport", true}}}}},
                {"hover", QJsonObject{}},
                {"definition", QJsonObject{}}
            }}
        }},
        {"clientInfo", QJsonObject{{"name", "LocalCodeIDE"}, {"version", "0.3.0"}}}
    });
}

bool LspClient::ensureDocumentOpened(const QString& filePath, const QString& text, int version) {
    const QString uri = uriForPath(filePath);
    if (m_openDocuments.contains(uri)) {
        return false;
    }

    m_openDocuments.insert(uri);
    sendNotification("textDocument/didOpen", QJsonObject{
        {"textDocument", QJsonObject{
            {"uri", uri},
            {"languageId", languageIdForPath(filePath)},
            {"version", version},
            {"text", text}
        }}
    });
    return true;
}

void LspClient::waitUntilInitialized(int waitMs) {
    QElapsedTimer timer;
    timer.start();
    while (!m_initialized && timer.elapsed() < waitMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::msleep(5);
    }
}

QJsonObject LspClient::waitForResponse(int requestId, int waitMs) {
    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < waitMs) {
        if (m_pendingResponses.contains(requestId)) {
            return m_pendingResponses.take(requestId);
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::msleep(5);
    }

    return QJsonObject{};
}

QJsonObject LspClient::makeTextDocumentPositionParams(const QString& filePath,
                                                      const QString& text,
                                                      const EditorPosition& position) {
    const QString uri = uriForPath(filePath);
    const int version = m_documentVersions.value(uri, 1);
    ensureDocumentOpened(filePath, text, version);
    return QJsonObject{
        {"textDocument", QJsonObject{{"uri", uri}}},
        {"position", QJsonObject{{"line", qMax(0, position.line - 1)}, {"character", qMax(0, position.column - 1)}}}
    };
}

void LspClient::flushPendingDocumentOpens() {
    if (!m_initialized || m_pendingDocumentOpensByUri.isEmpty()) {
        return;
    }

    for (auto it = m_pendingDocumentOpensByUri.begin(); it != m_pendingDocumentOpensByUri.end(); ++it) {
        const QString uri = it.key();
        const PendingDocumentOpen& pending = it.value();
        const int version = m_documentVersions.value(uri, 1);
        ensureDocumentOpened(pending.filePath, pending.text, version);
    }
    m_pendingDocumentOpensByUri.clear();
}

QString LspClient::uriForPath(const QString& filePath) const {
    const QString normalized = filePath.startsWith("untitled")
        ? QDir::temp().filePath(filePath)
        : QFileInfo(filePath).absoluteFilePath();
    return QUrl::fromLocalFile(normalized).toString();
}

QString LspClient::languageIdForPath(const QString& filePath) const {
    const QString lower = filePath.toLower();
    if (lower.endsWith(".json") || lower.endsWith(".jsonc") || lower.endsWith(".json5")) {
        return "json";
    }
    if (lower.endsWith(".yaml") || lower.endsWith(".yml")) {
        return "yaml";
    }
    if (lower.endsWith(".toml")) {
        return "toml";
    }
    if (lower.endsWith(".md") || lower.endsWith(".markdown") || lower.endsWith(".rst")) {
        return "markdown";
    }
    if (lower.endsWith(".ps1") || lower.endsWith(".psm1") || lower.endsWith(".psd1")) {
        return "powershell";
    }
    if (lower.endsWith(".rs") || lower.endsWith(".ron")) {
        return "rust";
    }
    if (lower.endsWith(".py")) {
        return "python";
    }
    if (lower.endsWith(".js") || lower.endsWith(".ts") || lower.endsWith(".tsx") || lower.endsWith(".jsx")
        || lower.endsWith(".mjs") || lower.endsWith(".cjs") || lower.endsWith(".cts") || lower.endsWith(".mts")) {
        return "typescript";
    }
    if (lower.endsWith(".qml")) {
        return "qml";
    }
    return "cpp";
}

std::vector<Diagnostic> LspClient::parseDiagnostics(const QJsonArray& diagnostics, const QString& filePath, const QString& source) {
    std::vector<Diagnostic> result;
    result.reserve(static_cast<std::size_t>(diagnostics.size()));

    for (const QJsonValue& value : diagnostics) {
        const QJsonObject obj = value.toObject();
        const QJsonObject range = obj.value("range").toObject();
        const QJsonObject start = range.value("start").toObject();
        const QJsonObject end = range.value("end").toObject();

        Diagnostic diag;
        diag.filePath = filePath;
        diag.lineStart = start.value("line").toInt() + 1;
        diag.columnStart = start.value("character").toInt() + 1;
        diag.lineEnd = end.value("line").toInt(diag.lineStart - 1) + 1;
        diag.columnEnd = end.value("character").toInt(diag.columnStart) + 1;
        diag.message = obj.value("message").toString();
        diag.source = source.isEmpty() ? QStringLiteral("lsp") : source;
        diag.code = obj.value("code").toString();

        const int severity = obj.value("severity").toInt(3);
        switch (severity) {
        case 1:
            diag.severity = Diagnostic::Severity::Error;
            break;
        case 2:
            diag.severity = Diagnostic::Severity::Warning;
            break;
        default:
            diag.severity = Diagnostic::Severity::Info;
            break;
        }

        result.push_back(std::move(diag));
    }

    return result;
}

std::vector<CompletionItem> LspClient::parseCompletionItems(const QJsonValue& value) {
    QJsonArray itemsArray;

    if (value.isArray()) {
        itemsArray = value.toArray();
    } else if (value.isObject()) {
        itemsArray = value.toObject().value("items").toArray();
    }

    std::vector<CompletionItem> items;
    const int limit = qMin(itemsArray.size(), 40);
    items.reserve(static_cast<std::size_t>(limit));

    for (int i = 0; i < limit; ++i) {
        const QJsonObject item = itemsArray.at(i).toObject();
        CompletionItem completion;
        completion.label = item.value("label").toString();
        completion.detail = item.value("detail").toString();
        completion.insertText = item.value("insertText").toString(completion.label);
        completion.kind = QString::number(item.value("kind").toInt());
        items.push_back(std::move(completion));
    }

    return items;
}

HoverInfo LspClient::parseHover(const QJsonValue& value) {
    HoverInfo info;
    if (!value.isObject()) {
        return info;
    }

    info.contents = markdownOrPlain(value.toObject().value("contents"));
    return info;
}

std::optional<DefinitionLocation> LspClient::parseDefinition(const QJsonValue& value) {
    QJsonObject target;

    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        if (array.isEmpty()) {
            return std::nullopt;
        }
        target = array.first().toObject();
    } else if (value.isObject()) {
        target = value.toObject();
    } else {
        return std::nullopt;
    }

    const QString uri = target.value("uri").toString(target.value("targetUri").toString());
    QJsonObject range = target.value("range").toObject();
    if (range.isEmpty()) {
        range = target.value("targetSelectionRange").toObject();
    }
    const QJsonObject start = range.value("start").toObject();

    if (uri.isEmpty()) {
        return std::nullopt;
    }

    return DefinitionLocation{
        locationUriToPath(uri),
        start.value("line").toInt() + 1,
        start.value("character").toInt() + 1
    };
}

} // namespace ide::adapters::diagnostics
