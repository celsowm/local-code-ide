#include "adapters/ai/MockAiBackend.hpp"

namespace ide::adapters::ai {

ide::services::interfaces::AiResponse MockAiBackend::complete(const ide::services::interfaces::AiRequest& request) {
    const int lines = request.documentText.count('\n') + (request.documentText.isEmpty() ? 0 : 1);
    ide::services::interfaces::AiResponse response;
    response.content = QString(
        "[mock-ai]\n"
        "Prompt: %1\n\n"
        "Arquivo atual: %2\n"
        "Linhas em contexto: %3\n"
        "Diagnostics: %4\n"
        "Git summary: %5\n"
        "Workspace ctx: %6\n"
        "Tools: %7\n\n"
        "Próximo passo real: subir o llama-server local com suporte a tools, habilitar search/create/edit/read e deixar o modelo decidir quando agir."
    ).arg(
        request.prompt,
        request.currentPath.isEmpty() ? QStringLiteral("untitled") : request.currentPath,
        QString::number(lines),
        request.includeDiagnostics && !request.diagnosticsText.isEmpty() ? QStringLiteral("on") : QStringLiteral("off"),
        request.includeGitSummary && !request.gitSummary.isEmpty() ? request.gitSummary : QStringLiteral("off"),
        request.includeWorkspaceContext && !request.workspaceContextText.isEmpty() ? QString::number(request.workspaceContextText.size()) + QStringLiteral(" chars") : QStringLiteral("off"),
        request.enableTools ? QStringLiteral("available") : QStringLiteral("off")
    );
    return response;
}

QString MockAiBackend::name() const {
    return "Mock AI Backend";
}

QString MockAiBackend::statusLine() const {
    return "mock mode";
}

bool MockAiBackend::isAvailable() const {
    return true;
}

} // namespace ide::adapters::ai
