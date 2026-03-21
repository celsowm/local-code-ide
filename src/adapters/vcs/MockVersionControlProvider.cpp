#include "adapters/vcs/MockVersionControlProvider.hpp"

namespace ide::adapters::vcs {

QString MockVersionControlProvider::summary(const QString& workspacePath) {
    const QString workspace = workspacePath.isEmpty() ? "." : workspacePath;
    return QString("main • 1 modified • workspace: %1").arg(workspace);
}

std::vector<ide::services::interfaces::GitChange> MockVersionControlProvider::listChanges(const QString&) {
    return {
        {"src/main.cpp", "M ", "modified", true, true, false, false},
        {"README.md", "??", "untracked", false, true, false, true}
    };
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
