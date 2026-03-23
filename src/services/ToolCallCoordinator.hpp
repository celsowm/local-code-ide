#pragma once

#include "services/ToolCallingService.hpp"
#include "services/interfaces/IAiBackend.hpp"
#include <QString>
#include <QStringList>
#include <vector>

namespace ide::services {

struct ToolExecutionResult {
    QString content;
    int toolCallCount = 0;
    QStringList touchedPaths;
    QString log;
    bool completed = false;
};

class ToolCallCoordinator {
public:
    explicit ToolCallCoordinator(ToolCallingService* toolCallingService);

    ToolExecutionResult executeRounds(interfaces::AiRequest& request,
                                       const QString& workspaceRoot,
                                       const QString& currentPath,
                                       int maxRounds);

    void reset();
    QString lastLog() const;
    QStringList lastTouchedPaths() const;
    int lastCallCount() const;

private:
    void syncStateFromRunner();

    ToolCallingService* m_toolCallingService;
    QString m_lastToolLog;
    QStringList m_lastToolTouchedPaths;
    int m_lastToolCallCount = 0;
};

} // namespace ide::services
