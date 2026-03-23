#pragma once

#include <QString>
#include <functional>
#include <optional>
#include <vector>

namespace ide::services::interfaces {

struct EditorPosition {
    int line = 1;
    int column = 1;
};

struct CompletionItem {
    QString label;
    QString detail;
    QString insertText;
    QString kind;
};

struct HoverInfo {
    QString contents;
};

struct DefinitionLocation {
    QString path;
    int line = 1;
    int column = 1;
};

class ICodeIntelProvider {
public:
    virtual ~ICodeIntelProvider() = default;

    virtual std::vector<CompletionItem> completions(const QString& filePath,
                                                    const QString& text,
                                                    const EditorPosition& position) = 0;
    virtual HoverInfo hover(const QString& filePath,
                            const QString& text,
                            const EditorPosition& position) = 0;
    virtual std::optional<DefinitionLocation> definition(const QString& filePath,
                                                         const QString& text,
                                                         const EditorPosition& position) = 0;
    virtual void completionsAsync(const QString& filePath,
                                  const QString& text,
                                  const EditorPosition& position,
                                  std::function<void(std::vector<CompletionItem>)> onReady) = 0;
    virtual void hoverAsync(const QString& filePath,
                            const QString& text,
                            const EditorPosition& position,
                            std::function<void(HoverInfo)> onReady) = 0;
    virtual void definitionAsync(const QString& filePath,
                                 const QString& text,
                                 const EditorPosition& position,
                                 std::function<void(std::optional<DefinitionLocation>)> onReady) = 0;
    virtual QString name() const = 0;
};
} // namespace ide::services::interfaces
