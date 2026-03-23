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

void MockCodeIntelProvider::completionsAsync(const QString& filePath,
                                             const QString& text,
                                             const EditorPosition& position,
                                             std::function<void(std::vector<CompletionItem>)> onReady) {
    if (!onReady) {
        return;
    }
    onReady(completions(filePath, text, position));
}

void MockCodeIntelProvider::hoverAsync(const QString& filePath,
                                       const QString& text,
                                       const EditorPosition& position,
                                       std::function<void(HoverInfo)> onReady) {
    if (!onReady) {
        return;
    }
    onReady(hover(filePath, text, position));
}

void MockCodeIntelProvider::definitionAsync(const QString& filePath,
                                            const QString& text,
                                            const EditorPosition& position,
                                            std::function<void(std::optional<DefinitionLocation>)> onReady) {
    if (!onReady) {
        return;
    }
    onReady(definition(filePath, text, position));
}

QString MockCodeIntelProvider::name() const {
    return "Mock Code Intel Provider";
}

} // namespace ide::adapters::codeintel
