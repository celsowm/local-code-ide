#include "services/ToolCallingService.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QStringList>

namespace ide::services {

ToolCallingService::ToolCallingService(std::vector<std::unique_ptr<interfaces::ITool>> tools)
    : m_tools(std::move(tools)) {}

std::vector<interfaces::AiToolDefinition> ToolCallingService::definitions() const {
    std::vector<interfaces::AiToolDefinition> defs;
    defs.reserve(m_tools.size());
    for (const auto& tool : m_tools) {
        defs.push_back(tool->definition());
    }
    return defs;
}

interfaces::ToolResult ToolCallingService::execute(const interfaces::AiToolCall& call,
                                                   const QString& workspaceRoot,
                                                   const QString& currentPath) {
    ++m_lastCallCount;

    for (const auto& tool : m_tools) {
        const auto definition = tool->definition();
        if (definition.name != call.name) {
            continue;
        }

        const QString argsText = QString::fromUtf8(QJsonDocument(call.arguments).toJson(QJsonDocument::Compact));
        if (definition.destructive && m_requireApprovalForDestructive) {
            const auto pending = makePending(call, definition);
            m_pendingApprovals.push_back(pending);
            m_lastLog += QStringLiteral("[%1] %2(%3) -> queued_for_approval %4
")
                .arg(QString::number(m_lastCallCount), call.name, argsText, pending.approvalId);

            interfaces::ToolResult result;
            result.success = false;
            result.humanSummary = QStringLiteral("Tool destrutiva `%1` enviada para aprovação manual (%2).")
                .arg(call.name, pending.approvalId);
            result.payload = QJsonObject{
                {QStringLiteral("error"), QStringLiteral("requires_confirmation")},
                {QStringLiteral("requires_confirmation"), true},
                {QStringLiteral("approval_id"), pending.approvalId},
                {QStringLiteral("tool"), call.name},
                {QStringLiteral("summary"), pending.summary}
            };
            return result;
        }

        return runTool(call, workspaceRoot, currentPath, false);
    }

    interfaces::ToolResult missing;
    missing.success = false;
    missing.humanSummary = QStringLiteral("Tool `%1` não registrada.").arg(call.name);
    missing.payload = QJsonObject{
        {QStringLiteral("error"), QStringLiteral("tool_not_found")},
        {QStringLiteral("tool"), call.name}
    };
    m_lastLog += QStringLiteral("[%1] %2 -> tool_not_found
")
        .arg(QString::number(m_lastCallCount), call.name);
    return missing;
}

interfaces::ToolResult ToolCallingService::approvePending(const QString& approvalId,
                                                          const QString& workspaceRoot,
                                                          const QString& currentPath) {
    for (auto it = m_pendingApprovals.begin(); it != m_pendingApprovals.end(); ++it) {
        if (it->approvalId != approvalId) {
            continue;
        }
        const auto call = it->call;
        m_pendingApprovals.erase(it);
        ++m_lastCallCount;
        return runTool(call, workspaceRoot, currentPath, true);
    }

    interfaces::ToolResult result;
    result.success = false;
    result.humanSummary = QStringLiteral("Approval id não encontrada: %1").arg(approvalId);
    result.payload = QJsonObject{{QStringLiteral("error"), QStringLiteral("approval_not_found")}};
    return result;
}

bool ToolCallingService::rejectPending(const QString& approvalId) {
    for (auto it = m_pendingApprovals.begin(); it != m_pendingApprovals.end(); ++it) {
        if (it->approvalId != approvalId) {
            continue;
        }
        m_lastLog += QStringLiteral("[approval] rejected %1 %2
").arg(it->toolName, approvalId);
        m_pendingApprovals.erase(it);
        return true;
    }
    return false;
}

void ToolCallingService::clearPendingApprovals() {
    if (!m_pendingApprovals.empty()) {
        m_lastLog += QStringLiteral("[approval] cleared %1 pending item(s)
")
            .arg(QString::number(static_cast<int>(m_pendingApprovals.size())));
    }
    m_pendingApprovals.clear();
}

void ToolCallingService::setRequireApprovalForDestructive(bool value) {
    m_requireApprovalForDestructive = value;
}

bool ToolCallingService::requireApprovalForDestructive() const {
    return m_requireApprovalForDestructive;
}

std::vector<PendingToolApproval> ToolCallingService::pendingApprovals() const {
    return m_pendingApprovals;
}

int ToolCallingService::pendingApprovalCount() const {
    return static_cast<int>(m_pendingApprovals.size());
}

QString ToolCallingService::pendingApprovalSummary() const {
    if (m_pendingApprovals.empty()) {
        return QStringLiteral("no pending approvals");
    }
    QStringList parts;
    for (const auto& item : m_pendingApprovals) {
        parts << QStringLiteral("%1[%2]").arg(item.toolName, item.approvalId);
    }
    return parts.join(QStringLiteral(", "));
}

void ToolCallingService::resetLastRun() {
    m_lastLog.clear();
    m_lastTouchedPaths.clear();
    m_lastCallCount = 0;
}

QString ToolCallingService::lastLog() const {
    return m_lastLog.trimmed();
}

QStringList ToolCallingService::lastTouchedPaths() const {
    return m_lastTouchedPaths;
}

int ToolCallingService::lastCallCount() const {
    return m_lastCallCount;
}

QString ToolCallingService::catalogSummary() const {
    QStringList names;
    for (const auto& tool : m_tools) {
        const auto def = tool->definition();
        names << QStringLiteral("%1%2")
            .arg(def.name, def.destructive ? QStringLiteral("*") : QString());
    }
    return names.join(QStringLiteral(", "));
}

interfaces::ToolResult ToolCallingService::runTool(const interfaces::AiToolCall& call,
                                                   const QString& workspaceRoot,
                                                   const QString& currentPath,
                                                   bool fromApprovalQueue) {
    for (const auto& tool : m_tools) {
        if (tool->definition().name != call.name) {
            continue;
        }

        auto result = tool->invoke(workspaceRoot, currentPath, call.arguments);
        const QString argsText = QString::fromUtf8(QJsonDocument(call.arguments).toJson(QJsonDocument::Compact));
        m_lastLog += QStringLiteral("[%1] %2(%3) -> %4%5
")
            .arg(QString::number(m_lastCallCount),
                 call.name,
                 argsText,
                 result.humanSummary.isEmpty() ? QStringLiteral("(sem resumo)") : result.humanSummary,
                 fromApprovalQueue ? QStringLiteral(" [approved]") : QString());

        for (const auto& touched : result.touchedPaths) {
            if (!m_lastTouchedPaths.contains(touched)) {
                m_lastTouchedPaths.push_back(touched);
            }
        }

        result.payload.insert(QStringLiteral("ok"), result.success);
        result.payload.insert(QStringLiteral("tool"), call.name);
        result.payload.insert(QStringLiteral("summary"), result.humanSummary);
        result.payload.insert(QStringLiteral("approved_run"), fromApprovalQueue);
        return result;
    }

    interfaces::ToolResult missing;
    missing.success = false;
    missing.humanSummary = QStringLiteral("Tool `%1` não registrada.").arg(call.name);
    missing.payload = QJsonObject{
        {QStringLiteral("error"), QStringLiteral("tool_not_found")},
        {QStringLiteral("tool"), call.name}
    };
    return missing;
}

PendingToolApproval ToolCallingService::makePending(const interfaces::AiToolCall& call,
                                                    const interfaces::AiToolDefinition& definition) const {
    PendingToolApproval pending;
    pending.approvalId = call.id.trimmed().isEmpty()
        ? QStringLiteral("approval-%1").arg(QString::number(m_pendingApprovals.size() + 1))
        : call.id;
    pending.toolName = definition.name;
    pending.summary = QStringLiteral("%1 · %2")
        .arg(definition.description, summarizePathsFromArgs(call.arguments));
    pending.argumentsText = QString::fromUtf8(QJsonDocument(call.arguments).toJson(QJsonDocument::Indented));
    const QString pathHint = summarizePathsFromArgs(call.arguments);
    if (!pathHint.isEmpty()) {
        pending.pathHints << pathHint;
    }
    pending.destructive = definition.destructive;
    pending.call = call;
    return pending;
}

QString ToolCallingService::summarizePathsFromArgs(const QJsonObject& args) const {
    QStringList hints;
    const QString path = args.value(QStringLiteral("path")).toString().trimmed();
    if (!path.isEmpty()) {
        hints << path;
    }
    const QStringList keys{QStringLiteral("diff"), QStringLiteral("content")};
    for (const auto& key : keys) {
        const QString value = args.value(key).toString();
        if (value.contains(QStringLiteral("--- ")) || value.contains(QStringLiteral("+++ "))) {
            const auto lines = value.split('
');
            for (const auto& line : lines) {
                if (line.startsWith(QStringLiteral("--- ")) || line.startsWith(QStringLiteral("+++ "))) {
                    hints << line.mid(4).trimmed();
                }
            }
            break;
        }
    }
    hints.removeDuplicates();
    return hints.join(QStringLiteral(", "));
}

} // namespace ide::services
