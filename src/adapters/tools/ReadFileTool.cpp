#include "adapters/tools/ReadFileTool.hpp"
#include "adapters/tools/ToolPathUtils.hpp"

#include <QFile>
#include <QJsonArray>
#include <QTextStream>

namespace ide::adapters::tools {

ide::services::interfaces::AiToolDefinition ReadFileTool::definition() const {
    return {
        QStringLiteral("read_file"),
        QStringLiteral("Read a file from the current workspace or the current editor path."),
        QJsonObject{
            {QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("properties"), QJsonObject{
                {QStringLiteral("path"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
                {QStringLiteral("max_chars"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")}}}
            }}
        },
        false
    };
}

ide::services::interfaces::ToolResult ReadFileTool::invoke(const QString& workspaceRoot,
                                                           const QString& currentPath,
                                                           const QJsonObject& arguments) {
    const QString path = resolveToolPath(workspaceRoot, currentPath, arguments.value(QStringLiteral("path")).toString());
    const int maxChars = qMax(256, arguments.value(QStringLiteral("max_chars")).toInt(16000));

    ide::services::interfaces::ToolResult result;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.success = false;
        result.humanSummary = QStringLiteral("Falha ao abrir %1").arg(displayToolPath(workspaceRoot, path));
        result.payload = QJsonObject{
            {QStringLiteral("path"), displayToolPath(workspaceRoot, path)},
            {QStringLiteral("error"), QStringLiteral("open_failed")}
        };
        return result;
    }

    QTextStream stream(&file);
    const QString full = stream.readAll();
    const bool truncated = full.size() > maxChars;
    const QString content = truncated ? full.left(maxChars) : full;

    result.humanSummary = QStringLiteral("Lido %1 (%2 chars%3)")
        .arg(displayToolPath(workspaceRoot, path),
             QString::number(content.size()),
             truncated ? QStringLiteral(", truncated") : QString());
    result.payload = QJsonObject{
        {QStringLiteral("path"), displayToolPath(workspaceRoot, path)},
        {QStringLiteral("content"), content},
        {QStringLiteral("truncated"), truncated},
        {QStringLiteral("total_chars"), full.size()}
    };
    return result;
}

} // namespace ide::adapters::tools
