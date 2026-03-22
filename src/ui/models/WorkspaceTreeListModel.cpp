#include "ui/models/WorkspaceTreeListModel.hpp"

#include <QDir>
#include <QFileInfo>
#include <QStringList>

#include <functional>
#include <memory>
#include <algorithm>

namespace ide::ui::models {
namespace {
struct TreeNode {
    QString id;
    QString path;
    QString relativePath;
    QString name;
    bool directory = false;
    std::vector<std::shared_ptr<TreeNode>> children;
};

QString keyForPath(const QString& workspaceRoot, const QString& path) {
    QFileInfo info(path);
    if (info.isAbsolute() && !workspaceRoot.isEmpty()) {
        return QDir(workspaceRoot).relativeFilePath(path);
    }
    return path;
}

QString normalizedKey(const QString& text) {
    return QDir::fromNativeSeparators(text).trimmed();
}

QString qmlIconUrl(const QString& baseName) {
    return QStringLiteral("qrc:/qt/qml/LocalCodeIDE/qml/assets/icons/%1.svg").arg(baseName);
}

QString qmlFileTypeIconUrl(const QString& baseName) {
    return QStringLiteral("qrc:/qt/qml/LocalCodeIDE/qml/assets/icons/filetypes/%1.svg").arg(baseName);
}

bool startsWithAny(const QString& value, const QStringList& prefixes) {
    return std::any_of(prefixes.begin(), prefixes.end(), [&](const QString& prefix) {
        return value.startsWith(prefix);
    });
}

bool nodeLessThan(const std::shared_ptr<TreeNode>& lhs, const std::shared_ptr<TreeNode>& rhs) {
    if (lhs->directory != rhs->directory) {
        return lhs->directory && !rhs->directory;
    }
    return lhs->name.toLower() < rhs->name.toLower();
}

WorkspaceTreeListModel::GitBadgeInfo badgeInfoForChange(const ide::services::interfaces::GitChange& change) {
    if (change.untracked) {
        return {QStringLiteral("U"), QStringLiteral("#73c991"), 4};
    }
    if (change.renamed) {
        return {QStringLiteral("R"), QStringLiteral("#c586c0"), 3};
    }
    if (change.modified) {
        return {QStringLiteral("M"), QStringLiteral("#e2c08d"), 2};
    }
    if (change.staged) {
        return {QStringLiteral("S"), QStringLiteral("#4fc1ff"), 1};
    }
    if (!change.statusCode.trimmed().isEmpty()) {
        return {change.statusCode.left(1).toUpper(), QStringLiteral("#9cdcfe"), 0};
    }
    return {};
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
    case IconSourceRole: return item.iconSource;
    case GitBadgeRole: return item.gitBadge;
    case GitBadgeColorRole: return item.gitBadgeColor;
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
        {HasChildrenRole, "hasChildren"},
        {IconSourceRole, "iconSource"},
        {GitBadgeRole, "gitBadge"},
        {GitBadgeColorRole, "gitBadgeColor"}
    };
}

void WorkspaceTreeListModel::setWorkspaceRoot(const QString& workspaceRoot) {
    if (workspaceRoot == m_workspaceRoot) return;
    m_workspaceRoot = workspaceRoot;
    clearExpansionState();
    m_gitBadges.clear();
    rebuildFlat();
}

void WorkspaceTreeListModel::setFiles(const std::vector<ide::services::interfaces::WorkspaceFile>& files) {
    m_files = files;
    if (!m_autoExpandInitialized) {
        for (const auto& file : m_files) {
            const auto parts = QDir::fromNativeSeparators(file.relativePath).split('/', Qt::SkipEmptyParts);
            if (parts.size() > 1) {
                m_expandedDirs.insert(parts.first());
            }
        }
        m_autoExpandInitialized = true;
    }
    rebuildFlat();
}

void WorkspaceTreeListModel::setGitChanges(const std::vector<ide::services::interfaces::GitChange>& changes) {
    const auto previous = m_gitBadges;
    m_gitBadges.clear();
    for (const auto& change : changes) {
        const auto badge = badgeInfoForChange(change);
        if (badge.text.isEmpty()) {
            continue;
        }
        registerBadgeForPath(change.path, badge);
    }
    if (m_gitBadges == previous) {
        return;
    }
    rebuildFlat();
}

void WorkspaceTreeListModel::setCurrentFilePath(const QString& path) {
    const QString normalizedPath = normalizeRelativePath(path);
    if (normalizedPath == m_currentFilePath) {
        return;
    }
    m_currentFilePath = normalizedPath;
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

void WorkspaceTreeListModel::collapseAll() {
    if (m_expandedDirs.isEmpty()) {
        return;
    }
    m_expandedDirs.clear();
    rebuildFlat();
}

void WorkspaceTreeListModel::expandToPath(const QString& path) {
    QString relative = normalizeRelativePath(path);
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
            auto it = std::find_if(parent->children.begin(), parent->children.end(), [&](const auto& child) {
                return child->name == part;
            });
            if (it == parent->children.end()) {
                auto node = std::make_shared<TreeNode>();
                node->id = running;
                node->relativePath = running;
                node->name = part;
                node->directory = directory;
                node->path = directory
                    ? QDir(m_workspaceRoot).filePath(running)
                    : file.path;
                parent->children.push_back(node);
                it = std::prev(parent->children.end());
            } else if (!directory) {
                (*it)->path = file.path;
            }
            parent = (*it).get();
        }
    }

