#pragma once

#include "services/interfaces/ITool.hpp"

namespace ide::adapters::tools {

class ListWorkspaceFilesTool final : public ide::services::interfaces::ITool {
public:
    ide::services::interfaces::AiToolDefinition definition() const override;
    ide::services::interfaces::ToolResult invoke(const QString& workspaceRoot,
                                                 const QString& currentPath,
                                                 const QJsonObject& arguments) override;
};

} // namespace ide::adapters::tools
