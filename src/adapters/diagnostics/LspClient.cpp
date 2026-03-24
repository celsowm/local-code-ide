#include "adapters/diagnostics/LspClient.hpp"
#include "adapters/diagnostics/LspDocumentManager.hpp"
#include "adapters/diagnostics/LspFeatureProvider.hpp"
#include "adapters/diagnostics/LspProtocol.hpp"
#include "adapters/diagnostics/LspTransport.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTextStream>
#include <QTimer>
#include <QUrl>

namespace ide::adapters::diagnostics {
using namespace ide::services::interfaces;

namespace {
QString locationUriToPath(const QString& uri) {
    return QUrl(uri).toLocalFile();
}
} // namespace

LspClient::LspClient(const QString& program,
                     const QStringList& args,
                     std::function<QString(const QString&)> languageIdResolver,
                     QObject* parent)
    : QObject(parent) {
    m_protocol = std::make_unique<LspProtocol>(program, args, this);
    m_transport = std::make_unique<LspTransport>(m_protocol->process(), this);
    m_documentManager = std::make_unique<LspDocumentManager>(*m_transport, std::move(languageIdResolver), this);
    m_featureProvider = std::make_unique<LspFeatureProvider>(*m_transport, *m_documentManager, this);

    connect(m_transport.get(), &LspTransport::messageReceived, this, &LspClient::onTransportMessage);
    connect(m_protocol.get(), &LspProtocol::statusChanged, this, &LspClient::serverStatusChanged);
    connect(m_protocol.get(), &LspProtocol::stopped, this, [this]() {
        m_diagnosticsByUri.clear();
        m_diagnosticVersionsByUri.clear();
    });
    connect(m_protocol.get(), &LspProtocol::started, this, [this]() {
        const QJsonObject initMsg{
            {"jsonrpc", "2.0"},
            {"id", m_protocol->nextRequestId()},
            {"method", "initialize"},
            {"params", QJsonObject{
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
                {"clientInfo", QJsonObject{{"name", "LocalCodeIDE"}, {"version", "0.10.1"}}}
            }}
        };
        
        // Debug log
        QFile logFile(QCoreApplication::applicationDirPath() + "/lsp_debug.log");
        logFile.open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream out(&logFile);
        out << "LspClient::sending initialize for:" << m_protocol->program() << "\n";
        logFile.close();
        
        m_transport->send(initMsg);
    });
    connect(m_featureProvider.get(), &LspFeatureProvider::completionsReady,
            this, &LspClient::completionsReady);
    connect(m_featureProvider.get(), &LspFeatureProvider::hoverReady,
            this, &LspClient::hoverReady);
    connect(m_featureProvider.get(), &LspFeatureProvider::definitionReady,
            this, &LspClient::definitionReady);
}

LspClient::~LspClient() {
    stop();
}

bool LspClient::ensureStarted() {
    if (m_protocol->isRunning()) {
        return true;
    }
    m_protocol->start();
    return m_protocol->isRunning();
}

bool LspClient::isRunning() const {
    return m_protocol->isRunning();
}

void LspClient::stop() {
    if (!m_protocol->isRunning()) {
        return;
    }

    // Send LSP shutdown sequence
    m_transport->send(QJsonObject{
        {"jsonrpc", "2.0"},
        {"method", "shutdown"},
        {"params", QJsonObject{}}
    });
    m_transport->send(QJsonObject{
        {"jsonrpc", "2.0"},
        {"method", "exit"},
        {"params", QJsonObject{}}
    });

    m_protocol->stop();
    m_transport->reset();
    m_documentManager->reset();
}

QString LspClient::statusLine() const {
    return m_protocol->statusLine();
}

int LspClient::publishDocument(const QString& filePath, const QString& text) {
    if (!ensureStarted()) {
        return -1;
    }

    if (!m_protocol->isReady()) {
        // Queue document open for after initialization
        return m_documentManager->publish(filePath, text);
    }

    return m_documentManager->publish(filePath, text);
}

std::vector<Diagnostic> LspClient::latestDiagnostics(const QString& filePath) const {
    return m_diagnosticsByUri.value(uriForPath(filePath));
}

int LspClient::latestDiagnosticsVersion(const QString& filePath) const {
    return m_diagnosticVersionsByUri.value(uriForPath(filePath), -1);
}

std::vector<Diagnostic> LspClient::publishAndCollect(const QString& filePath, const QString& text, int waitMs) {
    publishDocument(filePath, text);

    // Use event loop to wait for diagnosticsPublished signal
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);

    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(this, &LspClient::diagnosticsPublished, this, [&](const QString& publishedPath, int, const QString&) {
        if (QFileInfo(publishedPath).absoluteFilePath() == QFileInfo(filePath).absoluteFilePath()) {
            QObject::disconnect(*conn);
            if (loop.isRunning()) {
                loop.quit();
            }
        }
    });
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    timeout.start(waitMs);
    loop.exec();
    QObject::disconnect(*conn);

