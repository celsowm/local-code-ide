#pragma once

#include "services/interfaces/ISearchProvider.hpp"

namespace ide::adapters::search {

class FileSystemSearchProvider final : public ide::services::interfaces::ISearchProvider {
public:
    std::vector<ide::services::interfaces::SearchResult> search(const QString& workspaceRoot,
                                                                const QString& pattern) override;
    QString name() const override;
};

} // namespace ide::adapters::search
