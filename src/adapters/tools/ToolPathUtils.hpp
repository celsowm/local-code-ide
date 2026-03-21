#pragma once

#include <QDir>
#include <QFileInfo>

namespace ide::adapters::tools {

inline QString resolveToolPath(const QString& workspaceRoot,
                               const QString& currentPath,
                               const QString& inputPath) {
    const QString raw = inputPath.trimmed();
    if (raw.isEmpty()) {
        return QFileInfo(currentPath).absoluteFilePath();
    }

    QFileInfo candidate(raw);
    if (candidate.isAbsolute()) {
        return candidate.absoluteFilePath();
    }

    if (!currentPath.trimmed().isEmpty()) {
        const QFileInfo currentInfo(currentPath);
        const QDir currentDir = currentInfo.absoluteDir();
        const QString fromCurrent = currentDir.absoluteFilePath(raw);
        if (QFileInfo::exists(fromCurrent)) {
            return QFileInfo(fromCurrent).absoluteFilePath();
        }
    }

    const QDir root(workspaceRoot.trimmed().isEmpty() ? QDir::currentPath() : workspaceRoot);
    return QFileInfo(root.absoluteFilePath(raw)).absoluteFilePath();
}

inline QString displayToolPath(const QString& workspaceRoot, const QString& absolutePath) {
    const QDir root(workspaceRoot.trimmed().isEmpty() ? QDir::currentPath() : workspaceRoot);
    return root.relativeFilePath(absolutePath);
}

} // namespace ide::adapters::tools
