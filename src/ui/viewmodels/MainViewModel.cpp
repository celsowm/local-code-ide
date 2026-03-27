#include "ui/viewmodels/MainViewModel.hpp"
#include "adapters/diagnostics/LocalSyntaxDiagnosticProvider.hpp"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QFileInfo>
#include <QStandardPaths>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QMetaObject>
#include <QTimer>
#include <QUrl>
#include <QVariantMap>
#include <QtGlobal>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>

namespace ide::ui::viewmodels {

namespace {
struct RelevantContextComputationResult {
    std::vector<ide::services::RelevantContextFile> files;
    QString rendered;
};

struct AssistantExecutionResult {
    QString response;
    QStringList touchedPaths;
    int pendingApprovalCount = 0;
    int toolCallCount = 0;
};

struct SearchExecutionResult {
    std::vector<ide::services::interfaces::SearchResult> results;
};

struct TerminalExecutionResult {
    ide::services::interfaces::TerminalResult terminalResult;
};

struct GitDiffExecutionResult {
    QString path;
    QString title;
    QString original;
    QString modified;
    bool empty = false;
};

QString inferLanguageId(const QString& path) {
    const QString lower = path.toLower();
    if (lower.endsWith(".rs") || lower.endsWith(".ron")) return "rust";
    if (lower.endsWith(".py")) return "python";
    if (lower.endsWith(".js") || lower.endsWith(".ts") || lower.endsWith(".tsx") || lower.endsWith(".jsx")) return "typescript";
    if (lower.endsWith(".qml")) return "qml";
    if (lower.endsWith(".md") || lower.endsWith(".markdown") || lower.endsWith(".rst")) return "markdown";
    if (lower.endsWith(".json") || lower.endsWith(".jsonc") || lower.endsWith(".json5")) return "json";
    if (lower.endsWith(".yaml") || lower.endsWith(".yml")) return "yaml";
    if (lower.endsWith(".toml")) return "toml";
    if (lower.endsWith(".ini") || lower.endsWith(".conf")) return "ini";
    if (lower.endsWith(".ps1") || lower.endsWith(".psm1") || lower.endsWith(".psd1")) return "powershell";
    return "cpp";
}

QString displayTitleForPath(const QString& path) {
    const QFileInfo info(path);
    return info.fileName().isEmpty() ? QStringLiteral("untitled") : info.fileName();
}

QString normalized(const QString& text) {
    return text.trimmed().toLower();
}

bool isTransientDiagnosticsStatus(const QString& line) {
    const QString lowered = line.trimmed().toLower();
    if (lowered.isEmpty()) {
        return false;
    }
    if (lowered.contains(QStringLiteral("ready")) || lowered.contains(QStringLiteral("basic mode"))) {
        return false;
    }
    return lowered.contains(QStringLiteral("starting")) ||
           lowered.contains(QStringLiteral("initializ")) ||
           lowered.contains(QStringLiteral("launch")) ||
           lowered.contains(QStringLiteral("install")) ||
           lowered.contains(QStringLiteral("download")) ||
           lowered.contains(QStringLiteral("index")) ||
           lowered.contains(QStringLiteral("analyz")) ||
           lowered.contains(QStringLiteral("wait"));
}

QString loadTextFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    QTextStream stream(&file);
    return stream.readAll();
}

bool saveTextFile(const QString& path, const QString& text) {
    const QFileInfo info(path);
    QDir().mkpath(info.dir().absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }
    QTextStream stream(&file);
    stream << text;
    return true;
}

QString resolveExistingFilePath(const QString& rawPath, const QString& workspaceRoot) {
    const QString trimmed = rawPath.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }

    QString candidate;
    const QUrl url(trimmed);
    if (url.isValid() && url.isLocalFile()) {
        candidate = QDir::cleanPath(url.toLocalFile());
    } else {
        candidate = QDir::cleanPath(trimmed);
    }

    QFileInfo directInfo(candidate);
    if (directInfo.exists() && directInfo.isFile()) {
        return directInfo.absoluteFilePath();
    }

    if (directInfo.isRelative() && !workspaceRoot.trimmed().isEmpty()) {
        const QString rooted = QDir(workspaceRoot).filePath(candidate);
        const QFileInfo rootedInfo(rooted);
        if (rootedInfo.exists() && rootedInfo.isFile()) {
            return rootedInfo.absoluteFilePath();
        }
    }

    return {};
}

QString uiStateSettingsPath() {
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (base.isEmpty()) {
        base = QDir::homePath() + QStringLiteral("/.localcodeide");
    }
    QDir().mkpath(base);
    return QDir(base).filePath(QStringLiteral("ui.ini"));
}
}

MainViewModel::MainViewModel(std::unique_ptr<ide::services::DocumentService> documentService,
                             std::unique_ptr<ide::services::DiagnosticCoordinator> diagnosticCoordinator,
                             std::unique_ptr<ide::services::CodeIntelService> codeIntelService,
                             std::unique_ptr<ide::services::AiService> aiService,
                             std::unique_ptr<ide::services::GitService> gitService,
                             std::unique_ptr<ide::services::WorkspaceService> workspaceService,
                             std::unique_ptr<ide::services::SearchService> searchService,
                             std::unique_ptr<ide::services::TerminalService> terminalService,
                             QObject* parent)
    : QObject(parent)
    , m_documentService(std::move(documentService))
    , m_diagnosticCoordinator(std::move(diagnosticCoordinator))
    , m_codeIntelService(std::move(codeIntelService))
    , m_aiService(std::move(aiService))
    , m_gitService(std::move(gitService))
    , m_workspaceService(std::move(workspaceService))
    , m_searchService(std::move(searchService))
    , m_terminalService(std::move(terminalService))
    , m_editorManager(m_documentService.get(), this)
    , m_gitViewModel(m_gitService.get(), this)
    , m_uiStateManager(uiStateSettingsPath(), this) {
    
    m_workspaceRootPath = QDir::currentPath();
    m_uiStateManager.load();
    
    connect(&m_editorManager, &EditorManager::editorTextChanged, this, &MainViewModel::editorTextChanged);
    connect(&m_editorManager, &EditorManager::currentPathChanged, this, &MainViewModel::currentPathChanged);
    connect(&m_editorManager, &EditorManager::splitEditorChanged, this, &MainViewModel::splitEditorChanged);
    connect(&m_editorManager, &EditorManager::openEditorsChanged, this, &MainViewModel::openEditorsChanged);
    connect(&m_editorManager, &EditorManager::workspaceChanged, this, &MainViewModel::workspaceChanged);
    connect(&m_editorManager, &EditorManager::gitChanged, this, &MainViewModel::gitChanged);
    
    connect(&m_gitViewModel, &GitViewModel::gitSummaryChanged, this, &MainViewModel::gitSummaryChanged);
    connect(&m_gitViewModel, &GitViewModel::gitChanged, this, [this]() {
        m_workspaceTreeModel.setGitChanges(m_gitViewModel.currentChanges());
        emit gitChanged();
    });
    connect(&m_gitViewModel, &GitViewModel::busyChanged, this, [this]() {
        emit gitOperationStateChanged();
        refreshActivityStatus();
    });
    connect(&m_gitViewModel, &GitViewModel::operationCompleted, this,
            [this](const QString&, const QString&, bool, const QString& message) {
        updateStatusMessage(message);
    });
    connect(&m_gitViewModel, &GitViewModel::scmCommitMessageChanged, this, &MainViewModel::scmCommitMessageChanged);
    
    connect(&m_uiStateManager, &UiStateManager::primaryViewIndexChanged, this, &MainViewModel::primaryViewIndexChanged);
    connect(&m_uiStateManager, &UiStateManager::secondaryAiChanged, this, &MainViewModel::secondaryAiChanged);
    connect(&m_uiStateManager, &UiStateManager::bottomPanelChanged, this, &MainViewModel::bottomPanelChanged);
    connect(&m_uiStateManager, &UiStateManager::recentFoldersChanged, this, &MainViewModel::recentFoldersChanged);

    connect(&m_editorManager, &EditorManager::currentPathChanged, this, [this]() {
        if (m_diagnosticCoordinator) {
            m_diagnosticCoordinator->setDocumentSnapshot(currentPath(), editorText(), true);
        }
    });
    connect(&m_editorManager, &EditorManager::editorTextChanged, this, [this]() {
        if (m_diagnosticCoordinator) {
            m_diagnosticCoordinator->setDocumentSnapshot(currentPath(), editorText(), true);
        }
    });

    if (m_diagnosticCoordinator) {
        m_diagnosticsProviderName = m_diagnosticCoordinator->activeProviderLabel();
        m_diagnosticsStatusLine = m_diagnosticCoordinator->statusLine();
        connect(m_diagnosticCoordinator.get(), &ide::services::DiagnosticCoordinator::diagnosticsUpdated, this,
                [this](const std::vector<ide::services::interfaces::Diagnostic>& diagnostics, int) {
            applyDiagnostics(diagnostics);
        });
        connect(m_diagnosticCoordinator.get(), &ide::services::DiagnosticCoordinator::statusLineChanged, this,
                [this](const QString& line) {
            m_diagnosticsStatusLine = line;
            emit diagnosticsStatusChanged();
            refreshActivityStatus();
        });
        connect(m_diagnosticCoordinator.get(), &ide::services::DiagnosticCoordinator::providerLabelChanged, this,
                [this](const QString& label) {
            m_diagnosticsProviderName = label;
            emit diagnosticsStatusChanged();
            refreshActivityStatus();
        });
        connect(m_diagnosticCoordinator.get(), &ide::services::DiagnosticCoordinator::toastRequested, this, &MainViewModel::showToast);

        if (auto* sharedHub = m_diagnosticCoordinator->languageServerHub()) {
            m_secondaryDiagnosticCoordinator = std::make_unique<ide::services::DiagnosticCoordinator>(
                std::make_unique<ide::adapters::diagnostics::LocalSyntaxDiagnosticProvider>(),
                sharedHub,
                this
            );
            connect(m_secondaryDiagnosticCoordinator.get(), &ide::services::DiagnosticCoordinator::diagnosticsUpdated, this,
                    [this](const std::vector<ide::services::interfaces::Diagnostic>& diagnostics, int) {
                QVariantList rows;
                rows.reserve(static_cast<int>(diagnostics.size()));
                for (const auto& item : diagnostics) {
                    QVariantMap row;
                    row.insert(QStringLiteral("filePath"), item.filePath);
                    row.insert(QStringLiteral("lineStart"), item.lineStart);
                    row.insert(QStringLiteral("columnStart"), item.columnStart);
                    row.insert(QStringLiteral("lineEnd"), item.lineEnd);
                    row.insert(QStringLiteral("columnEnd"), item.columnEnd);
                    row.insert(QStringLiteral("message"), item.message);
                    row.insert(QStringLiteral("source"), item.source);
                    row.insert(QStringLiteral("code"), item.code);
                    switch (item.severity) {
                    case ide::services::interfaces::Diagnostic::Severity::Info:
                        row.insert(QStringLiteral("severity"), QStringLiteral("info"));
                        break;
                    case ide::services::interfaces::Diagnostic::Severity::Warning:
                        row.insert(QStringLiteral("severity"), QStringLiteral("warning"));
                        break;
                    case ide::services::interfaces::Diagnostic::Severity::Error:
                        row.insert(QStringLiteral("severity"), QStringLiteral("error"));
                        break;
                    }
                    rows.push_back(row);
                }
                m_secondaryFileDiagnostics = rows;
                emit splitEditorChanged();
            });
            connect(m_secondaryDiagnosticCoordinator.get(), &ide::services::DiagnosticCoordinator::toastRequested,
                    this, &MainViewModel::showToast);
        }
    }

    connect(&m_editorManager, &EditorManager::splitEditorChanged, this, [this]() {
        if (!m_secondaryDiagnosticCoordinator) {
            return;
        }
        if (!splitEditorVisible() || diffEditorVisible() || secondaryEditorPath().trimmed().isEmpty()) {
            if (!m_secondaryFileDiagnostics.isEmpty()) {
                m_secondaryFileDiagnostics.clear();
                emit splitEditorChanged();
            }
            return;
        }
        m_secondaryDiagnosticCoordinator->setDocumentSnapshot(secondaryEditorPath(), secondaryEditorText(), true);
    });

    refreshWorkspace();
    refreshPendingApprovals();
    QTimer::singleShot(0, this, [this]() { analyzeNow(); });
    updateStatusMessage("LocalCodeIDE v3.3 workbench loaded.");
    refreshActivityStatus();
}

