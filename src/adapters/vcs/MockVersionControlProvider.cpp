#include "adapters/vcs/MockVersionControlProvider.hpp"

#include <QFileInfo>

#include <algorithm>

namespace ide::adapters::vcs {
namespace {
ide::services::interfaces::GitChange makeChange(const QString& path,
                                                const QString& statusCode,
                                                const QString& statusText,
                                                const QString& changeKind,
                                                bool staged,
                                                bool modified,
                                                bool renamed,
                                                bool untracked,
                                                bool hasIndexChanges,
                                                bool hasWorkingTreeChanges) {
    QFileInfo info(path);
    ide::services::interfaces::GitChange change;
    change.path = path;
    change.fileName = info.fileName();
    change.directory = info.path() == "." ? QString() : info.path();
    change.statusCode = statusCode;
    change.statusText = statusText;
    change.changeKind = changeKind;
    change.staged = staged;
    change.modified = modified;
    change.renamed = renamed;
    change.untracked = untracked;
    change.hasIndexChanges = hasIndexChanges;
    change.hasWorkingTreeChanges = hasWorkingTreeChanges;
    return change;
}
}

QString MockVersionControlProvider::summary(const QString& workspacePath) {
    const QString workspace = workspacePath.isEmpty() ? "." : workspacePath;
    return QString("main • 1 modified • workspace: %1").arg(workspace);
}

QString MockVersionControlProvider::branchLabel(const QString&) {
    return "main";
}

std::vector<ide::services::interfaces::GitChange> MockVersionControlProvider::listChanges(const QString&) {
    return {
        makeChange("src/main.cpp", "MM", "modified", "modified", true, true, false, false, true, true),
        makeChange("README.md", "??", "untracked", "untracked", false, true, false, true, false, true)
    };
}

std::vector<ide::services::interfaces::GitCommitSummary> MockVersionControlProvider::recentCommits(const QString&, int limit) {
    std::vector<ide::services::interfaces::GitCommitSummary> commits = {
        {"abc123456789", "abc1234", "Refine source control layout", "celso", "2 hours ago", true, "HEAD -> main"},
        {"def987654321", "def9876", "Add SCM history section", "celso", "1 day ago", false, QString()},
        {"001122334455", "0011223", "Tighten sidebar spacing", "celso", "2 days ago", false, QString()}
    };
    if (limit < static_cast<int>(commits.size())) {
        commits.resize(std::max(limit, 0));
    }
    return commits;
}

bool MockVersionControlProvider::stage(const QString&, const QString&) { return true; }
bool MockVersionControlProvider::unstage(const QString&, const QString&) { return true; }
bool MockVersionControlProvider::discard(const QString&, const QString&) { return true; }
bool MockVersionControlProvider::commit(const QString&, const QString& message) { return !message.trimmed().isEmpty(); }
QString MockVersionControlProvider::diffForPath(const QString&, const QString& path) {
    return QString("--- a/%1\n+++ b/%1\n@@ -1,2 +1,2 @@\n-old\n+new\n").arg(path);
}
QString MockVersionControlProvider::headFileContent(const QString&, const QString& path) {
    return QString("// HEAD version for %1\n").arg(path);
}

QString MockVersionControlProvider::name() const {
    return "Mock Version Control Provider";
}

} // namespace ide::adapters::vcs
