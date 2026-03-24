#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QObject>

class QProcess;

namespace ide::adapters::diagnostics {

class LspTransport final : public QObject {
    Q_OBJECT
public:
    explicit LspTransport(QProcess& process, QObject* parent = nullptr);

    void send(const QJsonObject& message);
    void reset();

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();

signals:
    void messageReceived(const QJsonObject& message);

private:
    void parseBufferedMessages();

    QProcess& m_process;
    QByteArray m_stdoutBuffer;
    QByteArray m_stderrBuffer;
};

} // namespace ide::adapters::diagnostics