    std::function<void(const TreeNode&, int)> appendChildren = [&](const TreeNode& parent, int depth) {
        std::vector<std::shared_ptr<TreeNode>> children = parent.children;
        std::sort(children.begin(), children.end(), nodeLessThan);
        for (const auto& child : children) {
            const auto& node = *child;
            WorkspaceTreeItem item;
            item.id = node.id;
            item.path = node.path;
            item.relativePath = node.relativePath;
            item.name = node.name;
            item.depth = depth;
            item.directory = node.directory;
            item.hasChildren = !node.children.empty();
            item.expanded = node.directory && m_expandedDirs.contains(node.id);
            item.iconSource = iconSourceForNode(item);
            const auto badgeIt = m_gitBadges.constFind(node.relativePath);
            if (badgeIt != m_gitBadges.cend()) {
                item.gitBadge = badgeIt->text;
                item.gitBadgeColor = badgeIt->color;
            }
            m_flat.push_back(item);
            if (node.directory && item.expanded) {
                appendChildren(node, depth + 1);
            }
        }
    };

    appendChildren(root, 0);
    endResetModel();
}

void WorkspaceTreeListModel::clearExpansionState() {
    m_expandedDirs.clear();
    m_autoExpandInitialized = false;
}

void WorkspaceTreeListModel::registerBadgeForPath(const QString& relativePath, const GitBadgeInfo& badge) {
    QString normalizedPath = normalizeRelativePath(relativePath);
    if (normalizedPath.isEmpty()) {
        return;
    }

    auto registerSingle = [&](const QString& key) {
        auto it = m_gitBadges.find(key);
        if (it == m_gitBadges.end() || badge.priority > it->priority) {
            m_gitBadges.insert(key, badge);
        }
    };

    registerSingle(normalizedPath);

    const auto parts = normalizedPath.split('/', Qt::SkipEmptyParts);
    QString running;
    for (int i = 0; i + 1 < parts.size(); ++i) {
        running = running.isEmpty() ? parts[i] : running + '/' + parts[i];
        registerSingle(running);
    }
}

QString WorkspaceTreeListModel::normalizeRelativePath(const QString& path) const {
    QString relative = keyForPath(m_workspaceRoot, path);
    return normalizedKey(relative);
}

