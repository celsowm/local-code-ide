#include "ui/viewmodels/ModelHubViewModel.hpp"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUrl>
#include <limits>
#include <utility>

namespace ide::ui::viewmodels {

namespace {
QString defaultDownloadDir() {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (!base.isEmpty()) {
        return QDir(base).filePath(QStringLiteral("models"));
    }
    return QDir(QDir::homePath()).filePath(QStringLiteral("LocalCodeIDE/models"));
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

QString ModelHubViewModel::autoDetectedProfile() const {
    return m_hardwareProfile.autoProfile(m_searchQuery);
}

QString ModelHubViewModel::hardwareSummary() const {
    return m_hardwareProfile.summary();
}

QString ModelHubViewModel::recommendationSummary() const {
    const double budget = m_hardwareProfile.recommendedBudgetGiB();
    QStringList parts;
    parts.push_back(QStringLiteral("perfil=%1").arg(m_recommendationProfile));
    parts.push_back(QStringLiteral("auto=%1").arg(autoDetectedProfile()));
    if (budget > 0.0) {
        parts.push_back(QStringLiteral("budget ~%1 GiB").arg(QString::number(budget, 'f', budget >= 10.0 ? 0 : 1)));
    }
    if (!m_recommendedFilePath.isEmpty()) {
        parts.push_back(QStringLiteral("sugestão=%1").arg(basenameFromRemotePath(m_recommendedFilePath)));
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
bool ModelHubViewModel::downloadActive() const { return m_downloader->isActive(); }
QString ModelHubViewModel::downloadedPath() const { return m_downloadedPath; }
QString ModelHubViewModel::providerName() const { return m_catalogService->providerName(); }
QString ModelHubViewModel::helperText() const {
    if (m_selectedRepoId.isEmpty()) {
        return QStringLiteral("Selecione um repositório GGUF. A recomendação agora usa RAM/VRAM detectadas localmente, com override opcional por env.");
    }

    return QStringLiteral("Hardware: %1. Dica padrão: laptop→Q4_K_M, balanced→Q5_K_M, quality→Q6_K/Q8_0, coding dá peso extra para arquivos coder/code.")
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
    return QStringLiteral("%1 -m /caminho/model.gguf -c %3 --host %4 --port %5%2")
        .arg(executable,
             extra.trimmed().isEmpty() ? QString() : QStringLiteral(" ") + extra.trimmed(),
             QString::number(runtimeContextSize()),
             host,
             QString::number(port));
}

QString ModelHubViewModel::currentLocalModelSummary() const {
    return m_runtimeService ? m_runtimeService->statusLine() : QStringLiteral("Runtime indisponível.");
}

QString ModelHubViewModel::currentLocalLaunchCommand() const {
    return m_runtimeService
        ? m_runtimeService->launchCommand(serverExecutablePath(), serverExtraArguments(), serverBaseUrl())
        : QStringLiteral("llama-server -m /caminho/model.gguf -c %1 --host 127.0.0.1 --port 8080").arg(QString::number(runtimeContextSize()));
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
    return m_serverService ? m_serverService->statusLine() : QStringLiteral("Gerenciador de llama-server indisponível.");
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
            ? QStringLiteral("Nenhum repositório encontrado.")
            : m_catalogService->lastError());
        return;
    }

    setStatusMessage(QStringLiteral("%1 repositório(s) encontrados em %2.")
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
    m_selectedFilePath.clear();
    m_recommendedFilePath.clear();
    emit selectedFileChanged();
    emit recommendedFileChanged();
    updateFileModel(files);
    if (files.empty()) {
        setStatusMessage(m_catalogService->lastError().isEmpty()
            ? QStringLiteral("Nenhum arquivo GGUF visível neste repositório.")
            : m_catalogService->lastError());
        return;
    }

    m_selectedFilePath = files.front().path;
    emit selectedFileChanged();

    suggestFile();
    setStatusMessage(QStringLiteral("%1 arquivo(s) GGUF carregados para %2.")
        .arg(QString::number(fileCount()), m_selectedRepoId));
}

void ModelHubViewModel::suggestFile() {
    const auto files = m_currentFiles.empty() ? m_filesModel.files() : m_currentFiles;
    m_recommendedFilePath = chooseRecommendedFile(files);
    emit recommendedFileChanged();
    updateFileModel(files);

    if (!m_recommendedFilePath.isEmpty()) {
        const QString basename = basenameFromRemotePath(m_recommendedFilePath);
        setStatusMessage(QStringLiteral("Sugestão: %1 · %2").arg(basename, recommendationSummary()));
    }
}

void ModelHubViewModel::startDownloadSelected() {
    if (m_selectedRepoId.isEmpty() || m_selectedFilePath.isEmpty()) {
        setStatusMessage(QStringLiteral("Selecione um repositório e um arquivo GGUF antes de baixar."));
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
        setStatusMessage(QStringLiteral("Não encontrei uma quant sugerida para o perfil atual."));
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
        setStatusMessage(QStringLiteral("Runtime local indisponível."));
        return;
    }
    if (m_selectedRepoId.isEmpty() || m_selectedFilePath.isEmpty()) {
        setStatusMessage(QStringLiteral("Selecione um GGUF antes de marcar como ativo."));
        return;
    }
    const QString candidate = candidateLocalPathForRemote(m_selectedFilePath);
    if (!QFileInfo::exists(candidate)) {
        setStatusMessage(QStringLiteral("Esse GGUF ainda não foi encontrado localmente. Faça o download antes de ativar."));
        return;
    }
    m_runtimeService->setActiveModel(m_selectedRepoId, m_selectedFilePath, candidate);
    setStatusMessage(QStringLiteral("Modelo ativo atualizado para %1").arg(QFileInfo(candidate).fileName()));
}

void ModelHubViewModel::useDownloadedAsCurrent() {
    if (!m_runtimeService) {
        setStatusMessage(QStringLiteral("Runtime local indisponível."));
        return;
    }
    if (m_downloadedPath.isEmpty() || !QFileInfo::exists(m_downloadedPath)) {
        setStatusMessage(QStringLiteral("Nenhum arquivo baixado disponível para ativar."));
        return;
    }
    const QString remotePath = !m_selectedFilePath.isEmpty() ? m_selectedFilePath : QFileInfo(m_downloadedPath).fileName();
    m_runtimeService->setActiveModel(m_selectedRepoId, remotePath, m_downloadedPath);
    setStatusMessage(QStringLiteral("Download marcado como modelo local ativo."));
}

void ModelHubViewModel::startServer() {
    if (!m_serverService || !m_runtimeService) {
        setStatusMessage(QStringLiteral("Gerenciador de llama-server indisponível."));
        return;
    }
    if (!m_runtimeService->hasActiveModel()) {
        setStatusMessage(QStringLiteral("Escolha um GGUF ativo antes de iniciar o servidor local."));
        return;
    }
    m_serverService->startWithModel(m_runtimeService->activeLocalPath(), m_runtimeService->contextSize());
}

void ModelHubViewModel::stopServer() {
    if (!m_serverService) {
        setStatusMessage(QStringLiteral("Gerenciador de llama-server indisponível."));
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
    return QDir(m_targetDownloadDir).filePath(remotePath);
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
