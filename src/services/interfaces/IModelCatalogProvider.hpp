
#pragma once

#include <QString>
#include <QStringList>
#include <vector>

namespace ide::services::interfaces {

struct ModelRepoSummary {
    QString id;
    QString author;
    QString name;
    QString pipelineTag;
    QString lastModified;
    qint64 downloads = 0;
    qint64 likes = 0;
    bool gated = false;
    QStringList tags;
};

struct ModelFileEntry {
    QString path;
    QString name;
    qint64 sizeBytes = -1;
    bool isGguf = false;
    bool isSplit = false;
    QString quantization;
};

class IModelCatalogProvider {
public:
    virtual ~IModelCatalogProvider() = default;
    virtual std::vector<ModelRepoSummary> searchRepos(const QString& author, const QString& query, int limit) = 0;
    virtual std::vector<ModelFileEntry> listFiles(const QString& repoId) = 0;
    virtual QString name() const = 0;
    virtual QString lastError() const = 0;
};

} // namespace ide::services::interfaces
