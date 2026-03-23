#include "ui/viewmodels/MainViewModel.hpp"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QFileInfo>
#include <QStandardPaths>
#include <QtGlobal>

#include <algorithm>

namespace ide::ui::viewmodels {

namespace {
QString inferLanguageId(const QString& path) {
    if (path.endsWith(".rs")) return "rust";
    if (path.endsWith(".py")) return "python";
    if (path.endsWith(".js") || path.endsWith(".ts") || path.endsWith(".tsx") || path.endsWith(".jsx")) return "typescript";
    if (path.endsWith(".qml")) return "qml";
    if (path.endsWith(".md")) return "markdown";
    return "cpp";
}

QString displayTitleForPath(const QString& path) {
    const QFileInfo info(path);
    return info.fileName().isEmpty() ? QStringLiteral("untitled") : info.fileName();
}

QString normalized(const QString& text) {
    return text.trimmed().toLower();
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
                             std::unique_ptr<ide::services::DiagnosticService> diagnosticService,
                             std::unique_ptr<ide::services::CodeIntelService> codeIntelService,
                             std::unique_ptr<ide::services::AiService> aiService,
                             std::unique_ptr<ide::services::GitService> gitService,
                             std::unique_ptr<ide::services::WorkspaceService> workspaceService,
                             std::unique_ptr<ide::services::SearchService> searchService,
                             std::unique_ptr<ide::services::TerminalService> terminalService,
                             QObject* parent)
    : QObject(parent)
    , m_documentService(std::move(documentService))
    , m_diagnosticService(std::move(diagnosticService))
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
    connect(&m_gitViewModel, &GitViewModel::gitChanged, this, &MainViewModel::gitChanged);
    connect(&m_gitViewModel, &GitViewModel::scmCommitMessageChanged, this, &MainViewModel::scmCommitMessageChanged);
    
    connect(&m_uiStateManager, &UiStateManager::primaryViewIndexChanged, this, &MainViewModel::primaryViewIndexChanged);
    connect(&m_uiStateManager, &UiStateManager::secondaryAiChanged, this, &MainViewModel::secondaryAiChanged);

