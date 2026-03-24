#pragma once

#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <functional>

namespace ide::adapters::diagnostics {

class LspTransport;

class LspDocumentManager final : public QObject {
    Q_OBJECT
public:
    explicit LspDocumentManager(LspTransport& transport,
                                std::function<QString(const QString&)> languageIdResolver = {},
                                QObject* parent = nullptr);

    int publish(const QString& filePath, const QString& text);
    bool isOpen(const QString& filePath) const;
    int version(const QString& filePath) const;
    void flushPending();
    void reset();

signals:
    void documentOpened(const QString& filePath);

private:
    struct PendingOpen {
        QString filePath;
        QString text;
    };

    void sendDidOpen(const QString& filePath, const QString& text, int version);
    void sendDidChange(const QString& filePath, const QString& text, int version);
    QString uriForPath(const QString& filePath) const;
    QString languageIdForPath(const QString& filePath) const;

    LspTransport& m_transport;
    std::function<QString(const QString&)> m_languageIdResolver;
    QSet<QString> m_openUris;
    QHash<QString, int> m_versionsByUri;
    QHash<QString, PendingOpen> m_pendingOpensByUri;
};

} // namespace ide::adapters::diagnostics
