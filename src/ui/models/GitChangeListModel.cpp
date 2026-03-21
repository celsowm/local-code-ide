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
    case StatusCodeRole: return item.statusCode;
    case StatusTextRole: return item.statusText;
    case StagedRole: return item.staged;
    case ModifiedRole: return item.modified;
    case RenamedRole: return item.renamed;
    case UntrackedRole: return item.untracked;
    default: return {};
    }
}

QHash<int, QByteArray> GitChangeListModel::roleNames() const {
    return {
        {PathRole, "path"},
        {StatusCodeRole, "statusCode"},
        {StatusTextRole, "statusText"},
        {StagedRole, "staged"},
        {ModifiedRole, "modified"},
        {RenamedRole, "renamed"},
        {UntrackedRole, "untracked"}
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

} // namespace ide::ui::models
