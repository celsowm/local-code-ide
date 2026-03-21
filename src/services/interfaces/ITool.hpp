#pragma once

#include "services/interfaces/IAiBackend.hpp"

#include <QJsonObject>
#include <QStringList>

namespace ide::services::interfaces {

struct ToolResult {
    bool success = true;
    QString humanSummary;
    QJsonObject payload;
    QStringList touchedPaths;
    bool workspaceChanged = false;
};

class ITool {
public:
    virtual ~ITool() = default;
    virtual AiToolDefinition definition() const = 0;
    virtual ToolResult invoke(const QString& workspaceRoot,
                              const QString& currentPath,
                              const QJsonObject& arguments) = 0;
};

} // namespace ide::services::interfaces
