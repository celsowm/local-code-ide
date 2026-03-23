#pragma once

#include "services/interfaces/IWorkspaceProvider.hpp"

namespace ide::adapters::workspace {

class FileSystemWorkspaceProvider final : public ide::services::interfaces::IWorkspaceProvider {
public:
    std::vector<ide::services::interfaces::WorkspaceFile> listFiles(
        const QString& workspaceRoot,
        const std::function<void(int, int)>& onProgress = {}) override;
    QString name() const override;
};

} // namespace ide::adapters::workspace