QString MainViewModel::editorText() const { return m_documentService->currentDocument().text(); }

void MainViewModel::setEditorText(const QString& text) {
    if (text == m_documentService->currentDocument().text()) {
        return;
    }
    m_documentService->setText(text);
    if (m_diagnosticCoordinator) {
        m_diagnosticCoordinator->setDocumentSnapshot(currentPath(), text, false);
    }
    emit editorTextChanged();
    emit aiSettingsChanged();
}

QString MainViewModel::currentPath() const { return m_documentService->currentDocument().path(); }
QString MainViewModel::languageId() const { return inferLanguageId(currentPath()); }
QString MainViewModel::editorTabTitle() const { return displayTitleForPath(currentPath()); }
QString MainViewModel::chatInput() const { return m_chatInput; }

void MainViewModel::setChatInput(const QString& text) {
    if (text == m_chatInput) return;
    m_chatInput = text;
    emit chatInputChanged();
}

QString MainViewModel::chatResponse() const { return m_chatResponse; }
QString MainViewModel::statusMessage() const { return m_statusMessage; }
QString MainViewModel::diagnosticsProviderName() const {
    return m_diagnosticsProviderName.isEmpty() ? QStringLiteral("Local Diagnostics") : m_diagnosticsProviderName;
}
QString MainViewModel::diagnosticsStatusLine() const { return m_diagnosticsStatusLine; }
bool MainViewModel::definitionAvailable() const {
    return !m_diagnosticsStatusLine.trimmed().toLower().contains(QStringLiteral("basic mode"));
}
QString MainViewModel::codeIntelProviderName() const { return m_codeIntelService->providerName(); }
QString MainViewModel::aiBackendName() const { return m_aiService->backendName(); }
QString MainViewModel::aiBackendStatusLine() const { return m_aiService->backendStatusLine(); }
bool MainViewModel::aiBusy() const { return m_aiBusy; }
int MainViewModel::diagnosticsCount() const { return m_diagnosticsModel.diagnosticCount(); }
int MainViewModel::workspaceFileCount() const { return m_workspaceFilesModel.fileCount(); }
bool MainViewModel::workspaceLoading() const { return m_workspaceLoading; }
QString MainViewModel::workspaceLoadingText() const { return m_workspaceLoadingText; }
int MainViewModel::searchResultCount() const { return m_searchResultsModel.resultCount(); }
bool MainViewModel::searchBusy() const { return m_searchBusy; }
int MainViewModel::completionCount() const { return m_completionModel.itemCount(); }
bool MainViewModel::codeIntelBusy() const { return m_codeIntelBusy; }
int MainViewModel::relevantContextCount() const { return m_relevantContextModel.fileCount(); }

QString MainViewModel::workspaceRootPath() const { return m_workspaceRootPath; }
void MainViewModel::setWorkspaceRootPath(const QString& path) {
    if (path == m_workspaceRootPath || path.trimmed().isEmpty()) return;
    m_workspaceRootPath = path;
    emit workspaceChanged();
    refreshWorkspace();
    emit aiSettingsChanged();
}

QString MainViewModel::searchPattern() const { return m_searchPattern; }
void MainViewModel::setSearchPattern(const QString& pattern) {
    if (pattern == m_searchPattern) return;
    m_searchPattern = pattern;
    emit searchPatternChanged();
}

QString MainViewModel::hoverText() const { return m_hoverText; }
QString MainViewModel::definitionText() const { return m_definitionText; }
QString MainViewModel::terminalCommand() const { return m_terminalCommand; }
void MainViewModel::setTerminalCommand(const QString& command) {
    if (command == m_terminalCommand) return;
    m_terminalCommand = command;
    emit terminalCommandChanged();
}
QString MainViewModel::terminalOutput() const { return m_terminalOutput; }
bool MainViewModel::terminalBusy() const { return m_terminalBusy; }
QString MainViewModel::terminalBackendName() const { return m_terminalService->backendName(); }

