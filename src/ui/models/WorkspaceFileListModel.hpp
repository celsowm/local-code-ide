#pragma once

#include "services/interfaces/IWorkspaceProvider.hpp"

#include <QAbstractListModel>
#include <vector>

namespace ide::ui::models {

class WorkspaceFileListModel final : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        PathRole = Qt::UserRole + 1,
        RelativePathRole,
        NameRole
    };

    explicit WorkspaceFileListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setFiles(std::vector<ide::services::interfaces::WorkspaceFile> files);
    int fileCount() const;

private:
    std::vector<ide::services::interfaces::WorkspaceFile> m_files;
};

} // namespace ide::ui::models
