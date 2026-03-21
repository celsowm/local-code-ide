#pragma once

#include "services/interfaces/ICodeIntelProvider.hpp"

#include <memory>
#include <optional>

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
    QString providerName() const;

private:
    std::unique_ptr<interfaces::ICodeIntelProvider> m_provider;
};

} // namespace ide::services
