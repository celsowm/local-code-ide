#pragma once

#include "services/interfaces/ITool.hpp"

#include <QStringList>
#include <memory>
#include <vector>

namespace ide::services {

struct PendingToolApproval {
    QString approvalId;
    QString toolName;
    QString summary;
    QString argumentsText;
    QStringList pathHints;
    bool destructive = true;
    interfaces::AiToolCall call;
};

class ToolCallingService {
public:
    explicit ToolCallingService(std::vector<std::unique_ptr<interfaces::ITool>> tools);

    std::vector<interfaces::AiToolDefinition> definitions() const;
    interfaces::ToolResult execute(const interfaces::AiToolCall& call,
                                   const QString& workspaceRoot,
                                   const QString& currentPath);

    interfaces::ToolResult approvePending(const QString& approvalId,
                                          const QString& workspaceRoot,
                                          const QString& currentPath);
    bool rejectPending(const QString& approvalId);
    void clearPendingApprovals();

    void setRequireApprovalForDestructive(bool value);
    bool requireApprovalForDestructive() const;

    std::vector<PendingToolApproval> pendingApprovals() const;
    int pendingApprovalCount() const;
    QString pendingApprovalSummary() const;

    void resetLastRun();
    QString lastLog() const;
    QStringList lastTouchedPaths() const;
    int lastCallCount() const;
    QString catalogSummary() const;

private:
    interfaces::ToolResult runTool(const interfaces::AiToolCall& call,
                                   const QString& workspaceRoot,
                                   const QString& currentPath,
                                   bool fromApprovalQueue);
    PendingToolApproval makePending(const interfaces::AiToolCall& call,
                                    const interfaces::AiToolDefinition& definition) const;
    QString summarizePathsFromArgs(const QJsonObject& args) const;

    std::vector<std::unique_ptr<interfaces::ITool>> m_tools;
    std::vector<PendingToolApproval> m_pendingApprovals;
    QString m_lastLog;
    QStringList m_lastTouchedPaths;
    int m_lastCallCount = 0;
    bool m_requireApprovalForDestructive = true;
};

} // namespace ide::services
