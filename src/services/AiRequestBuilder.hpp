#pragma once

#include "services/interfaces/IAiBackend.hpp"
#include <QString>
#include <vector>

namespace ide::services {

struct AiContextSettings {
    QString systemPrompt;
    int maxOutputTokens = 512;
    double temperature = 0.2;
    int documentContextChars = 12000;
    int workspaceContextChars = 16000;
    int workspaceContextMaxFiles = 4;
    bool includeDocument = true;
    bool includeDiagnostics = true;
    bool includeGitSummary = true;
    bool includeWorkspaceContext = true;
    bool enableTools = true;
    bool requireApprovalForDestructiveTools = true;
    int maxToolRounds = 4;
};

class AiRequestBuilder {
public:
    explicit AiRequestBuilder(const AiContextSettings& settings);

    interfaces::AiRequest build(const QString& prompt,
                                const QString& currentPath,
                                const QString& documentText,
                                const QString& diagnosticsText,
                                const QString& gitSummary,
                                const QString& workspaceContextText,
                                const std::vector<interfaces::AiToolDefinition>& tools = {}) const;

    QString buildUserMessage(const QString& prompt,
                             const QString& currentPath,
                             const QString& documentText,
                             const QString& diagnosticsText,
                             const QString& gitSummary,
                             const QString& workspaceContextText) const;

    QString trimForContext(const QString& text, int maxChars) const;

private:
    std::vector<interfaces::AiMessage> buildInitialMessages(const QString& prompt,
                                                            const QString& currentPath,
                                                            const QString& documentText,
                                                            const QString& diagnosticsText,
                                                            const QString& gitSummary,
                                                            const QString& workspaceContextText) const;

    AiContextSettings m_settings;
};

} // namespace ide::services
