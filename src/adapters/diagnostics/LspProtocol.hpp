#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class QProcess;
class QJsonObject;

namespace ide::adapters::diagnostics {

class LspProtocol final : public QObject {
    Q_OBJECT
public:
    explicit LspProtocol(const QString& program, const QStringList& args, QObject* parent = nullptr);
    ~LspProtocol() override;

    void start();
    void stop();
    bool isRunning() const;
    bool isReady() const;
    void markReady();
    QString statusLine() const;
    QString program() const;
    QProcess& process();
    int nextRequestId();

signals:
    void started();
    void stopped();
    void statusChanged(const QString& statusLine);

private:
    QString m_program;
    QStringList m_args;
    QProcess* m_process = nullptr;
    bool m_initialized = false;
    int m_nextId = 1;
};

} // namespace ide::adapters::diagnostics
