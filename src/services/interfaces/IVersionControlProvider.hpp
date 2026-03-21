#pragma once

#include <QString>
#include <vector>

namespace ide::services::interfaces {

struct GitChange {
    QString path;
    QString statusCode;
    QString statusText;
    bool staged = false;
    bool modified = false;
    bool renamed = false;
    bool untracked = false;
};

class IVersionControlProvider {
public:
    virtual ~IVersionControlProvider() = default;
    virtual QString summary(const QString& workspacePath) = 0;
    virtual std::vector<GitChange> listChanges(const QString& workspacePath) = 0;
    virtual bool stage(const QString& workspacePath, const QString& path) = 0;
    virtual bool unstage(const QString& workspacePath, const QString& path) = 0;
    virtual bool discard(const QString& workspacePath, const QString& path) = 0;
    virtual bool commit(const QString& workspacePath, const QString& message) = 0;
    virtual QString diffForPath(const QString& workspacePath, const QString& path) = 0;
    virtual QString headFileContent(const QString& workspacePath, const QString& path) = 0;
    virtual QString name() const = 0;
};

} // namespace ide::services::interfaces
