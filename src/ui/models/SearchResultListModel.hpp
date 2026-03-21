#pragma once

#include "services/interfaces/ISearchProvider.hpp"

#include <QAbstractListModel>
#include <vector>

namespace ide::ui::models {

class SearchResultListModel final : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        PathRole = Qt::UserRole + 1,
        LineRole,
        ColumnRole,
        PreviewRole
    };

    explicit SearchResultListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setResults(std::vector<ide::services::interfaces::SearchResult> results);
    int resultCount() const;

private:
    std::vector<ide::services::interfaces::SearchResult> m_results;
};

} // namespace ide::ui::models