QString MainViewModel::aiSystemPrompt() const { return m_aiService->systemPrompt(); }
void MainViewModel::setAiSystemPrompt(const QString& value) { m_aiService->setSystemPrompt(value); emit aiSettingsChanged(); }
double MainViewModel::aiTemperature() const { return m_aiService->temperature(); }
void MainViewModel::setAiTemperature(double value) { m_aiService->setTemperature(value); emit aiSettingsChanged(); }
int MainViewModel::aiMaxOutputTokens() const { return m_aiService->maxOutputTokens(); }
void MainViewModel::setAiMaxOutputTokens(int value) { m_aiService->setMaxOutputTokens(value); emit aiSettingsChanged(); }
int MainViewModel::aiDocumentContextChars() const { return m_aiService->documentContextChars(); }
void MainViewModel::setAiDocumentContextChars(int value) { m_aiService->setDocumentContextChars(value); emit aiSettingsChanged(); }
int MainViewModel::aiWorkspaceContextChars() const { return m_aiService->workspaceContextChars(); }
void MainViewModel::setAiWorkspaceContextChars(int value) { m_aiService->setWorkspaceContextChars(value); refreshRelevantContext(); emit aiSettingsChanged(); }
int MainViewModel::aiWorkspaceContextMaxFiles() const { return m_aiService->workspaceContextMaxFiles(); }
void MainViewModel::setAiWorkspaceContextMaxFiles(int value) { m_aiService->setWorkspaceContextMaxFiles(value); refreshRelevantContext(); emit aiSettingsChanged(); }
int MainViewModel::aiMaxToolRounds() const { return m_aiService->maxToolRounds(); }
void MainViewModel::setAiMaxToolRounds(int value) { m_aiService->setMaxToolRounds(value); emit aiSettingsChanged(); }
bool MainViewModel::aiIncludeDocument() const { return m_aiService->includeDocument(); }
void MainViewModel::setAiIncludeDocument(bool value) { m_aiService->setIncludeDocument(value); emit aiSettingsChanged(); }
bool MainViewModel::aiIncludeDiagnostics() const { return m_aiService->includeDiagnostics(); }
void MainViewModel::setAiIncludeDiagnostics(bool value) { m_aiService->setIncludeDiagnostics(value); emit aiSettingsChanged(); }
bool MainViewModel::aiIncludeGitSummary() const { return m_aiService->includeGitSummary(); }
void MainViewModel::setAiIncludeGitSummary(bool value) { m_aiService->setIncludeGitSummary(value); emit aiSettingsChanged(); }
bool MainViewModel::aiIncludeWorkspaceContext() const { return m_aiService->includeWorkspaceContext(); }
void MainViewModel::setAiIncludeWorkspaceContext(bool value) { m_aiService->setIncludeWorkspaceContext(value); emit aiSettingsChanged(); }
bool MainViewModel::aiEnableTools() const { return m_aiService->toolsEnabled(); }
void MainViewModel::setAiEnableTools(bool value) { m_aiService->setToolsEnabled(value); emit aiSettingsChanged(); }
bool MainViewModel::aiRequireApprovalForDestructive() const { return m_aiService->requireApprovalForDestructiveTools(); }
void MainViewModel::setAiRequireApprovalForDestructive(bool value) { m_aiService->setRequireApprovalForDestructiveTools(value); emit aiSettingsChanged(); }
QString MainViewModel::aiContextSummary() const { return m_aiService->contextSummary(currentPath(), editorText(), diagnosticsSummary(), gitSummary(), m_renderedWorkspaceContext); }
QString MainViewModel::relevantContextSummary() const { return m_relevantContextSummary; }
QString MainViewModel::toolCallLog() const { return m_aiService->lastToolLog(); }
int MainViewModel::toolCallCount() const { return m_aiService->lastToolCallCount(); }
QString MainViewModel::toolCatalogSummary() const { return m_aiService->toolCatalogSummary(); }
int MainViewModel::pendingApprovalCount() const { return m_pendingApprovalModel.approvalCount(); }
QString MainViewModel::pendingApprovalSummary() const { return m_aiService->pendingApprovalSummary(); }

QObject* MainViewModel::diagnosticsModel() { return &m_diagnosticsModel; }
QVariantList MainViewModel::currentFileDiagnostics() const { return m_currentFileDiagnostics; }
QVariantList MainViewModel::secondaryFileDiagnostics() const { return m_secondaryFileDiagnostics; }
QObject* MainViewModel::workspaceFilesModel() { return &m_workspaceFilesModel; }
QObject* MainViewModel::workspaceTreeModel() { return &m_workspaceTreeModel; }
QObject* MainViewModel::searchResultsModel() { return &m_searchResultsModel; }
QObject* MainViewModel::completionModel() { return &m_completionModel; }
QObject* MainViewModel::relevantContextModel() { return &m_relevantContextModel; }
QObject* MainViewModel::pendingApprovalModel() { return &m_pendingApprovalModel; }

bool MainViewModel::hasPatchPreview() const { return m_patchPreview.hasPatch; }
bool MainViewModel::canApplyPatchPreview() const { return m_patchPreview.canApply; }
QString MainViewModel::patchSummary() const { return m_patchPreview.summary; }
QString MainViewModel::patchPreviewText() const { return m_patchPreview.diffText; }

QObject* MainViewModel::openEditorsModel() { return m_editorManager.openEditorsModel(); }
int MainViewModel::openEditorCount() const { return m_editorManager.openEditorCount(); }
bool MainViewModel::showWelcomeTab() const { return openEditorCount() == 0; }
QStringList MainViewModel::recentFolders() const {
    QStringList visible;
    const QStringList stored = m_uiStateManager.recentFolders();
    for (const QString& path : stored) {
        const QString cleaned = QDir::cleanPath(path.trimmed());
        if (cleaned.isEmpty()) {
            continue;
        }
        const QFileInfo info(cleaned);
        if (!info.exists() || !info.isDir()) {
            continue;
        }
        visible << info.absoluteFilePath();
    }
    return visible;
}
QObject* MainViewModel::commandPaletteModel() { return &m_commandPaletteModel; }
int MainViewModel::commandPaletteCount() const { return m_commandPaletteModel.itemCount(); }

int MainViewModel::primaryViewIndex() const { return m_uiStateManager.primaryViewIndex(); }
void MainViewModel::setPrimaryViewIndex(int value) { m_uiStateManager.setPrimaryViewIndex(value); m_uiStateManager.save(); }
bool MainViewModel::secondaryAiVisible() const { return m_uiStateManager.secondaryAiVisible(); }
void MainViewModel::setSecondaryAiVisible(bool value) { m_uiStateManager.setSecondaryAiVisible(value); m_uiStateManager.save(); }
int MainViewModel::secondaryAiTab() const { return m_uiStateManager.secondaryAiTab(); }
void MainViewModel::setSecondaryAiTab(int value) { m_uiStateManager.setSecondaryAiTab(value); m_uiStateManager.save(); }
int MainViewModel::secondaryAiWidth() const { return m_uiStateManager.secondaryAiWidth(); }
void MainViewModel::setSecondaryAiWidth(int value) { m_uiStateManager.setSecondaryAiWidth(value); m_uiStateManager.save(); }
int MainViewModel::bottomPanelTab() const { return m_uiStateManager.bottomPanelTab(); }
void MainViewModel::setBottomPanelTab(int value) { m_uiStateManager.setBottomPanelTab(value); m_uiStateManager.save(); }
int MainViewModel::bottomPanelHeight() const { return m_uiStateManager.bottomPanelHeight(); }
void MainViewModel::setBottomPanelHeight(int value) { m_uiStateManager.setBottomPanelHeight(value); m_uiStateManager.save(); }

bool MainViewModel::currentDocumentDirty() const { return editorText() != m_documentService->currentDocument().text(); }

QObject* MainViewModel::gitChangesModel() { return m_gitViewModel.gitChangesModel(); }
QObject* MainViewModel::scmSectionsModel() { return m_gitViewModel.scmSectionsModel(); }
QObject* MainViewModel::gitRecentCommitsModel() { return m_gitViewModel.gitRecentCommitsModel(); }
int MainViewModel::gitChangeCount() const { return m_gitViewModel.gitChangeCount(); }
int MainViewModel::gitStagedCount() const { return m_gitViewModel.gitStagedCount(); }
int MainViewModel::gitUnstagedCount() const { return m_gitViewModel.gitUnstagedCount(); }
int MainViewModel::gitUntrackedCount() const { return m_gitViewModel.gitUntrackedCount(); }
int MainViewModel::gitRecentCommitCount() const { return m_gitViewModel.gitRecentCommitCount(); }
QString MainViewModel::scmCommitMessage() const { return m_gitViewModel.scmCommitMessage(); }
void MainViewModel::setScmCommitMessage(const QString& value) { m_gitViewModel.setScmCommitMessage(value); }

QString MainViewModel::gitSummary() const { return m_gitViewModel.gitSummary(); }
QString MainViewModel::gitBranchLabel() const { return m_gitViewModel.gitBranchLabel(); }
bool MainViewModel::gitBusy() const { return m_gitViewModel.busy(); }
bool MainViewModel::gitDiffBusy() const { return m_gitDiffBusy; }

bool MainViewModel::splitEditorVisible() const { return m_editorManager.splitEditorVisible(); }
bool MainViewModel::diffEditorVisible() const { return m_editorManager.diffEditorVisible(); }
QString MainViewModel::secondaryEditorPath() const { return m_editorManager.secondaryEditorPath(); }
QString MainViewModel::secondaryEditorText() const { return m_editorManager.secondaryEditorText(); }
void MainViewModel::setSecondaryEditorText(const QString& text) { m_editorManager.setSecondaryEditorText(text); }
QString MainViewModel::secondaryLanguageId() const { return m_editorManager.secondaryLanguageId(); }
QString MainViewModel::secondaryEditorTitle() const { return m_editorManager.secondaryEditorTitle(); }
bool MainViewModel::secondaryEditorDirty() const { return m_editorManager.secondaryEditorDirty(); }
QString MainViewModel::diffOriginalText() const { return m_editorManager.diffOriginalText(); }
QString MainViewModel::diffModifiedText() const { return m_editorManager.diffModifiedText(); }
QString MainViewModel::diffEditorTitle() const { return m_editorManager.diffEditorTitle(); }
QString MainViewModel::toastMessage() const { return m_toastMessage; }
bool MainViewModel::toastVisible() const { return m_toastVisible; }
int MainViewModel::cursorLine() const { return m_cursorLine; }
int MainViewModel::cursorColumn() const { return m_cursorColumn; }

