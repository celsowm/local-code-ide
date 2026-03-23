#pragma once

#include "services/AiService.hpp"
#include "services/CodeIntelService.hpp"
#include "services/DiagnosticCoordinator.hpp"
#include "services/DocumentService.hpp"
#include "services/DiffApplyService.hpp"
#include "services/GitService.hpp"
#include "services/SearchService.hpp"
#include "services/TerminalService.hpp"
#include "services/WorkspaceContextService.hpp"
#include "services/WorkspaceService.hpp"
#include "ui/viewmodels/EditorManager.hpp"
#include "ui/viewmodels/GitViewModel.hpp"
#include "ui/viewmodels/UiStateManager.hpp"
#include "ui/models/CommandPaletteListModel.hpp"
#include "ui/models/CompletionListModel.hpp"
#include "ui/models/DiagnosticListModel.hpp"
#include "ui/models/PendingToolApprovalListModel.hpp"
#include "ui/models/RelevantContextListModel.hpp"
#include "ui/models/SearchResultListModel.hpp"
#include "ui/models/WorkspaceFileListModel.hpp"
#include "ui/models/WorkspaceTreeListModel.hpp"

#include <QObject>
#include <memory>
#include <QStringList>
#include <QVariantList>
#include <vector>

namespace ide::ui::viewmodels {

class MainViewModel final : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString editorText READ editorText WRITE setEditorText NOTIFY editorTextChanged)
    Q_PROPERTY(QString currentPath READ currentPath NOTIFY currentPathChanged)
    Q_PROPERTY(QString languageId READ languageId NOTIFY currentPathChanged)
    Q_PROPERTY(QString chatInput READ chatInput WRITE setChatInput NOTIFY chatInputChanged)
    Q_PROPERTY(QString chatResponse READ chatResponse NOTIFY chatResponseChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString diagnosticsProviderName READ diagnosticsProviderName NOTIFY diagnosticsStatusChanged)
    Q_PROPERTY(QString diagnosticsStatusLine READ diagnosticsStatusLine NOTIFY diagnosticsStatusChanged)
    Q_PROPERTY(bool definitionAvailable READ definitionAvailable NOTIFY diagnosticsStatusChanged)
    Q_PROPERTY(QString codeIntelProviderName READ codeIntelProviderName CONSTANT)
    Q_PROPERTY(QString aiBackendName READ aiBackendName CONSTANT)
    Q_PROPERTY(QString aiBackendStatusLine READ aiBackendStatusLine NOTIFY aiSettingsChanged)
    Q_PROPERTY(QString editorTabTitle READ editorTabTitle NOTIFY currentPathChanged)
    Q_PROPERTY(int diagnosticsCount READ diagnosticsCount NOTIFY diagnosticsCountChanged)
    Q_PROPERTY(int workspaceFileCount READ workspaceFileCount NOTIFY workspaceChanged)
    Q_PROPERTY(bool workspaceLoading READ workspaceLoading NOTIFY workspaceLoadingChanged)
    Q_PROPERTY(QString workspaceLoadingText READ workspaceLoadingText NOTIFY workspaceLoadingChanged)
    Q_PROPERTY(int searchResultCount READ searchResultCount NOTIFY searchResultsChanged)
    Q_PROPERTY(int completionCount READ completionCount NOTIFY completionsChanged)
    Q_PROPERTY(QString workspaceRootPath READ workspaceRootPath WRITE setWorkspaceRootPath NOTIFY workspaceChanged)
    Q_PROPERTY(QString searchPattern READ searchPattern WRITE setSearchPattern NOTIFY searchPatternChanged)
    Q_PROPERTY(QString hoverText READ hoverText NOTIFY hoverTextChanged)
    Q_PROPERTY(QString definitionText READ definitionText NOTIFY definitionTextChanged)
    Q_PROPERTY(QString terminalCommand READ terminalCommand WRITE setTerminalCommand NOTIFY terminalCommandChanged)
    Q_PROPERTY(QString terminalOutput READ terminalOutput NOTIFY terminalOutputChanged)
    Q_PROPERTY(QString terminalBackendName READ terminalBackendName CONSTANT)
    Q_PROPERTY(QString aiSystemPrompt READ aiSystemPrompt WRITE setAiSystemPrompt NOTIFY aiSettingsChanged)
    Q_PROPERTY(double aiTemperature READ aiTemperature WRITE setAiTemperature NOTIFY aiSettingsChanged)
    Q_PROPERTY(int aiMaxOutputTokens READ aiMaxOutputTokens WRITE setAiMaxOutputTokens NOTIFY aiSettingsChanged)
    Q_PROPERTY(int aiDocumentContextChars READ aiDocumentContextChars WRITE setAiDocumentContextChars NOTIFY aiSettingsChanged)
    Q_PROPERTY(int aiWorkspaceContextChars READ aiWorkspaceContextChars WRITE setAiWorkspaceContextChars NOTIFY aiSettingsChanged)
    Q_PROPERTY(int aiWorkspaceContextMaxFiles READ aiWorkspaceContextMaxFiles WRITE setAiWorkspaceContextMaxFiles NOTIFY aiSettingsChanged)
    Q_PROPERTY(bool aiIncludeDocument READ aiIncludeDocument WRITE setAiIncludeDocument NOTIFY aiSettingsChanged)
    Q_PROPERTY(bool aiIncludeDiagnostics READ aiIncludeDiagnostics WRITE setAiIncludeDiagnostics NOTIFY aiSettingsChanged)
    Q_PROPERTY(bool aiIncludeGitSummary READ aiIncludeGitSummary WRITE setAiIncludeGitSummary NOTIFY aiSettingsChanged)
    Q_PROPERTY(bool aiIncludeWorkspaceContext READ aiIncludeWorkspaceContext WRITE setAiIncludeWorkspaceContext NOTIFY aiSettingsChanged)
    Q_PROPERTY(bool aiEnableTools READ aiEnableTools WRITE setAiEnableTools NOTIFY aiSettingsChanged)
    Q_PROPERTY(int aiMaxToolRounds READ aiMaxToolRounds WRITE setAiMaxToolRounds NOTIFY aiSettingsChanged)
    Q_PROPERTY(bool aiRequireApprovalForDestructive READ aiRequireApprovalForDestructive WRITE setAiRequireApprovalForDestructive NOTIFY aiSettingsChanged)
    Q_PROPERTY(QString aiContextSummary READ aiContextSummary NOTIFY aiSettingsChanged)
    Q_PROPERTY(QString relevantContextSummary READ relevantContextSummary NOTIFY relevantContextChanged)
    Q_PROPERTY(int relevantContextCount READ relevantContextCount NOTIFY relevantContextChanged)
    Q_PROPERTY(QString toolCallLog READ toolCallLog NOTIFY aiSettingsChanged)
    Q_PROPERTY(int toolCallCount READ toolCallCount NOTIFY aiSettingsChanged)
    Q_PROPERTY(QString toolCatalogSummary READ toolCatalogSummary NOTIFY aiSettingsChanged)
    Q_PROPERTY(int pendingApprovalCount READ pendingApprovalCount NOTIFY aiSettingsChanged)
    Q_PROPERTY(QString pendingApprovalSummary READ pendingApprovalSummary NOTIFY aiSettingsChanged)
    Q_PROPERTY(QObject* diagnosticsModel READ diagnosticsModel CONSTANT)
    Q_PROPERTY(QVariantList currentFileDiagnostics READ currentFileDiagnostics NOTIFY diagnosticsChanged)
    Q_PROPERTY(QObject* workspaceFilesModel READ workspaceFilesModel CONSTANT)
    Q_PROPERTY(QObject* workspaceTreeModel READ workspaceTreeModel CONSTANT)
    Q_PROPERTY(QObject* searchResultsModel READ searchResultsModel CONSTANT)
    Q_PROPERTY(QObject* completionModel READ completionModel CONSTANT)
    Q_PROPERTY(QObject* relevantContextModel READ relevantContextModel CONSTANT)
    Q_PROPERTY(QObject* pendingApprovalModel READ pendingApprovalModel CONSTANT)
    Q_PROPERTY(bool hasPatchPreview READ hasPatchPreview NOTIFY patchPreviewChanged)
    Q_PROPERTY(bool canApplyPatchPreview READ canApplyPatchPreview NOTIFY patchPreviewChanged)
    Q_PROPERTY(QString patchSummary READ patchSummary NOTIFY patchPreviewChanged)
    Q_PROPERTY(QString patchPreviewText READ patchPreviewText NOTIFY patchPreviewChanged)

    Q_PROPERTY(QObject* openEditorsModel READ openEditorsModel CONSTANT)
    Q_PROPERTY(int openEditorCount READ openEditorCount NOTIFY openEditorsChanged)
    Q_PROPERTY(bool showWelcomeTab READ showWelcomeTab NOTIFY openEditorsChanged)
    Q_PROPERTY(QStringList recentFolders READ recentFolders NOTIFY recentFoldersChanged)
    Q_PROPERTY(QObject* commandPaletteModel READ commandPaletteModel CONSTANT)
    Q_PROPERTY(int commandPaletteCount READ commandPaletteCount NOTIFY commandPaletteChanged)

    Q_PROPERTY(int primaryViewIndex READ primaryViewIndex WRITE setPrimaryViewIndex NOTIFY primaryViewIndexChanged)
    Q_PROPERTY(bool secondaryAiVisible READ secondaryAiVisible WRITE setSecondaryAiVisible NOTIFY secondaryAiChanged)
    Q_PROPERTY(int secondaryAiTab READ secondaryAiTab WRITE setSecondaryAiTab NOTIFY secondaryAiChanged)
    Q_PROPERTY(int secondaryAiWidth READ secondaryAiWidth WRITE setSecondaryAiWidth NOTIFY secondaryAiChanged)
    Q_PROPERTY(int bottomPanelTab READ bottomPanelTab WRITE setBottomPanelTab NOTIFY bottomPanelChanged)
    Q_PROPERTY(int bottomPanelHeight READ bottomPanelHeight WRITE setBottomPanelHeight NOTIFY bottomPanelChanged)

    Q_PROPERTY(bool currentDocumentDirty READ currentDocumentDirty NOTIFY openEditorsChanged)

    Q_PROPERTY(QObject* gitChangesModel READ gitChangesModel CONSTANT)
    Q_PROPERTY(QObject* scmSectionsModel READ scmSectionsModel CONSTANT)
    Q_PROPERTY(QObject* gitRecentCommitsModel READ gitRecentCommitsModel CONSTANT)
    Q_PROPERTY(int gitChangeCount READ gitChangeCount NOTIFY gitChanged)
    Q_PROPERTY(int gitStagedCount READ gitStagedCount NOTIFY gitChanged)
    Q_PROPERTY(int gitUnstagedCount READ gitUnstagedCount NOTIFY gitChanged)
    Q_PROPERTY(int gitUntrackedCount READ gitUntrackedCount NOTIFY gitChanged)
    Q_PROPERTY(int gitRecentCommitCount READ gitRecentCommitCount NOTIFY gitChanged)
    Q_PROPERTY(QString scmCommitMessage READ scmCommitMessage WRITE setScmCommitMessage NOTIFY scmCommitMessageChanged)

    Q_PROPERTY(QString gitSummary READ gitSummary NOTIFY gitSummaryChanged)
    Q_PROPERTY(QString gitBranchLabel READ gitBranchLabel NOTIFY gitSummaryChanged)

    Q_PROPERTY(bool splitEditorVisible READ splitEditorVisible NOTIFY splitEditorChanged)
    Q_PROPERTY(bool diffEditorVisible READ diffEditorVisible NOTIFY splitEditorChanged)
    Q_PROPERTY(QString secondaryEditorPath READ secondaryEditorPath NOTIFY splitEditorChanged)
    Q_PROPERTY(QString secondaryEditorText READ secondaryEditorText WRITE setSecondaryEditorText NOTIFY splitEditorChanged)
    Q_PROPERTY(QString secondaryLanguageId READ secondaryLanguageId NOTIFY splitEditorChanged)
    Q_PROPERTY(QString secondaryEditorTitle READ secondaryEditorTitle NOTIFY splitEditorChanged)
    Q_PROPERTY(bool secondaryEditorDirty READ secondaryEditorDirty NOTIFY splitEditorChanged)
    Q_PROPERTY(QString diffOriginalText READ diffOriginalText NOTIFY splitEditorChanged)
    Q_PROPERTY(QString diffModifiedText READ diffModifiedText NOTIFY splitEditorChanged)
    Q_PROPERTY(QString diffEditorTitle READ diffEditorTitle NOTIFY splitEditorChanged)

    Q_PROPERTY(QString toastMessage READ toastMessage NOTIFY toastChanged)
    Q_PROPERTY(bool toastVisible READ toastVisible NOTIFY toastChanged)

