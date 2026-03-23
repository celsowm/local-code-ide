#pragma once

#include "adapters/diagnostics/LspClient.hpp"
#include "services/interfaces/ICodeIntelProvider.hpp"

#include <memory>

namespace ide::adapters::codeintel {

class LspCodeIntelProvider final : public ide::services::interfaces::ICodeIntelProvider {
public:
    explicit LspCodeIntelProvider(std::shared_ptr<ide::adapters::diagnostics::LspClient> client);

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
    std::shared_ptr<ide::adapters::diagnostics::LspClient> m_client;
};

} // namespace ide::adapters::codeintel