void MainViewModel::analyzeNow() {
    if (!m_diagnosticCoordinator) {
        return;
    }
    m_diagnosticCoordinator->setDocumentSnapshot(currentPath(), editorText(), true);
    updateStatusMessage(QString("Diagnostics refreshed (%1).").arg(diagnosticsProviderName()));
}

void MainViewModel::askAssistant() {
    if (m_aiBusy) {
        updateStatusMessage(QStringLiteral("Assistant request already running."));
        return;
    }

    refreshRelevantContext();
    const QString prompt = m_chatInput;
    const QString workspaceRoot = m_workspaceRootPath;
    const QString filePath = currentPath();
    const QString text = editorText();
    const QString diagnosticsText = diagnosticsSummary();
    const QString gitText = gitSummary();
    const QString workspaceContextText = m_renderedWorkspaceContext;
    const QString backendNameSnapshot = aiBackendName();
    const int generation = ++m_assistantRequestGeneration;

    m_aiBusy = true;
    emit aiBusyChanged();
    refreshActivityStatus();
    updateStatusMessage(QStringLiteral("Assistant is generating a response..."));

    auto* watcher = new QFutureWatcher<AssistantExecutionResult>(this);
    connect(watcher, &QFutureWatcher<AssistantExecutionResult>::finished, this,
            [this, watcher, generation, backendNameSnapshot]() {
        const AssistantExecutionResult result = watcher->result();
        watcher->deleteLater();

        if (generation != m_assistantRequestGeneration) {
            return;
        }

        m_chatResponse = result.response;
        emit chatResponseChanged();
        syncAfterToolRun(result.touchedPaths);
        extractPatchPreview();
        refreshPendingApprovals();
        emit aiSettingsChanged();

        m_aiBusy = false;
        emit aiBusyChanged();
        refreshActivityStatus();

        if (result.pendingApprovalCount > 0) {
            updateStatusMessage(QString("%1 tool(s) waiting for manual approval.").arg(QString::number(result.pendingApprovalCount)));
        } else if (result.toolCallCount > 0) {
            updateStatusMessage(QString("Response generated via %1 with %2 tool call(s).")
                                    .arg(backendNameSnapshot, QString::number(result.toolCallCount)));
        } else {
            updateStatusMessage(QString("Response generated via %1.").arg(backendNameSnapshot));
        }
    });

    watcher->setFuture(QtConcurrent::run([this, prompt, workspaceRoot, filePath, text, diagnosticsText, gitText, workspaceContextText]() {
        AssistantExecutionResult result;
        result.response = m_aiService->ask(
            prompt,
            workspaceRoot,
            filePath,
            text,
            diagnosticsText,
            gitText,
            workspaceContextText
        );
        result.touchedPaths = m_aiService->lastToolTouchedPaths();
        result.pendingApprovalCount = m_aiService->pendingApprovalCount();
        result.toolCallCount = m_aiService->lastToolCallCount();
        return result;
    }));
}

void MainViewModel::saveCurrent() {
    if (m_documentService->saveFile()) {
        m_editorManager.saveCurrent();
        analyzeNow();
        emit currentPathChanged();
        refreshWorkspace();
        refreshGitState();
        updateStatusMessage(QString("Saved %1.").arg(currentPath()));
        return;
    }
    updateStatusMessage("Failed to save file.");
}

void MainViewModel::loadSampleCpp() {
    setDocumentSample("sample/main.cpp",
        "#include <iostream>\n"
        "#include <vector>\n\n"
        "int sum(const std::vector<int>& xs) {\n"
        "    int total = 0;\n"
        "    for (int value : xs) total += value;\n"
        "    return total;\n"
        "}\n\n"
        "int main() {\n"
        "    std::vector<int> numbers{1,2,3};\n"
        "    std::cout << \"Hello from LocalCodeIDE v3.2\\n\";\n"
        "    return 0;\n"
        "}\n");
}

void MainViewModel::loadSampleRust() {
    setDocumentSample("sample/main.rs",
        "fn add(a: i32, b: i32) -> i32 {\n"
        "    a + b\n"
        "}\n\n"
        "fn main() {\n"
        "    println!(\"{}\", add(1, 2));\n"
        "}\n");
}

void MainViewModel::refreshWorkspace() {
    startWorkspaceRefresh(QStringLiteral("Indexing workspace files..."));
}

void MainViewModel::openWorkspaceFile(const QString& path) {
    const QString resolvedPath = resolveExistingFilePath(path, m_workspaceRootPath);
    if (resolvedPath.isEmpty()) {
        updateStatusMessage(QString("Failed to open file: %1").arg(path));
        return;
    }
    m_workspaceTreeModel.expandToPath(resolvedPath);
    goToDocumentLocation(resolvedPath, 1, 1);
}

void MainViewModel::triggerOpenFileDialog() {
    const QString selected = QFileDialog::getOpenFileName(
        nullptr,
        QStringLiteral("Open File"),
        m_workspaceRootPath
    );
    if (selected.trimmed().isEmpty()) {
        return;
    }
    openFileFromDialog(selected);
}

void MainViewModel::triggerOpenFolderDialog() {
    const QString selected = QFileDialog::getExistingDirectory(
        nullptr,
        QStringLiteral("Open Folder"),
        m_workspaceRootPath
    );
    if (selected.trimmed().isEmpty()) {
        return;
    }
    openFolderFromDialog(selected);
}

void MainViewModel::triggerQuickOpen() {
    emit quickOpenRequested();
}

void MainViewModel::openFileFromDialog(const QString& filePath) {
    const QString localPath = localPathFromUrlOrPath(filePath);
    const QFileInfo info(localPath);
    if (!info.exists() || !info.isFile()) {
        updateStatusMessage(QString("Selected file is not valid: %1").arg(localPath));
        return;
    }
    openWorkspaceFile(info.absoluteFilePath());
}

void MainViewModel::openFolderFromDialog(const QString& folderPath) {
    const QString localPath = localPathFromUrlOrPath(folderPath);
    const QFileInfo info(localPath);
    if (!info.exists() || !info.isDir()) {
        updateStatusMessage(QString("Selected folder is not valid: %1").arg(localPath));
        removeRecentFolder(localPath);
        return;
    }

    m_editorManager.closeAllEditors();
    setWorkspaceRootPath(info.absoluteFilePath());
    addRecentFolder(info.absoluteFilePath());
    clearPatchPreview();
    updateStatusMessage(QString("Opened folder: %1").arg(info.absoluteFilePath()));
}

void MainViewModel::reopenRecentFolder(const QString& folderPath) {
    const QString localPath = localPathFromUrlOrPath(folderPath);
    const QFileInfo info(localPath);
    if (!info.exists() || !info.isDir()) {
        removeRecentFolder(localPath);
        updateStatusMessage(QString("Recent folder no longer exists: %1").arg(localPath));
        return;
    }
    openFolderFromDialog(localPath);
}

void MainViewModel::removeRecentFolder(const QString& folderPath) {
    m_uiStateManager.removeRecentFolder(localPathFromUrlOrPath(folderPath));
    m_uiStateManager.save();
}

void MainViewModel::openWorkspaceFileInSplit(const QString& path) {
    const QString resolvedPath = resolveExistingFilePath(path, m_workspaceRootPath);
    if (resolvedPath.isEmpty()) {
        updateStatusMessage(QString("Failed to open split editor file: %1").arg(path));
        return;
    }
    m_editorManager.openFileInSplit(resolvedPath);
    m_workspaceTreeModel.expandToPath(resolvedPath);
    updateStatusMessage(QString("Split editor opened for %1").arg(resolvedPath));
}

void MainViewModel::createWorkspaceFile() {
    const QString path = uniqueWorkspaceChildPath(QStringLiteral("untitled.txt"));
    if (path.isEmpty()) {
        updateStatusMessage(QStringLiteral("Workspace inválido para criar arquivo."));
        return;
    }
    if (!saveTextFile(path, QString())) {
        updateStatusMessage(QString("Falha ao criar arquivo em %1").arg(path));
        return;
    }
    refreshWorkspace();
    refreshGitState();
    openWorkspaceFile(path);
    updateStatusMessage(QString("Arquivo criado: %1").arg(QFileInfo(path).fileName()));
}

void MainViewModel::createWorkspaceFolder() {
    const QString path = uniqueWorkspaceChildPath(QStringLiteral("NewFolder"));
    if (path.isEmpty()) {
        updateStatusMessage(QStringLiteral("Workspace inválido para criar pasta."));
        return;
    }
    QDir root(m_workspaceRootPath);
    if (!root.mkpath(QFileInfo(path).fileName())) {
        updateStatusMessage(QString("Falha ao criar pasta em %1").arg(path));
        return;
    }
    refreshWorkspace();
    refreshGitState();
    updateStatusMessage(QString("Pasta criada: %1").arg(QFileInfo(path).fileName()));
}

void MainViewModel::toggleWorkspaceFolder(const QString& id) {
    m_workspaceTreeModel.toggleExpanded(id);
}

