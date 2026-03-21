#include "services/LlamaServerProcessService.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QNetworkRequest>
#include <QUrl>

namespace ide::services {

LlamaServerProcessService::LlamaServerProcessService(QString baseUrl,
                                                     QString executablePath,
                                                     QString extraArguments,
                                                     QObject* parent)
    : QObject(parent)
    , m_executablePath(executablePath.trimmed().isEmpty() ? QStringLiteral("llama-server") : executablePath.trimmed())
    , m_extraArguments(std::move(extraArguments))
    , m_baseUrl(normalizeBaseUrl(std::move(baseUrl))) {
    m_process.setProcessChannelMode(QProcess::SeparateChannels);

    m_probeTimer.setInterval(2500);
    connect(&m_probeTimer, &QTimer::timeout, this, &LlamaServerProcessService::probeNow);

    connect(&m_process, &QProcess::started, this, [this]() {
        m_running = true;
        m_starting = false;
        emit runningChanged();
        emit startingChanged();
        setStatusLine(QStringLiteral("llama-server iniciado. Aguardando health check..."));
        m_probeTimer.start();
        probeNow();
    });

    connect(&m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        appendLog(QString::fromLocal8Bit(m_process.readAllStandardOutput()));
    });

    connect(&m_process, &QProcess::readyReadStandardError, this, [this]() {
        appendLog(QString::fromLocal8Bit(m_process.readAllStandardError()));
    });

    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        m_running = false;
        m_starting = false;
        emit runningChanged();
        emit startingChanged();
        setHealthy(false);

        QString message;
        switch (error) {
        case QProcess::FailedToStart:
            message = QStringLiteral("Falha ao iniciar llama-server. Confira o executável.");
            break;
        case QProcess::Crashed:
            message = QStringLiteral("llama-server caiu.");
            break;
        case QProcess::Timedout:
            message = QStringLiteral("Tempo esgotado ao gerenciar o processo.");
            break;
        case QProcess::WriteError:
            message = QStringLiteral("Erro ao escrever no processo llama-server.");
            break;
        case QProcess::ReadError:
            message = QStringLiteral("Erro ao ler a saída do processo llama-server.");
            break;
        case QProcess::UnknownError:
        default:
            message = QStringLiteral("Erro desconhecido no processo llama-server.");
            break;
        }
        setStatusLine(message);
    });

    connect(&m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
        m_probeTimer.stop();
        m_running = false;
        m_starting = false;
        emit runningChanged();
        emit startingChanged();
        setHealthy(false);
        setStatusLine(exitStatus == QProcess::NormalExit
            ? QStringLiteral("llama-server finalizado (exit=%1).")
                  .arg(QString::number(exitCode))
            : QStringLiteral("llama-server terminou de forma inesperada (exit=%1).")
                  .arg(QString::number(exitCode)));
    });
}

QString LlamaServerProcessService::executablePath() const { return m_executablePath; }
void LlamaServerProcessService::setExecutablePath(const QString& value) {
    const QString normalized = value.trimmed().isEmpty() ? QStringLiteral("llama-server") : value.trimmed();
    if (m_executablePath == normalized) {
        return;
    }
    m_executablePath = normalized;
    emit executablePathChanged();
}

QString LlamaServerProcessService::extraArguments() const { return m_extraArguments; }
void LlamaServerProcessService::setExtraArguments(const QString& value) {
    if (m_extraArguments == value) {
        return;
    }
    m_extraArguments = value;
    emit extraArgumentsChanged();
}

QString LlamaServerProcessService::baseUrl() const { return m_baseUrl; }
void LlamaServerProcessService::setBaseUrl(const QString& value) {
    const QString normalized = normalizeBaseUrl(value);
    if (m_baseUrl == normalized) {
        return;
    }
    m_baseUrl = normalized;
    emit baseUrlChanged();
}

QString LlamaServerProcessService::statusLine() const { return m_statusLine; }
QString LlamaServerProcessService::logs() const { return m_logs; }
bool LlamaServerProcessService::isRunning() const { return m_running; }
bool LlamaServerProcessService::isStarting() const { return m_starting; }
bool LlamaServerProcessService::isHealthy() const { return m_healthy; }