public:
    MainViewModel(std::unique_ptr<ide::services::DocumentService> documentService,
                  std::unique_ptr<ide::services::DiagnosticCoordinator> diagnosticCoordinator,
                  std::unique_ptr<ide::services::CodeIntelService> codeIntelService,
                  std::unique_ptr<ide::services::AiService> aiService,
                  std::unique_ptr<ide::services::GitService> gitService,
                  std::unique_ptr<ide::services::WorkspaceService> workspaceService,
                  std::unique_ptr<ide::services::SearchService> searchService,
                  std::unique_ptr<ide::services::TerminalService> terminalService,
                  QObject* parent = nullptr);

    QString editorText() const;
    void setEditorText(const QString& text);

    QString currentPath() const;
    QString languageId() const;
    QString editorTabTitle() const;

    QString chatInput() const;
    void setChatInput(const QString& text);

    QString chatResponse() const;
    QString statusMessage() const;
    QString diagnosticsProviderName() const;
    QString diagnosticsStatusLine() const;
    bool definitionAvailable() const;
    QString codeIntelProviderName() const;
    QString aiBackendName() const;
    QString aiBackendStatusLine() const;
    int diagnosticsCount() const;
    int workspaceFileCount() const;
    bool workspaceLoading() const;
    QString workspaceLoadingText() const;
    int searchResultCount() const;
    int completionCount() const;
    int relevantContextCount() const;

    QString workspaceRootPath() const;
    void setWorkspaceRootPath(const QString& path);

    QString searchPattern() const;
    void setSearchPattern(const QString& pattern);

    QString hoverText() const;
    QString definitionText() const;

    QString terminalCommand() const;
    void setTerminalCommand(const QString& command);
    QString terminalOutput() const;
    QString terminalBackendName() const;

    QString aiSystemPrompt() const;
    void setAiSystemPrompt(const QString& value);
    double aiTemperature() const;
    void setAiTemperature(double value);
    int aiMaxOutputTokens() const;
    void setAiMaxOutputTokens(int value);
    int aiDocumentContextChars() const;
    void setAiDocumentContextChars(int value);
    int aiWorkspaceContextChars() const;
    void setAiWorkspaceContextChars(int value);
    int aiWorkspaceContextMaxFiles() const;
    void setAiWorkspaceContextMaxFiles(int value);
    bool aiIncludeDocument() const;
    void setAiIncludeDocument(bool value);
    bool aiIncludeDiagnostics() const;
    void setAiIncludeDiagnostics(bool value);
    bool aiIncludeGitSummary() const;
    void setAiIncludeGitSummary(bool value);
    bool aiIncludeWorkspaceContext() const;
    void setAiIncludeWorkspaceContext(bool value);
    bool aiEnableTools() const;
    void setAiEnableTools(bool value);
    int aiMaxToolRounds() const;
    void setAiMaxToolRounds(int value);
    bool aiRequireApprovalForDestructive() const;
    void setAiRequireApprovalForDestructive(bool value);
    QString aiContextSummary() const;
    QString relevantContextSummary() const;
    QString toolCallLog() const;
    int toolCallCount() const;
    QString toolCatalogSummary() const;
    int pendingApprovalCount() const;
    QString pendingApprovalSummary() const;

    QObject* diagnosticsModel();
    QVariantList currentFileDiagnostics() const;
    QObject* workspaceFilesModel();
    QObject* workspaceTreeModel();
    QObject* searchResultsModel();
    QObject* completionModel();
    QObject* relevantContextModel();
    QObject* pendingApprovalModel();

    bool hasPatchPreview() const;
    bool canApplyPatchPreview() const;
    QString patchSummary() const;
    QString patchPreviewText() const;

    QObject* openEditorsModel();
    int openEditorCount() const;
    bool showWelcomeTab() const;
    QStringList recentFolders() const;
    QObject* commandPaletteModel();
    int commandPaletteCount() const;

    int primaryViewIndex() const;
    void setPrimaryViewIndex(int value);
    bool secondaryAiVisible() const;
    void setSecondaryAiVisible(bool value);
    int secondaryAiTab() const;
    void setSecondaryAiTab(int value);
    int secondaryAiWidth() const;
    void setSecondaryAiWidth(int value);
    int bottomPanelTab() const;
    void setBottomPanelTab(int value);
    int bottomPanelHeight() const;
    void setBottomPanelHeight(int value);

    bool currentDocumentDirty() const;

    QObject* gitChangesModel();
    QObject* scmSectionsModel();
    QObject* gitRecentCommitsModel();
    int gitChangeCount() const;
    int gitStagedCount() const;
    int gitUnstagedCount() const;
    int gitUntrackedCount() const;
    int gitRecentCommitCount() const;
    QString scmCommitMessage() const;
    void setScmCommitMessage(const QString& value);

    QString gitSummary() const;
    QString gitBranchLabel() const;

    bool splitEditorVisible() const;
    bool diffEditorVisible() const;
    QString secondaryEditorPath() const;
    QString secondaryEditorText() const;
    void setSecondaryEditorText(const QString& text);
    QString secondaryLanguageId() const;
    QString secondaryEditorTitle() const;
    bool secondaryEditorDirty() const;
    QString diffOriginalText() const;
    QString diffModifiedText() const;
    QString diffEditorTitle() const;
    QString toastMessage() const;
    bool toastVisible() const;

    Q_INVOKABLE void analyzeNow();
    Q_INVOKABLE void askAssistant();
    Q_INVOKABLE void saveCurrent();
    Q_INVOKABLE void loadSampleCpp();
    Q_INVOKABLE void loadSampleRust();
    Q_INVOKABLE void refreshWorkspace();
    Q_INVOKABLE void openWorkspaceFile(const QString& path);
    Q_INVOKABLE void triggerOpenFileDialog();
    Q_INVOKABLE void triggerOpenFolderDialog();
    Q_INVOKABLE void triggerQuickOpen();
    Q_INVOKABLE void openFileFromDialog(const QString& filePath);
    Q_INVOKABLE void openFolderFromDialog(const QString& folderPath);
    Q_INVOKABLE void reopenRecentFolder(const QString& folderPath);
    Q_INVOKABLE void removeRecentFolder(const QString& folderPath);
    Q_INVOKABLE void openWorkspaceFileInSplit(const QString& path);
    Q_INVOKABLE void createWorkspaceFile();
    Q_INVOKABLE void createWorkspaceFolder();
    Q_INVOKABLE void toggleWorkspaceFolder(const QString& id);
    Q_INVOKABLE void collapseWorkspaceFolders();
    Q_INVOKABLE void runSearch();
    Q_INVOKABLE void openSearchResult(const QString& path, int line, int column);
    Q_INVOKABLE void setCursorPosition(int line, int column);
    Q_INVOKABLE void requestCompletionsAtCursor();
    Q_INVOKABLE void requestHoverAtCursor();
    Q_INVOKABLE void requestDefinitionAtCursor();
    Q_INVOKABLE void runTerminalCommand();
    Q_INVOKABLE void dismissToast();
    Q_INVOKABLE void applyCompletionInsert(const QString& insertText);
    Q_INVOKABLE void refreshRelevantContext();
    Q_INVOKABLE void extractPatchPreview();
    Q_INVOKABLE void applyAssistantPatch();
    Q_INVOKABLE void clearPatchPreview();
    Q_INVOKABLE void openPatchPreviewDiff();
    Q_INVOKABLE void closeSplitEditor();
    Q_INVOKABLE void saveSecondaryEditor();
    Q_INVOKABLE void approvePendingTool(const QString& approvalId);
    Q_INVOKABLE void rejectPendingTool(const QString& approvalId);
    Q_INVOKABLE void approveAllPendingTools();
    Q_INVOKABLE void clearPendingTools();

    Q_INVOKABLE void refreshCommandPalette(const QString& query = QString());
    Q_INVOKABLE void runCommandPaletteCommand(const QString& commandId);
    Q_INVOKABLE void openFirstMatchingWorkspaceFile(const QString& query);
    Q_INVOKABLE void switchOpenEditor(const QString& path);
    Q_INVOKABLE void closeOpenEditor(const QString& path);
    Q_INVOKABLE void stageGitPath(const QString& path);
    Q_INVOKABLE void unstageGitPath(const QString& path);
    Q_INVOKABLE void discardGitPath(const QString& path);
    Q_INVOKABLE void openGitDiff(const QString& path);
    Q_INVOKABLE void commitGitChanges();
    Q_INVOKABLE void toggleSecondaryAiSidebar();
    Q_INVOKABLE void showAssistantSidebar();
    Q_INVOKABLE void showModelsSidebar();

