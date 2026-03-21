
#pragma once

#include "services/interfaces/IModelCatalogProvider.hpp"

#include <QNetworkAccessManager>
#include <QString>

namespace ide::adapters::modelhub {

class HuggingFaceModelCatalogProvider final : public ide::services::interfaces::IModelCatalogProvider {
public:
    explicit HuggingFaceModelCatalogProvider(QString token = {});

    std::vector<ide::services::interfaces::ModelRepoSummary> searchRepos(const QString& author, const QString& query, int limit) override;
    std::vector<ide::services::interfaces::ModelFileEntry> listFiles(const QString& repoId) override;
    QString name() const override;
    QString lastError() const override;

private:
    QByteArray getJson(const QUrl& url);
    QString detectQuantization(const QString& name) const;
    void setLastError(const QString& message);

    QString m_token;
    QString m_lastError;
    QNetworkAccessManager m_network;
};

} // namespace ide::adapters::modelhub
