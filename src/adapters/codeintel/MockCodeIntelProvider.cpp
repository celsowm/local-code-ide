#include "adapters/codeintel/MockCodeIntelProvider.hpp"

namespace ide::adapters::codeintel {
using namespace ide::services::interfaces;

std::vector<CompletionItem> MockCodeIntelProvider::completions(const QString&, const QString&, const EditorPosition&) {
    return {
        {"std::cout", "mock completion", "std::cout", "field"},
        {"std::vector", "mock completion", "std::vector", "class"},
        {"TODO", "mock completion", "TODO", "text"}
    };
}

HoverInfo MockCodeIntelProvider::hover(const QString&, const QString&, const EditorPosition& position) {
    HoverInfo info;
    info.contents = QString("Mock hover em %1:%2").arg(position.line).arg(position.column);
    return info;
}

std::optional<DefinitionLocation> MockCodeIntelProvider::definition(const QString& filePath,
                                                                    const QString&,
                                                                    const EditorPosition&) {
    return DefinitionLocation{filePath, 1, 1};
}

QString MockCodeIntelProvider::name() const {
    return "Mock Code Intel Provider";
}

} // namespace ide::adapters::codeintel
