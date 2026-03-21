#pragma once

#include "services/interfaces/ICodeIntelProvider.hpp"

#include <QAbstractListModel>
#include <vector>

namespace ide::ui::models {

class CompletionListModel final : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        LabelRole = Qt::UserRole + 1,
        DetailRole,
        InsertTextRole,
        KindRole
    };

    explicit CompletionListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setItems(std::vector<ide::services::interfaces::CompletionItem> items);
    int itemCount() const;

private:
    std::vector<ide::services::interfaces::CompletionItem> m_items;
};

} // namespace ide::ui::models
