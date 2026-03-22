#pragma once

#include <QString>
#include <vector>

namespace ide::services::interfaces {

struct GitChange {
    QString path;
    QString fileName;
    QString directory;
    QString statusCode;
    QString statusText;
    QString changeKind;
    bool staged = false;
    bool modified = false;
    bool renamed = false;
    bool untracked = false;
    bool hasIndexChanges = false;
    bool hasWorkingTreeChanges = false;
};

struct GitCommitSummary {
    QString hash;
    QString shortHash;
    QString subject;
    QString author;
    QString relativeTime;
    bool isHead = false;
    QString refLabel;
};

class IVersionControlProvider {
public:
    virtual ~IVersionControlProvider() = default;
    virtual QString summary(const QString& workspacePath) = 0;
    virtual QString branchLabel(const QString& workspacePath) = 0;
    virtual std::vector<GitChange> listChanges(const QString& workspacePath) = 0;
    virtual std::vector<GitCommitSummary> recentCommits(const QString& workspacePath, int limit) = 0;
    virtual bool stage(const QString& workspacePath, const QString& path) = 0;
    virtual bool unstage(const QString& workspacePath, const QString& path) = 0;
    virtual bool discard(const QString& workspacePath, const QString& path) = 0;
    virtual bool commit(const QString& workspacePath, const QString& message) = 0;
    virtual QString diffForPath(const QString& workspacePath, const QString& path) = 0;
    virtual QString headFileContent(const QString& workspacePath, const QString& path) = 0;
    virtual QString name() const = 0;
};

} // namespace ide::services::interfaces
