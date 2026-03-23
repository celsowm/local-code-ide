#include "adapters/codeintel/LspCodeIntelProvider.hpp"

#include <memory>

namespace ide::adapters::codeintel {
using namespace ide::services::interfaces;

LspCodeIntelProvider::LspCodeIntelProvider(std::shared_ptr<ide::adapters::diagnostics::LspClient> client)
    : m_client(std::move(client)) {}

std::vector<CompletionItem> LspCodeIntelProvider::completions(const QString& filePath,
                                                              const QString& text,
                                                              const EditorPosition& position) {
    return m_client ? m_client->requestCompletions(filePath, text, position) : std::vector<CompletionItem>{};
}

HoverInfo LspCodeIntelProvider::hover(const QString& filePath,
                                      const QString& text,
                                      const EditorPosition& position) {
    return m_client ? m_client->requestHover(filePath, text, position) : HoverInfo{};
}

std::optional<DefinitionLocation> LspCodeIntelProvider::definition(const QString& filePath,
                                                                   const QString& text,
                                                                   const EditorPosition& position) {
    return m_client ? m_client->requestDefinition(filePath, text, position) : std::optional<DefinitionLocation>{};
}

void LspCodeIntelProvider::completionsAsync(const QString& filePath,
                                            const QString& text,
                                            const EditorPosition& position,
                                            std::function<void(std::vector<CompletionItem>)> onReady) {
    if (!onReady) {
        return;
    }
    if (!m_client) {
        onReady({});
        return;
    }

    const int requestId = m_client->requestCompletionsAsync(filePath, text, position);
    if (requestId < 0) {
        onReady({});
        return;
    }
    auto connection = std::make_shared<QMetaObject::Connection>();
    *connection = QObject::connect(
        m_client.get(),
        &ide::adapters::diagnostics::LspClient::completionsReady,
        m_client.get(),
        [requestId, connection, onReady = std::move(onReady)](int completedRequestId,
                                                              const QString&,
                                                              const std::vector<CompletionItem>& items) mutable {
        if (completedRequestId != requestId) {
            return;
        }
        QObject::disconnect(*connection);
        onReady(items);
    });
}

void LspCodeIntelProvider::hoverAsync(const QString& filePath,
                                      const QString& text,
                                      const EditorPosition& position,
                                      std::function<void(HoverInfo)> onReady) {
    if (!onReady) {
        return;
    }
    if (!m_client) {
        onReady({});
        return;
    }

    const int requestId = m_client->requestHoverAsync(filePath, text, position);
    if (requestId < 0) {
        onReady({});
        return;
    }
    auto connection = std::make_shared<QMetaObject::Connection>();
    *connection = QObject::connect(
        m_client.get(),
        &ide::adapters::diagnostics::LspClient::hoverReady,
        m_client.get(),
        [requestId, connection, onReady = std::move(onReady)](int completedRequestId,
                                                              const QString&,
                                                              const HoverInfo& info) mutable {
        if (completedRequestId != requestId) {
            return;
        }
        QObject::disconnect(*connection);
        onReady(info);
    });
}

void LspCodeIntelProvider::definitionAsync(const QString& filePath,
                                           const QString& text,
                                           const EditorPosition& position,
                                           std::function<void(std::optional<DefinitionLocation>)> onReady) {
    if (!onReady) {
        return;
    }
    if (!m_client) {
        onReady(std::nullopt);
        return;
    }

    const int requestId = m_client->requestDefinitionAsync(filePath, text, position);
    if (requestId < 0) {
        onReady(std::nullopt);
        return;
    }
    auto connection = std::make_shared<QMetaObject::Connection>();
    *connection = QObject::connect(
        m_client.get(),
        &ide::adapters::diagnostics::LspClient::definitionReady,
        m_client.get(),
        [requestId, connection, onReady = std::move(onReady)](int completedRequestId,
                                                              const QString&,
                                                              bool found,
                                                              const DefinitionLocation& location) mutable {
        if (completedRequestId != requestId) {
            return;
        }
        QObject::disconnect(*connection);
        onReady(found ? std::optional<DefinitionLocation>{location} : std::nullopt);
    });
}

QString LspCodeIntelProvider::name() const {
    return "LSP Code Intel Provider";
}

} // namespace ide::adapters::codeintel
