#pragma once

#include "services/interfaces/ISearchProvider.hpp"

#include <memory>

namespace ide::services {

class SearchService {
public:
    explicit SearchService(std::unique_ptr<interfaces::ISearchProvider> provider);

    std::vector<interfaces::SearchResult> search(const QString& workspaceRoot, const QString& pattern);
    QString providerName() const;

private:
    std::unique_ptr<interfaces::ISearchProvider> m_provider;
};

} // namespace ide::services
