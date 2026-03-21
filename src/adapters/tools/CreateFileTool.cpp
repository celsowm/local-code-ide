#include "adapters/tools/CreateFileTool.hpp"
#include "adapters/tools/ToolPathUtils.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QJsonArray>

namespace ide::adapters::tools {

ide::services::interfaces::AiToolDefinition CreateFileTool::definition() const {
    return {
        QStringLiteral("create_file"),
        QStringLiteral("Create a new file in the workspace with the provided content."),
        QJsonObject{
            {QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("properties"), QJsonObject{
                {QStringLiteral("path"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
                {QStringLiteral("content"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
                {QStringLiteral("overwrite"), QJsonObject{{QStringLiteral("type"), QStringLiteral("boolean")}}}
            }},
            {QStringLiteral("required"), QJsonArray{QStringLiteral("path"), QStringLiteral("content")}}
        },
        true
    };
}

ide::services::interfaces::ToolResult CreateFileTool::invoke(const QString& workspaceRoot,
                                                             const QString& currentPath,
                                                             const QJsonObject& arguments) {
    const QString targetPath = resolveToolPath(workspaceRoot, currentPath, arguments.value(QStringLiteral("path")).toString());
    const QString content = arguments.value(QStringLiteral("content")).toString();
    const bool overwrite = arguments.value(QStringLiteral("overwrite")).toBool(false);

    ide::services::interfaces::ToolResult result;
    const QFileInfo info(targetPath);
    if (info.exists() && !overwrite) {
        result.success = false;
        result.humanSummary = QStringLiteral("Arquivo já existe: %1").arg(displayToolPath(workspaceRoot, targetPath));
        result.payload = QJsonObject{{QStringLiteral("path"), displayToolPath(workspaceRoot, targetPath)},
                                     {QStringLiteral("error"), QStringLiteral("already_exists")}};
        return result;
    }

    QDir().mkpath(info.dir().absolutePath());
    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        result.success = false;
        result.humanSummary = QStringLiteral("Falha ao criar %1").arg(displayToolPath(workspaceRoot, targetPath));
        result.payload = QJsonObject{{QStringLiteral("path"), displayToolPath(workspaceRoot, targetPath)},
                                     {QStringLiteral("error"), QStringLiteral("write_failed")}};
        return result;
    }

    QTextStream stream(&file);
    stream << content;

    result.humanSummary = QStringLiteral("Arquivo criado: %1").arg(displayToolPath(workspaceRoot, targetPath));
    result.touchedPaths << targetPath;
    result.workspaceChanged = true;
    result.payload = QJsonObject{
        {QStringLiteral("path"), displayToolPath(workspaceRoot, targetPath)},
        {QStringLiteral("bytes_written"), content.toUtf8().size()}
    };
    return result;
}

} // namespace ide::adapters::tools
