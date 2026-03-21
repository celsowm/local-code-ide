
#include "services/ModelCatalogService.hpp"

namespace ide::services {

ModelCatalogService::ModelCatalogService(std::unique_ptr<interfaces::IModelCatalogProvider> provider)
    : m_provider(std::move(provider)) {}

std::vector<interfaces::ModelRepoSummary> ModelCatalogService::searchRepos(const QString& author, const QString& query, int limit) {
    return m_provider->searchRepos(author, query, limit);
}

std::vector<interfaces::ModelFileEntry> ModelCatalogService::listFiles(const QString& repoId) {
    return m_provider->listFiles(repoId);
}

QString ModelCatalogService::providerName() const {
    return m_provider->name();
}

QString ModelCatalogService::lastError() const {
    return m_provider->lastError();
}

} // namespace ide::services
