#include "ui/models/WorkspaceTreeListModel.hpp"

#include <QDir>
#include <QFileInfo>
#include <QMap>

#include <memory>
#include <functional>

namespace ide::ui::models {
namespace {
struct TreeNode {
    QString id;
    QString path;
    QString relativePath;
    QString name;
    bool directory = false;
    QMap<QString, std::shared_ptr<TreeNode>> children;
};

QString keyForPath(const QString& workspaceRoot, const QString& path) {
    QFileInfo info(path);
    if (info.isAbsolute() && !workspaceRoot.isEmpty()) {
        return QDir(workspaceRoot).relativeFilePath(path);
    }
    return path;
}
}

WorkspaceTreeListModel::WorkspaceTreeListModel(QObject* parent)
    : QAbstractListModel(parent) {}

int WorkspaceTreeListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_flat.size());
}

QVariant WorkspaceTreeListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_flat.size())) {
        return {};
    }

    const auto& item = m_flat[static_cast<std::size_t>(index.row())];
    switch (role) {
    case IdRole: return item.id;
    case PathRole: return item.path;
    case RelativePathRole: return item.relativePath;
    case NameRole: return item.name;
    case DepthRole: return item.depth;
    case DirectoryRole: return item.directory;
    case ExpandedRole: return item.expanded;
    case HasChildrenRole: return item.hasChildren;
    default: return {};
    }
}

QHash<int, QByteArray> WorkspaceTreeListModel::roleNames() const {
    return {
        {IdRole, "nodeId"},
        {PathRole, "path"},
        {RelativePathRole, "relativePath"},
        {NameRole, "name"},
        {DepthRole, "depth"},
        {DirectoryRole, "directory"},
        {ExpandedRole, "expanded"},
        {HasChildrenRole, "hasChildren"}
    };
}

void WorkspaceTreeListModel::setWorkspaceRoot(const QString& workspaceRoot) {
    if (workspaceRoot == m_workspaceRoot) return;
    m_workspaceRoot = workspaceRoot;
    rebuildFlat();
}

void WorkspaceTreeListModel::setFiles(const std::vector<ide::services::interfaces::WorkspaceFile>& files) {
    m_files = files;
    if (m_expandedDirs.isEmpty()) {
        for (const auto& file : m_files) {
            const auto parts = file.relativePath.split('/', Qt::SkipEmptyParts);
            if (parts.size() > 1) {
                m_expandedDirs.insert(parts.first());
            }
        }
    }
    rebuildFlat();
}

void WorkspaceTreeListModel::toggleExpanded(const QString& id) {
    if (id.trimmed().isEmpty()) return;
    if (m_expandedDirs.contains(id)) {
        m_expandedDirs.remove(id);
    } else {
        m_expandedDirs.insert(id);
    }
    rebuildFlat();
}

void WorkspaceTreeListModel::expandToPath(const QString& path) {
    QString relative = keyForPath(m_workspaceRoot, path);
    relative = QDir::fromNativeSeparators(relative);
    const auto parts = relative.split('/', Qt::SkipEmptyParts);
    QString running;
    for (int i = 0; i + 1 < parts.size(); ++i) {
        running = running.isEmpty() ? parts[i] : running + '/' + parts[i];
        m_expandedDirs.insert(running);
    }
    rebuildFlat();
}

int WorkspaceTreeListModel::itemCount() const {
    return static_cast<int>(m_flat.size());
}

void WorkspaceTreeListModel::rebuildFlat() {
    beginResetModel();
    m_flat.clear();

    TreeNode root;
    root.directory = true;
    root.id = QStringLiteral("/");

    for (const auto& file : m_files) {
        const QString rel = QDir::fromNativeSeparators(file.relativePath);
        const auto parts = rel.split('/', Qt::SkipEmptyParts);
        if (parts.isEmpty()) continue;

        TreeNode* parent = &root;
        QString running;
        for (int i = 0; i < parts.size(); ++i) {
            const QString part = parts[i];
            running = running.isEmpty() ? part : running + '/' + part;
            const bool directory = i < parts.size() - 1;
            auto it = parent->children.find(part);
            if (it == parent->children.end()) {
                auto node = std::make_shared<TreeNode>();
                node->id = running;
                node->relativePath = running;
                node->name = part;
                node->directory = directory;
                node->path = directory
                    ? QDir(m_workspaceRoot).filePath(running)
                    : file.path;
                it = parent->children.insert(part, node);
            } else if (!directory) {
                it.value()->path = file.path;
            }
            parent = it.value().get();
        }
    }

    std::function<void(const TreeNode&, int)> appendChildren = [&](const TreeNode& parent, int depth) {
        for (auto it = parent.children.cbegin(); it != parent.children.cend(); ++it) {
            const auto& node = *it.value();
            WorkspaceTreeItem item;
            item.id = node.id;
            item.path = node.path;
            item.relativePath = node.relativePath;
            item.name = node.name;
            item.depth = depth;
            item.directory = node.directory;
            item.hasChildren = !node.children.isEmpty();
            item.expanded = node.directory && m_expandedDirs.contains(node.id);
            m_flat.push_back(item);
            if (node.directory && item.expanded) {
                appendChildren(node, depth + 1);
            }
        }
    };

    appendChildren(root, 0);
    endResetModel();
}

} // namespace ide::ui::models
