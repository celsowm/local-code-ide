#pragma once

#include <QAbstractListModel>
#include <QString>
#include <vector>

namespace ide::ui::models {

struct OpenEditorItem {
    QString path;
    QString title;
    bool dirty = false;
    bool active = false;
};

class OpenEditorListModel final : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        PathRole = Qt::UserRole + 1,
        TitleRole,
        DirtyRole,
        ActiveRole
    };

    explicit OpenEditorListModel(QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEditors(std::vector<OpenEditorItem> editors);
    int editorCount() const;

private:
    std::vector<OpenEditorItem> m_editors;
};

} // namespace ide::ui::models
