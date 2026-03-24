#include "adapters/diagnostics/LspProtocol.hpp"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTextStream>

namespace ide::adapters::diagnostics {

LspProtocol::LspProtocol(const QString& program, const QStringList& args, QObject* parent)
    : QObject(parent)
    , m_program(program)
    , m_args(args) {
    m_process = new QProcess(this);

    connect(m_process, &QProcess::started, this, [this]() {
        emit started();
        emit statusChanged(statusLine());
    });
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        m_initialized = false;
        
        // Debug log
        QFile logFile(QCoreApplication::applicationDirPath() + "/lsp_debug.log");
        logFile.open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream out(&logFile);
        out << "LspProtocol::onFinished - program:" << m_program << " exitCode:" << exitCode << " exitStatus:" << exitStatus << "\n";
        
        // Also log stderr if available
        QByteArray processStderr = m_process->readAllStandardError();
        if (!processStderr.isEmpty()) {
            out << "  stderr:" << QString::fromUtf8(processStderr).left(500) << "\n";
        }
        
        logFile.close();
        
        emit stopped();
        emit statusChanged(statusLine());
    });
}

LspProtocol::~LspProtocol() {
    stop();
}

void LspProtocol::start() {
    if (m_process->state() != QProcess::NotRunning) {
        return;
    }
    
    // Debug log to file
    QFile logFile(QCoreApplication::applicationDirPath() + "/lsp_debug.log");
    logFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream out(&logFile);
    out << "LspProtocol::start() - program:" << m_program << " args:" << m_args.join(" ") << "\n";
    out << "  CWD:" << QDir::currentPath() << "\n";
    out << "  appDir:" << QCoreApplication::applicationDirPath() << "\n";
    logFile.close();
    
    m_process->start(m_program, m_args);
    
    if (!m_process->waitForStarted(5000)) {
        QFile logFile2(QCoreApplication::applicationDirPath() + "/lsp_debug.log");
        logFile2.open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream out2(&logFile2);
        out2 << "  FAILED to start! error:" << m_process->errorString() << "\n";
        logFile2.close();
    } else {
        QFile logFile3(QCoreApplication::applicationDirPath() + "/lsp_debug.log");
        logFile3.open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream out3(&logFile3);
        out3 << "  Started successfully! state:" << m_process->state() << "\n";
        logFile3.close();
    }
    
    emit statusChanged(statusLine());
}

void LspProtocol::stop() {
    if (m_process->state() == QProcess::NotRunning) {
        return;
    }
    m_initialized = false;
    m_process->terminate();
    if (!m_process->waitForFinished(2000)) {
        m_process->kill();
        m_process->waitForFinished(500);
    }
}

bool LspProtocol::isRunning() const {
    return m_process->state() == QProcess::Running;
}

bool LspProtocol::isReady() const {
    return m_initialized;
}

void LspProtocol::markReady() {
    m_initialized = true;
    emit statusChanged(statusLine());
}

QString LspProtocol::statusLine() const {
    if (m_program.isEmpty()) {
        return QStringLiteral("disabled");
    }
    if (m_process->state() != QProcess::Running) {
        return QString("configured: %1 (not started)").arg(m_program);
    }
    if (!m_initialized) {
        return QString("%1 starting...").arg(m_program);
    }
    return QString("%1 running").arg(m_program);
}

QString LspProtocol::program() const {
    return m_program;
}

QProcess& LspProtocol::process() {
    return *m_process;
}

int LspProtocol::nextRequestId() {
    return m_nextId++;
}

} // namespace ide::adapters::diagnostics
