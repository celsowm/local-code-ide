#include "services/LanguageServerHub.hpp"

#include <QFileInfo>

namespace ide::services {
using Diagnostic = ide::services::interfaces::Diagnostic;
using CompletionItem = ide::services::interfaces::CompletionItem;
using EditorPosition = ide::services::interfaces::EditorPosition;
using HoverInfo = ide::services::interfaces::HoverInfo;
using DefinitionLocation = ide::services::interfaces::DefinitionLocation;

LanguageServerHub::LanguageServerHub(LanguagePackService* languagePackService,
                                     QString globalOverrideCommand,
                                     QStringList globalOverrideArgs,
                                     bool allowDevInstallerPath,
                                     QObject* parent)
    : QObject(parent)
    , m_languagePackService(languagePackService)
    , m_globalOverrideCommand(std::move(globalOverrideCommand))
    , m_globalOverrideArgs(std::move(globalOverrideArgs))
    , m_allowDevInstallerPath(allowDevInstallerPath) {
    if (m_languagePackService) {
        connect(m_languagePackService, &LanguagePackService::languageToastRequested, this, &LanguageServerHub::toastRequested);
    }
}

int LanguageServerHub::publishDocument(const QString& filePath, const QString& text) {
    const QString path = normalizedPath(filePath);
    const QString languageId = languageIdForPath(path);

    auto spec = resolveServerSpec(languageId);
    if (!spec.has_value() || !spec->ready) {
        DocumentState state;
        state.filePath = path;
        state.languageId = languageId;
        state.expectedVersion = -1;
        RuntimeStatus runtime;
        runtime.lspReady = false;
        runtime.languageId = languageId;
        runtime.providerLabel = QStringLiteral("Basic Mode");
        runtime.statusLine = spec.has_value() ? spec->statusLine : QStringLiteral("%1: basic mode").arg(languageId);
        state.runtime = runtime;
        m_documentStatesByPath.insert(path, state);
        m_diagnosticsByPath.insert(path, {});
        setRuntimeStatus(path, runtime);
        emit diagnosticsReady(path, {}, -1, runtime.providerLabel);
        return -1;
    }

    ActiveClient active = ensureClient(*spec);
    const int version = active.client->publishDocument(path, text);
    if (version < 0) {
        DocumentState state;
        state.filePath = path;
        state.languageId = languageId;
        state.expectedVersion = -1;
        RuntimeStatus runtime;
        runtime.lspReady = false;
        runtime.languageId = languageId;
        runtime.providerLabel = QStringLiteral("Basic Mode");
        runtime.statusLine = QStringLiteral("%1: language server launch failed; basic mode active").arg(languageId);
        state.runtime = runtime;
        m_documentStatesByPath.insert(path, state);
        m_diagnosticsByPath.insert(path, {});
        setRuntimeStatus(path, runtime);
        emit diagnosticsReady(path, {}, -1, runtime.providerLabel);
        return -1;
    }

    DocumentState state;
    state.filePath = path;
    state.languageId = languageId;
    state.clientKey = active.key;
    state.expectedVersion = version;
    RuntimeStatus runtime;
    const QString clientStatus = active.client->statusLine();
    runtime.lspReady = clientStatus.contains(QStringLiteral(" running"));
    runtime.languageId = languageId;
    runtime.providerLabel = active.providerLabel;
    runtime.statusLine = QStringLiteral("%1: %2").arg(languageId, clientStatus);
    state.runtime = runtime;
    m_documentStatesByPath.insert(path, state);
    setRuntimeStatus(path, runtime);
    m_diagnosticsByPath.insert(path, active.client->latestDiagnostics(path));
    emit diagnosticsReady(path, m_diagnosticsByPath.value(path), version, runtime.providerLabel);
    return version;
}

std::vector<Diagnostic> LanguageServerHub::latestDiagnostics(const QString& filePath) const {
    return m_diagnosticsByPath.value(normalizedPath(filePath));
}

std::vector<CompletionItem> LanguageServerHub::completions(const QString& filePath, const QString& text, const EditorPosition& position) {
    const int publishedVersion = publishDocument(filePath, text);
    const DocumentState* state = documentStateFor(filePath);
    if (!state || !state->runtime.lspReady || publishedVersion < 0 || !m_clientsByKey.contains(state->clientKey)) {
        return {};
    }
    return m_clientsByKey.value(state->clientKey)->requestCompletions(normalizedPath(filePath), text, position);
}

HoverInfo LanguageServerHub::hover(const QString& filePath, const QString& text, const EditorPosition& position) {
    const int publishedVersion = publishDocument(filePath, text);
    const DocumentState* state = documentStateFor(filePath);
    if (!state || !state->runtime.lspReady || publishedVersion < 0 || !m_clientsByKey.contains(state->clientKey)) {
        return {};
    }
    return m_clientsByKey.value(state->clientKey)->requestHover(normalizedPath(filePath), text, position);
}

std::optional<DefinitionLocation> LanguageServerHub::definition(const QString& filePath, const QString& text, const EditorPosition& position) {
    const int publishedVersion = publishDocument(filePath, text);
    const DocumentState* state = documentStateFor(filePath);
    if (!state || !state->runtime.lspReady || publishedVersion < 0 || !m_clientsByKey.contains(state->clientKey)) {
        return std::nullopt;
    }
    return m_clientsByKey.value(state->clientKey)->requestDefinition(normalizedPath(filePath), text, position);
}

void LanguageServerHub::completionsAsync(const QString& filePath,
                                         const QString& text,
                                         const EditorPosition& position,
                                         std::function<void(std::vector<CompletionItem>)> onReady) {
    const int publishedVersion = publishDocument(filePath, text);
    const DocumentState* state = documentStateFor(filePath);
    if (!state || !state->runtime.lspReady || publishedVersion < 0 || !m_clientsByKey.contains(state->clientKey)) {
        if (onReady) {
            onReady({});
        }
        return;
    }
    const int requestId = m_clientsByKey.value(state->clientKey)->requestCompletionsAsync(normalizedPath(filePath), text, position);
    if (requestId < 0) {
        if (onReady) {
            onReady({});
        }
        return;
    }
    m_pendingCompletionCallbacks.insert(pendingRequestKey(state->clientKey, requestId), std::move(onReady));
}

void LanguageServerHub::hoverAsync(const QString& filePath,
                                   const QString& text,
                                   const EditorPosition& position,
                                   std::function<void(HoverInfo)> onReady) {
    const int publishedVersion = publishDocument(filePath, text);
    const DocumentState* state = documentStateFor(filePath);
    if (!state || !state->runtime.lspReady || publishedVersion < 0 || !m_clientsByKey.contains(state->clientKey)) {
        if (onReady) {
            onReady({});
        }
        return;
    }
    const int requestId = m_clientsByKey.value(state->clientKey)->requestHoverAsync(normalizedPath(filePath), text, position);
    if (requestId < 0) {
        if (onReady) {
            onReady({});
        }
        return;
    }
    m_pendingHoverCallbacks.insert(pendingRequestKey(state->clientKey, requestId), std::move(onReady));
}

void LanguageServerHub::definitionAsync(const QString& filePath,
                                        const QString& text,
                                        const EditorPosition& position,
                                        std::function<void(std::optional<DefinitionLocation>)> onReady) {
    const int publishedVersion = publishDocument(filePath, text);
    const DocumentState* state = documentStateFor(filePath);
    if (!state || !state->runtime.lspReady || publishedVersion < 0 || !m_clientsByKey.contains(state->clientKey)) {
        if (onReady) {
            onReady(std::nullopt);
        }
        return;
    }
    const int requestId = m_clientsByKey.value(state->clientKey)->requestDefinitionAsync(normalizedPath(filePath), text, position);
    if (requestId < 0) {
        if (onReady) {
            onReady(std::nullopt);
        }
        return;
    }
    m_pendingDefinitionCallbacks.insert(pendingRequestKey(state->clientKey, requestId), std::move(onReady));
}

LanguageServerHub::RuntimeStatus LanguageServerHub::runtimeStatus(const QString& filePathOrLanguageId) const {
    const QString maybePath = normalizedPath(filePathOrLanguageId);
    if (m_documentStatesByPath.contains(maybePath)) {
        return m_documentStatesByPath.value(maybePath).runtime;
    }
    return m_statusByLanguage.value(filePathOrLanguageId.toLower(), RuntimeStatus{
        false,
        filePathOrLanguageId.toLower(),
        QStringLiteral("Basic Mode"),
        QStringLiteral("%1: basic mode").arg(filePathOrLanguageId.toLower())
    });
}

QString LanguageServerHub::languageIdForPath(const QString& filePath) const {
    if (!m_languagePackService) {
        const QString lower = filePath.toLower();
        if (lower.endsWith(".rs")) return QStringLiteral("rust");
        if (lower.endsWith(".py")) return QStringLiteral("python");
        if (lower.endsWith(".js") || lower.endsWith(".ts") || lower.endsWith(".tsx") || lower.endsWith(".jsx")) return QStringLiteral("typescript");
        return QStringLiteral("cpp");
    }
    return m_languagePackService->languageForFilePath(filePath);
}

QString LanguageServerHub::normalizedPath(const QString& filePath) const {
    return QFileInfo(filePath).absoluteFilePath();
}

std::optional<ide::services::LanguagePackService::ServerSpec> LanguageServerHub::resolveServerSpec(const QString& languageId) {
    if (!m_globalOverrideCommand.trimmed().isEmpty()) {
        ide::services::LanguagePackService::ServerSpec spec;
        spec.ready = true;
        spec.command = m_globalOverrideCommand;
        spec.args = m_globalOverrideArgs;
        spec.source = QFileInfo(m_globalOverrideCommand).fileName();
        spec.statusLine = QStringLiteral("%1: using developer override language server").arg(languageId);
        return spec;
    }
    if (!m_languagePackService) {
        return std::nullopt;
    }
    return m_languagePackService->ensureServerReady(languageId, m_allowDevInstallerPath);
}

LanguageServerHub::ActiveClient LanguageServerHub::ensureClient(const ide::services::LanguagePackService::ServerSpec& spec) {
    const QString key = QStringLiteral("%1|%2").arg(spec.command, spec.args.join(';'));
    if (m_clientsByKey.contains(key)) {
        return {m_clientsByKey.value(key), key, spec.source.isEmpty() ? QFileInfo(spec.command).fileName() : spec.source};
    }

    auto client = std::make_shared<ide::adapters::diagnostics::LspClient>(spec.command, spec.args,
        [this](const QString& filePath) -> QString {
            return languageIdForPath(filePath);
        });
    connect(client.get(), &ide::adapters::diagnostics::LspClient::diagnosticsPublished, this,
            [this, weakClient = std::weak_ptr<ide::adapters::diagnostics::LspClient>(client)](
                const QString& filePath, int version, const QString& source) {
        auto locked = weakClient.lock();
        if (!locked) {
            return;
        }

        const QString path = normalizedPath(filePath);
        if (!m_documentStatesByPath.contains(path)) {
            return;
        }
        const auto state = m_documentStatesByPath.value(path);
        if (version >= 0 && state.expectedVersion >= 0 && version < state.expectedVersion) {
            return;
        }
        const auto diagnostics = locked->latestDiagnostics(path);
        m_diagnosticsByPath.insert(path, diagnostics);
        const QString provider = source.isEmpty() ? state.runtime.providerLabel : source;
        emit diagnosticsReady(path, diagnostics, version, provider);
    });
    connect(client.get(), &ide::adapters::diagnostics::LspClient::serverStatusChanged, this,
            [this, key](const QString& status) {
        if (status.trimmed().isEmpty()) {
            return;
        }
        for (auto it = m_documentStatesByPath.begin(); it != m_documentStatesByPath.end(); ++it) {
            if (it->clientKey != key) {
                continue;
            }
            RuntimeStatus runtime = it->runtime;
            runtime.statusLine = status;
            it->runtime = runtime;
            setRuntimeStatus(it.key(), runtime);
        }
    });
    connect(client.get(), &ide::adapters::diagnostics::LspClient::completionsReady, this,
            [this, key](int requestId, const QString&, const std::vector<CompletionItem>& items) {
        const QString callbackKey = pendingRequestKey(key, requestId);
        if (!m_pendingCompletionCallbacks.contains(callbackKey)) {
            return;
        }
        auto callback = m_pendingCompletionCallbacks.take(callbackKey);
        if (callback) {
            callback(items);
        }
    });
    connect(client.get(), &ide::adapters::diagnostics::LspClient::hoverReady, this,
            [this, key](int requestId, const QString&, const HoverInfo& info) {
        const QString callbackKey = pendingRequestKey(key, requestId);
        if (!m_pendingHoverCallbacks.contains(callbackKey)) {
            return;
        }
        auto callback = m_pendingHoverCallbacks.take(callbackKey);
        if (callback) {
            callback(info);
        }
    });
    connect(client.get(), &ide::adapters::diagnostics::LspClient::definitionReady, this,
            [this, key](int requestId, const QString&, bool found, const DefinitionLocation& location) {
        const QString callbackKey = pendingRequestKey(key, requestId);
        if (!m_pendingDefinitionCallbacks.contains(callbackKey)) {
            return;
        }
        auto callback = m_pendingDefinitionCallbacks.take(callbackKey);
        if (callback) {
            callback(found ? std::optional<DefinitionLocation>{location} : std::nullopt);
        }
    });

    m_clientsByKey.insert(key, client);
    return {client, key, spec.source.isEmpty() ? QFileInfo(spec.command).fileName() : spec.source};
}

void LanguageServerHub::setRuntimeStatus(const QString& filePath, const RuntimeStatus& status) {
    const QString path = normalizedPath(filePath);
    if (m_documentStatesByPath.contains(path)) {
        auto state = m_documentStatesByPath.value(path);
        state.runtime = status;
        m_documentStatesByPath.insert(path, state);
    }
    if (!status.languageId.isEmpty()) {
        m_statusByLanguage.insert(status.languageId.toLower(), status);
    }
    emit runtimeStatusChanged(path, status.statusLine, status.providerLabel);
}

const LanguageServerHub::DocumentState* LanguageServerHub::documentStateFor(const QString& filePath) const {
    const QString path = normalizedPath(filePath);
    auto it = m_documentStatesByPath.find(path);
    return it == m_documentStatesByPath.end() ? nullptr : &it.value();
}

QString LanguageServerHub::pendingRequestKey(const QString& clientKey, int requestId) const {
    return QStringLiteral("%1|%2").arg(clientKey, QString::number(requestId));
}

void LanguageServerHub::shutdown() {
    for (auto it = m_clientsByKey.begin(); it != m_clientsByKey.end(); ++it) {
        it.value()->stop();
    }
    m_clientsByKey.clear();
    m_pendingCompletionCallbacks.clear();
    m_pendingHoverCallbacks.clear();
    m_pendingDefinitionCallbacks.clear();
}

} // namespace ide::services
