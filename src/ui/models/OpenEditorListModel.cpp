#include "ui/models/OpenEditorListModel.hpp"

namespace ide::ui::models {

OpenEditorListModel::OpenEditorListModel(QObject* parent)
    : QAbstractListModel(parent) {}

int OpenEditorListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_editors.size());
}

QVariant OpenEditorListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_editors.size())) {
        return {};
    }
    const auto& item = m_editors[static_cast<std::size_t>(index.row())];
    switch (role) {
    case PathRole: return item.path;
    case TitleRole: return item.title;
    case DirtyRole: return item.dirty;
    case ActiveRole: return item.active;
    default: return {};
    }
}

QHash<int, QByteArray> OpenEditorListModel::roleNames() const {
    return {
        {PathRole, "path"},
        {TitleRole, "title"},
        {DirtyRole, "dirty"},
        {ActiveRole, "active"}
    };
}

void OpenEditorListModel::setEditors(std::vector<OpenEditorItem> editors) {
    beginResetModel();
    m_editors = std::move(editors);
    endResetModel();
}

int OpenEditorListModel::editorCount() const {
    return static_cast<int>(m_editors.size());
}

} // namespace ide::ui::models
