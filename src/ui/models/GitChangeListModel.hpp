#pragma once

#include "services/interfaces/IVersionControlProvider.hpp"

#include <QAbstractListModel>
#include <vector>

namespace ide::ui::models {

class GitChangeListModel final : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        PathRole = Qt::UserRole + 1,
        FileNameRole,
        DirectoryRole,
        StatusCodeRole,
        StatusTextRole,
        ChangeKindRole,
        StagedRole,
        ModifiedRole,
        RenamedRole,
        UntrackedRole,
        HasIndexChangesRole,
        HasWorkingTreeChangesRole
    };

    explicit GitChangeListModel(QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setChanges(std::vector<ide::services::interfaces::GitChange> changes);
    int changeCount() const;
    int stagedCount() const;
    int unstagedCount() const;
    int untrackedCount() const;

private:
    std::vector<ide::services::interfaces::GitChange> m_changes;
};

} // namespace ide::ui::models
