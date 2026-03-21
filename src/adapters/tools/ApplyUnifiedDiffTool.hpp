#pragma once

#include "services/DiffApplyService.hpp"
#include "services/interfaces/ITool.hpp"

namespace ide::adapters::tools {

class ApplyUnifiedDiffTool final : public ide::services::interfaces::ITool {
public:
    ide::services::interfaces::AiToolDefinition definition() const override;
    ide::services::interfaces::ToolResult invoke(const QString& workspaceRoot,
                                                 const QString& currentPath,
                                                 const QJsonObject& arguments) override;

private:
    ide::services::DiffApplyService m_diffApplyService;
};

} // namespace ide::adapters::tools
