#pragma once

#include <QObject>
#include <QString>

namespace ide::adapters::modelhub {

struct DownloadRequest {
    QString repoId;
    QString filename;
    QString cacheDir;
    QString token;
};

class IFileDownloadBackend : public QObject {
    Q_OBJECT
public:
    explicit IFileDownloadBackend(QObject* parent = nullptr)
        : QObject(parent) {}
    ~IFileDownloadBackend() override = default;

    virtual QString backendName() const = 0;
    virtual bool isAvailable() const = 0;
    virtual bool isActive() const = 0;
    virtual void start(const DownloadRequest& request) = 0;
    virtual void cancel() = 0;

signals:
    void activeChanged();
    void progressChanged();
    void statusChanged();
    void diagnosticsChanged();
    void finishedSuccessfully(const QString& localPath);
    void failed(const QString& message);
};

} // namespace ide::adapters::modelhub
