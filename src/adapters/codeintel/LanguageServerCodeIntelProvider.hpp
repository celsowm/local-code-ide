#pragma once

#include "services/LanguageServerHub.hpp"
#include "services/interfaces/ICodeIntelProvider.hpp"

namespace ide::adapters::codeintel {

class LanguageServerCodeIntelProvider final : public ide::services::interfaces::ICodeIntelProvider {
public:
    explicit LanguageServerCodeIntelProvider(ide::services::LanguageServerHub* hub);

    std::vector<ide::services::interfaces::CompletionItem> completions(const QString& filePath,
                                                                       const QString& text,
                                                                       const ide::services::interfaces::EditorPosition& position) override;
    ide::services::interfaces::HoverInfo hover(const QString& filePath,
                                               const QString& text,
                                               const ide::services::interfaces::EditorPosition& position) override;
    std::optional<ide::services::interfaces::DefinitionLocation> definition(const QString& filePath,
                                                                            const QString& text,
                                                                            const ide::services::interfaces::EditorPosition& position) override;
    void completionsAsync(const QString& filePath,
                          const QString& text,
                          const ide::services::interfaces::EditorPosition& position,
                          std::function<void(std::vector<ide::services::interfaces::CompletionItem>)> onReady) override;
    void hoverAsync(const QString& filePath,
                    const QString& text,
                    const ide::services::interfaces::EditorPosition& position,
                    std::function<void(ide::services::interfaces::HoverInfo)> onReady) override;
    void definitionAsync(const QString& filePath,
                         const QString& text,
                         const ide::services::interfaces::EditorPosition& position,
                         std::function<void(std::optional<ide::services::interfaces::DefinitionLocation>)> onReady) override;
    QString name() const override;

private:
    std::vector<ide::services::interfaces::CompletionItem> basicCompletions(const QString& text) const;

    ide::services::LanguageServerHub* m_hub = nullptr;
};

} // namespace ide::adapters::codeintel
