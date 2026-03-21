#pragma once

#include "services/interfaces/IWorkspaceProvider.hpp"

#include <memory>

namespace ide::services {

class WorkspaceService {
public:
    explicit WorkspaceService(std::unique_ptr<interfaces::IWorkspaceProvider> provider);

    std::vector<interfaces::WorkspaceFile> files(const QString& workspaceRoot);
    QString providerName() const;

private:
    std::unique_ptr<interfaces::IWorkspaceProvider> m_provider;
};

} // namespace ide::services
