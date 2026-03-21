#include "adapters/tools/SearchWorkspaceTool.hpp"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonArray>
#include <QTextStream>

namespace ide::adapters::tools {

ide::services::interfaces::AiToolDefinition SearchWorkspaceTool::definition() const {
    return {
        QStringLiteral("search_workspace"),
        QStringLiteral("Search plain text across files in the workspace and return matching snippets."),
        QJsonObject{
            {QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("properties"), QJsonObject{
                {QStringLiteral("query"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
                {QStringLiteral("limit"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")}}}
            }},
            {QStringLiteral("required"), QJsonArray{QStringLiteral("query")}}
        },
        false
    };
}

ide::services::interfaces::ToolResult SearchWorkspaceTool::invoke(const QString& workspaceRoot,
                                                                  const QString&,
                                                                  const QJsonObject& arguments) {
    const QString query = arguments.value(QStringLiteral("query")).toString().trimmed();
    const int limit = qBound(1, arguments.value(QStringLiteral("limit")).toInt(20), 100);
    const QDir root(workspaceRoot.trimmed().isEmpty() ? QDir::currentPath() : workspaceRoot);

    ide::services::interfaces::ToolResult result;
    if (query.isEmpty()) {
        result.success = false;
        result.humanSummary = QStringLiteral("query vazio para search_workspace");
        result.payload = QJsonObject{{QStringLiteral("error"), QStringLiteral("empty_query")}};
        return result;
    }

    QJsonArray matches;
    QDirIterator it(root.absolutePath(), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext() && matches.size() < limit) {
        const QString path = it.next();
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        QTextStream stream(&file);
        int lineNumber = 0;
        while (!stream.atEnd() && matches.size() < limit) {
            const QString line = stream.readLine();
            ++lineNumber;
            if (!line.contains(query, Qt::CaseInsensitive)) {
                continue;
            }

            matches.append(QJsonObject{
                {QStringLiteral("path"), root.relativeFilePath(path)},
                {QStringLiteral("line"), lineNumber},
                {QStringLiteral("snippet"), line.trimmed()}
            });
        }
    }

    result.humanSummary = QStringLiteral("%1 match(es) para `%2`").arg(QString::number(matches.size()), query);
    result.payload = QJsonObject{
        {QStringLiteral("query"), query},
        {QStringLiteral("matches"), matches}
    };
    return result;
}

} // namespace ide::adapters::tools
