#include "adapters/tools/ApplyUnifiedDiffTool.hpp"

#include <QJsonArray>

namespace ide::adapters::tools {

ide::services::interfaces::AiToolDefinition ApplyUnifiedDiffTool::definition() const {
    return {
        QStringLiteral("apply_unified_diff"),
        QStringLiteral("Apply a unified diff to one or more files in the workspace. Use dry_run=true first when uncertain."),
        QJsonObject{
            {QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("properties"), QJsonObject{
                {QStringLiteral("diff"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
                {QStringLiteral("dry_run"), QJsonObject{{QStringLiteral("type"), QStringLiteral("boolean")}}}
            }},
            {QStringLiteral("required"), QJsonArray{QStringLiteral("diff")}}
        },
        true
    };
}

ide::services::interfaces::ToolResult ApplyUnifiedDiffTool::invoke(const QString& workspaceRoot,
                                                                   const QString& currentPath,
                                                                   const QJsonObject& arguments) {
    const QString diff = arguments.value(QStringLiteral("diff")).toString();
    const bool dryRun = arguments.value(QStringLiteral("dry_run")).toBool(false);

    ide::services::interfaces::ToolResult result;
    if (diff.trimmed().isEmpty()) {
        result.success = false;
        result.humanSummary = QStringLiteral("diff vazio para apply_unified_diff");
        result.payload = QJsonObject{{QStringLiteral("error"), QStringLiteral("empty_diff")}};
        return result;
    }

    const auto patch = m_diffApplyService.applyPatchSet(diff, workspaceRoot, currentPath, !dryRun);
    result.success = patch.success;
    result.workspaceChanged = patch.success && !dryRun;
    result.touchedPaths = patch.touchedPaths;
    result.humanSummary = patch.summary;
    QJsonArray touched;
    for (const auto& path : patch.touchedPaths) {
        touched.append(path);
    }
    result.payload = QJsonObject{
        {QStringLiteral("dry_run"), dryRun},
        {QStringLiteral("file_count"), patch.fileCount},
        {QStringLiteral("additions"), patch.additions},
        {QStringLiteral("deletions"), patch.deletions},
        {QStringLiteral("hunks"), patch.hunks},
        {QStringLiteral("touched_paths"), touched},
        {QStringLiteral("error"), patch.error},
        {QStringLiteral("summary"), patch.summary}
    };
    return result;
}

} // namespace ide::adapters::tools