signals:
    void editorTextChanged();
    void currentPathChanged();
    void chatInputChanged();
    void chatResponseChanged();
    void statusMessageChanged();
    void diagnosticsCountChanged();
    void diagnosticsChanged();
    void diagnosticsStatusChanged();
    void workspaceChanged();
    void workspaceLoadingChanged();
    void searchPatternChanged();
    void searchResultsChanged();
    void hoverTextChanged();
    void definitionTextChanged();
    void terminalCommandChanged();
    void terminalOutputChanged();
    void completionsChanged();
    void aiSettingsChanged();
    void relevantContextChanged();
    void patchPreviewChanged();
    void splitEditorChanged();
    void openEditorsChanged();
    void recentFoldersChanged();
    void commandPaletteChanged();
    void gitChanged();
    void primaryViewIndexChanged();
    void secondaryAiChanged();
    void bottomPanelChanged();
    void scmCommitMessageChanged();
    void gitSummaryChanged();
    void quickOpenRequested();
    void toastChanged();

private:
    struct CommandSpec {
        QString id;
        QString title;
        QString category;
        QString hint;
    };

    QString diagnosticsSummary() const;
    void updateStatusMessage(const QString& message);
    void applyDiagnostics(const std::vector<ide::services::interfaces::Diagnostic>& diagnostics);
    void showToast(const QString& message);
    void setDocumentSample(const QString& path, const QString& text);
    void goToDocumentLocation(const QString& path, int line, int column);
    void syncAfterToolRun(const QStringList& touchedPaths);
    void refreshPendingApprovals();
    void rebuildCommandPalette(const QString& query);
    void rebuildWorkspaceModels();
    void startWorkspaceRefresh(const QString& statusHint);
    void refreshGitState();
    QString uniqueWorkspaceChildPath(const QString& preferredName) const;
    QString localPathFromUrlOrPath(const QString& rawPath) const;
    void addRecentFolder(const QString& folderPath);

    std::unique_ptr<ide::services::DocumentService> m_documentService;
    std::unique_ptr<ide::services::DiagnosticCoordinator> m_diagnosticCoordinator;
    std::unique_ptr<ide::services::CodeIntelService> m_codeIntelService;
    std::unique_ptr<ide::services::AiService> m_aiService;
    std::unique_ptr<ide::services::GitService> m_gitService;
    std::unique_ptr<ide::services::WorkspaceService> m_workspaceService;
    std::unique_ptr<ide::services::SearchService> m_searchService;
    std::unique_ptr<ide::services::TerminalService> m_terminalService;

    ide::services::WorkspaceContextService m_workspaceContextService;
    ide::services::DiffApplyService m_diffApplyService;

    ide::ui::models::DiagnosticListModel m_diagnosticsModel;
    ide::ui::models::WorkspaceFileListModel m_workspaceFilesModel;
    ide::ui::models::WorkspaceTreeListModel m_workspaceTreeModel;
    ide::ui::models::SearchResultListModel m_searchResultsModel;
    ide::ui::models::CompletionListModel m_completionModel;
    ide::ui::models::RelevantContextListModel m_relevantContextModel;
    ide::ui::models::PendingToolApprovalListModel m_pendingApprovalModel;
    ide::ui::models::CommandPaletteListModel m_commandPaletteModel;

    std::vector<CommandSpec> m_commandCatalog;
    int m_cursorLine = 1;
    int m_cursorColumn = 1;

    ide::ui::viewmodels::EditorManager m_editorManager;
    ide::ui::viewmodels::GitViewModel m_gitViewModel;
    ide::ui::viewmodels::UiStateManager m_uiStateManager;

    QString m_chatInput;
    QString m_chatResponse;
    QString m_statusMessage;
    QString m_diagnosticsProviderName;
    QString m_diagnosticsStatusLine;
    QString m_workspaceRootPath;
    QString m_searchPattern;
    QString m_hoverText;
    QString m_definitionText;
    QString m_terminalCommand = "pwd";
    QString m_terminalOutput;
    QVariantList m_currentFileDiagnostics;
    QString m_toastMessage;
    bool m_toastVisible = false;
    QString m_renderedWorkspaceContext;
    QString m_relevantContextSummary;
    ide::services::PatchPreview m_patchPreview;
    std::vector<ide::services::interfaces::WorkspaceFile> m_workspaceFiles;
    int m_workspaceRefreshGeneration = 0;
    bool m_workspaceLoading = false;
    QString m_workspaceLoadingText;
};

} // namespace ide::ui::viewmodels
