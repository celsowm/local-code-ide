#include "ui/models/ScmSectionListModel.hpp"

namespace ide::ui::models {

ScmSectionListModel::ScmSectionListModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_sections{{
        {QStringLiteral("staged"), QStringLiteral("Staged Changes"), QStringLiteral("No staged changes."), GitChangeListModel(this), false},
        {QStringLiteral("changes"), QStringLiteral("Changes"), QStringLiteral("No tracked changes."), GitChangeListModel(this), false},
        {QStringLiteral("untracked"), QStringLiteral("Untracked"), QStringLiteral("No untracked files."), GitChangeListModel(this), false}
    }} {}

int ScmSectionListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_visibleSectionIndexes.size());
}

QVariant ScmSectionListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_visibleSectionIndexes.size())) {
        return {};
    }

    const auto& section = m_sections[static_cast<std::size_t>(m_visibleSectionIndexes[static_cast<std::size_t>(index.row())])];
    switch (role) {
    case KeyRole: return section.key;
    case TitleRole: return section.title;
    case CountRole: return section.model.changeCount();
    case ModelRole: return QVariant::fromValue(static_cast<QObject*>(const_cast<GitChangeListModel*>(&section.model)));
    case EmptyTextRole: return section.emptyText;
    default: return {};
    }
}

QHash<int, QByteArray> ScmSectionListModel::roleNames() const {
    return {
        {KeyRole, "sectionKey"},
        {TitleRole, "sectionTitle"},
        {CountRole, "sectionCount"},
        {ModelRole, "sectionEntriesModel"},
        {EmptyTextRole, "emptyText"}
    };
}

void ScmSectionListModel::setChanges(const std::vector<ide::services::interfaces::GitChange>& changes) {
    std::vector<ide::services::interfaces::GitChange> staged;
    std::vector<ide::services::interfaces::GitChange> tracked;
    std::vector<ide::services::interfaces::GitChange> untracked;

    for (const auto& change : changes) {
        if (change.untracked) {
            untracked.push_back(change);
        }
        if (change.staged) {
            staged.push_back(change);
        }
        if (!change.untracked && (change.hasWorkingTreeChanges || (!change.staged && change.modified))) {
            tracked.push_back(change);
        }
    }

    beginResetModel();
    m_sections[0].model.setChanges(std::move(staged));
    m_sections[1].model.setChanges(std::move(tracked));
    m_sections[2].model.setChanges(std::move(untracked));
    m_sections[0].visible = m_sections[0].model.changeCount() > 0;
    m_sections[1].visible = m_sections[1].model.changeCount() > 0;
    m_sections[2].visible = m_sections[2].model.changeCount() > 0;
    refreshVisibleOrder();
    endResetModel();
}

int ScmSectionListModel::totalVisibleSectionCount() const {
    return static_cast<int>(m_visibleSectionIndexes.size());
}

int ScmSectionListModel::stagedCount() const {
    return m_sections[0].model.changeCount();
}

int ScmSectionListModel::unstagedCount() const {
    return m_sections[1].model.changeCount();
}

int ScmSectionListModel::untrackedCount() const {
    return m_sections[2].model.changeCount();
}

void ScmSectionListModel::refreshVisibleOrder() {
    m_visibleSectionIndexes.clear();
    for (int index = 0; index < static_cast<int>(m_sections.size()); ++index) {
        if (m_sections[static_cast<std::size_t>(index)].visible) {
            m_visibleSectionIndexes.push_back(index);
        }
    }
}

} // namespace ide::ui::models