    return latestDiagnostics(filePath);
}

std::vector<CompletionItem> LspClient::requestCompletions(
    const QString& filePath,
    const QString& text,
    const EditorPosition& position,
    int waitMs) {
    if (!ensureStarted() || !m_protocol->isReady()) {
        return {};
    }
    publishDocument(filePath, text);
    return m_featureProvider->requestCompletions(filePath, text, position, waitMs);
}

HoverInfo LspClient::requestHover(
    const QString& filePath,
    const QString& text,
    const EditorPosition& position,
    int waitMs) {
    if (!ensureStarted() || !m_protocol->isReady()) {
        return {};
    }
    publishDocument(filePath, text);
    return m_featureProvider->requestHover(filePath, text, position, waitMs);
}

std::optional<DefinitionLocation> LspClient::requestDefinition(
    const QString& filePath,
    const QString& text,
    const EditorPosition& position,
    int waitMs) {
    if (!ensureStarted() || !m_protocol->isReady()) {
        return std::nullopt;
    }
    publishDocument(filePath, text);
    return m_featureProvider->requestDefinition(filePath, text, position, waitMs);
}

int LspClient::requestCompletionsAsync(
    const QString& filePath,
    const QString& text,
    const EditorPosition& position) {
    if (!ensureStarted() || !m_protocol->isReady()) {
        return -1;
    }
    publishDocument(filePath, text);
    return m_featureProvider->requestCompletionsAsync(filePath, text, position);
}

int LspClient::requestHoverAsync(
    const QString& filePath,
    const QString& text,
    const EditorPosition& position) {
    if (!ensureStarted() || !m_protocol->isReady()) {
        return -1;
    }
    publishDocument(filePath, text);
    return m_featureProvider->requestHoverAsync(filePath, text, position);
}

int LspClient::requestDefinitionAsync(
    const QString& filePath,
    const QString& text,
    const EditorPosition& position) {
    if (!ensureStarted() || !m_protocol->isReady()) {
        return -1;
    }
    publishDocument(filePath, text);
    return m_featureProvider->requestDefinitionAsync(filePath, text, position);
}

void LspClient::onTransportMessage(const QJsonObject& message) {
    const QString method = message.value("method").toString();

    if (method == "textDocument/publishDiagnostics") {
        handlePublishDiagnostics(message.value("params").toObject());
        return;
    }

    if (message.contains("id")) {
        const int id = message.value("id").toInt();

        // Let feature provider handle its own responses
        m_featureProvider->handleResponse(id, message);

        // Handle initialize response
        if (!m_protocol->isReady() && !message.contains("error")) {
            handleInitializeResponse(message);
        }
    }
}

void LspClient::handleInitializeResponse(const QJsonObject& message) {
    Q_UNUSED(message)
    m_protocol->markReady();
    m_transport->send(QJsonObject{
        {"jsonrpc", "2.0"},
        {"method", "initialized"},
        {"params", QJsonObject{}}
    });
    m_documentManager->flushPending();
    emit serverStatusChanged(statusLine());
}

void LspClient::handlePublishDiagnostics(const QJsonObject& params) {
    const QString uri = params.value("uri").toString();
    const QString filePath = locationUriToPath(uri);
    const int version = params.contains("version") ? params.value("version").toInt(-1) : -1;
    const QString source = QFileInfo(m_protocol->program()).baseName();

    m_diagnosticsByUri.insert(uri, parseDiagnostics(params.value("diagnostics").toArray(), filePath, source));
    m_diagnosticVersionsByUri.insert(uri, version);
    emit diagnosticsPublished(filePath, version, source);
}

QString LspClient::uriForPath(const QString& filePath) const {
    const QString normalized = filePath.startsWith("untitled")
        ? QDir::temp().filePath(filePath)
        : QFileInfo(filePath).absoluteFilePath();
    return QUrl::fromLocalFile(normalized).toString();
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

} // namespace ide::adapters::diagnostics