void MainViewModel::collapseWorkspaceFolders() {
    m_workspaceTreeModel.collapseAll();
}

void MainViewModel::runSearch() {
    if (m_searchBusy) {
        updateStatusMessage(QStringLiteral("Search already running."));
        return;
    }

    const QString workspaceRoot = m_workspaceRootPath;
    const QString pattern = m_searchPattern;
    const int generation = ++m_searchGeneration;
    m_searchBusy = true;
    emit searchBusyChanged();
    refreshActivityStatus();
    updateStatusMessage(QStringLiteral("Searching workspace..."));

    auto* watcher = new QFutureWatcher<SearchExecutionResult>(this);
    connect(watcher, &QFutureWatcher<SearchExecutionResult>::finished, this, [this, watcher, generation]() {
        const SearchExecutionResult result = watcher->result();
        watcher->deleteLater();

        if (generation != m_searchGeneration) {
            return;
        }

        m_searchResultsModel.setResults(result.results);
        emit searchResultsChanged();

        m_searchBusy = false;
        emit searchBusyChanged();
        refreshActivityStatus();
        updateStatusMessage(QString("Search returned %1 result(s).").arg(QString::number(searchResultCount())));
    });

    watcher->setFuture(QtConcurrent::run([this, workspaceRoot, pattern]() {
        SearchExecutionResult result;
        result.results = m_searchService->search(workspaceRoot, pattern);
        return result;
    }));
}

void MainViewModel::openSearchResult(const QString& path, int line, int column) { goToDocumentLocation(path, line, column); }

void MainViewModel::setCursorPosition(int line, int column) {
    if (m_cursorLine == line && m_cursorColumn == column) return;
    m_cursorLine = qMax(1, line);
    m_cursorColumn = qMax(1, column);
    emit cursorPositionChanged();
}

void MainViewModel::requestCompletionsAtCursor() {
    if (m_codeIntelBusy) {
        updateStatusMessage(QStringLiteral("Code intel request already running."));
        return;
    }

    const ide::services::interfaces::EditorPosition position{m_cursorLine, m_cursorColumn};
    const QString path = currentPath();
    const QString text = editorText();
    const int line = m_cursorLine;
    const int column = m_cursorColumn;
    const int generation = ++m_codeIntelGeneration;
    m_codeIntelBusy = true;
    emit codeIntelBusyChanged();
    refreshActivityStatus();
    updateStatusMessage(QString("Completion requested at %1:%2 via %3...")
                            .arg(QString::number(line), QString::number(column), codeIntelProviderName()));

    m_codeIntelService->completionsAsync(path, text, position, [this, generation, line, column](std::vector<ide::services::interfaces::CompletionItem> items) {
        if (generation != m_codeIntelGeneration) {
            return;
        }
        m_completionModel.setItems(std::move(items));
        emit completionsChanged();
        m_codeIntelBusy = false;
        emit codeIntelBusyChanged();
        refreshActivityStatus();
        updateStatusMessage(QString("Completion ready at %1:%2 via %3.")
                                .arg(QString::number(line), QString::number(column), codeIntelProviderName()));
    });
    QTimer::singleShot(2000, this, [this, generation]() {
        if (generation == m_codeIntelGeneration && m_codeIntelBusy) {
            m_codeIntelBusy = false;
            emit codeIntelBusyChanged();
            refreshActivityStatus();
            updateStatusMessage(QStringLiteral("Completion request timed out."));
        }
    });
}

void MainViewModel::requestHoverAtCursor() {
    if (m_codeIntelBusy) {
        updateStatusMessage(QStringLiteral("Code intel request already running."));
        return;
    }

    const ide::services::interfaces::EditorPosition position{m_cursorLine, m_cursorColumn};
    const QString path = currentPath();
    const QString text = editorText();
    const int line = m_cursorLine;
    const int column = m_cursorColumn;
    const int generation = ++m_codeIntelGeneration;
    m_codeIntelBusy = true;
    emit codeIntelBusyChanged();
    refreshActivityStatus();
    updateStatusMessage(QString("Hover requested at %1:%2...").arg(QString::number(line), QString::number(column)));

    m_codeIntelService->hoverAsync(path, text, position, [this, generation, line, column](ide::services::interfaces::HoverInfo info) {
        if (generation != m_codeIntelGeneration) {
            return;
        }
        m_hoverText = info.contents;
        emit hoverTextChanged();
        m_codeIntelBusy = false;
        emit codeIntelBusyChanged();
        refreshActivityStatus();
        updateStatusMessage(QString("Hover ready at %1:%2.").arg(QString::number(line), QString::number(column)));
    });
    QTimer::singleShot(2000, this, [this, generation]() {
        if (generation == m_codeIntelGeneration && m_codeIntelBusy) {
            m_codeIntelBusy = false;
            emit codeIntelBusyChanged();
            refreshActivityStatus();
            updateStatusMessage(QStringLiteral("Hover request timed out."));
        }
    });
}

void MainViewModel::requestDefinitionAtCursor() {
    if (!definitionAvailable()) {
        m_definitionText = QStringLiteral("Go to Definition is unavailable in basic mode for this language.");
        emit definitionTextChanged();
        updateStatusMessage(m_definitionText);
        return;
    }
    const ide::services::interfaces::EditorPosition position{m_cursorLine, m_cursorColumn};
    if (m_codeIntelBusy) {
        updateStatusMessage(QStringLiteral("Code intel request already running."));
        return;
    }

    const QString path = currentPath();
    const QString text = editorText();
    const int generation = ++m_codeIntelGeneration;
    m_codeIntelBusy = true;
    emit codeIntelBusyChanged();
    refreshActivityStatus();
    updateStatusMessage(QString("Definition requested at %1:%2...").arg(QString::number(m_cursorLine), QString::number(m_cursorColumn)));

    m_codeIntelService->definitionAsync(path, text, position, [this, generation](std::optional<ide::services::interfaces::DefinitionLocation> location) {
        if (generation != m_codeIntelGeneration) {
            return;
        }
        m_codeIntelBusy = false;
        emit codeIntelBusyChanged();
        refreshActivityStatus();
        if (!location) {
            m_definitionText = QStringLiteral("Definition not found at the current cursor position.");
            emit definitionTextChanged();
            updateStatusMessage(m_definitionText);
            return;
        }
        m_definitionText = QString("%1:%2:%3").arg(location->path, QString::number(location->line), QString::number(location->column));
        emit definitionTextChanged();
        goToDocumentLocation(location->path, location->line, location->column);
    });
    QTimer::singleShot(2000, this, [this, generation]() {
        if (generation == m_codeIntelGeneration && m_codeIntelBusy) {
            m_codeIntelBusy = false;
            emit codeIntelBusyChanged();
            refreshActivityStatus();
            updateStatusMessage(QStringLiteral("Definition request timed out."));
        }
    });
}

void MainViewModel::runTerminalCommand() {
    if (m_terminalBusy) {
        updateStatusMessage(QStringLiteral("Terminal command already running."));
        return;
    }

    const QString workspaceRoot = m_workspaceRootPath;
    const QString command = m_terminalCommand;
    const int generation = ++m_terminalGeneration;
    m_terminalBusy = true;
    emit terminalBusyChanged();
    refreshActivityStatus();
    updateStatusMessage(QString("Running command via %1...").arg(terminalBackendName()));

    auto* watcher = new QFutureWatcher<TerminalExecutionResult>(this);
    connect(watcher, &QFutureWatcher<TerminalExecutionResult>::finished, this, [this, watcher, generation]() {
        const TerminalExecutionResult result = watcher->result();
        watcher->deleteLater();

        if (generation != m_terminalGeneration) {
            return;
        }

        m_terminalOutput = QString("$ %1\n(exit=%2)\n%3")
                               .arg(result.terminalResult.command,
                                    QString::number(result.terminalResult.exitCode),
                                    result.terminalResult.output);
        emit terminalOutputChanged();

        m_terminalBusy = false;
        emit terminalBusyChanged();
        refreshActivityStatus();
        updateStatusMessage(QString("Command executed via %1.").arg(terminalBackendName()));
    });

    watcher->setFuture(QtConcurrent::run([this, workspaceRoot, command]() {
        TerminalExecutionResult result;
        result.terminalResult = m_terminalService->run(workspaceRoot, command);
        return result;
    }));
}

void MainViewModel::dismissToast() {
    showToast(QString());
}

void MainViewModel::applyCompletionInsert(const QString& insertText) {
    if (insertText.trimmed().isEmpty()) return;
    QString newText = editorText();
    newText.append(insertText);
    setEditorText(newText);
    analyzeNow();
    updateStatusMessage(QString("Completion aplicada: %1").arg(insertText));
}

