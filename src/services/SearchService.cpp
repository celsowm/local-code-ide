#include "services/SearchService.hpp"

namespace ide::services {

SearchService::SearchService(std::unique_ptr<interfaces::ISearchProvider> provider)
    : m_provider(std::move(provider)) {}

std::vector<interfaces::SearchResult> SearchService::search(const QString& workspaceRoot, const QString& pattern) {
    return m_provider ? m_provider->search(workspaceRoot, pattern) : std::vector<interfaces::SearchResult>{};
}

QString SearchService::providerName() const {
    return m_provider ? m_provider->name() : "No Search Provider";
}

} // namespace ide::services
