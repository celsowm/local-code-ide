#pragma once

#include "services/interfaces/IVersionControlProvider.hpp"

namespace ide::adapters::vcs {

class MockVersionControlProvider final : public ide::services::interfaces::IVersionControlProvider {
public:
    QString summary(const QString& workspacePath) override;
    std::vector<ide::services::interfaces::GitChange> listChanges(const QString& workspacePath) override;
    bool stage(const QString& workspacePath, const QString& path) override;
    bool unstage(const QString& workspacePath, const QString& path) override;
    bool discard(const QString& workspacePath, const QString& path) override;
    bool commit(const QString& workspacePath, const QString& message) override;
    QString diffForPath(const QString& workspacePath, const QString& path) override;
    QString headFileContent(const QString& workspacePath, const QString& path) override;
    QString name() const override;
};

} // namespace ide::adapters::vcs
