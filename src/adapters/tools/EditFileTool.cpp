#include "adapters/tools/EditFileTool.hpp"
#include "adapters/tools/ToolPathUtils.hpp"

#include <QFile>
#include <QTextStream>
#include <QJsonArray>

namespace ide::adapters::tools {

ide::services::interfaces::AiToolDefinition EditFileTool::definition() const {
    return {
        QStringLiteral("edit_file"),
        QStringLiteral("Edit a file using replace_all, append, prepend, or find_replace operations."),
        QJsonObject{
            {QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("properties"), QJsonObject{
                {QStringLiteral("path"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
                {QStringLiteral("mode"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
                                                     {QStringLiteral("enum"), QJsonArray{
                                                         QStringLiteral("replace_all"),
                                                         QStringLiteral("append"),
                                                         QStringLiteral("prepend"),
                                                         QStringLiteral("find_replace")
                                                     }}}},
                {QStringLiteral("content"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
                {QStringLiteral("find_text"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
                {QStringLiteral("replace_text"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}}
            }}
        },
        true
    };
}

ide::services::interfaces::ToolResult EditFileTool::invoke(const QString& workspaceRoot,
                                                           const QString& currentPath,
                                                           const QJsonObject& arguments) {
    const QString targetPath = resolveToolPath(workspaceRoot, currentPath, arguments.value(QStringLiteral("path")).toString());
    const QString mode = arguments.value(QStringLiteral("mode")).toString(QStringLiteral("replace_all")).trimmed();
    const QString content = arguments.value(QStringLiteral("content")).toString();
    const QString findText = arguments.value(QStringLiteral("find_text")).toString();
    const QString replaceText = arguments.value(QStringLiteral("replace_text")).toString();

    ide::services::interfaces::ToolResult result;
    QFile file(targetPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.success = false;
        result.humanSummary = QStringLiteral("Falha ao abrir para editar: %1").arg(displayToolPath(workspaceRoot, targetPath));
        result.payload = QJsonObject{{QStringLiteral("path"), displayToolPath(workspaceRoot, targetPath)},
                                     {QStringLiteral("error"), QStringLiteral("open_failed")}};
        return result;
    }

    QTextStream input(&file);
    const QString original = input.readAll();
    file.close();

    QString updated = original;
    if (mode == QStringLiteral("append")) {
        updated += content;
    } else if (mode == QStringLiteral("prepend")) {
        updated = content + updated;
    } else if (mode == QStringLiteral("find_replace")) {
        if (findText.isEmpty()) {
            result.success = false;
            result.humanSummary = QStringLiteral("find_text vazio em find_replace");
            result.payload = QJsonObject{{QStringLiteral("error"), QStringLiteral("missing_find_text")}};
            return result;
        }
        updated.replace(findText, replaceText.isEmpty() ? content : replaceText);
    } else {
        updated = content;
    }

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        result.success = false;
        result.humanSummary = QStringLiteral("Falha ao gravar edição em %1").arg(displayToolPath(workspaceRoot, targetPath));
        result.payload = QJsonObject{{QStringLiteral("path"), displayToolPath(workspaceRoot, targetPath)},
                                     {QStringLiteral("error"), QStringLiteral("write_failed")}};
        return result;
    }

    QTextStream output(&file);
    output << updated;

    result.humanSummary = QStringLiteral("Arquivo editado: %1 via %2").arg(displayToolPath(workspaceRoot, targetPath), mode);
    result.touchedPaths << targetPath;
    result.workspaceChanged = true;
    result.payload = QJsonObject{
        {QStringLiteral("path"), displayToolPath(workspaceRoot, targetPath)},
        {QStringLiteral("mode"), mode},
        {QStringLiteral("chars_before"), original.size()},
        {QStringLiteral("chars_after"), updated.size()}
    };
    return result;
}

} // namespace ide::adapters::tools
