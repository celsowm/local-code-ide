#include "services/DiffApplyService.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>
#include <QtGlobal>

namespace ide::services {

QString DiffApplyService::extractDiffBlock(const QString& text) const {
    const QRegularExpression fenced(QStringLiteral("```diff\s*([\s\S]*?)```"), QRegularExpression::CaseInsensitiveOption);
    const auto match = fenced.match(text);
    if (match.hasMatch()) {
        return match.captured(1).trimmed();
    }

    const int start = text.indexOf(QStringLiteral("--- "));
    if (start >= 0) {
        return text.mid(start).trimmed();
    }
    return {};
}

QString DiffApplyService::normalizePatchPath(QString path) const {
    path = path.trimmed();
    if (path.startsWith(QStringLiteral("a/")) || path.startsWith(QStringLiteral("b/"))) {
        path = path.mid(2);
    }
    return QDir::fromNativeSeparators(path);
}

std::vector<DiffApplyService::ParsedFilePatch> DiffApplyService::parseFilePatches(const QString& diffText) const {
    std::vector<ParsedFilePatch> patches;
    const auto lines = diffText.split('
');
    int i = 0;
    while (i < lines.size()) {
        if (!lines[i].startsWith(QStringLiteral("--- "))) {
            ++i;
            continue;
        }

        const int start = i;
        QString oldPath = normalizePatchPath(lines[i].mid(4));
        ++i;
        if (i >= lines.size() || !lines[i].startsWith(QStringLiteral("+++ "))) {
            continue;
        }
        QString newPath = normalizePatchPath(lines[i].mid(4));
        ++i;
        while (i < lines.size() && !lines[i].startsWith(QStringLiteral("--- "))) {
            ++i;
        }

        ParsedFilePatch patch;
        patch.rawDiff = lines.mid(start, i - start).join('
');
        patch.oldPath = oldPath;
        patch.newPath = newPath;
        patch.isCreate = oldPath == QStringLiteral("/dev/null");
        patch.isDelete = newPath == QStringLiteral("/dev/null");
        patch.targetPath = patch.isDelete ? oldPath : newPath;
        if (patch.targetPath.isEmpty() || patch.targetPath == QStringLiteral("/dev/null")) {
            patch.targetPath = oldPath;
        }
        patches.push_back(std::move(patch));
    }
    return patches;
}

PatchPreview DiffApplyService::preview(const QString& assistantOutput,
                                       const QString& currentPath,
                                       const QString& currentText) const {
    PatchPreview preview;
    preview.diffText = extractDiffBlock(assistantOutput);
    preview.hasPatch = !preview.diffText.isEmpty();
    if (!preview.hasPatch) {
        preview.summary = QStringLiteral("No unified diff detected in assistant output.");
        return preview;
    }

    const auto patches = parseFilePatches(preview.diffText);
    if (patches.empty()) {
        preview.summary = QStringLiteral("Patch detected but header parsing failed.");
        return preview;
    }
    if (patches.size() > 1) {
        preview.summary = QStringLiteral("Multi-file patch detected. Use apply_unified_diff tool or workspace apply flow.");
        return preview;
    }

    const auto& patch = patches.front();
    preview.targetPath = patch.targetPath;

    const QFileInfo currentInfo(QDir::fromNativeSeparators(currentPath));
    const QFileInfo targetInfo(patch.targetPath);
    const bool compatible = patch.targetPath.isEmpty()
        || patch.targetPath == QDir::fromNativeSeparators(currentPath)
        || patch.targetPath.endsWith(QStringLiteral("/") + currentInfo.fileName())
        || targetInfo.fileName().compare(currentInfo.fileName(), Qt::CaseInsensitive) == 0;
    if (!compatible) {
        preview.summary = QStringLiteral("Patch targets another file (%1)").arg(patch.targetPath);
        return preview;
    }

    preview.canApply = applyUnifiedDiffToText(patch.rawDiff,
                                              currentText,
                                              &preview.patchedText,
                                              &preview.additions,
                                              &preview.deletions,
                                              &preview.hunks,
                                              &preview.error);

    if (preview.canApply) {
        preview.summary = QStringLiteral("Patch ready · %1 hunks · +%2/-%3 · target=%4")
            .arg(QString::number(preview.hunks), QString::number(preview.additions), QString::number(preview.deletions), preview.targetPath.isEmpty() ? QStringLiteral("current file") : preview.targetPath);
    } else {
        preview.summary = QStringLiteral("Patch detected but not applicable: %1").arg(preview.error);
    }
    return preview;
}

WorkspacePatchApplyResult DiffApplyService::applyPatchSet(const QString& assistantOutputOrDiff,
                                                          const QString& workspaceRoot,
                                                          const QString& currentPath,
                                                          bool writeFiles) const {
    WorkspacePatchApplyResult result;
    result.diffText = extractDiffBlock(assistantOutputOrDiff);
    if (result.diffText.isEmpty() && assistantOutputOrDiff.contains(QStringLiteral("--- "))) {
        result.diffText = assistantOutputOrDiff.trimmed();
    }
    result.hasPatch = !result.diffText.isEmpty();
    result.dryRun = !writeFiles;
    if (!result.hasPatch) {
        result.error = QStringLiteral("no_diff_detected");
        result.summary = QStringLiteral("Nenhum unified diff detectado.");
        return result;
    }

    const auto patches = parseFilePatches(result.diffText);
    if (patches.empty()) {
        result.error = QStringLiteral("parse_failed");
        result.summary = QStringLiteral("Não foi possível segmentar o unified diff.");
        return result;
    }

    QStringList touched;
    int additions = 0;
    int deletions = 0;
    int hunks = 0;
    int filesApplied = 0;

    for (const auto& patch : patches) {
        const QString absolutePath = resolvePatchPath(workspaceRoot, currentPath, patch.targetPath);
        QString original;
        if (!patch.isCreate) {
            QFile input(absolutePath);
            if (!input.open(QIODevice::ReadOnly | QIODevice::Text)) {
                result.error = QStringLiteral("open_failed:%1").arg(absolutePath);
                result.summary = QStringLiteral("Falha ao abrir %1 para aplicar patch.").arg(absolutePath);
                return result;
            }
            QTextStream stream(&input);
            original = stream.readAll();
        }

        QString patched;
        int fileAdds = 0;
        int fileDels = 0;
        int fileHunks = 0;
        QString error;
        if (!applyUnifiedDiffToText(patch.rawDiff, original, &patched, &fileAdds, &fileDels, &fileHunks, &error)) {
            result.error = error;
            result.summary = QStringLiteral("Falha ao aplicar patch em %1: %2").arg(patch.targetPath, error);
            return result;
        }

        if (writeFiles) {
            if (patch.isDelete) {
                QFile::remove(absolutePath);
            } else {
                QFileInfo info(absolutePath);
                QDir().mkpath(info.dir().absolutePath());
                QFile output(absolutePath);
                if (!output.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                    result.error = QStringLiteral("write_failed:%1").arg(absolutePath);
                    result.summary = QStringLiteral("Falha ao gravar %1.").arg(absolutePath);
                    return result;
                }
                QTextStream stream(&output);
                stream << patched;
            }
        }

        touched << absolutePath;
        additions += fileAdds;
        deletions += fileDels;
        hunks += fileHunks;
        ++filesApplied;
    }

    result.success = filesApplied > 0;
    result.fileCount = filesApplied;
    result.additions = additions;
    result.deletions = deletions;
    result.hunks = hunks;
    result.touchedPaths = touched;
    result.summary = QStringLiteral("%1patch ready · %2 file(s) · %3 hunks · +%4/-%5")
        .arg(writeFiles ? QString() : QStringLiteral("dry-run · "))
        .arg(QString::number(filesApplied), QString::number(hunks), QString::number(additions), QString::number(deletions));
    return result;
}

bool DiffApplyService::applyUnifiedDiffToText(const QString& diffText,
                                              const QString& currentText,
                                              QString* patchedText,
                                              int* additions,
                                              int* deletions,
                                              int* hunks,
                                              QString* error) const {
    QStringList lines = diffText.split('
');
    int i = 0;
    for (; i < lines.size(); ++i) {
        if (lines[i].startsWith(QStringLiteral("+++ "))) {
            ++i;
            break;
        }
    }

    const bool hadTrailingNewline = currentText.endsWith('
');
    const QStringList sourceLines = currentText.split('
');
    QStringList output;
    int sourceIndex = 0;
    int addCount = 0;
    int delCount = 0;
    int hunkCount = 0;

    const QRegularExpression hunkHeader(QStringLiteral("^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@"));
    while (i < lines.size()) {
        if (lines[i].trimmed().isEmpty()) {
            ++i;
            continue;
        }

        const auto match = hunkHeader.match(lines[i]);
        if (!match.hasMatch()) {
            ++i;
            continue;
        }

        ++hunkCount;
        const int oldStart = match.captured(1).toInt();
        const int sourceTargetIndex = qMax(0, oldStart - 1);
        while (sourceIndex < sourceTargetIndex && sourceIndex < sourceLines.size()) {
            output << sourceLines[sourceIndex++];
        }

        ++i;
        while (i < lines.size() && !lines[i].startsWith(QStringLiteral("@@ ")) && !lines[i].startsWith(QStringLiteral("--- "))) {
            const QString line = lines[i];
            if (line.startsWith(QStringLiteral("\ No newline at end of file"))) {
                ++i;
                continue;
            }
            if (line.isEmpty()) {
                if (sourceIndex >= sourceLines.size() || !sourceLines[sourceIndex].isEmpty()) {
                    if (error) {
                        *error = QStringLiteral("empty diff line mismatch near hunk %1").arg(QString::number(hunkCount));
                    }
                    return false;
                }
                output << sourceLines[sourceIndex++];
                ++i;
                continue;
            }

            const QChar op = line.front();
            const QString payload = line.mid(1);
            if (op == QChar(' ')) {
                if (sourceIndex >= sourceLines.size() || sourceLines[sourceIndex] != payload) {
                    if (error) {
                        *error = QStringLiteral("context mismatch near '%1'").arg(payload.left(80));
                    }
                    return false;
                }
                output << sourceLines[sourceIndex++];
            } else if (op == QChar('-')) {
                if (sourceIndex >= sourceLines.size() || sourceLines[sourceIndex] != payload) {
                    if (error) {
                        *error = QStringLiteral("remove mismatch near '%1'").arg(payload.left(80));
                    }
                    return false;
                }
                ++sourceIndex;
                ++delCount;
            } else if (op == QChar('+')) {
                output << payload;
                ++addCount;
            }
            ++i;
        }
    }

    while (sourceIndex < sourceLines.size()) {
        output << sourceLines[sourceIndex++];
    }

    QString result = output.join('
');
    if (hadTrailingNewline && !result.endsWith('
')) {
        result += '
';
    }
    if (patchedText) {
        *patchedText = result;
    }
    if (additions) {
        *additions = addCount;
    }
    if (deletions) {
        *deletions = delCount;
    }
    if (hunks) {
        *hunks = hunkCount;
    }
    return hunkCount > 0;
}

QString DiffApplyService::resolvePatchPath(const QString& workspaceRoot,
                                           const QString& currentPath,
                                           const QString& patchPath) const {
    const QString normalized = normalizePatchPath(patchPath);
    if (normalized.isEmpty()) {
        return QFileInfo(currentPath).absoluteFilePath();
    }
    QFileInfo info(normalized);
    if (info.isAbsolute()) {
        return info.absoluteFilePath();
    }
    const QDir root(workspaceRoot.trimmed().isEmpty() ? QDir::currentPath() : workspaceRoot);
    return QFileInfo(root.absoluteFilePath(normalized)).absoluteFilePath();
}

} // namespace ide::services
