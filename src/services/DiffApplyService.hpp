#pragma once

#include <QString>
#include <QStringList>
#include <vector>

namespace ide::services {

struct PatchPreview {
    bool hasPatch = false;
    bool canApply = false;
    QString targetPath;
    QString diffText;
    QString patchedText;
    QString summary;
    QString error;
    int additions = 0;
    int deletions = 0;
    int hunks = 0;
};

struct WorkspacePatchApplyResult {
    bool hasPatch = false;
    bool success = false;
    bool dryRun = false;
    QString summary;
    QString error;
    QString diffText;
    QStringList touchedPaths;
    int fileCount = 0;
    int additions = 0;
    int deletions = 0;
    int hunks = 0;
};

class DiffApplyService {
public:
    PatchPreview preview(const QString& assistantOutput,
                         const QString& currentPath,
                         const QString& currentText) const;

    WorkspacePatchApplyResult applyPatchSet(const QString& assistantOutputOrDiff,
                                            const QString& workspaceRoot,
                                            const QString& currentPath,
                                            bool writeFiles) const;

private:
    struct ParsedFilePatch {
        QString rawDiff;
        QString oldPath;
        QString newPath;
        QString targetPath;
        bool isCreate = false;
        bool isDelete = false;
    };

    QString extractDiffBlock(const QString& text) const;
    QString normalizePatchPath(QString path) const;
    std::vector<ParsedFilePatch> parseFilePatches(const QString& diffText) const;
    bool applyUnifiedDiffToText(const QString& diffText,
                                const QString& currentText,
                                QString* patchedText,
                                int* additions,
                                int* deletions,
                                int* hunks,
                                QString* error) const;
    QString resolvePatchPath(const QString& workspaceRoot,
                             const QString& currentPath,
                             const QString& patchPath) const;
};

} // namespace ide::services
