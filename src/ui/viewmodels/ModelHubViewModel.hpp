#pragma once

#include "adapters/modelhub/HuggingFaceFileDownloader.hpp"
#include "services/HardwareProfileService.hpp"
#include "services/LocalModelRuntimeService.hpp"
#include "services/LlamaServerProcessService.hpp"
#include "services/ModelCatalogService.hpp"
#include "ui/models/ModelFileListModel.hpp"
#include "ui/models/ModelRepoListModel.hpp"

#include <QObject>
#include <QSettings>
#include <memory>
#include <QStringList>
#include <vector>

namespace ide::ui::viewmodels {

class ModelHubViewModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString author READ author WRITE setAuthor NOTIFY authorChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString selectedRepoId READ selectedRepoId NOTIFY selectedRepoChanged)
    Q_PROPERTY(QString selectedFilePath READ selectedFilePath WRITE setSelectedFilePath NOTIFY selectedFileChanged)
    Q_PROPERTY(QString recommendedFilePath READ recommendedFilePath NOTIFY recommendedFileChanged)
    Q_PROPERTY(int wizardStep READ wizardStep WRITE setWizardStep NOTIFY wizardStepChanged)
    Q_PROPERTY(QString recommendationProfile READ recommendationProfile WRITE setRecommendationProfile NOTIFY recommendationProfileChanged)
    Q_PROPERTY(QString quantFilter READ quantFilter WRITE setQuantFilter NOTIFY quantFilterChanged)
    Q_PROPERTY(QStringList quantOptions READ quantOptions NOTIFY quantOptionsChanged)
    Q_PROPERTY(QString autoDetectedProfile READ autoDetectedProfile NOTIFY hardwareChanged)
    Q_PROPERTY(QString hardwareSummary READ hardwareSummary NOTIFY hardwareChanged)
    Q_PROPERTY(QString recommendationSummary READ recommendationSummary NOTIFY filesChanged)
    Q_PROPERTY(QString targetDownloadDir READ targetDownloadDir WRITE setTargetDownloadDir NOTIFY targetDownloadDirChanged)
    Q_PROPERTY(QString downloadStatus READ downloadStatus NOTIFY downloadStatusChanged)
    Q_PROPERTY(double downloadProgress READ downloadProgress NOTIFY downloadProgressChanged)
    Q_PROPERTY(QString downloadSpeedText READ downloadSpeedText NOTIFY downloadProgressChanged)
    Q_PROPERTY(QString downloadEtaText READ downloadEtaText NOTIFY downloadProgressChanged)
    Q_PROPERTY(QString downloadProgressDetails READ downloadProgressDetails NOTIFY downloadProgressChanged)
    Q_PROPERTY(QString downloadPhase READ downloadPhase NOTIFY downloadStatusChanged)
    Q_PROPERTY(QString downloadDiagnostics READ downloadDiagnostics NOTIFY downloadDiagnosticsChanged)
    Q_PROPERTY(bool downloadActive READ downloadActive NOTIFY downloadActiveChanged)
    Q_PROPERTY(QString downloadedPath READ downloadedPath NOTIFY downloadedPathChanged)
    Q_PROPERTY(QString providerName READ providerName CONSTANT)
    Q_PROPERTY(QString helperText READ helperText NOTIFY selectedRepoChanged)
    Q_PROPERTY(QString llamaLaunchHint READ llamaLaunchHint NOTIFY downloadedPathChanged)
    Q_PROPERTY(QString currentLocalModelSummary READ currentLocalModelSummary NOTIFY currentLocalModelChanged)
    Q_PROPERTY(QString currentLocalLaunchCommand READ currentLocalLaunchCommand NOTIFY currentLocalModelChanged)
    Q_PROPERTY(bool hasCurrentLocalModel READ hasCurrentLocalModel NOTIFY currentLocalModelChanged)
    Q_PROPERTY(bool canUseSelectedAsCurrent READ canUseSelectedAsCurrent NOTIFY selectedFileChanged)
    Q_PROPERTY(int runtimeContextSize READ runtimeContextSize WRITE setRuntimeContextSize NOTIFY currentLocalModelChanged)
    Q_PROPERTY(QString serverExecutablePath READ serverExecutablePath WRITE setServerExecutablePath NOTIFY serverRuntimeChanged)
    Q_PROPERTY(QString serverExtraArguments READ serverExtraArguments WRITE setServerExtraArguments NOTIFY serverRuntimeChanged)
    Q_PROPERTY(QString serverBaseUrl READ serverBaseUrl WRITE setServerBaseUrl NOTIFY serverRuntimeChanged)
    Q_PROPERTY(QString serverStatusLine READ serverStatusLine NOTIFY serverRuntimeChanged)
    Q_PROPERTY(QString serverLogs READ serverLogs NOTIFY serverLogsChanged)
    Q_PROPERTY(bool serverRunning READ serverRunning NOTIFY serverRuntimeChanged)
    Q_PROPERTY(bool serverStarting READ serverStarting NOTIFY serverRuntimeChanged)
    Q_PROPERTY(bool serverHealthy READ serverHealthy NOTIFY serverRuntimeChanged)
    Q_PROPERTY(bool canStartServer READ canStartServer NOTIFY serverRuntimeChanged)
    Q_PROPERTY(int repoCount READ repoCount NOTIFY reposChanged)
    Q_PROPERTY(int fileCount READ fileCount NOTIFY filesChanged)
    Q_PROPERTY(QObject* reposModel READ reposModel CONSTANT)
    Q_PROPERTY(QObject* filesModel READ filesModel CONSTANT)

