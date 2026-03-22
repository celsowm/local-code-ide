#pragma once

#include "services/interfaces/IVersionControlProvider.hpp"

namespace ide::adapters::vcs {

class GitCliProvider final : public ide::services::interfaces::IVersionControlProvider {
public:
    QString summary(const QString& workspacePath) override;
    QString branchLabel(const QString& workspacePath) override;
    std::vector<ide::services::interfaces::GitChange> listChanges(const QString& workspacePath) override;
    std::vector<ide::services::interfaces::GitCommitSummary> recentCommits(const QString& workspacePath, int limit) override;
    bool stage(const QString& workspacePath, const QString& path) override;
    bool unstage(const QString& workspacePath, const QString& path) override;
    bool discard(const QString& workspacePath, const QString& path) override;
    bool commit(const QString& workspacePath, const QString& message) override;
    QString diffForPath(const QString& workspacePath, const QString& path) override;
    QString headFileContent(const QString& workspacePath, const QString& path) override;
    QString name() const override;
};

} // namespace ide::adapters::vcs
