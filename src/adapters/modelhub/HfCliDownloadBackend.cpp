#include "adapters/modelhub/HfCliDownloadBackend.hpp"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QStandardPaths>

namespace ide::adapters::modelhub {

HfCliDownloadBackend::HfCliDownloadBackend(QObject* parent)
    : IFileDownloadBackend(parent) {
    // Fast non-blocking availability check for startup path.
    m_available = !QStandardPaths::findExecutable(QStringLiteral("hf")).trimmed().isEmpty();

    connect(&m_process, &QProcess::started, this, [this]() {
        setPhase(QStringLiteral("downloading"));
        setStatusLine(QStringLiteral("Downloading via hf CLI..."));
        appendDiagnostic(QStringLiteral("process started"));
        emit activeChanged();
        emit progressChanged();
    });

    connect(&m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        handleChunk(QString::fromLocal8Bit(m_process.readAllStandardOutput()), false);
    });
    connect(&m_process, &QProcess::readyReadStandardError, this, [this]() {
        handleChunk(QString::fromLocal8Bit(m_process.readAllStandardError()), true);
    });

    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        Q_UNUSED(error);
        setPhase(QStringLiteral("failed"));
        setStatusLine(QStringLiteral("hf CLI process error."));
        appendDiagnostic(QStringLiteral("process error=%1").arg(QString::number(static_cast<int>(error))));
        emit failed(m_statusLine);
    });

    connect(&m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
        emit activeChanged();
        if (m_cancelRequested) {
            m_cancelRequested = false;
            setPhase(QStringLiteral("canceled"));
            setStatusLine(QStringLiteral("Download canceled."));
            appendDiagnostic(QStringLiteral("canceled"));
            emit failed(m_statusLine);
            return;
        }

        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            setPhase(QStringLiteral("failed"));
            setStatusLine(QStringLiteral("hf download failed (exit=%1).").arg(QString::number(exitCode)));
            appendDiagnostic(QStringLiteral("finished with failure exit=%1").arg(QString::number(exitCode)));
            emit failed(m_statusLine);
            return;
        }

        if (m_lastDetectedPath.isEmpty()) {
            m_lastDetectedPath = locateDownloadedFile(m_cacheDir, m_repoId, m_filename);
        }
        if (m_lastDetectedPath.isEmpty()) {
            setPhase(QStringLiteral("failed"));
            setStatusLine(QStringLiteral("hf finished but output path could not be resolved."));
            appendDiagnostic(QStringLiteral("could not resolve local file path"));
            emit failed(m_statusLine);
            return;
        }

        setPhase(QStringLiteral("completed"));
        setStatusLine(QStringLiteral("Download completed: %1").arg(m_lastDetectedPath));
        appendDiagnostic(QStringLiteral("completed path=%1").arg(m_lastDetectedPath));
        emit progressChanged();
        emit finishedSuccessfully(m_lastDetectedPath);
    });
}

QString HfCliDownloadBackend::backendName() const { return QStringLiteral("hf-cli"); }
bool HfCliDownloadBackend::isAvailable() const { return m_available; }
bool HfCliDownloadBackend::isActive() const { return m_process.state() != QProcess::NotRunning; }
qint64 HfCliDownloadBackend::receivedBytes() const { return 0; }
qint64 HfCliDownloadBackend::totalBytes() const { return 0; }
double HfCliDownloadBackend::speedBytesPerSec() const { return 0.0; }
qint64 HfCliDownloadBackend::etaSeconds() const { return -1; }
QString HfCliDownloadBackend::phase() const { return m_phase; }
QString HfCliDownloadBackend::statusLine() const { return m_statusLine; }
QString HfCliDownloadBackend::diagnosticsText() const { return m_diagnosticsText; }

QStringList HfCliDownloadBackend::parseLines(const QString& chunk, QString& carry) {
    carry += chunk;
    QStringList lines;
    int newline = carry.indexOf('\n');
    while (newline >= 0) {
        QString line = carry.left(newline);
        if (line.endsWith('\r')) {
            line.chop(1);
        }
        lines.push_back(line);
        carry.remove(0, newline + 1);
        newline = carry.indexOf('\n');
    }
    return lines;
}

QString HfCliDownloadBackend::repoCacheDirName(const QString& repoId) {
    QString normalized = repoId.trimmed();
    normalized.replace('/', QStringLiteral("--"));
    return QStringLiteral("models--") + normalized;
}