public:
    ModelHubViewModel(std::unique_ptr<ide::services::ModelCatalogService> catalogService,
                      std::unique_ptr<ide::adapters::modelhub::HuggingFaceFileDownloader> downloader,
                      std::unique_ptr<ide::services::HardwareProfileService> hardwareService,
                      ide::services::LocalModelRuntimeService* runtimeService,
                      std::unique_ptr<ide::services::LlamaServerProcessService> serverService,
                      QObject* parent = nullptr);

    QString author() const;
    void setAuthor(const QString& value);

    QString searchQuery() const;
    void setSearchQuery(const QString& value);

    QString statusMessage() const;
    QString selectedRepoId() const;

    QString selectedFilePath() const;
    void setSelectedFilePath(const QString& value);

    QString recommendedFilePath() const;

    int wizardStep() const;
    void setWizardStep(int value);

    QString recommendationProfile() const;
    void setRecommendationProfile(const QString& value);
    QString quantFilter() const;
    void setQuantFilter(const QString& value);
    QStringList quantOptions() const;
    QString autoDetectedProfile() const;
    QString hardwareSummary() const;
    QString recommendationSummary() const;

    QString targetDownloadDir() const;
    void setTargetDownloadDir(const QString& value);

    QString downloadStatus() const;
    double downloadProgress() const;
    QString downloadSpeedText() const;
    QString downloadEtaText() const;
    QString downloadProgressDetails() const;
    QString downloadPhase() const;
    QString downloadDiagnostics() const;
    bool downloadActive() const;
    QString downloadedPath() const;
    QString providerName() const;
    QString helperText() const;
    QString llamaLaunchHint() const;

    QString currentLocalModelSummary() const;
    QString currentLocalLaunchCommand() const;
    bool hasCurrentLocalModel() const;
    bool canUseSelectedAsCurrent() const;
    int runtimeContextSize() const;
    void setRuntimeContextSize(int value);
    QString serverExecutablePath() const;
    void setServerExecutablePath(const QString& value);
    QString serverExtraArguments() const;
    void setServerExtraArguments(const QString& value);
    QString serverBaseUrl() const;
    void setServerBaseUrl(const QString& value);
    QString serverStatusLine() const;
    QString serverLogs() const;
    bool serverRunning() const;
    bool serverStarting() const;
    bool serverHealthy() const;
    bool canStartServer() const;

    int repoCount() const;
    int fileCount() const;

    QObject* reposModel();
    QObject* filesModel();

    Q_INVOKABLE void searchRepos();
    Q_INVOKABLE void selectRepo(const QString& repoId);
    Q_INVOKABLE void suggestFile();
    Q_INVOKABLE void startDownloadSelected();
    Q_INVOKABLE void downloadSuggested();
    Q_INVOKABLE void cancelDownload();
    Q_INVOKABLE void refreshHardware();
    Q_INVOKABLE void applyDetectedProfile();
    Q_INVOKABLE void useSelectedAsCurrent();
    Q_INVOKABLE void useDownloadedAsCurrent();
    Q_INVOKABLE void startServer();
    Q_INVOKABLE void stopServer();
    Q_INVOKABLE void probeServer();
    Q_INVOKABLE void clearServerLogs();

