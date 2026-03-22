#include "ui/models/GitChangeListModel.hpp"

namespace ide::ui::models {

GitChangeListModel::GitChangeListModel(QObject* parent)
    : QAbstractListModel(parent) {}

int GitChangeListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_changes.size());
}

QVariant GitChangeListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_changes.size())) {
        return {};
    }
    const auto& item = m_changes[static_cast<std::size_t>(index.row())];
    switch (role) {
    case PathRole: return item.path;
    case FileNameRole: return item.fileName;
    case DirectoryRole: return item.directory;
    case StatusCodeRole: return item.statusCode;
    case StatusTextRole: return item.statusText;
    case ChangeKindRole: return item.changeKind;
    case StagedRole: return item.staged;
    case ModifiedRole: return item.modified;
    case RenamedRole: return item.renamed;
    case UntrackedRole: return item.untracked;
    case HasIndexChangesRole: return item.hasIndexChanges;
    case HasWorkingTreeChangesRole: return item.hasWorkingTreeChanges;
    default: return {};
    }
}

QHash<int, QByteArray> GitChangeListModel::roleNames() const {
    return {
        {PathRole, "path"},
        {FileNameRole, "fileName"},
        {DirectoryRole, "directory"},
        {StatusCodeRole, "statusCode"},
        {StatusTextRole, "statusText"},
        {ChangeKindRole, "changeKind"},
        {StagedRole, "staged"},
        {ModifiedRole, "modified"},
        {RenamedRole, "renamed"},
        {UntrackedRole, "untracked"},
        {HasIndexChangesRole, "hasIndexChanges"},
        {HasWorkingTreeChangesRole, "hasWorkingTreeChanges"}
    };
}

void GitChangeListModel::setChanges(std::vector<ide::services::interfaces::GitChange> changes) {
    beginResetModel();
    m_changes = std::move(changes);
    endResetModel();
}

int GitChangeListModel::changeCount() const {
    return static_cast<int>(m_changes.size());
}

int GitChangeListModel::stagedCount() const {
    int total = 0;
    for (const auto& c : m_changes) {
        if (c.staged) ++total;
    }
    return total;
}

int GitChangeListModel::unstagedCount() const {
    int total = 0;
    for (const auto& c : m_changes) {
        if (!c.untracked && c.hasWorkingTreeChanges) ++total;
    }
    return total;
}

int GitChangeListModel::untrackedCount() const {
    int total = 0;
    for (const auto& c : m_changes) {
        if (c.untracked) ++total;
    }
    return total;
}

} // namespace ide::ui::models
