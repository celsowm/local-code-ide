#pragma once

#include "services/interfaces/ICodeIntelProvider.hpp"

namespace ide::adapters::codeintel {

class MockCodeIntelProvider final : public ide::services::interfaces::ICodeIntelProvider {
public:
    std::vector<ide::services::interfaces::CompletionItem> completions(const QString& filePath,
                                                                       const QString& text,
                                                                       const ide::services::interfaces::EditorPosition& position) override;
    ide::services::interfaces::HoverInfo hover(const QString& filePath,
                                               const QString& text,
                                               const ide::services::interfaces::EditorPosition& position) override;
    std::optional<ide::services::interfaces::DefinitionLocation> definition(const QString& filePath,
                                                                            const QString& text,
                                                                            const ide::services::interfaces::EditorPosition& position) override;
    QString name() const override;
};

} // namespace ide::adapters::codeintel
