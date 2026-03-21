#pragma once

#include <QString>
#include <vector>

namespace ide::services::interfaces {

struct WorkspaceFile {
    QString path;
    QString relativePath;
};

class IWorkspaceProvider {
public:
    virtual ~IWorkspaceProvider() = default;
    virtual std::vector<WorkspaceFile> listFiles(const QString& workspaceRoot) = 0;
    virtual QString name() const = 0;
};

} // namespace ide::services::interfaces
