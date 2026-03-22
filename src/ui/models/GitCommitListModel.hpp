#pragma once

#include "services/interfaces/IVersionControlProvider.hpp"

#include <QAbstractListModel>
#include <vector>

namespace ide::ui::models {

class GitCommitListModel final : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        HashRole = Qt::UserRole + 1,
        ShortHashRole,
        SubjectRole,
        AuthorRole,
        RelativeTimeRole,
        HeadRole,
        RefLabelRole
    };

    explicit GitCommitListModel(QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setCommits(std::vector<ide::services::interfaces::GitCommitSummary> commits);
    int commitCount() const;

private:
    std::vector<ide::services::interfaces::GitCommitSummary> m_commits;
};

} // namespace ide::ui::models
