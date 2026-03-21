
#include "ui/models/ModelRepoListModel.hpp"

namespace ide::ui::models {

ModelRepoListModel::ModelRepoListModel(QObject* parent)
    : QAbstractListModel(parent) {}

int ModelRepoListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_repos.size());
}

QVariant ModelRepoListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_repos.size())) {
        return {};
    }

    const auto& repo = m_repos[static_cast<std::size_t>(index.row())];
    switch (role) {
    case IdRole:
        return repo.id;
    case NameRole:
        return repo.name;
    case PipelineTagRole:
        return repo.pipelineTag;
    case DownloadsRole:
        return repo.downloads;
    case LikesRole:
        return repo.likes;
    case LastModifiedRole:
        return repo.lastModified;
    case GatedRole:
        return repo.gated;
    case TagsRole:
        return repo.tags.join(", ");
    default:
        return {};
    }
}

QHash<int, QByteArray> ModelRepoListModel::roleNames() const {
    return {
        {IdRole, "repoId"},
        {NameRole, "name"},
        {PipelineTagRole, "pipelineTag"},
        {DownloadsRole, "downloads"},
        {LikesRole, "likes"},
        {LastModifiedRole, "lastModified"},
        {GatedRole, "gated"},
        {TagsRole, "tagsText"}
    };
}

void ModelRepoListModel::setRepos(std::vector<ide::services::interfaces::ModelRepoSummary> repos) {
    beginResetModel();
    m_repos = std::move(repos);
    endResetModel();
}

int ModelRepoListModel::repoCount() const {
    return static_cast<int>(m_repos.size());
}

ide::services::interfaces::ModelRepoSummary ModelRepoListModel::repoAt(int index) const {
    if (index < 0 || index >= static_cast<int>(m_repos.size())) {
        return {};
    }
    return m_repos[static_cast<std::size_t>(index)];
}

} // namespace ide::ui::models
