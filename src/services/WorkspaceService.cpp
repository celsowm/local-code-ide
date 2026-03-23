#include "services/WorkspaceService.hpp"

namespace ide::services {

WorkspaceService::WorkspaceService(std::unique_ptr<interfaces::IWorkspaceProvider> provider)
    : m_provider(std::move(provider)) {}

std::vector<interfaces::WorkspaceFile> WorkspaceService::files(const QString& workspaceRoot,
                                                               const std::function<void(int, int)>& onProgress) {
    return m_provider ? m_provider->listFiles(workspaceRoot, onProgress) : std::vector<interfaces::WorkspaceFile>{};
}

QString WorkspaceService::providerName() const {
    return m_provider ? m_provider->name() : "No Workspace Provider";
}

} // namespace ide::services