void MainViewModel::refreshRelevantContext() {
    const ide::services::WorkspaceContextService::Options options{
        m_aiService->workspaceContextMaxFiles(),
        m_aiService->workspaceContextChars(),
        qMax(512, m_aiService->workspaceContextChars() / qMax(1, m_aiService->workspaceContextMaxFiles()))
    };
    const QString workspaceRoot = m_workspaceRootPath;
    const QString currentDocumentPath = currentPath();
    const QString prompt = m_chatInput;
    const auto workspaceFilesSnapshot = m_workspaceFiles;
    const int maxContextChars = m_aiService->workspaceContextChars();
    const int generation = ++m_relevantContextRefreshGeneration;

    auto* watcher = new QFutureWatcher<RelevantContextComputationResult>(this);
    connect(watcher, &QFutureWatcher<RelevantContextComputationResult>::finished, this,
            [this, watcher, generation, maxContextChars]() {
        const RelevantContextComputationResult result = watcher->result();
        watcher->deleteLater();

        if (generation != m_relevantContextRefreshGeneration) {
            return;
        }

        m_renderedWorkspaceContext = result.rendered;
        m_relevantContextModel.setFiles(result.files);
        m_relevantContextSummary = QString("%1 file(s) selected · %2 chars budget")
            .arg(QString::number(static_cast<int>(result.files.size())), QString::number(maxContextChars));
        emit relevantContextChanged();
        emit aiSettingsChanged();
    });

    watcher->setFuture(QtConcurrent::run([this, workspaceRoot, currentDocumentPath, prompt, workspaceFilesSnapshot, options, maxContextChars]() {
        RelevantContextComputationResult result;
        result.files = m_workspaceContextService.collect(workspaceRoot, currentDocumentPath, prompt, workspaceFilesSnapshot, options);
        result.rendered = m_workspaceContextService.render(result.files, maxContextChars);
        return result;
    }));
}

void MainViewModel::extractPatchPreview() {
    m_patchPreview = m_diffApplyService.preview(m_chatResponse, currentPath(), editorText());
    emit patchPreviewChanged();
}

void MainViewModel::applyAssistantPatch() {
    if (!m_patchPreview.canApply) {
        updateStatusMessage(m_patchPreview.summary.isEmpty() ? QStringLiteral("Nenhum patch aplicável.") : m_patchPreview.summary);
        return;
    }
    setEditorText(m_patchPreview.patchedText);
    analyzeNow();
    emit patchPreviewChanged();
    updateStatusMessage(QString("Patch aplicado no editor · +%1/-%2").arg(QString::number(m_patchPreview.additions), QString::number(m_patchPreview.deletions)));
}

void MainViewModel::clearPatchPreview() {
    m_patchPreview = {};
    emit patchPreviewChanged();
}

void MainViewModel::openPatchPreviewDiff() {
    if (!m_patchPreview.hasPatch) {
        extractPatchPreview();
    }
    if (!m_patchPreview.hasPatch) {
        updateStatusMessage(QStringLiteral("Nenhum patch disponível para diff."));
        return;
    }
    m_editorManager.setupDiffView(
        m_patchPreview.targetPath.isEmpty() ? editorTabTitle() : displayTitleForPath(m_patchPreview.targetPath),
        editorText(),
        m_patchPreview.patchedText.isEmpty() ? editorText() : m_patchPreview.patchedText
    );
    updateStatusMessage(m_patchPreview.summary.isEmpty() ? QStringLiteral("Diff aberto.") : m_patchPreview.summary);
}

void MainViewModel::closeSplitEditor() {
    m_editorManager.closeSplitEditor();
}

void MainViewModel::saveSecondaryEditor() {
    m_editorManager.saveSecondaryEditor();
    refreshWorkspace();
    refreshGitState();
    updateStatusMessage(QString("Arquivo salvo no split: %1").arg(m_editorManager.secondaryEditorPath()));
}

void MainViewModel::approvePendingTool(const QString& approvalId) {
    const auto result = m_aiService->approvePendingTool(approvalId, m_workspaceRootPath, currentPath());
    refreshPendingApprovals();
    syncAfterToolRun(result.touchedPaths);
    emit aiSettingsChanged();
    updateStatusMessage(result.humanSummary.isEmpty() ? QStringLiteral("Pending tool aprovada.") : result.humanSummary);
}

void MainViewModel::rejectPendingTool(const QString& approvalId) {
    const bool ok = m_aiService->rejectPendingTool(approvalId);
    refreshPendingApprovals();
    emit aiSettingsChanged();
    updateStatusMessage(ok ? QStringLiteral("Pending tool rejeitada.") : QStringLiteral("Approval id não encontrada."));
}

void MainViewModel::approveAllPendingTools() {
    const auto approvals = m_aiService->pendingApprovals();
    QStringList touched;
    int approved = 0;
    for (const auto& item : approvals) {
        const auto result = m_aiService->approvePendingTool(item.approvalId, m_workspaceRootPath, currentPath());
        touched.append(result.touchedPaths);
        if (result.success) ++approved;
    }
    refreshPendingApprovals();
    syncAfterToolRun(touched);
    emit aiSettingsChanged();
    updateStatusMessage(QString("%1 pending tool(s) aprovadas.").arg(QString::number(approved)));
}

void MainViewModel::clearPendingTools() {
    m_aiService->clearPendingApprovals();
    refreshPendingApprovals();
    emit aiSettingsChanged();
    updateStatusMessage(QStringLiteral("Fila de approvals limpa."));
}

void MainViewModel::refreshCommandPalette(const QString& query) { rebuildCommandPalette(query); }

void MainViewModel::runCommandPaletteCommand(const QString& commandId) {
    if (commandId == "workbench.view.explorer") {
        setPrimaryViewIndex(0);
    } else if (commandId == "workbench.view.search") {
        setPrimaryViewIndex(1);
    } else if (commandId == "workbench.view.scm") {
        setPrimaryViewIndex(2);
    } else if (commandId == "workbench.aiSidebar.toggle") {
        toggleSecondaryAiSidebar();
    } else if (commandId == "workbench.aiSidebar.showAssistant") {
        showAssistantSidebar();
    } else if (commandId == "workbench.aiSidebar.showModels") {
        showModelsSidebar();
    } else if (commandId == "workbench.action.quickOpen") {
        triggerQuickOpen();
    } else if (commandId == "file.openFile") {
        triggerOpenFileDialog();
    } else if (commandId == "file.openFolder") {
        triggerOpenFolderDialog();
    } else if (commandId == "file.save") {
        saveCurrent();
    } else if (commandId == "file.openSampleCpp") {
        loadSampleCpp();
    } else if (commandId == "file.openSampleRust") {
        loadSampleRust();
    } else if (commandId == "code.analyze") {
        analyzeNow();
    } else if (commandId == "code.complete") {
        requestCompletionsAtCursor();
    } else if (commandId == "code.hover") {
        requestHoverAtCursor();
    } else if (commandId == "code.definition") {
        if (definitionAvailable()) {
            requestDefinitionAtCursor();
        } else {
            updateStatusMessage(QStringLiteral("Go to Definition is unavailable in basic mode for this language."));
        }
    } else if (commandId == "search.focus") {
        setPrimaryViewIndex(1);
    } else if (commandId == "assistant.ask") {
        askAssistant();
    } else if (commandId == "assistant.extractPatch") {
        extractPatchPreview();
    } else if (commandId == "assistant.openPatchDiff") {
        openPatchPreviewDiff();
    } else if (commandId == "assistant.applyPatch") {
        applyAssistantPatch();
    } else if (commandId == "workbench.action.splitEditorRight") {
        openWorkspaceFileInSplit(currentPath());
    } else if (commandId == "workbench.action.closeSplitEditor") {
        closeSplitEditor();
    } else if (commandId == "git.refresh") {
        refreshGitState();
    } else if (commandId == "git.commit") {
        commitGitChanges();
    } else if (commandId == "terminal.run") {
        runTerminalCommand();
    }
}

void MainViewModel::openFirstMatchingWorkspaceFile(const QString& query) {
    const QString needle = normalized(query);
    if (needle.isEmpty()) return;
    auto it = std::find_if(m_workspaceFiles.begin(), m_workspaceFiles.end(), [&](const auto& file) {
        return normalized(file.relativePath).contains(needle) || normalized(file.path).contains(needle);
    });
    if (it == m_workspaceFiles.end()) {
        updateStatusMessage(QString("Nenhum arquivo encontrado para '%1'.").arg(query));
        return;
    }
    openWorkspaceFile(it->path);
}

void MainViewModel::switchOpenEditor(const QString& path) {
    m_editorManager.switchOpenEditor(path);
}

void MainViewModel::closeOpenEditor(const QString& path) {
    m_editorManager.closeOpenEditor(path);
}

void MainViewModel::stageGitPath(const QString& path) {
    if (m_gitViewModel.busy()) {
        updateStatusMessage(QStringLiteral("Git operation already running."));
        return;
    }
    m_gitViewModel.stage(m_workspaceRootPath, path);
    updateStatusMessage(QString("Staging %1...").arg(path));
}

void MainViewModel::unstageGitPath(const QString& path) {
    if (m_gitViewModel.busy()) {
        updateStatusMessage(QStringLiteral("Git operation already running."));
        return;
    }
    m_gitViewModel.unstage(m_workspaceRootPath, path);
    updateStatusMessage(QString("Unstaging %1...").arg(path));
}