signals:
    void authorChanged();
    void searchQueryChanged();
    void statusMessageChanged();
    void selectedRepoChanged();
    void selectedFileChanged();
    void recommendedFileChanged();
    void wizardStepChanged();
    void recommendationProfileChanged();
    void quantFilterChanged();
    void quantOptionsChanged();
    void targetDownloadDirChanged();
    void downloadStatusChanged();
    void downloadProgressChanged();
    void downloadDiagnosticsChanged();
    void downloadActiveChanged();
    void downloadedPathChanged();
    void reposChanged();
    void filesChanged();
    void hardwareChanged();
    void currentLocalModelChanged();
    void serverRuntimeChanged();
    void serverLogsChanged();

private:
    QString chooseRecommendedFile(const std::vector<ide::services::interfaces::ModelFileEntry>& files) const;
    std::vector<ide::services::interfaces::ModelFileEntry> filteredFiles(const std::vector<ide::services::interfaces::ModelFileEntry>& files) const;
    void rebuildQuantOptions(const std::vector<ide::services::interfaces::ModelFileEntry>& files);
    int scoreFile(const ide::services::interfaces::ModelFileEntry& file) const;
    double fileSizeGiB(const ide::services::interfaces::ModelFileEntry& file) const;
    QString candidateLocalPathForRemote(const QString& remotePath) const;
    void setStatusMessage(const QString& message);
    void updateFileModel(const std::vector<ide::services::interfaces::ModelFileEntry>& files);
    void loadUiState();
    void saveUiState();

    std::unique_ptr<ide::services::ModelCatalogService> m_catalogService;
    std::unique_ptr<ide::adapters::modelhub::HuggingFaceFileDownloader> m_downloader;
    std::unique_ptr<ide::services::HardwareProfileService> m_hardwareService;
    ide::services::LocalModelRuntimeService* m_runtimeService = nullptr;
    std::unique_ptr<ide::services::LlamaServerProcessService> m_serverService;
    ide::services::HardwareProfile m_hardwareProfile;
    ide::ui::models::ModelRepoListModel m_reposModel;
    ide::ui::models::ModelFileListModel m_filesModel;
    QSettings m_uiSettings;

    QString m_author = QStringLiteral("bartowski");
    QString m_searchQuery;
    QString m_statusMessage;
    QString m_selectedRepoId;
    QString m_selectedFilePath;
    QString m_recommendedFilePath;
    int m_wizardStep = 0;
    QString m_recommendationProfile = QStringLiteral("balanced");
    QString m_quantFilter = QStringLiteral("AUTO");
    QStringList m_quantOptions = {QStringLiteral("AUTO")};
    QString m_targetDownloadDir;
    QString m_downloadedPath;
    std::vector<ide::services::interfaces::ModelFileEntry> m_currentFiles;
};

} // namespace ide::ui::viewmodels
