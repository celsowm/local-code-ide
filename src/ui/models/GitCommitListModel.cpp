#include "ui/models/GitCommitListModel.hpp"

namespace ide::ui::models {

GitCommitListModel::GitCommitListModel(QObject* parent)
    : QAbstractListModel(parent) {}

int GitCommitListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_commits.size());
}

QVariant GitCommitListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_commits.size())) {
        return {};
    }

    const auto& item = m_commits[static_cast<std::size_t>(index.row())];
    switch (role) {
    case HashRole: return item.hash;
    case ShortHashRole: return item.shortHash;
    case SubjectRole: return item.subject;
    case AuthorRole: return item.author;
    case RelativeTimeRole: return item.relativeTime;
    case HeadRole: return item.isHead;
    case RefLabelRole: return item.refLabel;
    default: return {};
    }
}

QHash<int, QByteArray> GitCommitListModel::roleNames() const {
    return {
        {HashRole, "hash"},
        {ShortHashRole, "shortHash"},
        {SubjectRole, "subject"},
        {AuthorRole, "author"},
        {RelativeTimeRole, "relativeTime"},
        {HeadRole, "head"},
        {RefLabelRole, "refLabel"}
    };
}

void GitCommitListModel::setCommits(std::vector<ide::services::interfaces::GitCommitSummary> commits) {
    beginResetModel();
    m_commits = std::move(commits);
    endResetModel();
}

int GitCommitListModel::commitCount() const {
    return static_cast<int>(m_commits.size());
}

} // namespace ide::ui::models