void MainViewModel::discardGitPath(const QString& path) {
    if (m_gitViewModel.busy()) {
        updateStatusMessage(QStringLiteral("Git operation already running."));
        return;
    }
    m_gitViewModel.discard(m_workspaceRootPath, path);
    updateStatusMessage(QString("Discarding changes in %1...").arg(path));
}

void MainViewModel::openGitDiff(const QString& path) {
    if (m_gitDiffBusy) {
        updateStatusMessage(QStringLiteral("Diff already loading."));
        return;
    }

    const QString relativePath = path;
    const QString workspaceRoot = m_workspaceRootPath;
    const QString absolutePath = QDir(workspaceRoot).filePath(relativePath);
    const QString activePath = QFileInfo(currentPath()).absoluteFilePath();
    const QString activeText = editorText();
    const int generation = ++m_gitDiffGeneration;

    m_gitDiffBusy = true;
    emit gitOperationStateChanged();
    refreshActivityStatus();
    updateStatusMessage(QString("Loading diff for %1...").arg(relativePath));

    auto* watcher = new QFutureWatcher<GitDiffExecutionResult>(this);
    connect(watcher, &QFutureWatcher<GitDiffExecutionResult>::finished, this, [this, watcher, generation, relativePath]() {
        const GitDiffExecutionResult result = watcher->result();
        watcher->deleteLater();

        if (generation != m_gitDiffGeneration) {
            return;
        }

        m_gitDiffBusy = false;
        emit gitOperationStateChanged();
        refreshActivityStatus();

        if (result.empty) {
            updateStatusMessage(QString("No diff available for %1").arg(relativePath));
            return;
        }

        m_editorManager.setupDiffView(
            result.title,
            result.original.isEmpty() ? QStringLiteral("<new file or unavailable in HEAD>\n") : result.original,
            result.modified
        );
        updateStatusMessage(QString("Opened diff for %1").arg(relativePath));
    });

    watcher->setFuture(QtConcurrent::run([this, relativePath, workspaceRoot, absolutePath, activePath, activeText]() {
        GitDiffExecutionResult result;
        result.path = relativePath;
        result.title = displayTitleForPath(relativePath);
        result.original = m_gitViewModel.headFileContent(workspaceRoot, relativePath);
        result.modified = QFileInfo(absolutePath).absoluteFilePath() == activePath ? activeText : loadTextFile(absolutePath);
        result.empty = result.original.isEmpty() && result.modified.isEmpty();
        return result;
    }));
}

void MainViewModel::commitGitChanges() {
    if (m_gitViewModel.busy()) {
        updateStatusMessage(QStringLiteral("Git operation already running."));
        return;
    }
    m_gitViewModel.commit(m_workspaceRootPath);
    updateStatusMessage(QStringLiteral("Creating commit..."));
}

void MainViewModel::refreshGitChanges() {
    if (m_gitViewModel.busy()) {
        updateStatusMessage(QStringLiteral("Git operation running. Refresh will happen automatically."));
        return;
    }
    refreshGitState();
    updateStatusMessage(QStringLiteral("Refreshing source control..."));
}

void MainViewModel::toggleSecondaryAiSidebar() {
    setSecondaryAiVisible(!m_uiStateManager.secondaryAiVisible());
}

void MainViewModel::showAssistantSidebar() {
    if (!m_uiStateManager.secondaryAiVisible()) {
        setSecondaryAiVisible(true);
    }
    m_uiStateManager.setSecondaryAiTab(0);
}

void MainViewModel::showModelsSidebar() {
    if (!m_uiStateManager.secondaryAiVisible()) {
        setSecondaryAiVisible(true);
    }
    m_uiStateManager.setSecondaryAiTab(1);
}

QString MainViewModel::diagnosticsSummary() const { return m_diagnosticsModel.summaryText(); }

void MainViewModel::updateStatusMessage(const QString& message) {
    if (message.trimmed().isEmpty()) {
        return;
    }
    m_lastExplicitStatusMessage = message;
    if (m_activityStatusActive) {
        return;
    }
    if (m_statusMessage == message) {
        return;
    }
    m_statusMessage = message;
    emit statusMessageChanged();
}

QString MainViewModel::currentActivityStatusMessage() const {
    if (m_workspaceLoading) {
        const QString detail = m_workspaceLoadingText.trimmed().isEmpty()
            ? QStringLiteral("Indexing workspace files...")
            : m_workspaceLoadingText.trimmed();
        return QStringLiteral("Workspace: %1").arg(detail);
    }
    if (m_gitViewModel.busy()) {
        return QStringLiteral("Source Control: running Git operation...");
    }
    if (m_gitDiffBusy) {
        return QStringLiteral("Source Control: loading diff...");
    }
    if (m_searchBusy) {
        return QStringLiteral("Search: scanning workspace...");
    }
    if (m_terminalBusy) {
        return QStringLiteral("Terminal: running command...");
    }
    if (m_codeIntelBusy) {
        return QStringLiteral("Code Intelligence: querying language server...");
    }
    if (m_aiBusy) {
        return QStringLiteral("Assistant: generating response...");
    }
    if (isTransientDiagnosticsStatus(m_diagnosticsStatusLine)) {
        return QStringLiteral("Diagnostics: %1").arg(m_diagnosticsStatusLine.trimmed());
    }
    return {};
}

void MainViewModel::applyActivityStatusMessage(const QString& activityMessage) {
    if (activityMessage.trimmed().isEmpty()) {
        if (!m_activityStatusActive) {
            return;
        }
        m_activityStatusActive = false;
        const QString fallback = m_lastExplicitStatusMessage.trimmed().isEmpty()
            ? QStringLiteral("Ready.")
            : m_lastExplicitStatusMessage;
        if (m_statusMessage == fallback) {
            return;
        }
        m_statusMessage = fallback;
        emit statusMessageChanged();
        return;
    }

    if (m_activityStatusActive && m_statusMessage == activityMessage) {
        return;
    }
    m_activityStatusActive = true;
    m_statusMessage = activityMessage;
    emit statusMessageChanged();
}

void MainViewModel::refreshActivityStatus() {
    applyActivityStatusMessage(currentActivityStatusMessage());
}

void MainViewModel::applyDiagnostics(const std::vector<ide::services::interfaces::Diagnostic>& diagnostics) {
    m_diagnosticsModel.setDiagnostics(diagnostics);
    m_currentFileDiagnostics = m_diagnosticsModel.asVariantList();
    emit diagnosticsChanged();
    emit diagnosticsCountChanged();
    emit aiSettingsChanged();
}

void MainViewModel::showToast(const QString& message) {
    if (message.trimmed().isEmpty()) {
        if (!m_toastVisible && m_toastMessage.isEmpty()) {
            return;
        }
        m_toastVisible = false;
        m_toastMessage.clear();
        emit toastChanged();
        return;
    }
    m_toastMessage = message;
    m_toastVisible = true;
    emit toastChanged();
}

void MainViewModel::setDocumentSample(const QString& path, const QString& text) {
    m_editorManager.setInMemoryDocument(path, text);
    analyzeNow();
    refreshRelevantContext();
    clearPatchPreview();
    emit aiSettingsChanged();
}

void MainViewModel::goToDocumentLocation(const QString& path, int line, int column) {
    const QString resolvedPath = resolveExistingFilePath(path, m_workspaceRootPath);
    const QFileInfo info(resolvedPath);
    if (!info.exists() || !info.isFile()) {
        updateStatusMessage(QString("Failed to open target file: %1").arg(path));
        return;
    }
    const QString targetPath = info.absoluteFilePath();
    m_editorManager.openFile(targetPath);
    if (QFileInfo(currentPath()).absoluteFilePath() != targetPath) {
        updateStatusMessage(QString("Failed to open file: %1").arg(targetPath));
        return;
    }
    analyzeNow();
    refreshRelevantContext();
    clearPatchPreview();
    refreshGitState();
    updateStatusMessage(QString("Navegado para %1:%2:%3").arg(targetPath, QString::number(line), QString::number(column)));
}

void MainViewModel::refreshPendingApprovals() { m_pendingApprovalModel.setApprovals(m_aiService->pendingApprovals()); }

void MainViewModel::syncAfterToolRun(const QStringList& touchedPaths) {
    if (touchedPaths.isEmpty()) return;
    refreshWorkspace();

    const QString currentAbsolute = QFileInfo(currentPath()).absoluteFilePath();
    const QString secondaryAbsolute = QFileInfo(m_editorManager.secondaryEditorPath()).absoluteFilePath();
    for (const auto& touched : touchedPaths) {
        const QString touchedAbsolute = QFileInfo(touched).absoluteFilePath();
        if (touchedAbsolute == secondaryAbsolute && !m_editorManager.secondaryEditorPath().isEmpty()) {
            m_editorManager.openFileInSplit(touched);
            emit splitEditorChanged();
        }
        if (touchedAbsolute == currentAbsolute) {
            m_documentService->openFile(touched);
            emit editorTextChanged();
            emit currentPathChanged();
            analyzeNow();
            refreshRelevantContext();
            clearPatchPreview();
            break;
        }
    }
    refreshGitState();
}

