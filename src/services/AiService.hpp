#pragma once

#include "services/ToolCallingService.hpp"
#include "services/interfaces/IAiBackend.hpp"

#include <QStringList>
#include <memory>

namespace ide::services {

class AiService {
public:
    struct Settings {
        QString systemPrompt = QStringLiteral(
            "You are an offline coding assistant embedded in a desktop IDE. "
            "Be precise, practical, and propose code-level changes when useful. "
            "When editing code, prefer returning a unified diff inside a ```diff fenced block. "
            "When a workspace action is needed and tools are available, use the provided tools instead of inventing file contents or search results."
        );
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

    explicit AiService(std::unique_ptr<interfaces::IAiBackend> backend,
                       std::unique_ptr<ToolCallingService> toolCallingService = nullptr);

    QString ask(const QString& prompt,
                const QString& workspaceRoot,
                const QString& currentPath,
                const QString& documentText,
                const QString& diagnosticsText,
                const QString& gitSummary,
                const QString& workspaceContextText);
    QString backendName() const;
    QString backendStatusLine() const;
    bool isBackendAvailable() const;

    QString systemPrompt() const;
    void setSystemPrompt(const QString& value);
    int maxOutputTokens() const;
    void setMaxOutputTokens(int value);
    double temperature() const;
    void setTemperature(double value);
    int documentContextChars() const;
    void setDocumentContextChars(int value);
    int workspaceContextChars() const;
    void setWorkspaceContextChars(int value);
    int workspaceContextMaxFiles() const;
    void setWorkspaceContextMaxFiles(int value);
    bool includeDocument() const;
    void setIncludeDocument(bool value);
    bool includeDiagnostics() const;
    void setIncludeDiagnostics(bool value);
    bool includeGitSummary() const;
    void setIncludeGitSummary(bool value);
    bool includeWorkspaceContext() const;
    void setIncludeWorkspaceContext(bool value);
    bool toolsEnabled() const;
    void setToolsEnabled(bool value);
    bool requireApprovalForDestructiveTools() const;
    void setRequireApprovalForDestructiveTools(bool value);
    int maxToolRounds() const;
    void setMaxToolRounds(int value);

    QString contextSummary(const QString& currentPath,
                           const QString& documentText,
                           const QString& diagnosticsText,
                           const QString& gitSummary,
                           const QString& workspaceContextText) const;

    QString lastToolLog() const;
    int lastToolCallCount() const;
    QStringList lastToolTouchedPaths() const;
    QString toolCatalogSummary() const;
    int pendingApprovalCount() const;
    QString pendingApprovalSummary() const;
    std::vector<PendingToolApproval> pendingApprovals() const;
    interfaces::ToolResult approvePendingTool(const QString& approvalId,
                                              const QString& workspaceRoot,
                                              const QString& currentPath);
    bool rejectPendingTool(const QString& approvalId);
    void clearPendingApprovals();

private:
    interfaces::AiRequest buildRequest(const QString& prompt,
                                       const QString& currentPath,
                                       const QString& documentText,
                                       const QString& diagnosticsText,
                                       const QString& gitSummary,
                                       const QString& workspaceContextText) const;
    std::vector<interfaces::AiMessage> buildInitialMessages(const QString& prompt,
                                                            const QString& currentPath,
                                                            const QString& documentText,
                                                            const QString& diagnosticsText,
                                                            const QString& gitSummary,
                                                            const QString& workspaceContextText) const;
    QString buildUserMessage(const QString& prompt,
                             const QString& currentPath,
                             const QString& documentText,
                             const QString& diagnosticsText,
                             const QString& gitSummary,
                             const QString& workspaceContextText) const;
    QString trimForContext(const QString& text, int maxChars) const;
    void syncToolStateFromRunner();

    std::unique_ptr<interfaces::IAiBackend> m_backend;
    std::unique_ptr<ToolCallingService> m_toolCallingService;
    Settings m_settings;
    QString m_lastToolLog;
    QStringList m_lastToolTouchedPaths;
    int m_lastToolCallCount = 0;
};

} // namespace ide::services
