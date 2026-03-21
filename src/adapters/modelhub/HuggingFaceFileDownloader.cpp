
#include "adapters/modelhub/HuggingFaceFileDownloader.hpp"

#include <QDir>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

namespace ide::adapters::modelhub {

HuggingFaceFileDownloader::HuggingFaceFileDownloader(QString token, QObject* parent)
    : QObject(parent)
    , m_token(std::move(token)) {}

bool HuggingFaceFileDownloader::isActive() const {
    return !m_reply.isNull();
}

QString HuggingFaceFileDownloader::localPath() const {
    return m_localPath;
}

QString HuggingFaceFileDownloader::statusLine() const {
    return m_statusLine;
}

qint64 HuggingFaceFileDownloader::receivedBytes() const {
    return m_receivedBytes;
}

qint64 HuggingFaceFileDownloader::totalBytes() const {
    return m_totalBytes;
}

void HuggingFaceFileDownloader::startDownload(const QString& repoId, const QString& filename, const QString& localDir) {
    if (repoId.trimmed().isEmpty() || filename.trimmed().isEmpty() || localDir.trimmed().isEmpty()) {
        setStatusLine(QStringLiteral("Preencha repositório, arquivo e pasta de destino."));
        emit failed(m_statusLine);
        return;
    }

    if (isActive()) {
        cancel();
    }

    m_cancelRequested = false;
    resetState();
    emit progressChanged();
    m_repoId = repoId.trimmed();
    m_filename = filename;
    m_localPath = QDir(localDir).filePath(filename);
    const QFileInfo targetInfo(m_localPath);
    QDir().mkpath(targetInfo.dir().absolutePath());
    m_partialPath = m_localPath + QStringLiteral(".part");
    m_resumeOffset = QFileInfo::exists(m_partialPath) ? QFileInfo(m_partialPath).size() : 0;

    m_file.setFileName(m_partialPath);
    if (!m_file.open(m_resumeOffset > 0 ? (QIODevice::WriteOnly | QIODevice::Append) : QIODevice::WriteOnly)) {
        setStatusLine(QStringLiteral("Não foi possível abrir o arquivo temporário para escrita."));
        emit failed(m_statusLine);
        return;
    }

    const QString encodedFilename = QString::fromUtf8(QUrl::toPercentEncoding(m_filename, "/"));
    QUrl url(QStringLiteral("https://huggingface.co/%1/resolve/main/%2").arg(m_repoId, encodedFilename));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("download"), QStringLiteral("true"));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    if (!m_token.trimmed().isEmpty()) {
        request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(m_token).toUtf8());
    }
    if (m_resumeOffset > 0) {
        request.setRawHeader("Range", QStringLiteral("bytes=%1-").arg(m_resumeOffset).toUtf8());
    }

    m_reply = m_network.get(request);
    emit activeChanged();
    setStatusLine(m_resumeOffset > 0
        ? QStringLiteral("Retomando download de %1").arg(m_filename)
        : QStringLiteral("Baixando %1").arg(m_filename));

    connect(m_reply, &QNetworkReply::metaDataChanged, this, [this]() {
        if (!m_reply || m_resumeOffset <= 0) {
            return;
        }
        const int statusCode = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 200) {
            m_resumeOffset = 0;
            if (m_file.isOpen()) {
                m_file.resize(0);
                m_file.seek(0);
            }
        }
    });

    connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        m_receivedBytes = m_resumeOffset + qMax<qint64>(0, received);
        m_totalBytes = total > 0 ? (m_resumeOffset + total) : 0;
        emit progressChanged();
    });

    connect(m_reply, &QIODevice::readyRead, this, [this]() {
        if (m_reply) {
            m_file.write(m_reply->readAll());
        }
    });

    connect(m_reply, &QNetworkReply::finished, this, [this]() {
        if (!m_reply) {
            return;
        }

        m_file.write(m_reply->readAll());
        m_file.flush();
        m_file.close();

        const auto error = m_reply->error();
        const QString errorText = m_reply->errorString();
        m_reply->deleteLater();
        m_reply = nullptr;
        emit activeChanged();

        if (error != QNetworkReply::NoError) {
            if (m_cancelRequested && error == QNetworkReply::OperationCanceledError) {
                setStatusLine(QStringLiteral("Download cancelado. O arquivo parcial foi mantido para retomada."));
                emit failed(m_statusLine);
                m_cancelRequested = false;
                return;
            }
            setStatusLine(QStringLiteral("Download falhou: %1").arg(errorText));
            emit failed(m_statusLine);
            return;
        }

        finalizeSuccess();
    });

    connect(m_reply, &QNetworkReply::errorOccurred, this, [this](QNetworkReply::NetworkError) {
        emit progressChanged();
    });
}

void HuggingFaceFileDownloader::cancel() {
    if (!isActive()) {
        return;
    }

    m_cancelRequested = true;
    m_reply->abort();
    if (m_file.isOpen()) {
        m_file.flush();
        m_file.close();
    }
    setStatusLine(QStringLiteral("Download cancelado. O arquivo parcial foi mantido para retomada."));
}

void HuggingFaceFileDownloader::resetState() {
    m_receivedBytes = 0;
    m_totalBytes = 0;
    m_resumeOffset = 0;
}

void HuggingFaceFileDownloader::setStatusLine(const QString& message) {
    m_statusLine = message;
    emit statusChanged();
}

void HuggingFaceFileDownloader::finalizeSuccess() {
    QFile::remove(m_localPath);
    if (!QFile::rename(m_partialPath, m_localPath)) {
        setStatusLine(QStringLiteral("Download concluído, mas falhou ao renomear o arquivo final."));
        emit failed(m_statusLine);
        return;
    }

    m_receivedBytes = QFileInfo(m_localPath).size();
    m_totalBytes = m_receivedBytes;
    emit progressChanged();
    setStatusLine(QStringLiteral("Download concluído: %1").arg(m_localPath));
    emit finishedSuccessfully(m_localPath);
}

} // namespace ide::adapters::modelhub
