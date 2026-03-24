#include "adapters/diagnostics/LspTransport.hpp"

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QProcess>
#include <QTextStream>

#include <cstring>

namespace ide::adapters::diagnostics {

LspTransport::LspTransport(QProcess& process, QObject* parent)
    : QObject(parent)
    , m_process(process) {
    QObject::connect(&m_process, &QProcess::readyReadStandardOutput, this, &LspTransport::onReadyReadStandardOutput);
    QObject::connect(&m_process, &QProcess::readyReadStandardError, this, &LspTransport::onReadyReadStandardError);
}

void LspTransport::send(const QJsonObject& message) {
    if (m_process.state() != QProcess::Running) {
        return;
    }

    const QByteArray payload = QJsonDocument(message).toJson(QJsonDocument::Compact);
    const QByteArray header = "Content-Length: " + QByteArray::number(payload.size()) + "\r\n\r\n";
    m_process.write(header);
    m_process.write(payload);
    m_process.waitForBytesWritten(250);
}

void LspTransport::reset() {
    m_stdoutBuffer.clear();
    m_stderrBuffer.clear();
}

void LspTransport::onReadyReadStandardOutput() {
    m_stdoutBuffer.append(m_process.readAllStandardOutput());
    parseBufferedMessages();
}

void LspTransport::onReadyReadStandardError() {
    const QByteArray stderrData = m_process.readAllStandardError();
    m_stderrBuffer.append(stderrData);
    
    // Debug log stderr
    if (!stderrData.isEmpty()) {
        QFile logFile(QCoreApplication::applicationDirPath() + "/lsp_debug.log");
        logFile.open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream out(&logFile);
        out << "LspTransport::stderr:" << QString::fromUtf8(stderrData).left(500) << "\n";
        logFile.close();
    }
}

void LspTransport::parseBufferedMessages() {
    while (true) {
        const int headerEnd = m_stdoutBuffer.indexOf("\r\n\r\n");
        if (headerEnd < 0) {
            return;
        }

        const QByteArray headers = m_stdoutBuffer.left(headerEnd);
        int contentLength = -1;
        for (const QByteArray& line : headers.split('\n')) {
            const QByteArray trimmed = line.trimmed();
            if (trimmed.startsWith("Content-Length:")) {
                contentLength = trimmed.mid(static_cast<int>(strlen("Content-Length:"))).trimmed().toInt();
            }
        }

        if (contentLength < 0) {
            m_stdoutBuffer.remove(0, headerEnd + 4);
            continue;
        }

        const int totalSize = headerEnd + 4 + contentLength;
        if (m_stdoutBuffer.size() < totalSize) {
            return;
        }

        const QByteArray payload = m_stdoutBuffer.mid(headerEnd + 4, contentLength);
        m_stdoutBuffer.remove(0, totalSize);

        const QJsonDocument json = QJsonDocument::fromJson(payload);
        if (json.isObject()) {
            // Debug log received message
            QFile logFile(QCoreApplication::applicationDirPath() + "/lsp_debug.log");
            logFile.open(QIODevice::WriteOnly | QIODevice::Append);
            QTextStream out(&logFile);
            out << "LspTransport::received:" << QString::fromUtf8(payload).left(200) << "\n";
            logFile.close();
            
            emit messageReceived(json.object());
        }
    }
}

} // namespace ide::adapters::diagnostics
