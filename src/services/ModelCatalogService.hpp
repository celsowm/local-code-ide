
#pragma once

#include "services/interfaces/IModelCatalogProvider.hpp"

#include <memory>
#include <vector>

namespace ide::services {

class ModelCatalogService {
public:
    explicit ModelCatalogService(std::unique_ptr<interfaces::IModelCatalogProvider> provider);

    std::vector<interfaces::ModelRepoSummary> searchRepos(const QString& author, const QString& query, int limit);
    std::vector<interfaces::ModelFileEntry> listFiles(const QString& repoId);
    QString providerName() const;
    QString lastError() const;

private:
    std::unique_ptr<interfaces::IModelCatalogProvider> m_provider;
};

} // namespace ide::services
