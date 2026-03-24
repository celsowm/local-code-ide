#include "adapters/diagnostics/LspDocumentManager.hpp"
#include "adapters/diagnostics/LspTransport.hpp"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrl>

namespace ide::adapters::diagnostics {

LspDocumentManager::LspDocumentManager(LspTransport& transport,
                                       std::function<QString(const QString&)> languageIdResolver,
                                       QObject* parent)
    : QObject(parent)
    , m_transport(transport)
    , m_languageIdResolver(std::move(languageIdResolver)) {
}

int LspDocumentManager::publish(const QString& filePath, const QString& text) {
    const QString uri = uriForPath(filePath);
    const int nextVersion = m_versionsByUri.value(uri, 0) + 1;
    m_versionsByUri.insert(uri, nextVersion);

    if (m_openUris.contains(uri)) {
        sendDidChange(filePath, text, nextVersion);
    } else {
        // Queue for when protocol becomes ready
        m_pendingOpensByUri.insert(uri, PendingOpen{filePath, text});
        sendDidOpen(filePath, text, nextVersion);
    }

    return nextVersion;
}

bool LspDocumentManager::isOpen(const QString& filePath) const {
    return m_openUris.contains(uriForPath(filePath));
}

int LspDocumentManager::version(const QString& filePath) const {
    return m_versionsByUri.value(uriForPath(filePath), 0);
}

void LspDocumentManager::flushPending() {
    for (auto it = m_pendingOpensByUri.begin(); it != m_pendingOpensByUri.end(); ++it) {
        const PendingOpen& pending = it.value();
        const int ver = m_versionsByUri.value(it.key(), 1);
        sendDidOpen(pending.filePath, pending.text, ver);
    }
    m_pendingOpensByUri.clear();
}

void LspDocumentManager::reset() {
    m_openUris.clear();
    m_versionsByUri.clear();
    m_pendingOpensByUri.clear();
}

void LspDocumentManager::sendDidOpen(const QString& filePath, const QString& text, int version) {
    const QString uri = uriForPath(filePath);
    if (m_openUris.contains(uri)) {
        return;
    }

    m_openUris.insert(uri);
    m_transport.send(QJsonObject{
        {"jsonrpc", "2.0"},
        {"method", "textDocument/didOpen"},
        {"params", QJsonObject{
            {"textDocument", QJsonObject{
                {"uri", uri},
                {"languageId", languageIdForPath(filePath)},
                {"version", version},
                {"text", text}
            }}
        }}
    });
    emit documentOpened(filePath);
}

void LspDocumentManager::sendDidChange(const QString& filePath, const QString& text, int version) {
    const QString uri = uriForPath(filePath);
    m_transport.send(QJsonObject{
        {"jsonrpc", "2.0"},
        {"method", "textDocument/didChange"},
        {"params", QJsonObject{
            {"textDocument", QJsonObject{{"uri", uri}, {"version", version}}},
            {"contentChanges", QJsonArray{QJsonObject{{"text", text}}}}
        }}
    });
}

QString LspDocumentManager::uriForPath(const QString& filePath) const {
    const QString normalized = filePath.startsWith("untitled")
        ? QDir::temp().filePath(filePath)
        : QFileInfo(filePath).absoluteFilePath();
    return QUrl::fromLocalFile(normalized).toString();
}

QString LspDocumentManager::languageIdForPath(const QString& filePath) const {
    if (m_languageIdResolver) {
        return m_languageIdResolver(filePath);
    }
    const QString lower = filePath.toLower();
    if (lower.endsWith(".json") || lower.endsWith(".jsonc") || lower.endsWith(".json5")) {
        return QStringLiteral("json");
    }
    if (lower.endsWith(".yaml") || lower.endsWith(".yml")) {
        return QStringLiteral("yaml");
    }
    if (lower.endsWith(".toml")) {
        return QStringLiteral("toml");
    }
    if (lower.endsWith(".md") || lower.endsWith(".markdown") || lower.endsWith(".rst")) {
        return QStringLiteral("markdown");
    }
    if (lower.endsWith(".ps1") || lower.endsWith(".psm1") || lower.endsWith(".psd1")) {
        return QStringLiteral("powershell");
    }
    if (lower.endsWith(".rs") || lower.endsWith(".ron")) {
        return QStringLiteral("rust");
    }
    if (lower.endsWith(".py")) {
        return QStringLiteral("python");
    }
    if (lower.endsWith(".js") || lower.endsWith(".ts") || lower.endsWith(".tsx") || lower.endsWith(".jsx")
        || lower.endsWith(".mjs") || lower.endsWith(".cjs") || lower.endsWith(".cts") || lower.endsWith(".mts")) {
        return QStringLiteral("typescript");
    }
    if (lower.endsWith(".qml")) {
        return QStringLiteral("qml");
    }
    return QStringLiteral("cpp");
}

} // namespace ide::adapters::diagnostics
