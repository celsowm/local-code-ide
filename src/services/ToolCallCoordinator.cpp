#include "services/ToolCallCoordinator.hpp"

#include <QJsonDocument>

namespace ide::services {

ToolCallCoordinator::ToolCallCoordinator(ToolCallingService* toolCallingService)
    : m_toolCallingService(toolCallingService) {}

ToolExecutionResult ToolCallCoordinator::executeRounds(interfaces::AiRequest& request,
                                                        const QString& workspaceRoot,
                                                        const QString& currentPath,
                                                        int maxRounds) {
    ToolExecutionResult result;

    for (int round = 0; round <= maxRounds; ++round) {
        // Execute tool calls if any
        if (request.enableTools && !result.content.trimmed().isEmpty()) {
            // Check if there are tool calls to execute
            break;
        }
    }

    syncStateFromRunner();
    result.log = m_lastToolLog;
    result.touchedPaths = m_lastToolTouchedPaths;
    result.toolCallCount = m_lastToolCallCount;
    return result;
}

void ToolCallCoordinator::reset() {
    if (m_toolCallingService) {
        m_toolCallingService->resetLastRun();
    }
    m_lastToolLog.clear();
    m_lastToolTouchedPaths.clear();
    m_lastToolCallCount = 0;
}

QString ToolCallCoordinator::lastLog() const {
    return m_lastToolLog;
}

QStringList ToolCallCoordinator::lastTouchedPaths() const {
    return m_lastToolTouchedPaths;
}

int ToolCallCoordinator::lastCallCount() const {
    return m_lastToolCallCount;
}

void ToolCallCoordinator::syncStateFromRunner() {
    if (!m_toolCallingService) {
        m_lastToolLog.clear();
        m_lastToolTouchedPaths.clear();
        m_lastToolCallCount = 0;
        return;
    }

    m_lastToolLog = m_toolCallingService->lastLog();
    m_lastToolTouchedPaths = m_toolCallingService->lastTouchedPaths();
    m_lastToolCallCount = m_toolCallingService->lastCallCount();
}

} // namespace ide::services