QString HfCliDownloadBackend::locateDownloadedFile(const QString& cacheDir, const QString& repoId, const QString& filename) {
    const QString repoRoot = QDir(cacheDir).filePath(repoCacheDirName(repoId));
    const QString snapshotsRoot = QDir(repoRoot).filePath(QStringLiteral("snapshots"));
    const QString preferredPath = QDir(snapshotsRoot).filePath(QStringLiteral("main/%1").arg(filename));
    if (QFileInfo::exists(preferredPath)) {
        return preferredPath;
    }

    QDir snapshotsDir(snapshotsRoot);
    if (snapshotsDir.exists()) {
        const QFileInfoList snapshotDirs = snapshotsDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
        for (const QFileInfo& snapshotInfo : snapshotDirs) {
            const QString candidate = QDir(snapshotInfo.absoluteFilePath()).filePath(filename);
            if (QFileInfo::exists(candidate)) {
                return candidate;
            }
        }
    }
    return {};
}

void HfCliDownloadBackend::handleChunk(const QString& chunk, bool stderrStream) {
    const auto lines = parseLines(chunk, stderrStream ? m_stderrCarry : m_stdoutCarry);
    static const QRegularExpression pathPattern(QStringLiteral(R"(([A-Za-z]:\\[^\r\n]+\.gguf))"), QRegularExpression::CaseInsensitiveOption);
    for (const QString& rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty()) {
            continue;
        }
        appendDiagnostic((stderrStream ? QStringLiteral("stderr: ") : QStringLiteral("stdout: ")) + line);
        const auto pathMatch = pathPattern.match(line);
        if (pathMatch.hasMatch()) {
            const QString path = pathMatch.captured(1);
            if (QFileInfo::exists(path)) {
                m_lastDetectedPath = path;
            }
        } else if (line.endsWith(QStringLiteral(".gguf"), Qt::CaseInsensitive) && QFileInfo::exists(line)) {
            m_lastDetectedPath = line;
        }
    }
}

void HfCliDownloadBackend::setPhase(const QString& value) {
    if (m_phase == value) {
        return;
    }
    m_phase = value;
    emit statusChanged();
}

void HfCliDownloadBackend::setStatusLine(const QString& value) {
    if (m_statusLine == value) {
        return;
    }
    m_statusLine = value;
    emit statusChanged();
}

void HfCliDownloadBackend::appendDiagnostic(const QString& line) {
    if (line.trimmed().isEmpty()) {
        return;
    }
    QStringList rows = m_diagnosticsText.split('\n', Qt::SkipEmptyParts);
    rows.push_back(QStringLiteral("[%1][%2] %3")
                       .arg(QDateTime::currentDateTime().toString(Qt::ISODate), backendName(), line.trimmed()));
    while (rows.size() > 40) {
        rows.removeFirst();
    }
    m_diagnosticsText = rows.join('\n');
    emit diagnosticsChanged();
}

void HfCliDownloadBackend::resetState() {
    m_statusLine.clear();
    m_phase = QStringLiteral("idle");
    m_diagnosticsText.clear();
    m_lastDetectedPath.clear();
    m_stdoutCarry.clear();
    m_stderrCarry.clear();
}

void HfCliDownloadBackend::start(const DownloadRequest& request) {
    if (!m_available) {
        setPhase(QStringLiteral("failed"));
        setStatusLine(QStringLiteral("hf CLI is not available in PATH."));
        appendDiagnostic(QStringLiteral("unavailable backend"));
        emit failed(m_statusLine);
        return;
    }
    if (isActive()) {
        cancel();
    }
    resetState();

    m_repoId = request.repoId.trimmed();
    m_filename = request.filename.trimmed();
    m_cacheDir = request.cacheDir.trimmed();
    if (m_repoId.isEmpty() || m_filename.isEmpty() || m_cacheDir.isEmpty()) {
        setPhase(QStringLiteral("failed"));
        setStatusLine(QStringLiteral("Missing repository, file name, or cache directory."));
        appendDiagnostic(QStringLiteral("invalid start request"));
        emit failed(m_statusLine);
        return;
    }

    m_process.setProgram(QStringLiteral("hf"));
    m_process.setArguments({
        QStringLiteral("download"),
        m_repoId,
        m_filename,
        QStringLiteral("--cache-dir"),
        m_cacheDir
    });

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!request.token.trimmed().isEmpty()) {
        env.insert(QStringLiteral("HF_TOKEN"), request.token.trimmed());
    }
    m_process.setProcessEnvironment(env);

    setPhase(QStringLiteral("starting"));
    setStatusLine(QStringLiteral("Starting hf download..."));
    appendDiagnostic(QStringLiteral("start repo=%1 file=%2 cache=%3").arg(m_repoId, m_filename, m_cacheDir));
    m_process.start();
}

void HfCliDownloadBackend::cancel() {
    if (!isActive()) {
        return;
    }
    m_cancelRequested = true;
    appendDiagnostic(QStringLiteral("cancel requested"));
    setPhase(QStringLiteral("canceling"));
    setStatusLine(QStringLiteral("Canceling hf download..."));
    m_process.kill();
}

} // namespace ide::adapters::modelhub