    refreshWorkspace();
    refreshGitState();
    refreshRelevantContext();
    refreshPendingApprovals();
    updateStatusMessage("LocalCodeIDE v3.3 workbench carregado.");
}

QString MainViewModel::editorText() const { return m_documentService->currentDocument().text(); }

void MainViewModel::setEditorText(const QString& text) {
    if (text == m_documentService->currentDocument().text()) {
        return;
    }
    m_documentService->setText(text);
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
QString MainViewModel::diagnosticsProviderName() const { return m_diagnosticService->providerName(); }
QString MainViewModel::codeIntelProviderName() const { return m_codeIntelService->providerName(); }
QString MainViewModel::aiBackendName() const { return m_aiService->backendName(); }
QString MainViewModel::aiBackendStatusLine() const { return m_aiService->backendStatusLine(); }
int MainViewModel::diagnosticsCount() const { return m_diagnosticsModel.diagnosticCount(); }
int MainViewModel::workspaceFileCount() const { return m_workspaceFilesModel.fileCount(); }
int MainViewModel::searchResultCount() const { return m_searchResultsModel.resultCount(); }
int MainViewModel::completionCount() const { return m_completionModel.itemCount(); }
int MainViewModel::relevantContextCount() const { return m_relevantContextModel.fileCount(); }

QString MainViewModel::workspaceRootPath() const { return m_workspaceRootPath; }
void MainViewModel::setWorkspaceRootPath(const QString& path) {
    if (path == m_workspaceRootPath || path.trimmed().isEmpty()) return;
    m_workspaceRootPath = path;
    emit workspaceChanged();
    refreshWorkspace();
    refreshGitState();
    refreshRelevantContext();
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

void MainViewModel::analyzeNow() {
    const auto diagnostics = m_diagnosticService->refresh(currentPath(), editorText());
    m_diagnosticsModel.setDiagnostics(diagnostics);
    emit diagnosticsCountChanged();
    emit aiSettingsChanged();
    updateStatusMessage(QString("%1 diagnósticos via %2.").arg(QString::number(diagnosticsCount()), diagnosticsProviderName()));
}

void MainViewModel::askAssistant() {
    refreshRelevantContext();
    m_chatResponse = m_aiService->ask(m_chatInput, m_workspaceRootPath, currentPath(), editorText(), diagnosticsSummary(), gitSummary(), m_renderedWorkspaceContext);
    emit chatResponseChanged();
    syncAfterToolRun(m_aiService->lastToolTouchedPaths());
    extractPatchPreview();
    refreshPendingApprovals();
    emit aiSettingsChanged();

    if (pendingApprovalCount() > 0) {
        updateStatusMessage(QString("%1 tool(s) aguardando aprovação manual.").arg(QString::number(pendingApprovalCount())));
    } else if (toolCallCount() > 0) {
        updateStatusMessage(QString("Resposta gerada via %1 com %2 tool call(s).").arg(aiBackendName(), QString::number(toolCallCount())));
    } else {
        updateStatusMessage(QString("Resposta gerada via %1.").arg(aiBackendName()));
    }
}

void MainViewModel::saveCurrent() {
    if (m_documentService->saveFile()) {
        m_editorManager.saveCurrent();
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
    m_workspaceFiles = m_workspaceService->files(m_workspaceRootPath);
    rebuildWorkspaceModels();
    emit workspaceChanged();
    refreshRelevantContext();
    refreshPendingApprovals();
    rebuildCommandPalette(QString());
    updateStatusMessage(QString("Workspace indexado: %1 arquivos.").arg(QString::number(workspaceFileCount())));
}

void MainViewModel::openWorkspaceFile(const QString& path) {
    m_workspaceTreeModel.expandToPath(path);
    goToDocumentLocation(path, 1, 1);
}

void MainViewModel::openWorkspaceFileInSplit(const QString& path) {
    m_editorManager.openFileInSplit(path);
    m_workspaceTreeModel.expandToPath(path);
    updateStatusMessage(QString("Split editor aberto para %1").arg(path));
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
    const auto results = m_searchService->search(m_workspaceRootPath, m_searchPattern);
    m_searchResultsModel.setResults(results);
    emit searchResultsChanged();
    updateStatusMessage(QString("Busca retornou %1 resultado(s).").arg(QString::number(searchResultCount())));
}

void MainViewModel::openSearchResult(const QString& path, int line, int column) { goToDocumentLocation(path, line, column); }

void MainViewModel::setCursorPosition(int line, int column) {
    m_cursorLine = qMax(1, line);
    m_cursorColumn = qMax(1, column);
}

void MainViewModel::requestCompletionsAtCursor() {
    const ide::services::interfaces::EditorPosition position{m_cursorLine, m_cursorColumn};
    auto items = m_codeIntelService->completions(currentPath(), editorText(), position);
    m_completionModel.setItems(std::move(items));
    emit completionsChanged();
    updateStatusMessage(QString("Completion em %1:%2 via %3.").arg(QString::number(m_cursorLine), QString::number(m_cursorColumn), codeIntelProviderName()));
}

void MainViewModel::requestHoverAtCursor() {
    const ide::services::interfaces::EditorPosition position{m_cursorLine, m_cursorColumn};
    m_hoverText = m_codeIntelService->hover(currentPath(), editorText(), position).contents;
    emit hoverTextChanged();
    updateStatusMessage(QString("Hover em %1:%2.").arg(QString::number(m_cursorLine), QString::number(m_cursorColumn)));
}

void MainViewModel::requestDefinitionAtCursor() {
    const ide::services::interfaces::EditorPosition position{m_cursorLine, m_cursorColumn};
    const auto location = m_codeIntelService->definition(currentPath(), editorText(), position);
    if (!location) {
        m_definitionText = "Definition não encontrada.";
        emit definitionTextChanged();
        updateStatusMessage("Definition não encontrada.");
        return;
    }
    m_definitionText = QString("%1:%2:%3").arg(location->path, QString::number(location->line), QString::number(location->column));
    emit definitionTextChanged();
    goToDocumentLocation(location->path, location->line, location->column);
}

void MainViewModel::runTerminalCommand() {
    const auto result = m_terminalService->run(m_workspaceRootPath, m_terminalCommand);
    m_terminalOutput = QString("$ %1\n(exit=%2)\n%3").arg(result.command, QString::number(result.exitCode), result.output);
    emit terminalOutputChanged();
    updateStatusMessage(QString("Comando executado via %1.").arg(terminalBackendName()));
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
    const auto files = m_workspaceContextService.collect(m_workspaceRootPath, currentPath(), m_chatInput, m_workspaceFiles, options);
    m_renderedWorkspaceContext = m_workspaceContextService.render(files, m_aiService->workspaceContextChars());
    m_relevantContextModel.setFiles(files);
    m_relevantContextSummary = QString("%1 file(s) selected · %2 chars budget").arg(QString::number(relevantContextCount()), QString::number(m_aiService->workspaceContextChars()));
    emit relevantContextChanged();
    emit aiSettingsChanged();
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
        updateStatusMessage("Quick Open pronto para uso (Ctrl+P). Digite um arquivo.");
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
        requestDefinitionAtCursor();
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
    m_gitViewModel.stage(m_workspaceRootPath, path);
    updateStatusMessage(QString("Staged %1").arg(path));
}

void MainViewModel::unstageGitPath(const QString& path) {
    m_gitViewModel.unstage(m_workspaceRootPath, path);
    updateStatusMessage(QString("Unstaged %1").arg(path));
}

void MainViewModel::discardGitPath(const QString& path) {
    m_gitViewModel.discard(m_workspaceRootPath, path);
    updateStatusMessage(QString("Discarded changes in %1").arg(path));
}

void MainViewModel::openGitDiff(const QString& path) {
    const QString absolutePath = QDir(m_workspaceRootPath).filePath(path);
    const QString original = m_gitViewModel.headFileContent(m_workspaceRootPath, path);
    QString modified = QFileInfo(absolutePath).absoluteFilePath() == QFileInfo(currentPath()).absoluteFilePath() ? editorText() : loadTextFile(absolutePath);
    if (original.isEmpty() && modified.isEmpty()) {
        updateStatusMessage(QString("No diff available for %1").arg(path));
        return;
    }
    m_editorManager.setupDiffView(displayTitleForPath(path), 
        original.isEmpty() ? QStringLiteral("<new file or unavailable in HEAD>\n") : original, 
        modified);
    updateStatusMessage(QString("Opened diff for %1").arg(path));
}

void MainViewModel::commitGitChanges() {
    if (m_gitViewModel.commit(m_workspaceRootPath)) {
        updateStatusMessage(QStringLiteral("Commit created."));
    } else {
        updateStatusMessage(QStringLiteral("Failed to create commit."));
    }
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
    m_statusMessage = message;
    emit statusMessageChanged();
}

void MainViewModel::setDocumentSample(const QString& path, const QString& text) {
    m_documentService->setPath(path);
    m_documentService->setText(text);
    emit editorTextChanged();
    emit currentPathChanged();
    emit aiSettingsChanged();
    analyzeNow();
    refreshRelevantContext();
    clearPatchPreview();
}

void MainViewModel::goToDocumentLocation(const QString& path, int line, int column) {
    if (!m_documentService->openFile(path)) {
        updateStatusMessage(QString("Falha ao abrir definição em %1").arg(path));
        return;
    }
    emit editorTextChanged();
    emit currentPathChanged();
    emit aiSettingsChanged();
    analyzeNow();
    refreshRelevantContext();
    clearPatchPreview();
    refreshGitState();
    updateStatusMessage(QString("Navegado para %1:%2:%3").arg(path, QString::number(line), QString::number(column)));
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
    m_workspaceTreeModel.setGitChanges(m_gitService->listChanges(m_workspaceRootPath));
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