void MainViewModel::rebuildCommandPalette(const QString& query) {
    if (m_commandCatalog.empty()) {
        m_commandCatalog = {
            {"workbench.view.explorer", "View: Focus Explorer", "Workbench", "Activity Bar"},
            {"workbench.view.search", "View: Focus Search", "Workbench", "Activity Bar"},
            {"workbench.view.scm", "View: Focus Source Control", "Workbench", "Activity Bar"},
            {"workbench.aiSidebar.toggle", "View: Toggle AI Sidebar", "Workbench", "Secondary Side Bar"},
            {"workbench.aiSidebar.showAssistant", "View: Show Assistant", "Workbench", "Secondary Side Bar"},
            {"workbench.aiSidebar.showModels", "View: Show Models", "Workbench", "Secondary Side Bar"},
            {"workbench.action.quickOpen", "Go to File", "Workbench", "Quick Open"},
            {"workbench.action.splitEditorRight", "View: Split Editor Right", "Workbench", "Ctrl+\\"},
            {"workbench.action.closeSplitEditor", "View: Close Secondary Editor", "Workbench", "Split"},
            {"file.openFile", "File: Open File...", "File", "Ctrl+O"},
            {"file.openFolder", "File: Open Folder...", "File", "Workspace"},
            {"file.save", "File: Save", "File", "Ctrl+S"},
            {"file.openSampleCpp", "File: Open Sample C++", "File", "Demo"},
            {"file.openSampleRust", "File: Open Sample Rust", "File", "Demo"},
            {"code.analyze", "Code: Analyze Diagnostics", "Code", "Problems"},
            {"code.complete", "Code: Trigger Completion", "Code", "LSP"},
            {"code.hover", "Code: Show Hover", "Code", "LSP"},
            {"code.definition", "Code: Go to Definition", "Code", "LSP"},
            {"search.focus", "Search: Focus Search Panel", "Search", "Workspace"},
            {"assistant.ask", "Assistant: Ask", "AI", "Chat"},
            {"assistant.extractPatch", "Assistant: Extract Patch Preview", "AI", "Diff"},
            {"assistant.openPatchDiff", "Assistant: Open Patch Diff", "AI", "Diff Editor"},
            {"assistant.applyPatch", "Assistant: Apply Patch Preview", "AI", "Diff"},
            {"git.refresh", "Git: Refresh Changes", "SCM", "Source Control"},
            {"git.commit", "Git: Commit Staged Changes", "SCM", "Source Control"},
            {"terminal.run", "Terminal: Run Current Command", "Terminal", "Panel"}
        };
    }

    const QString needle = normalized(query);
    std::vector<ide::ui::models::CommandPaletteItem> items;
    for (const auto& spec : m_commandCatalog) {
        const QString haystack = normalized(spec.title + " " + spec.category + " " + spec.hint + " " + spec.id);
        if (!needle.isEmpty() && !haystack.contains(needle)) {
            continue;
        }
        items.push_back({spec.id, spec.title, spec.category, spec.hint});
    }
    m_commandPaletteModel.setItems(std::move(items));
    emit commandPaletteChanged();
}

void MainViewModel::refreshGitState() {
    m_gitViewModel.refresh(m_workspaceRootPath);
}

void MainViewModel::startWorkspaceRefresh(const QString& statusHint) {
    const QString rootPath = m_workspaceRootPath;
    if (rootPath.trimmed().isEmpty()) {
        m_workspaceFiles.clear();
        rebuildWorkspaceModels();
        emit workspaceChanged();
        m_workspaceLoading = false;
        m_workspaceLoadingText.clear();
        emit workspaceLoadingChanged();
        refreshActivityStatus();
        return;
    }

    const int generation = ++m_workspaceRefreshGeneration;
    m_workspaceLoading = true;
    m_workspaceLoadingText = statusHint.trimmed().isEmpty()
        ? QStringLiteral("Indexing workspace files...")
        : statusHint.trimmed();
    emit workspaceLoadingChanged();
    refreshActivityStatus();

    auto* watcher = new QFutureWatcher<std::vector<ide::services::interfaces::WorkspaceFile>>(this);
    connect(watcher, &QFutureWatcher<std::vector<ide::services::interfaces::WorkspaceFile>>::finished, this,
            [this, watcher, generation]() {
        const std::vector<ide::services::interfaces::WorkspaceFile> files = watcher->result();
        watcher->deleteLater();

        if (generation != m_workspaceRefreshGeneration) {
            return;
        }

        m_workspaceLoadingText = QStringLiteral("Finalizing workspace view... 99%");
        emit workspaceLoadingChanged();
        refreshActivityStatus();

        QTimer::singleShot(0, this, [this, generation, files]() {
            if (generation != m_workspaceRefreshGeneration || !m_workspaceLoading) {
                return;
            }
            m_workspaceFiles = files;
            rebuildWorkspaceModels();
            emit workspaceChanged();

            m_workspaceLoadingText = QStringLiteral("Finalizing workspace view... 100%");
            emit workspaceLoadingChanged();
            refreshActivityStatus();

            QTimer::singleShot(0, this, [this, generation]() {
                if (generation != m_workspaceRefreshGeneration || !m_workspaceLoading) {
                    return;
                }
                refreshPendingApprovals();
                rebuildCommandPalette(QString());

                m_workspaceLoading = false;
                m_workspaceLoadingText.clear();
                emit workspaceLoadingChanged();
                refreshActivityStatus();
                updateStatusMessage(QString("Workspace indexed: %1 files.").arg(QString::number(workspaceFileCount())));

                // Heavier tasks after overlay closes, to avoid "Not Responding" perception.
                QTimer::singleShot(0, this, [this, generation]() {
                    if (generation != m_workspaceRefreshGeneration) {
                        return;
                    }
                    refreshRelevantContext();
                    refreshGitState();
                });
            });
        });
    });

    watcher->setFuture(QtConcurrent::run([this, rootPath, generation]() {
        return m_workspaceService->files(rootPath, [this, generation](int indexedCount, int totalCount) {
            QMetaObject::invokeMethod(this, [this, generation, indexedCount, totalCount]() {
                if (generation != m_workspaceRefreshGeneration || !m_workspaceLoading) {
                    return;
                }
                int percent = 0;
                if (totalCount > 0) {
                    percent = qBound(0, (indexedCount * 100) / totalCount, 100);
                }
                if (indexedCount < totalCount) {
                    percent = qMin(percent, 99);
                }
                m_workspaceLoadingText = QString("Indexing workspace files... %1% (%2 / %3 files)")
                    .arg(QString::number(percent),
                         QString::number(indexedCount),
                         QString::number(totalCount));
                emit workspaceLoadingChanged();
                refreshActivityStatus();
            }, Qt::QueuedConnection);
        });
    }));
}

QString MainViewModel::localPathFromUrlOrPath(const QString& rawPath) const {
    const QString trimmed = rawPath.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }

    const QUrl url(trimmed);
    if (url.isValid() && url.isLocalFile()) {
        return QDir::cleanPath(url.toLocalFile());
    }
    return QDir::cleanPath(trimmed);
}

void MainViewModel::addRecentFolder(const QString& folderPath) {
    const QString localPath = localPathFromUrlOrPath(folderPath);
    if (localPath.isEmpty()) {
        return;
    }
    m_uiStateManager.addRecentFolder(localPath);
    m_uiStateManager.save();
}

void MainViewModel::rebuildWorkspaceModels() {
    m_workspaceFilesModel.setFiles(m_workspaceFiles);
    m_workspaceTreeModel.setWorkspaceRoot(m_workspaceRootPath);
    m_workspaceTreeModel.setFiles(m_workspaceFiles);
    m_workspaceTreeModel.setCurrentFilePath(currentPath());
}

QString MainViewModel::uniqueWorkspaceChildPath(const QString& preferredName) const {
    if (m_workspaceRootPath.trimmed().isEmpty()) {
        return {};
    }

    const QFileInfo preferredInfo(preferredName);
    const QString baseName = preferredInfo.completeBaseName().isEmpty()
        ? preferredInfo.fileName()
        : preferredInfo.completeBaseName();
    const QString suffix = preferredInfo.completeSuffix();

    QDir root(m_workspaceRootPath);
    QString candidate = root.filePath(preferredName);
    if (!QFileInfo::exists(candidate)) {
        return candidate;
    }

    for (int index = 1; index < 1000; ++index) {
        const QString numbered = suffix.isEmpty()
            ? QStringLiteral("%1%2").arg(baseName, QString::number(index))
            : QStringLiteral("%1%2.%3").arg(baseName, QString::number(index), suffix);
        candidate = root.filePath(numbered);
        if (!QFileInfo::exists(candidate)) {
            return candidate;
        }
    }

    return {};
}

} // namespace ide::ui::viewmodels
