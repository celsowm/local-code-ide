#include "services/AiService.hpp"

#include <QJsonDocument>
#include <QStringList>
#include <QtGlobal>

namespace ide::services {

AiService::AiService(std::unique_ptr<interfaces::IAiBackend> backend,
                     std::unique_ptr<ToolCallingService> toolCallingService)
    : m_backend(std::move(backend))
    , m_toolCallingService(std::move(toolCallingService)) {
    m_requestBuilder = std::make_unique<AiRequestBuilder>(m_settings);
    m_toolCoordinator = std::make_unique<ToolCallCoordinator>(m_toolCallingService.get());
}

QString AiService::ask(const QString& prompt,
                       const QString& workspaceRoot,
                       const QString& currentPath,
                       const QString& documentText,
                       const QString& diagnosticsText,
                       const QString& gitSummary,
                       const QString& workspaceContextText) {
    m_lastToolLog.clear();
    m_lastToolTouchedPaths.clear();
    m_lastToolCallCount = 0;

    if (m_toolCoordinator) {
        m_toolCoordinator->reset();
    }

    std::vector<interfaces::AiToolDefinition> tools;
    if (m_settings.enableTools && m_toolCallingService) {
        m_toolCallingService->setRequireApprovalForDestructive(m_settings.requireApprovalForDestructiveTools);
        tools = m_toolCallingService->definitions();
    }

    auto request = m_requestBuilder->build(prompt, currentPath, documentText, diagnosticsText, gitSummary, workspaceContextText, tools);
    QString finalContent;

    const int rounds = request.enableTools ? qMax(0, request.maxToolRounds) : 0;
    for (int round = 0; round <= rounds; ++round) {
        const auto response = m_backend->complete(request);
        if (!response.content.trimmed().isEmpty()) {
            finalContent = response.content;
        }

        if (!request.enableTools || response.toolCalls.empty()) {
            syncToolStateFromRunner();
            if (!response.ok && finalContent.trimmed().isEmpty()) {
                return response.errorText.isEmpty() ? QStringLiteral("[assistant] sem resposta.") : response.errorText;
            }
            return finalContent.trimmed().isEmpty()
                ? QStringLiteral("[assistant] sem conteúdo retornado.")
                : finalContent;
        }

        interfaces::AiMessage assistantMessage;
        assistantMessage.role = QStringLiteral("assistant");
        assistantMessage.content = response.content;
        assistantMessage.toolCalls = response.toolCalls;
        request.messages.push_back(assistantMessage);

        for (const auto& call : response.toolCalls) {
            if (!m_toolCallingService) {
                continue;
            }

            auto toolResult = m_toolCallingService->execute(call, workspaceRoot, currentPath);
            toolResult.payload.insert(QStringLiteral("tool_call_id"), call.id);

            interfaces::AiMessage toolMessage;
            toolMessage.role = QStringLiteral("tool");
            toolMessage.toolCallId = call.id;
            toolMessage.toolName = call.name;
            toolMessage.content = QString::fromUtf8(
                QJsonDocument(toolResult.payload).toJson(QJsonDocument::Compact)
            );
            request.messages.push_back(toolMessage);
        }
    }

    syncToolStateFromRunner();
    if (!finalContent.trimmed().isEmpty()) {
        return finalContent + QStringLiteral("\n\n[tooling] Max tool rounds reached before a clean stop.");
    }
    return QStringLiteral("[tooling] Max tool rounds reached sem resposta final.");
}

QString AiService::backendName() const {
    return m_backend->name();
}

QString AiService::backendStatusLine() const {
    return m_backend->statusLine();
}

bool AiService::isBackendAvailable() const {
    return m_backend->isAvailable();
}

QString AiService::systemPrompt() const { return m_settings.systemPrompt; }
void AiService::setSystemPrompt(const QString& value) { 
    m_settings.systemPrompt = value.trimmed().isEmpty() ? Settings{}.systemPrompt : value; 
    m_requestBuilder = std::make_unique<AiRequestBuilder>(m_settings);
}
int AiService::maxOutputTokens() const { return m_settings.maxOutputTokens; }
void AiService::setMaxOutputTokens(int value) { 
    m_settings.maxOutputTokens = qBound(64, value, 8192); 
    m_requestBuilder = std::make_unique<AiRequestBuilder>(m_settings);
}
double AiService::temperature() const { return m_settings.temperature; }
void AiService::setTemperature(double value) { 
    m_settings.temperature = qBound(0.0, value, 2.0); 
    m_requestBuilder = std::make_unique<AiRequestBuilder>(m_settings);
}
int AiService::documentContextChars() const { return m_settings.documentContextChars; }
void AiService::setDocumentContextChars(int value) { 
    m_settings.documentContextChars = qBound(512, value, 200000); 
    m_requestBuilder = std::make_unique<AiRequestBuilder>(m_settings);
}
int AiService::workspaceContextChars() const { return m_settings.workspaceContextChars; }
void AiService::setWorkspaceContextChars(int value) { 
    m_settings.workspaceContextChars = qBound(0, value, 400000); 
    m_requestBuilder = std::make_unique<AiRequestBuilder>(m_settings);
}
int AiService::workspaceContextMaxFiles() const { return m_settings.workspaceContextMaxFiles; }
void AiService::setWorkspaceContextMaxFiles(int value) { 
    m_settings.workspaceContextMaxFiles = qBound(0, value, 24); 
    m_requestBuilder = std::make_unique<AiRequestBuilder>(m_settings);
}
bool AiService::includeDocument() const { return m_settings.includeDocument; }
void AiService::setIncludeDocument(bool value) { 
    m_settings.includeDocument = value; 
    m_requestBuilder = std::make_unique<AiRequestBuilder>(m_settings);
}
bool AiService::includeDiagnostics() const { return m_settings.includeDiagnostics; }
void AiService::setIncludeDiagnostics(bool value) { 
    m_settings.includeDiagnostics = value; 
    m_requestBuilder = std::make_unique<AiRequestBuilder>(m_settings);
}
bool AiService::includeGitSummary() const { return m_settings.includeGitSummary; }
void AiService::setIncludeGitSummary(bool value) { 
    m_settings.includeGitSummary = value; 
    m_requestBuilder = std::make_unique<AiRequestBuilder>(m_settings);
}
bool AiService::includeWorkspaceContext() const { return m_settings.includeWorkspaceContext; }
void AiService::setIncludeWorkspaceContext(bool value) { 
    m_settings.includeWorkspaceContext = value; 
    m_requestBuilder = std::make_unique<AiRequestBuilder>(m_settings);
}
bool AiService::toolsEnabled() const { return m_settings.enableTools && m_toolCallingService != nullptr; }
void AiService::setToolsEnabled(bool value) { 
    m_settings.enableTools = value; 
    m_requestBuilder = std::make_unique<AiRequestBuilder>(m_settings);
}
bool AiService::requireApprovalForDestructiveTools() const {
    return m_toolCallingService ? m_toolCallingService->requireApprovalForDestructive() : m_settings.requireApprovalForDestructiveTools;
}
void AiService::setRequireApprovalForDestructiveTools(bool value) {
    m_settings.requireApprovalForDestructiveTools = value;
    if (m_toolCallingService) {
        m_toolCallingService->setRequireApprovalForDestructive(value);
    }
}
int AiService::maxToolRounds() const { return m_settings.maxToolRounds; }
void AiService::setMaxToolRounds(int value) { 
    m_settings.maxToolRounds = qBound(0, value, 12); 
    m_requestBuilder = std::make_unique<AiRequestBuilder>(m_settings);
}

QString AiService::contextSummary(const QString& currentPath,
                                  const QString& documentText,
                                  const QString& diagnosticsText,
                                  const QString& gitSummary,
                                  const QString& workspaceContextText) const {
    QStringList parts;
    parts << QStringLiteral("file=%1").arg(currentPath.isEmpty() ? QStringLiteral("untitled") : currentPath);
    parts << QStringLiteral("temp=%1").arg(QString::number(m_settings.temperature, 'f', 2));
    parts << QStringLiteral("max=%1").arg(QString::number(m_settings.maxOutputTokens));
    if (m_settings.includeDocument) {
        const QString trimmed = trimForContext(documentText, m_settings.documentContextChars);
        parts << QStringLiteral("doc=%1 chars").arg(QString::number(trimmed.size()));
    } else {
        parts << QStringLiteral("doc=off");
    }
    parts << QStringLiteral("diag=%1").arg(m_settings.includeDiagnostics ? QString::number(diagnosticsText.size()) + QStringLiteral(" chars") : QStringLiteral("off"));
    parts << QStringLiteral("git=%1").arg(m_settings.includeGitSummary ? QString::number(gitSummary.size()) + QStringLiteral(" chars") : QStringLiteral("off"));
    if (m_settings.includeWorkspaceContext) {
        const QString trimmed = trimForContext(workspaceContextText, m_settings.workspaceContextChars);
        parts << QStringLiteral("workspace=%1 chars/%2 files")
            .arg(QString::number(trimmed.size()))
            .arg(QString::number(m_settings.workspaceContextMaxFiles));
    } else {
        parts << QStringLiteral("workspace=off");
    }
    parts << QStringLiteral("tools=%1").arg(
        (m_settings.enableTools && m_toolCallingService)
            ? QStringLiteral("on/%1 rounds/%2")
                  .arg(QString::number(m_settings.maxToolRounds),
                       requireApprovalForDestructiveTools() ? QStringLiteral("confirm-destr") : QStringLiteral("auto-destr"))
            : QStringLiteral("off")
    );
    return parts.join(QStringLiteral(" · "));
}

QString AiService::lastToolLog() const {
    return m_lastToolLog;
}

int AiService::lastToolCallCount() const {
    return m_lastToolCallCount;
}

QStringList AiService::lastToolTouchedPaths() const {
    return m_lastToolTouchedPaths;
}

QString AiService::toolCatalogSummary() const {
    return m_toolCallingService ? m_toolCallingService->catalogSummary() : QStringLiteral("no tools");
}

int AiService::pendingApprovalCount() const {
    return m_toolCallingService ? m_toolCallingService->pendingApprovalCount() : 0;
}

QString AiService::pendingApprovalSummary() const {
    return m_toolCallingService ? m_toolCallingService->pendingApprovalSummary() : QStringLiteral("no approvals");
}

std::vector<PendingToolApproval> AiService::pendingApprovals() const {
    return m_toolCallingService ? m_toolCallingService->pendingApprovals() : std::vector<PendingToolApproval>{};
}

interfaces::ToolResult AiService::approvePendingTool(const QString& approvalId,
                                                     const QString& workspaceRoot,
                                                     const QString& currentPath) {
    if (!m_toolCallingService) {
        interfaces::ToolResult result;
        result.success = false;
        result.humanSummary = QStringLiteral("Tool calling indisponível.");
        result.payload = QJsonObject{{QStringLiteral("error"), QStringLiteral("tooling_unavailable")}};
        return result;
    }
    auto result = m_toolCallingService->approvePending(approvalId, workspaceRoot, currentPath);
    syncToolStateFromRunner();
    return result;
}

bool AiService::rejectPendingTool(const QString& approvalId) {
    if (!m_toolCallingService) {
        return false;
    }
    const bool ok = m_toolCallingService->rejectPending(approvalId);
    syncToolStateFromRunner();
    return ok;
}

void AiService::clearPendingApprovals() {
    if (!m_toolCallingService) {
        return;
    }
    m_toolCallingService->clearPendingApprovals();
    syncToolStateFromRunner();
}

QString AiService::trimForContext(const QString& text, int maxChars) const {
    if (maxChars <= 0 || text.size() <= maxChars) {
        return text;
    }

    const int head = maxChars * 7 / 10;
    const int tail = maxChars - head;
    return text.left(head)
        + QStringLiteral("\n\n/* ... context truncated for budget ... */\n\n")
        + text.right(tail);
}

void AiService::syncToolStateFromRunner() {
    if (!m_toolCallingService) {
        m_lastToolLog.clear();
        m_lastToolTouchedPaths.clear();
        m_lastToolCallCount = 0;
        return;
    }

    m_lastToolLog = m_toolCallingService->lastLog();
    m_lastToolTouchedPaths = m_toolCallingService->lastTouchedPaths();
    m_lastToolCallCount = m_toolCallingService->lastCallCount();
}

} // namespace ide::services
