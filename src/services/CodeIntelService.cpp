#include "services/CodeIntelService.hpp"

namespace ide::services {

CodeIntelService::CodeIntelService(std::unique_ptr<interfaces::ICodeIntelProvider> provider)
    : m_provider(std::move(provider)) {}

std::vector<interfaces::CompletionItem> CodeIntelService::completions(const QString& filePath,
                                                                      const QString& text,
                                                                      const interfaces::EditorPosition& position) {
    return m_provider ? m_provider->completions(filePath, text, position) : std::vector<interfaces::CompletionItem>{};
}

interfaces::HoverInfo CodeIntelService::hover(const QString& filePath,
                                              const QString& text,
                                              const interfaces::EditorPosition& position) {
    return m_provider ? m_provider->hover(filePath, text, position) : interfaces::HoverInfo{};
}

std::optional<interfaces::DefinitionLocation> CodeIntelService::definition(const QString& filePath,
                                                                           const QString& text,
                                                                           const interfaces::EditorPosition& position) {
    return m_provider ? m_provider->definition(filePath, text, position) : std::optional<interfaces::DefinitionLocation>{};
}

QString CodeIntelService::providerName() const {
    return m_provider ? m_provider->name() : "No Code Intel Provider";
}

} // namespace ide::services
