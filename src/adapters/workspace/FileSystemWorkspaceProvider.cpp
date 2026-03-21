#include "adapters/workspace/FileSystemWorkspaceProvider.hpp"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include <algorithm>

namespace ide::adapters::workspace {
using WorkspaceFile = ide::services::interfaces::WorkspaceFile;

namespace {
bool shouldSkipPath(const QString& path) {
    return path.contains("/.git/") || path.contains("/build/") || path.contains("/node_modules/");
}
} // namespace

std::vector<WorkspaceFile> FileSystemWorkspaceProvider::listFiles(const QString& workspaceRoot) {
    std::vector<WorkspaceFile> files;
    if (workspaceRoot.isEmpty()) {
        return files;
    }

    QDir root(workspaceRoot);
    if (!root.exists()) {
        return files;
    }

    QDirIterator it(workspaceRoot, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        if (shouldSkipPath(path)) {
            continue;
        }

        WorkspaceFile file;
        file.path = path;
        file.relativePath = root.relativeFilePath(path);
        files.push_back(std::move(file));
    }

    std::sort(files.begin(), files.end(), [](const WorkspaceFile& a, const WorkspaceFile& b) {
        return a.relativePath.toLower() < b.relativePath.toLower();
    });

    return files;
}

QString FileSystemWorkspaceProvider::name() const {
    return "Filesystem Workspace Provider";
}

} // namespace ide::adapters::workspace
