#include "adapters/diagnostics/LspFeatureProvider.hpp"
#include "adapters/diagnostics/LspDocumentManager.hpp"
#include "adapters/diagnostics/LspTransport.hpp"

#include <QEventLoop>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTimer>
#include <QUrl>

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

LspFeatureProvider::LspFeatureProvider(LspTransport& transport,
                                       LspDocumentManager& documentManager,
                                       QObject* parent)
    : QObject(parent)
    , m_transport(transport)
    , m_documentManager(documentManager) {
}

std::vector<CompletionItem> LspFeatureProvider::requestCompletions(
    const QString& filePath,
    const QString& text,
    const EditorPosition& position,
    int waitMs) {
    const int requestId = sendRequest("textDocument/completion",
                                      makeTextDocumentPositionParams(filePath, text, position));
    return parseCompletionItems(waitForResponse(requestId, waitMs).value("result"));
}

HoverInfo LspFeatureProvider::requestHover(
    const QString& filePath,
    const QString& text,
    const EditorPosition& position,
    int waitMs) {
    const int requestId = sendRequest("textDocument/hover",
                                      makeTextDocumentPositionParams(filePath, text, position));
    return parseHover(waitForResponse(requestId, waitMs).value("result"));
}

std::optional<DefinitionLocation> LspFeatureProvider::requestDefinition(
    const QString& filePath,
    const QString& text,
    const EditorPosition& position,
    int waitMs) {
    const int requestId = sendRequest("textDocument/definition",
                                      makeTextDocumentPositionParams(filePath, text, position));
    return parseDefinition(waitForResponse(requestId, waitMs).value("result"));
}

int LspFeatureProvider::requestCompletionsAsync(
    const QString& filePath,
    const QString& text,
    const EditorPosition& position) {
    const int requestId = sendRequest("textDocument/completion",
                                      makeTextDocumentPositionParams(filePath, text, position));
    m_pendingCompletionByRequestId.insert(requestId, QFileInfo(filePath).absoluteFilePath());
    return requestId;
}

int LspFeatureProvider::requestHoverAsync(
    const QString& filePath,
    const QString& text,
    const EditorPosition& position) {
    const int requestId = sendRequest("textDocument/hover",
                                      makeTextDocumentPositionParams(filePath, text, position));
    m_pendingHoverByRequestId.insert(requestId, QFileInfo(filePath).absoluteFilePath());
    return requestId;
}

int LspFeatureProvider::requestDefinitionAsync(
    const QString& filePath,
    const QString& text,
    const EditorPosition& position) {
    const int requestId = sendRequest("textDocument/definition",
                                      makeTextDocumentPositionParams(filePath, text, position));
    m_pendingDefinitionByRequestId.insert(requestId, QFileInfo(filePath).absoluteFilePath());
    return requestId;
}

void LspFeatureProvider::handleResponse(int requestId, const QJsonObject& response) {
    if (m_pendingCompletionByRequestId.contains(requestId)) {
        const QString filePath = m_pendingCompletionByRequestId.take(requestId);
        emit completionsReady(requestId, filePath, parseCompletionItems(response.value("result")));
        return;
    }
    if (m_pendingHoverByRequestId.contains(requestId)) {
        const QString filePath = m_pendingHoverByRequestId.take(requestId);
        emit hoverReady(requestId, filePath, parseHover(response.value("result")));
        return;
    }
    if (m_pendingDefinitionByRequestId.contains(requestId)) {
        const QString filePath = m_pendingDefinitionByRequestId.take(requestId);
        const auto def = parseDefinition(response.value("result"));
        if (def.has_value()) {
            emit definitionReady(requestId, filePath, true, def.value());
        } else {
            emit definitionReady(requestId, filePath, false, DefinitionLocation{});
        }
        return;
    }
    // Store for sync callers
    m_pendingResponses.insert(requestId, response);
}

int LspFeatureProvider::sendRequest(const QString& method, const QJsonObject& params) {
    const int id = m_nextId++;
    m_transport.send(QJsonObject{
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", method},
        {"params", params}
    });
    return id;
}

QJsonObject LspFeatureProvider::waitForResponse(int requestId, int waitMs) {
    if (m_pendingResponses.contains(requestId)) {
        return m_pendingResponses.take(requestId);
    }

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);

    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(this, &LspFeatureProvider::completionsReady, &loop,
                    [&loop, conn]() {
        QObject::disconnect(*conn);
        if (loop.isRunning()) {
            loop.quit();
        }
    });

    QTimer pollTimer;
    pollTimer.setInterval(10);
    connect(&pollTimer, &QTimer::timeout, &loop,
            [this, requestId, &loop, conn, &pollTimer]() {
        if (m_pendingResponses.contains(requestId)) {
            QObject::disconnect(*conn);
            pollTimer.stop();
            if (loop.isRunning()) {
                loop.quit();
            }
        }
    });

    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    pollTimer.start();
    timeout.start(waitMs);
    loop.exec();
    QObject::disconnect(*conn);
    pollTimer.stop();

    return m_pendingResponses.take(requestId);
}

QJsonObject LspFeatureProvider::makeTextDocumentPositionParams(
    const QString& filePath,
    const QString& text,
    const EditorPosition& position) {
    const QString uri = QUrl::fromLocalFile(QFileInfo(filePath).absoluteFilePath()).toString();
    m_documentManager.publish(filePath, text);
    return QJsonObject{
        {"textDocument", QJsonObject{{"uri", uri}}},
        {"position", QJsonObject{
            {"line", qMax(0, position.line - 1)},
            {"character", qMax(0, position.column - 1)}
        }}
    };
}

std::vector<CompletionItem> LspFeatureProvider::parseCompletionItems(const QJsonValue& value) {
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

HoverInfo LspFeatureProvider::parseHover(const QJsonValue& value) {
    HoverInfo info;
    if (!value.isObject()) {
        return info;
    }
    info.contents = markdownOrPlain(value.toObject().value("contents"));
    return info;
}

std::optional<DefinitionLocation> LspFeatureProvider::parseDefinition(const QJsonValue& value) {
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
