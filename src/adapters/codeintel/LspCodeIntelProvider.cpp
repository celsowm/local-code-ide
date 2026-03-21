#include "adapters/codeintel/LspCodeIntelProvider.hpp"

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

QString LspCodeIntelProvider::name() const {
    return "LSP Code Intel Provider";
}

} // namespace ide::adapters::codeintel
