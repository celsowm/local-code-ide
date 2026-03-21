
#pragma once

#include <QFile>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QNetworkReply>
#include <QString>
#include <QObject>

namespace ide::adapters::modelhub {

class HuggingFaceFileDownloader final : public QObject {
    Q_OBJECT
public:
    explicit HuggingFaceFileDownloader(QString token = {}, QObject* parent = nullptr);

    bool isActive() const;
    QString localPath() const;
    QString statusLine() const;
    qint64 receivedBytes() const;
    qint64 totalBytes() const;

    Q_INVOKABLE void startDownload(const QString& repoId, const QString& filename, const QString& localDir);
    Q_INVOKABLE void cancel();

signals:
    void activeChanged();
    void progressChanged();
    void statusChanged();
    void finishedSuccessfully(const QString& localPath);
    void failed(const QString& message);

private:
    void resetState();
    void setStatusLine(const QString& message);
    void finalizeSuccess();

    QString m_token;
    QString m_repoId;
    QString m_filename;
    QString m_localPath;
    QString m_partialPath;
    QString m_statusLine;
    qint64 m_receivedBytes = 0;
    qint64 m_totalBytes = 0;
    qint64 m_resumeOffset = 0;
    bool m_cancelRequested = false;
    QFile m_file;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
};

} // namespace ide::adapters::modelhub