void LlamaServerProcessService::startWithModel(const QString& modelPath, int contextSize) {
    const QString absoluteModelPath = modelPath.trimmed();
    if (absoluteModelPath.isEmpty()) {
        setStatusLine(QStringLiteral("Selecione um GGUF ativo antes de iniciar o llama-server."));
        return;
    }
    if (m_running || m_starting) {
        setStatusLine(QStringLiteral("llama-server já está em execução."));
        return;
    }

    m_process.setWorkingDirectory(QCoreApplication::applicationDirPath());
    m_process.setProgram(m_executablePath);
    m_process.setArguments(launchArguments(absoluteModelPath, contextSize));

    m_starting = true;
    emit startingChanged();
    setHealthy(false);
    appendLog(QStringLiteral("\n[%1] starting: %2 %3\n")
        .arg(QDateTime::currentDateTime().toString(Qt::ISODate), m_executablePath,
             m_process.arguments().join(' ')));
    setStatusLine(QStringLiteral("Iniciando llama-server..."));
    m_process.start();
}

void LlamaServerProcessService::stop() {
    if (!m_running && !m_starting) {
        setStatusLine(QStringLiteral("llama-server já está parado."));
        return;
    }

    setStatusLine(QStringLiteral("Parando llama-server..."));
    m_probeTimer.stop();
    if (m_probeReply) {
        m_probeReply->abort();
        m_probeReply->deleteLater();
        m_probeReply = nullptr;
    }

    m_process.terminate();
    if (!m_process.waitForFinished(2500)) {
        m_process.kill();
    }
}

void LlamaServerProcessService::probeNow() {
    if (m_probeReply) {
        return;
    }

    beginProbe(QUrl(m_baseUrl + QStringLiteral("/health")), true);
}

void LlamaServerProcessService::clearLogs() {
    if (m_logs.isEmpty()) {
        return;
    }
    m_logs.clear();
    emit logsChanged();
}

QString LlamaServerProcessService::normalizeBaseUrl(QString value) const {
    value = value.trimmed();
    if (value.isEmpty()) {
        value = QStringLiteral("http://127.0.0.1:8080");
    }
    if (value.endsWith('/')) {
        value.chop(1);
    }
    return value;
}

void LlamaServerProcessService::appendLog(const QString& chunk) {
    if (chunk.isEmpty()) {
        return;
    }
    m_logs += chunk;
    constexpr int maxChars = 48000;
    if (m_logs.size() > maxChars) {
        m_logs = m_logs.right(maxChars);
    }
    emit logsChanged();
}

void LlamaServerProcessService::setStatusLine(const QString& value) {
    if (m_statusLine == value) {
        return;
    }
    m_statusLine = value;
    emit statusChanged();
}

void LlamaServerProcessService::setHealthy(bool value) {
    if (m_healthy == value) {
        return;
    }
    m_healthy = value;
    emit healthyChanged();
}

void LlamaServerProcessService::beginProbe(const QUrl& url, bool allowFallback) {
    QNetworkRequest request(url);
    request.setTransferTimeout(1800);
    auto* reply = m_network.get(request);
    m_probeReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, allowFallback]() {
        handleProbeFinished(reply, allowFallback);
    });
}

void LlamaServerProcessService::handleProbeFinished(QNetworkReply* reply, bool allowFallback) {
    if (!reply) {
        return;
    }

    if (reply == m_probeReply) {
        m_probeReply = nullptr;
    }

    const auto error = reply->error();
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    reply->deleteLater();

    const bool ok = error == QNetworkReply::NoError && statusCode >= 200 && statusCode < 500;
    if (ok) {
        setHealthy(true);
        if (m_running || m_starting) {
            setStatusLine(QStringLiteral("llama-server online em %1").arg(m_baseUrl));
        } else {
            setStatusLine(QStringLiteral("Endpoint respondeu em %1").arg(m_baseUrl));
        }
        return;
    }

    if (allowFallback) {
        beginProbe(QUrl(m_baseUrl + QStringLiteral("/v1/models")), false);
        return;
    }

    setHealthy(false);
    if (m_running || m_starting) {
        setStatusLine(QStringLiteral("Processo rodando, mas a API ainda não respondeu em %1").arg(m_baseUrl));
    } else {
        setStatusLine(QStringLiteral("API local indisponível em %1").arg(m_baseUrl));
    }
}

QStringList LlamaServerProcessService::launchArguments(const QString& modelPath, int contextSize) const {
    const QUrl url(m_baseUrl);
    const QString host = url.host().isEmpty() ? QStringLiteral("127.0.0.1") : url.host();
    const int port = url.port(8080);

    QStringList args;
    args << QStringLiteral("-m") << modelPath;
    args << QStringLiteral("-c") << QString::number(contextSize > 0 ? contextSize : 8192);
    args << QStringLiteral("--host") << host;
    args << QStringLiteral("--port") << QString::number(port);

    const QStringList extra = QProcess::splitCommand(m_extraArguments);
    for (const auto& item : extra) {
        if (!item.trimmed().isEmpty()) {
            args << item;
        }
    }
    return args;
}

} // namespace ide::services
