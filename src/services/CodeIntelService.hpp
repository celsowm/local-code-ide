#pragma once

#include "services/interfaces/ICodeIntelProvider.hpp"

#include <memory>
#include <optional>
#include <functional>

namespace ide::services {

class CodeIntelService {
public:
    explicit CodeIntelService(std::unique_ptr<interfaces::ICodeIntelProvider> provider);

    std::vector<interfaces::CompletionItem> completions(const QString& filePath,
                                                        const QString& text,
                                                        const interfaces::EditorPosition& position);
    interfaces::HoverInfo hover(const QString& filePath,
                                const QString& text,
                                const interfaces::EditorPosition& position);
    std::optional<interfaces::DefinitionLocation> definition(const QString& filePath,
                                                             const QString& text,
                                                             const interfaces::EditorPosition& position);
    void completionsAsync(const QString& filePath,
                          const QString& text,
                          const interfaces::EditorPosition& position,
                          std::function<void(std::vector<interfaces::CompletionItem>)> onReady);
    void hoverAsync(const QString& filePath,
                    const QString& text,
                    const interfaces::EditorPosition& position,
                    std::function<void(interfaces::HoverInfo)> onReady);
    void definitionAsync(const QString& filePath,
                         const QString& text,
                         const interfaces::EditorPosition& position,
                         std::function<void(std::optional<interfaces::DefinitionLocation>)> onReady);
    QString providerName() const;

private:
    std::unique_ptr<interfaces::ICodeIntelProvider> m_provider;
};

} // namespace ide::services
