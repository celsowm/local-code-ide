#pragma once

#include "services/interfaces/ICodeIntelProvider.hpp"
#include "services/interfaces/IDiagnosticProvider.hpp"

#include <QString>
#include <functional>
#include <optional>
#include <vector>

namespace ide::services::interfaces {

class ILanguageServerHub {
public:
    struct RuntimeStatus {
        bool lspReady = false;
        QString languageId;
        QString providerLabel;
        QString statusLine;
    };

    virtual ~ILanguageServerHub() = default;

    virtual int publishDocument(const QString& filePath, const QString& text) = 0;
    virtual std::vector<Diagnostic> latestDiagnostics(const QString& filePath) const = 0;
    virtual RuntimeStatus runtimeStatus(const QString& filePathOrLanguageId) const = 0;
    virtual QString languageIdForPath(const QString& filePath) const = 0;

    virtual void completionsAsync(
        const QString& filePath,
        const QString& text,
        const EditorPosition& position,
        std::function<void(std::vector<CompletionItem>)> onReady) = 0;
    virtual void hoverAsync(
        const QString& filePath,
        const QString& text,
        const EditorPosition& position,
        std::function<void(HoverInfo)> onReady) = 0;
    virtual void definitionAsync(
        const QString& filePath,
        const QString& text,
        const EditorPosition& position,
        std::function<void(std::optional<DefinitionLocation>)> onReady) = 0;

    virtual void shutdown() = 0;
};

} // namespace ide::services::interfaces
