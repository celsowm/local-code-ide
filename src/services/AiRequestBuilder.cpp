#include "services/AiRequestBuilder.hpp"

namespace ide::services {

AiRequestBuilder::AiRequestBuilder(const AiContextSettings& settings)
    : m_settings(settings) {}

interfaces::AiRequest AiRequestBuilder::build(const QString& prompt,
                                               const QString& currentPath,
                                               const QString& documentText,
                                               const QString& diagnosticsText,
                                               const QString& gitSummary,
                                               const QString& workspaceContextText,
                                               const std::vector<interfaces::AiToolDefinition>& tools) const {
    interfaces::AiRequest request;
    request.prompt = prompt;
    request.currentPath = currentPath;
    request.systemPrompt = m_settings.systemPrompt;
    request.maxOutputTokens = m_settings.maxOutputTokens;
    request.temperature = m_settings.temperature;
    request.documentContextChars = m_settings.documentContextChars;
    request.workspaceContextChars = m_settings.workspaceContextChars;
    request.workspaceContextMaxFiles = m_settings.workspaceContextMaxFiles;
    request.includeDocument = m_settings.includeDocument;
    request.includeDiagnostics = m_settings.includeDiagnostics;
    request.includeGitSummary = m_settings.includeGitSummary;
    request.includeWorkspaceContext = m_settings.includeWorkspaceContext;
    request.enableTools = m_settings.enableTools && !tools.empty();
    request.maxToolRounds = m_settings.maxToolRounds;
    request.tools = tools;

    if (m_settings.includeDocument) {
        request.documentText = trimForContext(documentText, m_settings.documentContextChars);
    }
    if (m_settings.includeDiagnostics) {
        request.diagnosticsText = diagnosticsText;
    }
    if (m_settings.includeGitSummary) {
        request.gitSummary = gitSummary;
    }
    if (m_settings.includeWorkspaceContext) {
        request.workspaceContextText = trimForContext(workspaceContextText, m_settings.workspaceContextChars);
    }

    request.messages = buildInitialMessages(prompt, currentPath, documentText, diagnosticsText, gitSummary, workspaceContextText);
    return request;
}

QString AiRequestBuilder::buildUserMessage(const QString& prompt,
                                            const QString& currentPath,
                                            const QString& documentText,
                                            const QString& diagnosticsText,
                                            const QString& gitSummary,
                                            const QString& workspaceContextText) const {
    QString userMessage = QStringLiteral("Current file: %1\n")
        .arg(currentPath.isEmpty() ? QStringLiteral("untitled") : currentPath);

    if (m_settings.includeDocument && !documentText.isEmpty()) {
        userMessage += QStringLiteral("\nCurrent document:\n```\n%1\n```\n")
            .arg(trimForContext(documentText, m_settings.documentContextChars));
    }
    if (m_settings.includeDiagnostics && !diagnosticsText.isEmpty()) {
        userMessage += QStringLiteral("\nDiagnostics:\n%1\n").arg(diagnosticsText);
    }
    if (m_settings.includeGitSummary && !gitSummary.isEmpty()) {
        userMessage += QStringLiteral("\nGit summary:\n%1\n").arg(gitSummary);
    }
    if (m_settings.includeWorkspaceContext && !workspaceContextText.isEmpty()) {
        userMessage += QStringLiteral("\nRelevant workspace context:\n%1\n")
            .arg(trimForContext(workspaceContextText, m_settings.workspaceContextChars));
    }

    userMessage += QStringLiteral(
        "\nUser request: %1\n\n"
        "Prefer a unified diff inside a ```diff fenced block when proposing concrete edits. "
        "If tool calling is available and a file/workspace action is required, call a tool."
    ).arg(prompt);

    return userMessage;
}

QString AiRequestBuilder::trimForContext(const QString& text, int maxChars) const {
    if (maxChars <= 0 || text.size() <= maxChars) {
        return text;
    }

    const int head = maxChars * 7 / 10;
    const int tail = maxChars - head;
    return text.left(head)
        + QStringLiteral("\n\n/* ... context truncated for budget ... */\n\n")
        + text.right(tail);
}

std::vector<interfaces::AiMessage> AiRequestBuilder::buildInitialMessages(const QString& prompt,
                                                                           const QString& currentPath,
                                                                           const QString& documentText,
                                                                           const QString& diagnosticsText,
                                                                           const QString& gitSummary,
                                                                           const QString& workspaceContextText) const {
    std::vector<interfaces::AiMessage> messages;
    messages.push_back(interfaces::AiMessage{
        QStringLiteral("system"),
        m_settings.systemPrompt,
        {},
        {},
        {}
    });
    messages.push_back(interfaces::AiMessage{
        QStringLiteral("user"),
        buildUserMessage(prompt, currentPath, documentText, diagnosticsText, gitSummary, workspaceContextText),
        {},
        {},
        {}
    });
    return messages;
}

} // namespace ide::services
