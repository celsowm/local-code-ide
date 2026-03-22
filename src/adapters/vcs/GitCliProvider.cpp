#include "adapters/vcs/GitCliProvider.hpp"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStringList>

#include <algorithm>

namespace ide::adapters::vcs {
namespace {
QString trimmedBranchLabel(const QString& statusOutput) {
    const QString firstLine = statusOutput.section('\n', 0, 0).trimmed();
    if (!firstLine.startsWith("##")) {
        return firstLine.isEmpty() ? QStringLiteral("git") : firstLine;
    }
    return firstLine.mid(2).trimmed();
}

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

ide::services::interfaces::GitChange parseChange(const QString& rawLine) {
    const QChar x = rawLine[0];
    const QChar y = rawLine[1];
    QString path = rawLine.mid(3).trimmed();
    if (path.contains(" -> ")) {
        path = path.section(" -> ", -1);
    }

    QFileInfo info(path);
    ide::services::interfaces::GitChange change;
    change.path = path;
    change.fileName = info.fileName();
    change.directory = info.path() == "." ? QString() : info.path();
    change.statusCode = QString(x) + QString(y);
    change.statusText = decodeStatus(change.statusCode);
    change.changeKind = decodeStatus(change.statusCode);
    change.staged = x != ' ' && x != '?';
    change.modified = (x == 'M' || y == 'M') || (x == ' ' && y != ' ');
    change.renamed = x == 'R' || y == 'R';
    change.untracked = x == '?' || y == '?';
    change.hasIndexChanges = x != ' ' && x != '?';
    change.hasWorkingTreeChanges = y != ' ' && y != '?';
    if (change.untracked) {
        change.hasWorkingTreeChanges = true;
    }
    return change;
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
    const QString branch = trimmedBranchLabel(output);
    const int changedCount = std::max(0, static_cast<int>(lines.size()) - 1);
    return QString("%1 • %2 changed").arg(branch, QString::number(changedCount));
}

QString GitCliProvider::branchLabel(const QString& workspacePath) {
    bool ok = false;
    const QString output = runGit(workspacePath, {"status", "--short", "--branch"}, &ok).trimmed();
    if (!ok || output.isEmpty()) {
        return QStringLiteral("git");
    }
    return trimmedBranchLabel(output);
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
        changes.push_back(parseChange(rawLine));
    }
    return changes;
}

std::vector<ide::services::interfaces::GitCommitSummary> GitCliProvider::recentCommits(const QString& workspacePath, int limit) {
    const int boundedLimit = std::max(limit, 0);
    if (boundedLimit == 0) {
        return {};
    }

    bool ok = false;
    const QString pretty = "%H%x1f%h%x1f%s%x1f%an%x1f%ar%x1f%D";
    const QString output = runGit(workspacePath, {"log", QStringLiteral("-%1").arg(boundedLimit), "--date=relative",
                                                  QStringLiteral("--pretty=format:%1").arg(pretty)}, &ok);
    std::vector<ide::services::interfaces::GitCommitSummary> commits;
    if (!ok) {
        return commits;
    }

    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    commits.reserve(lines.size());
    for (const QString& line : lines) {
        const QStringList fields = line.split(QChar(0x1f));
        if (fields.size() < 6) {
            continue;
        }
        ide::services::interfaces::GitCommitSummary commit;
        commit.hash = fields[0].trimmed();
        commit.shortHash = fields[1].trimmed();
        commit.subject = fields[2].trimmed();
        commit.author = fields[3].trimmed();
        commit.relativeTime = fields[4].trimmed();
        const QString refs = fields[5].trimmed();
        commit.isHead = refs.contains("HEAD");
        commit.refLabel = refs;
        commits.push_back(commit);
    }
    return commits;
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
