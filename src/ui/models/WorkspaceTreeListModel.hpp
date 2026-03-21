#pragma once

#include "services/interfaces/IWorkspaceProvider.hpp"

#include <QAbstractListModel>
#include <QSet>
#include <vector>

namespace ide::ui::models {

struct WorkspaceTreeItem {
    QString id;
    QString path;
    QString relativePath;
    QString name;
    int depth = 0;
    bool directory = false;
    bool expanded = false;
    bool hasChildren = false;
};

class WorkspaceTreeListModel final : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        PathRole,
        RelativePathRole,
        NameRole,
        DepthRole,
        DirectoryRole,
        ExpandedRole,
        HasChildrenRole
    };

    explicit WorkspaceTreeListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setWorkspaceRoot(const QString& workspaceRoot);
    void setFiles(const std::vector<ide::services::interfaces::WorkspaceFile>& files);
    Q_INVOKABLE void toggleExpanded(const QString& id);
    void expandToPath(const QString& path);
    int itemCount() const;

private:
    void rebuildFlat();

    QString m_workspaceRoot;
    std::vector<ide::services::interfaces::WorkspaceFile> m_files;
    std::vector<WorkspaceTreeItem> m_flat;
    QSet<QString> m_expandedDirs;
};

} // namespace ide::ui::models