QString WorkspaceTreeListModel::iconSourceForNode(const WorkspaceTreeItem& item) const {
    const QString lowerName = item.name.toLower();

    if (item.directory) {
        if (lowerName == QStringLiteral("src")) {
            return qmlFileTypeIconUrl(QStringLiteral("folder-src"));
        }
        if (lowerName == QStringLiteral("scripts") || lowerName == QStringLiteral("script")) {
            return qmlFileTypeIconUrl(QStringLiteral("folder-scripts"));
        }
        if (lowerName == QStringLiteral("assets") || lowerName == QStringLiteral("images")
            || lowerName == QStringLiteral("icons") || lowerName == QStringLiteral("img")) {
            return qmlFileTypeIconUrl(QStringLiteral("folder-images"));
        }
        if (lowerName == QStringLiteral("json")) {
            return qmlFileTypeIconUrl(QStringLiteral("folder-json"));
        }
        return qmlIconUrl(QStringLiteral("folder"));
    }

    if (lowerName == QStringLiteral("cmakelists.txt") || lowerName == QStringLiteral("cmakecache.txt")) {
        return qmlFileTypeIconUrl(QStringLiteral("cmake"));
    }
    if (lowerName == QStringLiteral("cargo.toml")) {
        return qmlFileTypeIconUrl(QStringLiteral("rust"));
    }
    if (lowerName == QStringLiteral("readme") || lowerName == QStringLiteral("readme.md")
        || lowerName == QStringLiteral("readme.rst") || lowerName == QStringLiteral("readme.txt")) {
        return qmlFileTypeIconUrl(QStringLiteral("readme"));
    }
    if (lowerName == QStringLiteral("license") || lowerName == QStringLiteral("license.md")
        || lowerName == QStringLiteral("license.txt") || lowerName == QStringLiteral("copying")) {
        return qmlFileTypeIconUrl(QStringLiteral("license"));
    }
    if (startsWithAny(lowerName, {QStringLiteral(".git"), QStringLiteral("gitignore"), QStringLiteral("gitattributes")})) {
        return qmlFileTypeIconUrl(QStringLiteral("git"));
    }
    if (lowerName == QStringLiteral("package.json") || lowerName == QStringLiteral("package-lock.json")
        || lowerName == QStringLiteral("composer.json")) {
        return qmlFileTypeIconUrl(QStringLiteral("json"));
    }
    if (lowerName.startsWith(QStringLiteral("tsconfig")) && lowerName.endsWith(QStringLiteral(".json"))) {
        return qmlFileTypeIconUrl(QStringLiteral("javascript"));
    }

    const QFileInfo info(item.name);
    const QString suffix = info.suffix().toLower();
    const QString completeSuffix = info.completeSuffix().toLower();

    if (completeSuffix == QStringLiteral("d.ts")) {
        return qmlFileTypeIconUrl(QStringLiteral("javascript"));
    }
    if (suffix == QStringLiteral("rs") || suffix == QStringLiteral("ron")) {
        return qmlFileTypeIconUrl(QStringLiteral("rust"));
    }
    if (suffix == QStringLiteral("c") || suffix == QStringLiteral("i") || suffix == QStringLiteral("mi")) {
        return qmlFileTypeIconUrl(QStringLiteral("c"));
    }
    if (suffix == QStringLiteral("h")) {
        return qmlFileTypeIconUrl(QStringLiteral("h"));
    }
    if (suffix == QStringLiteral("cpp") || suffix == QStringLiteral("cxx") || suffix == QStringLiteral("cc")
        || suffix == QStringLiteral("c++") || suffix == QStringLiteral("cp") || suffix == QStringLiteral("hpp")
        || suffix == QStringLiteral("hxx") || suffix == QStringLiteral("hh") || suffix == QStringLiteral("h++")) {
        return qmlFileTypeIconUrl(QStringLiteral("cpp"));
    }
    if (suffix == QStringLiteral("py")) {
        return qmlFileTypeIconUrl(QStringLiteral("python"));
    }
    if (suffix == QStringLiteral("js") || suffix == QStringLiteral("mjs") || suffix == QStringLiteral("cjs")
        || suffix == QStringLiteral("ts") || suffix == QStringLiteral("cts") || suffix == QStringLiteral("mts")) {
        return qmlFileTypeIconUrl(QStringLiteral("javascript"));
    }
    if (suffix == QStringLiteral("jsx")) {
        return qmlFileTypeIconUrl(QStringLiteral("react"));
    }
    if (suffix == QStringLiteral("tsx")) {
        return qmlFileTypeIconUrl(QStringLiteral("react_ts"));
    }
    if (suffix == QStringLiteral("json") || suffix == QStringLiteral("jsonc") || suffix == QStringLiteral("json5")) {
        return qmlFileTypeIconUrl(QStringLiteral("json"));
    }
    if (suffix == QStringLiteral("md") || suffix == QStringLiteral("markdown") || suffix == QStringLiteral("rst")) {
        return qmlFileTypeIconUrl(QStringLiteral("markdown"));
    }
    if (suffix == QStringLiteral("ps1") || suffix == QStringLiteral("psm1") || suffix == QStringLiteral("psd1")
        || suffix == QStringLiteral("ps1xml") || suffix == QStringLiteral("psc1") || suffix == QStringLiteral("pssc")) {
        return qmlFileTypeIconUrl(QStringLiteral("powershell"));
    }
    if (suffix == QStringLiteral("cmake")) {
        return qmlFileTypeIconUrl(QStringLiteral("cmake"));
    }
    if (suffix == QStringLiteral("toml") || suffix == QStringLiteral("yaml") || suffix == QStringLiteral("yml")
        || suffix == QStringLiteral("qml") || suffix == QStringLiteral("conf") || suffix == QStringLiteral("ini")) {
        return qmlFileTypeIconUrl(QStringLiteral("settings"));
    }

    return qmlIconUrl(QStringLiteral("file"));
}

} // namespace ide::ui::models
