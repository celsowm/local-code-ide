#include "services/GitService.hpp"

namespace ide::services {

GitService::GitService(std::unique_ptr<interfaces::IVersionControlProvider> provider)
    : m_provider(std::move(provider)) {}

QString GitService::summary(const QString& workspacePath) const {
    return m_provider->summary(workspacePath);
}

QString GitService::branchLabel(const QString& workspacePath) const {
    return m_provider->branchLabel(workspacePath);
}

std::vector<interfaces::GitChange> GitService::listChanges(const QString& workspacePath) const {
    return m_provider->listChanges(workspacePath);
}

std::vector<interfaces::GitCommitSummary> GitService::recentCommits(const QString& workspacePath, int limit) const {
    return m_provider->recentCommits(workspacePath, limit);
}

bool GitService::stage(const QString& workspacePath, const QString& path) const {
    return m_provider->stage(workspacePath, path);
}

bool GitService::unstage(const QString& workspacePath, const QString& path) const {
    return m_provider->unstage(workspacePath, path);
}

bool GitService::discard(const QString& workspacePath, const QString& path) const {
    return m_provider->discard(workspacePath, path);
}

bool GitService::commit(const QString& workspacePath, const QString& message) const {
    return m_provider->commit(workspacePath, message);
}

QString GitService::diffForPath(const QString& workspacePath, const QString& path) const {
    return m_provider->diffForPath(workspacePath, path);
}

QString GitService::headFileContent(const QString& workspacePath, const QString& path) const {
    return m_provider->headFileContent(workspacePath, path);
}

QString GitService::name() const {
    return m_provider->name();
}

QString GitService::providerName() const {
    return m_provider->name();
}

} // namespace ide::services
