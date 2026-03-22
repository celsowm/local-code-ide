#include "ui/viewmodels/ModelHubViewModel.hpp"

#include <QDir>
#include <QProcessEnvironment>
#include <QFileInfo>
#include <QSet>
#include <QStandardPaths>
#include <QUrl>
#include <limits>
#include <utility>

namespace ide::ui::viewmodels {

namespace {
QString repoCacheDirName(const QString& repoId) {
    QString normalized = repoId.trimmed();
    normalized.replace('/', QStringLiteral("--"));
    return QStringLiteral("models--") + normalized;
}

QString huggingFaceHubCacheDir() {
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString explicitHubCache = env.value(QStringLiteral("HF_HUB_CACHE")).trimmed();
    if (!explicitHubCache.isEmpty()) {
        return explicitHubCache;
    }

    const QString hfHome = env.value(QStringLiteral("HF_HOME")).trimmed();
    if (!hfHome.isEmpty()) {
        return QDir(hfHome).filePath(QStringLiteral("hub"));
    }

    return QDir(QDir::homePath()).filePath(QStringLiteral(".cache/huggingface/hub"));
}

QString defaultDownloadDir() {
    return huggingFaceHubCacheDir();
}

QString uiStateSettingsPath() {
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (base.isEmpty()) {
        base = QDir::homePath() + QStringLiteral("/.localcodeide");
    }
    QDir().mkpath(base);
    return QDir(base).filePath(QStringLiteral("ui.ini"));
}

QString basenameFromRemotePath(const QString& remotePath) {
    return remotePath.section('/', -1);
}

bool isCodingBiased(const QString& text) {
    return text.contains(QStringLiteral("code"), Qt::CaseInsensitive)
        || text.contains(QStringLiteral("coder"), Qt::CaseInsensitive)
        || text.contains(QStringLiteral("dev"), Qt::CaseInsensitive)
        || text.contains(QStringLiteral("rust"), Qt::CaseInsensitive)
        || text.contains(QStringLiteral("cpp"), Qt::CaseInsensitive)
        || text.contains(QStringLiteral("c++"), Qt::CaseInsensitive)
        || text.contains(QStringLiteral("python"), Qt::CaseInsensitive);
}
}

ModelHubViewModel::ModelHubViewModel(std::unique_ptr<ide::services::ModelCatalogService> catalogService,
                                     std::unique_ptr<ide::adapters::modelhub::HuggingFaceFileDownloader> downloader,
                                     std::unique_ptr<ide::services::HardwareProfileService> hardwareService,
                                     ide::services::LocalModelRuntimeService* runtimeService,
                                     std::unique_ptr<ide::services::LlamaServerProcessService> serverService,
                                     QObject* parent)
    : QObject(parent)
    , m_catalogService(std::move(catalogService))
    , m_downloader(std::move(downloader))
    , m_hardwareService(std::move(hardwareService))
    , m_runtimeService(runtimeService)
    , m_serverService(std::move(serverService))
    , m_uiSettings(uiStateSettingsPath(), QSettings::IniFormat)
    , m_targetDownloadDir(defaultDownloadDir()) {
    connect(m_downloader.get(), &ide::adapters::modelhub::HuggingFaceFileDownloader::statusChanged, this, [this]() {
        emit downloadStatusChanged();
        setStatusMessage(m_downloader->statusLine());
    });
    connect(m_downloader.get(), &ide::adapters::modelhub::HuggingFaceFileDownloader::progressChanged, this, [this]() {
        emit downloadProgressChanged();
    });
    connect(m_downloader.get(), &ide::adapters::modelhub::HuggingFaceFileDownloader::activeChanged, this, [this]() {
        emit downloadActiveChanged();
    });
    connect(m_downloader.get(), &ide::adapters::modelhub::HuggingFaceFileDownloader::diagnosticsChanged, this, [this]() {
        emit downloadDiagnosticsChanged();
    });
    connect(m_downloader.get(), &ide::adapters::modelhub::HuggingFaceFileDownloader::finishedSuccessfully, this, [this](const QString& localPath) {
        m_downloadedPath = localPath;
        emit downloadedPathChanged();
        emit selectedFileChanged();
    });
    connect(m_downloader.get(), &ide::adapters::modelhub::HuggingFaceFileDownloader::failed, this, [this](const QString&) {
        emit downloadProgressChanged();
    });

    if (m_runtimeService) {
        connect(m_runtimeService, &ide::services::LocalModelRuntimeService::activeModelChanged, this, [this]() {
            emit currentLocalModelChanged();
            emit downloadedPathChanged();
            emit selectedFileChanged();
            emit serverRuntimeChanged();
            updateFileModel(m_currentFiles);
            setStatusMessage(currentLocalModelSummary());
        });
        connect(m_runtimeService, &ide::services::LocalModelRuntimeService::runtimeConfigChanged, this, [this]() {
            emit currentLocalModelChanged();
            emit downloadedPathChanged();
            emit selectedFileChanged();
            emit serverRuntimeChanged();
            setStatusMessage(currentLocalModelSummary());
        });
    }

    if (m_serverService) {
        connect(m_serverService.get(), &ide::services::LlamaServerProcessService::statusChanged, this, [this]() {
            emit serverRuntimeChanged();
            setStatusMessage(serverStatusLine());
        });
        connect(m_serverService.get(), &ide::services::LlamaServerProcessService::logsChanged, this, [this]() {
            emit serverLogsChanged();
        });
        connect(m_serverService.get(), &ide::services::LlamaServerProcessService::runningChanged, this, [this]() {
            emit serverRuntimeChanged();
        });
        connect(m_serverService.get(), &ide::services::LlamaServerProcessService::startingChanged, this, [this]() {
            emit serverRuntimeChanged();
        });
        connect(m_serverService.get(), &ide::services::LlamaServerProcessService::healthyChanged, this, [this]() {
            emit serverRuntimeChanged();
        });
        connect(m_serverService.get(), &ide::services::LlamaServerProcessService::executablePathChanged, this, [this]() {
            emit serverRuntimeChanged();
            emit currentLocalModelChanged();
            emit downloadedPathChanged();
        });
        connect(m_serverService.get(), &ide::services::LlamaServerProcessService::extraArgumentsChanged, this, [this]() {
            emit serverRuntimeChanged();
            emit currentLocalModelChanged();
            emit downloadedPathChanged();
        });
        connect(m_serverService.get(), &ide::services::LlamaServerProcessService::baseUrlChanged, this, [this]() {
            emit serverRuntimeChanged();
            emit currentLocalModelChanged();
            emit downloadedPathChanged();
        });
    }

    loadUiState();
    refreshHardware();
}

QString ModelHubViewModel::author() const { return m_author; }
void ModelHubViewModel::setAuthor(const QString& value) {
    const QString trimmed = value.trimmed();
    if (m_author == trimmed) {
        return;
    }
    m_author = trimmed;
    emit authorChanged();
}

QString ModelHubViewModel::searchQuery() const { return m_searchQuery; }
void ModelHubViewModel::setSearchQuery(const QString& value) {
    if (m_searchQuery == value) {
        return;
    }
    m_searchQuery = value;
    emit searchQueryChanged();
    emit hardwareChanged();
    emit filesChanged();
    emit selectedRepoChanged();
}

QString ModelHubViewModel::statusMessage() const { return m_statusMessage; }
QString ModelHubViewModel::selectedRepoId() const { return m_selectedRepoId; }
QString ModelHubViewModel::selectedFilePath() const { return m_selectedFilePath; }
void ModelHubViewModel::setSelectedFilePath(const QString& value) {
    if (m_selectedFilePath == value) {
        return;
    }
    m_selectedFilePath = value;
    emit selectedFileChanged();
}
QString ModelHubViewModel::recommendedFilePath() const { return m_recommendedFilePath; }
int ModelHubViewModel::wizardStep() const { return m_wizardStep; }
void ModelHubViewModel::setWizardStep(int value) {
    const int bounded = qBound(0, value, 3);
    if (m_wizardStep == bounded) {
        return;
    }
    m_wizardStep = bounded;
    emit wizardStepChanged();
    saveUiState();
}

QString ModelHubViewModel::recommendationProfile() const { return m_recommendationProfile; }
void ModelHubViewModel::setRecommendationProfile(const QString& value) {
    const QString normalized = value.trimmed().toLower();
    if (normalized.isEmpty() || m_recommendationProfile == normalized) {
        return;
    }
    m_recommendationProfile = normalized;
    emit recommendationProfileChanged();
    suggestFile();
}
QString ModelHubViewModel::quantFilter() const { return m_quantFilter; }
void ModelHubViewModel::setQuantFilter(const QString& value) {
    const QString normalized = value.trimmed().toUpper();
    const QString next = normalized.isEmpty() ? QStringLiteral("AUTO") : normalized;
    if (m_quantFilter.compare(next, Qt::CaseInsensitive) == 0) {
        return;
    }
    m_quantFilter = next;
    emit quantFilterChanged();

    const auto visible = filteredFiles(m_currentFiles);
    updateFileModel(visible);

    if (!visible.empty()) {
        if (m_selectedFilePath.isEmpty()) {
            m_selectedFilePath = visible.front().path;
            emit selectedFileChanged();
        } else {
            const bool stillVisible = std::any_of(visible.begin(), visible.end(), [this](const auto& file) {
                return file.path == m_selectedFilePath;
            });
            if (!stillVisible) {
                m_selectedFilePath = visible.front().path;
                emit selectedFileChanged();
            }
        }
    } else if (!m_selectedFilePath.isEmpty()) {
        m_selectedFilePath.clear();
        emit selectedFileChanged();
    }

    suggestFile();
}
QStringList ModelHubViewModel::quantOptions() const { return m_quantOptions; }

QString ModelHubViewModel::autoDetectedProfile() const {
    return m_hardwareProfile.autoProfile(m_searchQuery);
}

QString ModelHubViewModel::hardwareSummary() const {
    return m_hardwareProfile.summary();
}

QString ModelHubViewModel::recommendationSummary() const {
    const double budget = m_hardwareProfile.recommendedBudgetGiB();
    QStringList parts;
    parts.push_back(QStringLiteral("profile=%1").arg(m_recommendationProfile));
    parts.push_back(QStringLiteral("auto=%1").arg(autoDetectedProfile()));
    if (budget > 0.0) {
        parts.push_back(QStringLiteral("budget ~%1 GiB").arg(QString::number(budget, 'f', budget >= 10.0 ? 0 : 1)));
    }
    if (!m_recommendedFilePath.isEmpty()) {
        parts.push_back(QStringLiteral("suggestion=%1").arg(basenameFromRemotePath(m_recommendedFilePath)));
    }
    return parts.join(QStringLiteral(" · "));
}

QString ModelHubViewModel::targetDownloadDir() const { return m_targetDownloadDir; }
void ModelHubViewModel::setTargetDownloadDir(const QString& value) {
    if (m_targetDownloadDir == value) {
        return;
    }
    m_targetDownloadDir = value;
    emit targetDownloadDirChanged();
    emit selectedFileChanged();
}
QString ModelHubViewModel::downloadStatus() const { return m_downloader->statusLine(); }
double ModelHubViewModel::downloadProgress() const {
    const auto total = m_downloader->totalBytes();
    if (total <= 0) {
        return 0.0;
    }
    return static_cast<double>(m_downloader->receivedBytes()) / static_cast<double>(total);
}
QString ModelHubViewModel::downloadSpeedText() const {
    const double bytesPerSec = m_downloader->speedBytesPerSec();
    if (bytesPerSec <= 0.0) {
        return QStringLiteral("—");
    }
    const double kb = bytesPerSec / 1024.0;
    if (kb < 1024.0) {
        return QStringLiteral("%1 KB/s").arg(QString::number(kb, 'f', kb >= 100.0 ? 0 : 1));
    }
    const double mb = kb / 1024.0;
    return QStringLiteral("%1 MB/s").arg(QString::number(mb, 'f', mb >= 100.0 ? 0 : 1));
}
QString ModelHubViewModel::downloadEtaText() const {
    const qint64 seconds = m_downloader->etaSeconds();
    if (seconds < 0) {
        return QStringLiteral("—");
    }
    const qint64 mins = seconds / 60;
    const qint64 secs = seconds % 60;
    if (mins > 0) {
        return QStringLiteral("%1m%2s").arg(QString::number(mins), QString::number(secs).rightJustified(2, '0'));
    }
    return QStringLiteral("%1s").arg(QString::number(secs));
}
QString ModelHubViewModel::downloadProgressDetails() const { return m_downloader->progressDetails(); }
QString ModelHubViewModel::downloadPhase() const { return m_downloader->phase(); }
QString ModelHubViewModel::downloadDiagnostics() const { return m_downloader->diagnosticsText(); }
bool ModelHubViewModel::downloadActive() const { return m_downloader->isActive(); }
QString ModelHubViewModel::downloadedPath() const { return m_downloadedPath; }
QString ModelHubViewModel::providerName() const { return m_catalogService->providerName(); }
QString ModelHubViewModel::helperText() const {
    if (m_selectedRepoId.isEmpty()) {
        return QStringLiteral("Select a GGUF repository. Recommendations now use locally detected RAM/VRAM, with optional environment overrides.");
    }

    return QStringLiteral("Hardware: %1. Default guidance: laptop -> Q4_K_M, balanced -> Q5_K_M, quality -> Q6_K/Q8_0, coding gives extra weight to coder/code files.")
        .arg(hardwareSummary());
}

QString ModelHubViewModel::llamaLaunchHint() const {
    const QString executable = serverExecutablePath();
    const QString extra = serverExtraArguments();
    const QUrl url(serverBaseUrl());
    const QString host = url.host().isEmpty() ? QStringLiteral("127.0.0.1") : url.host();
    const int port = url.port(8080);
    if (!m_downloadedPath.isEmpty()) {
        const QFileInfo info(m_downloadedPath);
        return QStringLiteral("%1 -m \"%2\" -c %3 --host %4 --port %5%6")
            .arg(executable,
                 info.absoluteFilePath(),
                 QString::number(runtimeContextSize()),
                 host,
                 QString::number(port),
                 extra.trimmed().isEmpty() ? QString() : QStringLiteral(" ") + extra.trimmed());
    }
    if (m_runtimeService && m_runtimeService->hasActiveModel()) {
        return m_runtimeService->launchCommand(executable, extra, serverBaseUrl());
    }
    return QStringLiteral("%1 -m /path/model.gguf -c %3 --host %4 --port %5%2")
        .arg(executable,
             extra.trimmed().isEmpty() ? QString() : QStringLiteral(" ") + extra.trimmed(),
             QString::number(runtimeContextSize()),
             host,
             QString::number(port));
}

QString ModelHubViewModel::currentLocalModelSummary() const {
    return m_runtimeService ? m_runtimeService->statusLine() : QStringLiteral("Local runtime unavailable.");
}

QString ModelHubViewModel::currentLocalLaunchCommand() const {
    return m_runtimeService
        ? m_runtimeService->launchCommand(serverExecutablePath(), serverExtraArguments(), serverBaseUrl())
        : QStringLiteral("llama-server -m /path/model.gguf -c %1 --host 127.0.0.1 --port 8080").arg(QString::number(runtimeContextSize()));
}

bool ModelHubViewModel::hasCurrentLocalModel() const {
    return m_runtimeService && m_runtimeService->hasActiveModel();
}

int ModelHubViewModel::runtimeContextSize() const {
    return m_runtimeService ? m_runtimeService->contextSize() : 8192;
}

void ModelHubViewModel::setRuntimeContextSize(int value) {
    if (!m_runtimeService) {
        return;
    }
    m_runtimeService->setContextSize(value);
}

bool ModelHubViewModel::canUseSelectedAsCurrent() const {
    if (m_selectedRepoId.isEmpty() || m_selectedFilePath.isEmpty()) {
        return false;
    }
    const QString candidate = candidateLocalPathForRemote(m_selectedFilePath);
    return QFileInfo::exists(candidate);
}

QString ModelHubViewModel::serverExecutablePath() const {
    return m_serverService ? m_serverService->executablePath() : QStringLiteral("llama-server");
}

void ModelHubViewModel::setServerExecutablePath(const QString& value) {
    if (!m_serverService) {
        return;
    }
    m_serverService->setExecutablePath(value);
}

QString ModelHubViewModel::serverExtraArguments() const {
    return m_serverService ? m_serverService->extraArguments() : QString();
}

void ModelHubViewModel::setServerExtraArguments(const QString& value) {
    if (!m_serverService) {
        return;
    }
    m_serverService->setExtraArguments(value);
}

QString ModelHubViewModel::serverBaseUrl() const {
    if (m_serverService) {
        return m_serverService->baseUrl();
    }
    return m_runtimeService ? m_runtimeService->baseUrl() : QStringLiteral("http://127.0.0.1:8080");
}

void ModelHubViewModel::setServerBaseUrl(const QString& value) {
    if (m_runtimeService) {
        m_runtimeService->setBaseUrl(value);
    }
    if (!m_serverService) {
        return;
    }
    m_serverService->setBaseUrl(value);
}

QString ModelHubViewModel::serverStatusLine() const {
    return m_serverService ? m_serverService->statusLine() : QStringLiteral("llama-server manager unavailable.");
}

QString ModelHubViewModel::serverLogs() const {
    return m_serverService ? m_serverService->logs() : QString();
}

bool ModelHubViewModel::serverRunning() const {
    return m_serverService && m_serverService->isRunning();
}

bool ModelHubViewModel::serverStarting() const {
    return m_serverService && m_serverService->isStarting();
}

bool ModelHubViewModel::serverHealthy() const {
    return m_serverService && m_serverService->isHealthy();
}

bool ModelHubViewModel::canStartServer() const {
    return m_serverService && m_runtimeService && m_runtimeService->hasActiveModel() && !serverRunning() && !serverStarting();
}

int ModelHubViewModel::repoCount() const { return m_reposModel.repoCount(); }
int ModelHubViewModel::fileCount() const { return m_filesModel.fileCount(); }
QObject* ModelHubViewModel::reposModel() { return &m_reposModel; }
QObject* ModelHubViewModel::filesModel() { return &m_filesModel; }

void ModelHubViewModel::searchRepos() {
    const auto repos = m_catalogService->searchRepos(m_author, m_searchQuery, 24);
    m_reposModel.setRepos(repos);
    emit reposChanged();

    if (repos.empty()) {
        setStatusMessage(m_catalogService->lastError().isEmpty()
            ? QStringLiteral("No repositories found.")
            : m_catalogService->lastError());
        return;
    }

    setStatusMessage(QStringLiteral("%1 repositories found on %2.")
        .arg(QString::number(repoCount()), providerName()));

    selectRepo(repos.front().id);
}

void ModelHubViewModel::selectRepo(const QString& repoId) {
    if (repoId.trimmed().isEmpty()) {
        return;
    }

    m_selectedRepoId = repoId.trimmed();
    emit selectedRepoChanged();

    const auto files = m_catalogService->listFiles(m_selectedRepoId);
    m_currentFiles = files;
    rebuildQuantOptions(files);
    const auto visibleFiles = filteredFiles(files);
    m_selectedFilePath.clear();
    m_recommendedFilePath.clear();
    emit selectedFileChanged();
    emit recommendedFileChanged();
    updateFileModel(visibleFiles);
    if (visibleFiles.empty()) {
        setStatusMessage(m_catalogService->lastError().isEmpty()
            ? QStringLiteral("No GGUF files visible for the current quant filter.")
            : m_catalogService->lastError());
        return;
    }

    m_selectedFilePath = visibleFiles.front().path;
    emit selectedFileChanged();

    suggestFile();
    setStatusMessage(QStringLiteral("%1 GGUF file(s) loaded for %2.")
        .arg(QString::number(fileCount()), m_selectedRepoId));
}

void ModelHubViewModel::suggestFile() {
    const auto files = filteredFiles(m_currentFiles.empty() ? m_filesModel.files() : m_currentFiles);
    m_recommendedFilePath = chooseRecommendedFile(files);
    emit recommendedFileChanged();
    updateFileModel(files);

    if (!m_recommendedFilePath.isEmpty()) {
        const QString basename = basenameFromRemotePath(m_recommendedFilePath);
        setStatusMessage(QStringLiteral("Suggested: %1 | %2").arg(basename, recommendationSummary()));
    }
}

void ModelHubViewModel::startDownloadSelected() {
    if (m_selectedRepoId.isEmpty() || m_selectedFilePath.isEmpty()) {
        setStatusMessage(QStringLiteral("Select a repository and a GGUF file before downloading."));
        return;
    }
    QDir().mkpath(m_targetDownloadDir);
    m_downloader->startDownload(m_selectedRepoId, m_selectedFilePath, m_targetDownloadDir);
    emit downloadProgressChanged();
}

void ModelHubViewModel::downloadSuggested() {
    if (m_recommendedFilePath.isEmpty()) {
        suggestFile();
    }
    if (m_recommendedFilePath.isEmpty()) {
        setStatusMessage(QStringLiteral("No suggested quantization found for the current profile."));
        return;
    }
    setSelectedFilePath(m_recommendedFilePath);
    startDownloadSelected();
}

void ModelHubViewModel::cancelDownload() {
    m_downloader->cancel();
}

void ModelHubViewModel::refreshHardware() {
    if (!m_hardwareService) {
        return;
    }
    m_hardwareProfile = m_hardwareService->detect();
    emit hardwareChanged();
    emit selectedRepoChanged();
    applyDetectedProfile();
}

void ModelHubViewModel::applyDetectedProfile() {
    const QString profile = autoDetectedProfile();
    if (!profile.isEmpty()) {
        m_recommendationProfile = profile;
        emit recommendationProfileChanged();
        suggestFile();
    }
}

void ModelHubViewModel::useSelectedAsCurrent() {
    if (!m_runtimeService) {
        setStatusMessage(QStringLiteral("Local runtime unavailable."));
        return;
    }
    if (m_selectedRepoId.isEmpty() || m_selectedFilePath.isEmpty()) {
        setStatusMessage(QStringLiteral("Select a GGUF file before setting it as active."));
        return;
    }
    const QString candidate = candidateLocalPathForRemote(m_selectedFilePath);
    if (!QFileInfo::exists(candidate)) {
        setStatusMessage(QStringLiteral("This GGUF file is not available locally yet. Download it before activating."));
        return;
    }
    m_runtimeService->setActiveModel(m_selectedRepoId, m_selectedFilePath, candidate);
    setStatusMessage(QStringLiteral("Active model updated to %1").arg(QFileInfo(candidate).fileName()));
}

void ModelHubViewModel::useDownloadedAsCurrent() {
    if (!m_runtimeService) {
        setStatusMessage(QStringLiteral("Local runtime unavailable."));
        return;
    }
    if (m_downloadedPath.isEmpty() || !QFileInfo::exists(m_downloadedPath)) {
        setStatusMessage(QStringLiteral("No downloaded file available to activate."));
        return;
    }
    const QString remotePath = !m_selectedFilePath.isEmpty() ? m_selectedFilePath : QFileInfo(m_downloadedPath).fileName();
    m_runtimeService->setActiveModel(m_selectedRepoId, remotePath, m_downloadedPath);
    setStatusMessage(QStringLiteral("Downloaded file set as active local model."));
}

void ModelHubViewModel::startServer() {
    if (!m_serverService || !m_runtimeService) {
        setStatusMessage(QStringLiteral("llama-server manager unavailable."));
        return;
    }
    if (!m_runtimeService->hasActiveModel()) {
        setStatusMessage(QStringLiteral("Choose an active GGUF file before starting the local server."));
        return;
    }
    m_serverService->startWithModel(m_runtimeService->activeLocalPath(), m_runtimeService->contextSize());
}

void ModelHubViewModel::stopServer() {
    if (!m_serverService) {
        setStatusMessage(QStringLiteral("llama-server manager unavailable."));
        return;
    }
    m_serverService->stop();
}

void ModelHubViewModel::probeServer() {
    if (!m_serverService) {
        return;
    }
    m_serverService->probeNow();
}

void ModelHubViewModel::clearServerLogs() {
    if (!m_serverService) {
        return;
    }
    m_serverService->clearLogs();
}

QString ModelHubViewModel::chooseRecommendedFile(const std::vector<ide::services::interfaces::ModelFileEntry>& files) const {
    int bestScore = std::numeric_limits<int>::min();
    QString bestPath;
    for (const auto& file : files) {
        const int currentScore = scoreFile(file);
        if (currentScore > bestScore) {
            bestScore = currentScore;
            bestPath = file.path;
        }
    }
    return bestPath;
}

std::vector<ide::services::interfaces::ModelFileEntry> ModelHubViewModel::filteredFiles(const std::vector<ide::services::interfaces::ModelFileEntry>& files) const {
    const QString filter = m_quantFilter.trimmed().toUpper();
    if (filter.isEmpty() || filter == QStringLiteral("AUTO")) {
        return files;
    }

    std::vector<ide::services::interfaces::ModelFileEntry> result;
    result.reserve(files.size());
    for (const auto& file : files) {
        if (file.quantization.trimmed().toUpper() == filter) {
            result.push_back(file);
        }
    }
    return result;
}

void ModelHubViewModel::rebuildQuantOptions(const std::vector<ide::services::interfaces::ModelFileEntry>& files) {
    QSet<QString> set;
    for (const auto& file : files) {
        const QString quant = file.quantization.trimmed().toUpper();
        if (!quant.isEmpty() && quant != QStringLiteral("UNKNOWN")) {
            set.insert(quant);
        }
    }

    QStringList options;
    options.push_back(QStringLiteral("AUTO"));
    QStringList sorted = set.values();
    std::sort(sorted.begin(), sorted.end(), [](const QString& a, const QString& b) {
        return a < b;
    });
    options.append(sorted);

    if (m_quantOptions != options) {
        m_quantOptions = options;
        emit quantOptionsChanged();
    }

    if (!m_quantOptions.contains(m_quantFilter, Qt::CaseInsensitive)) {
        m_quantFilter = QStringLiteral("AUTO");
        emit quantFilterChanged();
    }
}

int ModelHubViewModel::scoreFile(const ide::services::interfaces::ModelFileEntry& file) const {
    int score = 0;
    const QString name = file.name.toLower();
    const QString quant = file.quantization.toUpper();
    if (!file.isGguf) {
        return -100000;
    }
    if (file.isSplit) {
        score -= 10;
    }

    const bool codingBias = isCodingBiased(m_searchQuery) || m_recommendationProfile == QStringLiteral("coding");
    if (codingBias && (name.contains(QStringLiteral("coder")) || name.contains(QStringLiteral("code")))) {
        score += 24;
    }

    if (m_recommendationProfile == QStringLiteral("laptop") || m_recommendationProfile == QStringLiteral("small")) {
        if (quant == QStringLiteral("Q4_K_M")) score += 55;
        if (quant == QStringLiteral("Q4_K_S")) score += 50;
        if (quant == QStringLiteral("Q3_K_M")) score += 36;
        if (quant == QStringLiteral("Q5_K_M")) score += 20;
    } else if (m_recommendationProfile == QStringLiteral("quality")) {
        if (quant == QStringLiteral("Q6_K")) score += 56;
        if (quant == QStringLiteral("Q8_0")) score += 52;
        if (quant == QStringLiteral("Q5_K_M")) score += 36;
        if (quant == QStringLiteral("Q4_K_M")) score += 20;
    } else if (m_recommendationProfile == QStringLiteral("coding")) {
        if (quant == QStringLiteral("Q5_K_M")) score += 60;
        if (quant == QStringLiteral("Q4_K_M")) score += 48;
        if (quant == QStringLiteral("Q6_K")) score += 35;
        if (quant == QStringLiteral("Q8_0")) score += 18;
    } else {
        if (quant == QStringLiteral("Q5_K_M")) score += 58;
        if (quant == QStringLiteral("Q4_K_M")) score += 52;
        if (quant == QStringLiteral("Q6_K")) score += 42;
        if (quant == QStringLiteral("Q8_0")) score += 24;
    }

    const double budgetGiB = m_hardwareProfile.recommendedBudgetGiB();
    const double sizeGiB = fileSizeGiB(file);
    if (budgetGiB > 0.0 && sizeGiB > 0.0) {
        const double ratio = sizeGiB / budgetGiB;
        if (ratio <= 0.65) score += 18;
        else if (ratio <= 0.95) score += 30;
        else if (ratio <= 1.05) score += 12;
        else if (ratio <= 1.20) score -= 18;
        else if (ratio <= 1.40) score -= 46;
        else score -= 90;
    } else if (file.sizeBytes > 0) {
        score -= static_cast<int>(file.sizeBytes / (1024ll * 1024ll * 1024ll));
    }

    if (name.contains(QStringLiteral("imatrix"))) {
        score += 5;
    }
    if (name.contains(QStringLiteral("f16")) || name.contains(QStringLiteral("bf16"))) {
        score -= 70;
    }
    if (name.contains(QStringLiteral("q2")) || name.contains(QStringLiteral("q3"))) {
        score -= 12;
    }

    return score;
}

double ModelHubViewModel::fileSizeGiB(const ide::services::interfaces::ModelFileEntry& file) const {
    if (file.sizeBytes <= 0) {
        return -1.0;
    }
    return static_cast<double>(file.sizeBytes) / (1024.0 * 1024.0 * 1024.0);
}

QString ModelHubViewModel::candidateLocalPathForRemote(const QString& remotePath) const {
    if (m_selectedRepoId.trimmed().isEmpty()) {
        return QDir(m_targetDownloadDir).filePath(remotePath);
    }

    const QString repoRoot = QDir(m_targetDownloadDir).filePath(repoCacheDirName(m_selectedRepoId));
    const QString snapshotsRoot = QDir(repoRoot).filePath(QStringLiteral("snapshots"));
    const QString preferredPath = QDir(snapshotsRoot).filePath(QStringLiteral("main/%1").arg(remotePath));
    if (QFileInfo::exists(preferredPath)) {
        return preferredPath;
    }

    QDir snapshotsDir(snapshotsRoot);
    if (snapshotsDir.exists()) {
        const QFileInfoList snapshotDirs = snapshotsDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo& snapshotInfo : snapshotDirs) {
            const QString candidate = QDir(snapshotInfo.absoluteFilePath()).filePath(remotePath);
            if (QFileInfo::exists(candidate)) {
                return candidate;
            }
        }
    }

    return preferredPath;
}

void ModelHubViewModel::loadUiState() {
    m_wizardStep = qBound(0, m_uiSettings.value(QStringLiteral("modelHub/wizardStep"), 0).toInt(), 3);
}

void ModelHubViewModel::saveUiState() {
    m_uiSettings.setValue(QStringLiteral("modelHub/wizardStep"), m_wizardStep);
    m_uiSettings.sync();
}

void ModelHubViewModel::setStatusMessage(const QString& message) {
    m_statusMessage = message;
    emit statusMessageChanged();
}

void ModelHubViewModel::updateFileModel(const std::vector<ide::services::interfaces::ModelFileEntry>& files) {
    QString activeRemotePath;
    if (m_runtimeService && m_runtimeService->activeRepoId() == m_selectedRepoId) {
        activeRemotePath = m_runtimeService->activeRemotePath();
    }
    m_filesModel.setFiles(files, m_recommendedFilePath, activeRemotePath);
    emit filesChanged();
}

} // namespace ide::ui::viewmodels
