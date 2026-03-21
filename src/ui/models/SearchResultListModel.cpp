#include "ui/models/SearchResultListModel.hpp"

namespace ide::ui::models {

SearchResultListModel::SearchResultListModel(QObject* parent)
    : QAbstractListModel(parent) {}

int SearchResultListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_results.size());
}

QVariant SearchResultListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_results.size())) {
        return {};
    }

    const auto& result = m_results[static_cast<std::size_t>(index.row())];
    switch (role) {
    case PathRole:
        return result.path;
    case LineRole:
        return result.line;
    case ColumnRole:
        return result.column;
    case PreviewRole:
        return result.preview;
    default:
        return {};
    }
}

QHash<int, QByteArray> SearchResultListModel::roleNames() const {
    return {
        {PathRole, "path"},
        {LineRole, "line"},
        {ColumnRole, "column"},
        {PreviewRole, "preview"}
    };
}

void SearchResultListModel::setResults(std::vector<ide::services::interfaces::SearchResult> results) {
    beginResetModel();
    m_results = std::move(results);
    endResetModel();
}

int SearchResultListModel::resultCount() const {
    return static_cast<int>(m_results.size());
}

} // namespace ide::ui::models
