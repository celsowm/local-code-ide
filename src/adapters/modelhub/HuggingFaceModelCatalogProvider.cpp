
#include "adapters/modelhub/HuggingFaceModelCatalogProvider.hpp"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrlQuery>
#include <algorithm>

namespace ide::adapters::modelhub {

namespace {
QString repoNameFromId(const QString& id) {
    const int slash = id.indexOf('/');
    return slash >= 0 ? id.mid(slash + 1) : id;
}
}

HuggingFaceModelCatalogProvider::HuggingFaceModelCatalogProvider(QString token)
    : m_token(std::move(token)) {}

std::vector<ide::services::interfaces::ModelRepoSummary> HuggingFaceModelCatalogProvider::searchRepos(const QString& author, const QString& query, int limit) {
    QUrl url(QStringLiteral("https://huggingface.co/api/models"));
    QUrlQuery urlQuery;
    if (!author.trimmed().isEmpty()) {
        urlQuery.addQueryItem(QStringLiteral("author"), author.trimmed());
    }
    if (!query.trimmed().isEmpty()) {
        urlQuery.addQueryItem(QStringLiteral("search"), query.trimmed());
    }
    urlQuery.addQueryItem(QStringLiteral("limit"), QString::number(qMax(1, limit)));
    urlQuery.addQueryItem(QStringLiteral("full"), QStringLiteral("true"));
    url.setQuery(urlQuery);

    const auto payload = getJson(url);
    std::vector<ide::services::interfaces::ModelRepoSummary> repos;
    if (payload.isEmpty()) {
        return repos;
    }

    const auto root = QJsonDocument::fromJson(payload);
    if (!root.isArray()) {
        setLastError(QStringLiteral("Resposta inesperada ao buscar modelos."));
        return repos;
    }

    for (const auto& value : root.array()) {
        if (!value.isObject()) {
            continue;
        }

        const QJsonObject object = value.toObject();
        ide::services::interfaces::ModelRepoSummary item;
        item.id = object.value(QStringLiteral("id")).toString();
        item.author = author;
        item.name = repoNameFromId(item.id);
        item.pipelineTag = object.value(QStringLiteral("pipeline_tag")).toString();
        item.lastModified = object.value(QStringLiteral("lastModified")).toString();
        item.downloads = object.value(QStringLiteral("downloads")).toInteger();
        item.likes = object.value(QStringLiteral("likes")).toInteger();
        item.gated = object.value(QStringLiteral("gated")).toBool(false);

        const auto tags = object.value(QStringLiteral("tags")).toArray();
        for (const auto& tagValue : tags) {
            const QString tag = tagValue.toString();
            if (!tag.isEmpty()) {
                item.tags.push_back(tag);
            }
        }

        repos.push_back(std::move(item));
    }

    std::sort(repos.begin(), repos.end(), [](const auto& left, const auto& right) {
        if (left.downloads != right.downloads) {
            return left.downloads > right.downloads;
        }
        return left.id < right.id;
    });

    setLastError({});
    return repos;
}

std::vector<ide::services::interfaces::ModelFileEntry> HuggingFaceModelCatalogProvider::listFiles(const QString& repoId) {
    const QUrl url(QStringLiteral("https://huggingface.co/api/models/%1").arg(repoId));
    const auto payload = getJson(url);
    std::vector<ide::services::interfaces::ModelFileEntry> files;
    if (payload.isEmpty()) {
        return files;
    }

    const auto root = QJsonDocument::fromJson(payload);
    if (!root.isObject()) {
        setLastError(QStringLiteral("Resposta inesperada ao listar arquivos do repositório."));
        return files;
    }

    const auto siblings = root.object().value(QStringLiteral("siblings")).toArray();
    for (const auto& siblingValue : siblings) {
        if (!siblingValue.isObject()) {
            continue;
        }

        const auto sibling = siblingValue.toObject();
        ide::services::interfaces::ModelFileEntry entry;
        entry.path = sibling.value(QStringLiteral("rfilename")).toString();
        entry.name = entry.path.section('/', -1);
        entry.sizeBytes = sibling.value(QStringLiteral("size")).toInteger(-1);
        entry.isGguf = entry.path.endsWith(QStringLiteral(".gguf"), Qt::CaseInsensitive);
        entry.isSplit = entry.name.contains(QStringLiteral("-of-"), Qt::CaseInsensitive);
        entry.quantization = detectQuantization(entry.name);

        if (entry.isGguf) {
            files.push_back(std::move(entry));
        }
    }

    std::sort(files.begin(), files.end(), [](const auto& left, const auto& right) {
        if (left.sizeBytes > 0 && right.sizeBytes > 0 && left.sizeBytes != right.sizeBytes) {
            return left.sizeBytes < right.sizeBytes;
        }
        return left.name < right.name;
    });

    setLastError({});
    return files;
}

QString HuggingFaceModelCatalogProvider::name() const {
    return QStringLiteral("Hugging Face Hub API");
}

QString HuggingFaceModelCatalogProvider::lastError() const {
    return m_lastError;
}

QByteArray HuggingFaceModelCatalogProvider::getJson(const QUrl& url) {
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setRawHeader("Accept", "application/json");
    if (!m_token.trimmed().isEmpty()) {
        request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(m_token).toUtf8());
    }

    auto* reply = m_network.get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const auto error = reply->error();
    const QByteArray payload = reply->readAll();
    if (error != QNetworkReply::NoError) {
        setLastError(QStringLiteral("Falha HTTP: %1").arg(reply->errorString()));
        reply->deleteLater();
        return {};
    }

    reply->deleteLater();
    return payload;
}

QString HuggingFaceModelCatalogProvider::detectQuantization(const QString& name) const {
    static const std::vector<QRegularExpression> patterns = {
        QRegularExpression(QStringLiteral("(IQ\d(?:_[A-Z]+)?)"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral("(Q\d_K_[MSL])"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral("(Q\d_K)"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral("(Q8_0)"), QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(QStringLiteral("(Q\d_\d)"), QRegularExpression::CaseInsensitiveOption)
    };

    for (const auto& pattern : patterns) {
        const auto match = pattern.match(name);
        if (match.hasMatch()) {
            return match.captured(1).toUpper();
        }
    }
    return QStringLiteral("unknown");
}

void HuggingFaceModelCatalogProvider::setLastError(const QString& message) {
    m_lastError = message;
}

} // namespace ide::adapters::modelhub
