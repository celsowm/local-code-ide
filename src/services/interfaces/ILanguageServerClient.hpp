#pragma once

#include "services/interfaces/ICodeIntelProvider.hpp"
#include "services/interfaces/IDiagnosticProvider.hpp"

#include <QString>
#include <optional>
#include <vector>

namespace ide::services::interfaces {

class ILanguageServerClient {
public:
    virtual ~ILanguageServerClient() = default;

    virtual bool ensureStarted() = 0;
    virtual bool isRunning() const = 0;
    virtual void stop() = 0;
    virtual QString statusLine() const = 0;

    virtual int publishDocument(const QString& filePath, const QString& text) = 0;
    virtual std::vector<Diagnostic> latestDiagnostics(const QString& filePath) const = 0;
    virtual int latestDiagnosticsVersion(const QString& filePath) const = 0;

    virtual int requestCompletionsAsync(const QString& filePath,
                                        const QString& text,
                                        const EditorPosition& position) = 0;
    virtual int requestHoverAsync(const QString& filePath,
                                  const QString& text,
                                  const EditorPosition& position) = 0;
    virtual int requestDefinitionAsync(const QString& filePath,
                                       const QString& text,
                                       const EditorPosition& position) = 0;
};

} // namespace ide::services::interfaces
