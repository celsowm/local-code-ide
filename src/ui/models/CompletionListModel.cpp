#include "ui/models/CompletionListModel.hpp"

namespace ide::ui::models {

CompletionListModel::CompletionListModel(QObject* parent)
    : QAbstractListModel(parent) {}

int CompletionListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant CompletionListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_items.size())) {
        return {};
    }

    const auto& item = m_items[static_cast<std::size_t>(index.row())];
    switch (role) {
    case LabelRole:
        return item.label;
    case DetailRole:
        return item.detail;
    case InsertTextRole:
        return item.insertText;
    case KindRole:
        return item.kind;
    default:
        return {};
    }
}

QHash<int, QByteArray> CompletionListModel::roleNames() const {
    return {
        {LabelRole, "label"},
        {DetailRole, "detail"},
        {InsertTextRole, "insertText"},
        {KindRole, "kind"}
    };
}

void CompletionListModel::setItems(std::vector<ide::services::interfaces::CompletionItem> items) {
    beginResetModel();
    m_items = std::move(items);
    endResetModel();
}

int CompletionListModel::itemCount() const {
    return static_cast<int>(m_items.size());
}

} // namespace ide::ui::models
