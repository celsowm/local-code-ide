#pragma once

#include "services/interfaces/IVersionControlProvider.hpp"

#include <memory>
#include <vector>

namespace ide::services {

class GitService {
public:
    explicit GitService(std::unique_ptr<interfaces::IVersionControlProvider> provider);

    QString summary(const QString& workspacePath) const;
    QString branchLabel(const QString& workspacePath) const;
    std::vector<interfaces::GitChange> listChanges(const QString& workspacePath) const;
    std::vector<interfaces::GitCommitSummary> recentCommits(const QString& workspacePath, int limit) const;
    bool stage(const QString& workspacePath, const QString& path) const;
    bool unstage(const QString& workspacePath, const QString& path) const;
    bool discard(const QString& workspacePath, const QString& path) const;
    bool commit(const QString& workspacePath, const QString& message) const;
    QString diffForPath(const QString& workspacePath, const QString& path) const;
    QString headFileContent(const QString& workspacePath, const QString& path) const;
    QString name() const;
    QString providerName() const;

private:
    std::unique_ptr<interfaces::IVersionControlProvider> m_provider;
};

} // namespace ide::services
