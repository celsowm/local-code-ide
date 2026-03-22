#pragma once

#include "ui/models/GitChangeListModel.hpp"

#include <QAbstractListModel>
#include <array>

namespace ide::ui::models {

class ScmSectionListModel final : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        KeyRole = Qt::UserRole + 1,
        TitleRole,
        CountRole,
        ModelRole,
        EmptyTextRole
    };

    explicit ScmSectionListModel(QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setChanges(const std::vector<ide::services::interfaces::GitChange>& changes);
    int totalVisibleSectionCount() const;
    int stagedCount() const;
    int unstagedCount() const;
    int untrackedCount() const;

private:
    struct SectionState {
        QString key;
        QString title;
        QString emptyText;
        GitChangeListModel model;
        bool visible = false;
    };

    void refreshVisibleOrder();

    std::array<SectionState, 3> m_sections;
    std::vector<int> m_visibleSectionIndexes;
};

} // namespace ide::ui::models
