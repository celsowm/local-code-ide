#pragma once

#include "services/interfaces/IWorkspaceProvider.hpp"
#include "services/interfaces/IVersionControlProvider.hpp"

#include <QAbstractListModel>
#include <QHash>
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
    QString iconSource;
    QString gitBadge;
    QString gitBadgeColor;
};

class WorkspaceTreeListModel final : public QAbstractListModel {
    Q_OBJECT
public:
    struct GitBadgeInfo {
        QString text;
        QString color;
        int priority = -1;

        bool operator==(const GitBadgeInfo& other) const {
            return text == other.text && color == other.color && priority == other.priority;
        }
    };

    enum Roles {
        IdRole = Qt::UserRole + 1,
        PathRole,
        RelativePathRole,
        NameRole,
        DepthRole,
        DirectoryRole,
        ExpandedRole,
        HasChildrenRole,
        IconSourceRole,
        GitBadgeRole,
        GitBadgeColorRole
    };

    explicit WorkspaceTreeListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setWorkspaceRoot(const QString& workspaceRoot);
    void setFiles(const std::vector<ide::services::interfaces::WorkspaceFile>& files);
    void setGitChanges(const std::vector<ide::services::interfaces::GitChange>& changes);
    void setCurrentFilePath(const QString& path);
    Q_INVOKABLE void toggleExpanded(const QString& id);
    Q_INVOKABLE void collapseAll();
    void expandToPath(const QString& path);
    int itemCount() const;

private:
    void rebuildFlat();
    void clearExpansionState();
    void registerBadgeForPath(const QString& relativePath, const GitBadgeInfo& badge);
    QString normalizeRelativePath(const QString& path) const;
    QString iconSourceForNode(const WorkspaceTreeItem& item) const;

    QString m_workspaceRoot;
    QString m_currentFilePath;
    std::vector<ide::services::interfaces::WorkspaceFile> m_files;
    std::vector<WorkspaceTreeItem> m_flat;
    QSet<QString> m_expandedDirs;
    QHash<QString, GitBadgeInfo> m_gitBadges;
    bool m_autoExpandInitialized = false;
};

} // namespace ide::ui::models
