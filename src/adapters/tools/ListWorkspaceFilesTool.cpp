#include "adapters/tools/ListWorkspaceFilesTool.hpp"
#include "adapters/tools/ToolPathUtils.hpp"

#include <QDir>
#include <QDirIterator>
#include <QJsonArray>

namespace ide::adapters::tools {

ide::services::interfaces::AiToolDefinition ListWorkspaceFilesTool::definition() const {
    return {
        QStringLiteral("list_workspace_files"),
        QStringLiteral("List files in the workspace, optionally filtered by a substring."),
        QJsonObject{
            {QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("properties"), QJsonObject{
                {QStringLiteral("contains"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
                {QStringLiteral("limit"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")}}}
            }}
        },
        false
    };
}

ide::services::interfaces::ToolResult ListWorkspaceFilesTool::invoke(const QString& workspaceRoot,
                                                                     const QString&,
                                                                     const QJsonObject& arguments) {
    const QDir root(workspaceRoot.trimmed().isEmpty() ? QDir::currentPath() : workspaceRoot);
    const QString contains = arguments.value(QStringLiteral("contains")).toString().trimmed();
    const int limit = qBound(1, arguments.value(QStringLiteral("limit")).toInt(40), 500);

    QJsonArray files;
    QDirIterator it(root.absolutePath(), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext() && files.size() < limit) {
        const QString path = it.next();
        const QString relative = root.relativeFilePath(path);
        if (!contains.isEmpty() && !relative.contains(contains, Qt::CaseInsensitive)) {
            continue;
        }
        files.append(relative);
    }

    ide::services::interfaces::ToolResult result;
    result.humanSummary = QStringLiteral("%1 arquivo(s) listados em %2")
        .arg(QString::number(files.size()), root.dirName().isEmpty() ? root.absolutePath() : root.dirName());
    result.payload = QJsonObject{
        {QStringLiteral("workspace_root"), root.absolutePath()},
        {QStringLiteral("files"), files}
    };
    return result;
}

} // namespace ide::adapters::tools
