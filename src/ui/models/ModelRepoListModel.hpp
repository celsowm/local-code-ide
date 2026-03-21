
#pragma once

#include "services/interfaces/IModelCatalogProvider.hpp"

#include <QAbstractListModel>
#include <vector>

namespace ide::ui::models {

class ModelRepoListModel final : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        PipelineTagRole,
        DownloadsRole,
        LikesRole,
        LastModifiedRole,
        GatedRole,
        TagsRole
    };

    explicit ModelRepoListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRepos(std::vector<ide::services::interfaces::ModelRepoSummary> repos);
    int repoCount() const;
    ide::services::interfaces::ModelRepoSummary repoAt(int index) const;

private:
    std::vector<ide::services::interfaces::ModelRepoSummary> m_repos;
};

} // namespace ide::ui::models
