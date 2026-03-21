#pragma once

#include <QString>
#include <vector>

namespace ide::services::interfaces {

struct SearchResult {
    QString path;
    int line = 1;
    int column = 1;
    QString preview;
};

class ISearchProvider {
public:
    virtual ~ISearchProvider() = default;
    virtual std::vector<SearchResult> search(const QString& workspaceRoot, const QString& pattern) = 0;
    virtual QString name() const = 0;
};

} // namespace ide::services::interfaces
