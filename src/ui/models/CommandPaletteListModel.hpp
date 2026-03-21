#pragma once

#include <QAbstractListModel>
#include <QString>
#include <vector>

namespace ide::ui::models {

struct CommandPaletteItem {
    QString id;
    QString title;
    QString category;
    QString hint;
};

class CommandPaletteListModel final : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        CategoryRole,
        HintRole
    };

    explicit CommandPaletteListModel(QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setItems(std::vector<CommandPaletteItem> items);
    int itemCount() const;

private:
    std::vector<CommandPaletteItem> m_items;
};

} // namespace ide::ui::models
