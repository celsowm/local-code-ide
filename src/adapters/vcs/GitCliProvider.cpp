#include "adapters/vcs/GitCliProvider.hpp"

#include <QDir>
#include <QProcess>
#include <QStringList>

#include <algorithm>

namespace ide::adapters::vcs {
namespace {
QString runGit(const QString& workspacePath, const QStringList& args, bool* ok = nullptr) {
    QProcess process;
    process.setWorkingDirectory(workspacePath.isEmpty() ? QDir::currentPath() : workspacePath);
    process.start("git", args);
    const bool started = process.waitForStarted(1000);
    const bool finished = started && process.waitForFinished(5000);
    const bool success = started && finished && process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
    if (ok) *ok = success;
    return QString::fromUtf8(process.readAllStandardOutput());
}

QString decodeStatus(const QString& xy) {
    if (xy.contains('R')) return "renamed";
    if (xy.contains('A')) return "added";
    if (xy.contains('D')) return "deleted";
    if (xy.contains('M')) return "modified";
    if (xy.contains('?')) return "untracked";
    return "changed";
}
}

QString GitCliProvider::summary(const QString& workspacePath) {
    bool ok = false;
    const QString output = runGit(workspacePath, {"status", "--short", "--branch"}, &ok).trimmed();
    if (!ok) {
        return "git unavailable";
    }
    if (output.isEmpty()) {
        return "no repository or clean workspace";
    }

    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    const QString branch = lines.isEmpty() ? QString("git") : lines.first();
    const int changedCount = std::max(0, static_cast<int>(lines.size()) - 1);
    return QString("%1 • %2 changed").arg(branch, QString::number(changedCount));
}

std::vector<ide::services::interfaces::GitChange> GitCliProvider::listChanges(const QString& workspacePath) {
    bool ok = false;
    const QString output = runGit(workspacePath, {"status", "--porcelain=v1"}, &ok);
    std::vector<ide::services::interfaces::GitChange> changes;
    if (!ok) {
        return changes;
    }

    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString& rawLine : lines) {
        if (rawLine.size() < 4) continue;
        const QChar x = rawLine[0];
        const QChar y = rawLine[1];
        QString path = rawLine.mid(3).trimmed();
        if (path.contains(" -> ")) {
            path = path.section(" -> ", -1);
        }
        ide::services::interfaces::GitChange change;
        change.path = path;
        change.statusCode = QString(x) + QString(y);
        change.statusText = decodeStatus(change.statusCode);
        change.staged = x != ' ' && x != '?';
        change.modified = y != ' ' || x == 'M';
        change.renamed = x == 'R' || y == 'R';
        change.untracked = x == '?' || y == '?';
        changes.push_back(change);
    }
    return changes;
}

bool GitCliProvider::stage(const QString& workspacePath, const QString& path) {
    bool ok = false;
    runGit(workspacePath, {"add", "--", path}, &ok);
    return ok;
}

bool GitCliProvider::unstage(const QString& workspacePath, const QString& path) {
    bool ok = false;
    runGit(workspacePath, {"restore", "--staged", "--", path}, &ok);
    return ok;
}

bool GitCliProvider::discard(const QString& workspacePath, const QString& path) {
    bool ok = false;
    runGit(workspacePath, {"restore", "--", path}, &ok);
    return ok;
}

bool GitCliProvider::commit(const QString& workspacePath, const QString& message) {
    if (message.trimmed().isEmpty()) {
        return false;
    }
    bool ok = false;
    runGit(workspacePath, {"commit", "-m", message}, &ok);
    return ok;
}

QString GitCliProvider::diffForPath(const QString& workspacePath, const QString& path) {
    bool ok = false;
    const QString output = runGit(workspacePath, {"diff", "--", path}, &ok);
    return ok ? output : QString();
}

QString GitCliProvider::headFileContent(const QString& workspacePath, const QString& path) {
    bool ok = false;
    const QString output = runGit(workspacePath, {"show", QString("HEAD:%1").arg(path)}, &ok);
    return ok ? output : QString();
}

QString GitCliProvider::name() const {
    return "Git CLI Provider";
}

} // namespace ide::adapters::vcs
