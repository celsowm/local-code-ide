#include "ui/models/CommandPaletteListModel.hpp"

namespace ide::ui::models {

CommandPaletteListModel::CommandPaletteListModel(QObject* parent)
    : QAbstractListModel(parent) {}

int CommandPaletteListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant CommandPaletteListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_items.size())) {
        return {};
    }
    const auto& item = m_items[static_cast<std::size_t>(index.row())];
    switch (role) {
    case IdRole: return item.id;
    case TitleRole: return item.title;
    case CategoryRole: return item.category;
    case HintRole: return item.hint;
    default: return {};
    }
}

QHash<int, QByteArray> CommandPaletteListModel::roleNames() const {
    return {
        {IdRole, "commandId"},
        {TitleRole, "title"},
        {CategoryRole, "category"},
        {HintRole, "hint"}
    };
}

void CommandPaletteListModel::setItems(std::vector<CommandPaletteItem> items) {
    beginResetModel();
    m_items = std::move(items);
    endResetModel();
}

int CommandPaletteListModel::itemCount() const {
    return static_cast<int>(m_items.size());
}

} // namespace ide::ui::models
