#include "adapters/ai/AdaptiveAiBackend.hpp"
#include "adapters/ai/LlamaCppServerBackend.hpp"
#include "adapters/ai/MockAiBackend.hpp"
#include "adapters/codeintel/LspCodeIntelProvider.hpp"
#include "adapters/codeintel/MockCodeIntelProvider.hpp"
#include "adapters/diagnostics/HybridDiagnosticProvider.hpp"
#include "adapters/diagnostics/LocalSyntaxDiagnosticProvider.hpp"
#include "adapters/diagnostics/LspClient.hpp"
#include "adapters/diagnostics/LspDiagnosticProvider.hpp"
#include "adapters/modelhub/HuggingFaceFileDownloader.hpp"
#include "adapters/modelhub/HuggingFaceModelCatalogProvider.hpp"
#include "adapters/search/FileSystemSearchProvider.hpp"
#include "adapters/terminal/ProcessTerminalBackend.hpp"
#include "adapters/tools/CreateFileTool.hpp"
#include "adapters/tools/ApplyUnifiedDiffTool.hpp"
#include "adapters/tools/EditFileTool.hpp"
#include "adapters/tools/ListWorkspaceFilesTool.hpp"
#include "adapters/tools/ReadFileTool.hpp"
#include "adapters/tools/SearchWorkspaceTool.hpp"
#include "adapters/vcs/GitCliProvider.hpp"
#include "adapters/workspace/FileSystemWorkspaceProvider.hpp"
#include "services/AiService.hpp"
#include "services/CodeIntelService.hpp"
#include "services/DiagnosticService.hpp"
#include "services/DocumentService.hpp"
#include "services/GitService.hpp"
#include "services/HardwareProfileService.hpp"
#include "services/LlamaServerProcessService.hpp"
#include "services/LocalModelRuntimeService.hpp"
#include "services/ModelCatalogService.hpp"
#include "services/SearchService.hpp"
#include "services/TerminalService.hpp"
#include "services/ToolCallingService.hpp"
#include "services/WorkspaceService.hpp"
#include "ui/highlighting/DocumentHighlighter.hpp"
#include "ui/viewmodels/MainViewModel.hpp"
#include "ui/viewmodels/ModelHubViewModel.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQml>
#include <utility>

namespace {
QString envOrDefault(const char* key, const QString& fallback = {}) {
    const QByteArray value = qgetenv(key);
    return value.isEmpty() ? fallback : QString::fromUtf8(value);
}

QStringList splitArgs(const QString& value) {
    return value.split(';', Qt::SkipEmptyParts);
}
} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication::setOrganizationName(QStringLiteral("LocalCodeIDE"));
    QCoreApplication::setApplicationName(QStringLiteral("LocalCodeIDE"));

    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    auto documentService = std::make_unique<ide::services::DocumentService>();

    std::shared_ptr<ide::adapters::diagnostics::LspClient> sharedLspClient;
    const QString lspCommand = envOrDefault("LOCALCODEIDE_LSP_COMMAND");
    if (!lspCommand.isEmpty()) {
        sharedLspClient = std::make_shared<ide::adapters::diagnostics::LspClient>(
            lspCommand,
            splitArgs(envOrDefault("LOCALCODEIDE_LSP_ARGS"))
        );
    }

    auto diagnosticProviders = std::make_unique<ide::adapters::diagnostics::HybridDiagnosticProvider>();
    diagnosticProviders->addProvider(std::make_unique<ide::adapters::diagnostics::LocalSyntaxDiagnosticProvider>());
    if (sharedLspClient) {
        diagnosticProviders->addProvider(
            std::make_unique<ide::adapters::diagnostics::LspDiagnosticProvider>(sharedLspClient)
        );
    }
    auto diagnosticService = std::make_unique<ide::services::DiagnosticService>(std::move(diagnosticProviders));

    std::unique_ptr<ide::services::interfaces::ICodeIntelProvider> codeIntelProvider;
    if (sharedLspClient) {
        codeIntelProvider = std::make_unique<ide::adapters::codeintel::LspCodeIntelProvider>(sharedLspClient);
    } else {
        codeIntelProvider = std::make_unique<ide::adapters::codeintel::MockCodeIntelProvider>();
    }
    auto codeIntelService = std::make_unique<ide::services::CodeIntelService>(std::move(codeIntelProvider));

    auto localModelRuntimeService = std::make_unique<ide::services::LocalModelRuntimeService>(
        envOrDefault("LOCALCODEIDE_LLAMACPP_URL", "http://127.0.0.1:8080"),
        envOrDefault("LOCALCODEIDE_LLAMACPP_CONTEXT", "8192").toInt()
    );
    auto* localRuntimePtr = localModelRuntimeService.get();

    auto llamaServerProcessService = std::make_unique<ide::services::LlamaServerProcessService>(
        envOrDefault("LOCALCODEIDE_LLAMACPP_URL", "http://127.0.0.1:8080"),
        envOrDefault("LOCALCODEIDE_LLAMASERVER_BIN", "llama-server"),
        envOrDefault("LOCALCODEIDE_LLAMASERVER_ARGS")
    );
    auto* llamaServerPtr = llamaServerProcessService.get();

    std::vector<std::unique_ptr<ide::services::interfaces::ITool>> aiTools;
    aiTools.push_back(std::make_unique<ide::adapters::tools::ReadFileTool>());
    aiTools.push_back(std::make_unique<ide::adapters::tools::ListWorkspaceFilesTool>());
    aiTools.push_back(std::make_unique<ide::adapters::tools::SearchWorkspaceTool>());
    aiTools.push_back(std::make_unique<ide::adapters::tools::CreateFileTool>());
    aiTools.push_back(std::make_unique<ide::adapters::tools::EditFileTool>());
    aiTools.push_back(std::make_unique<ide::adapters::tools::ApplyUnifiedDiffTool>());
    auto toolCallingService = std::make_unique<ide::services::ToolCallingService>(std::move(aiTools));

    std::unique_ptr<ide::services::interfaces::IAiBackend> aiBackend;
    if (envOrDefault("LOCALCODEIDE_AI_BACKEND", "adaptive") == "llama") {
        aiBackend = std::make_unique<ide::adapters::ai::LlamaCppServerBackend>(
            envOrDefault("LOCALCODEIDE_LLAMACPP_URL", "http://127.0.0.1:8080"),
            envOrDefault("LOCALCODEIDE_LLAMACPP_MODEL")
        );
    } else if (envOrDefault("LOCALCODEIDE_AI_BACKEND", "adaptive") == "mock") {
        aiBackend = std::make_unique<ide::adapters::ai::MockAiBackend>();
    } else {
        aiBackend = std::make_unique<ide::adapters::ai::AdaptiveAiBackend>(
            localRuntimePtr,
            llamaServerPtr,
            envOrDefault("LOCALCODEIDE_LLAMACPP_TIMEOUT_MS", "30000").toInt()
        );
    }
    auto aiService = std::make_unique<ide::services::AiService>(
        std::move(aiBackend),
        std::move(toolCallingService)
    );

    auto gitService = std::make_unique<ide::services::GitService>(
        std::make_unique<ide::adapters::vcs::GitCliProvider>()
    );

    auto workspaceService = std::make_unique<ide::services::WorkspaceService>(
        std::make_unique<ide::adapters::workspace::FileSystemWorkspaceProvider>()
    );

    auto searchService = std::make_unique<ide::services::SearchService>(
        std::make_unique<ide::adapters::search::FileSystemSearchProvider>()
    );

    auto terminalService = std::make_unique<ide::services::TerminalService>(
        std::make_unique<ide::adapters::terminal::ProcessTerminalBackend>()
    );

    auto modelCatalogService = std::make_unique<ide::services::ModelCatalogService>(
        std::make_unique<ide::adapters::modelhub::HuggingFaceModelCatalogProvider>(
            envOrDefault("LOCALCODEIDE_HF_TOKEN")
        )
    );

    auto modelDownloader = std::make_unique<ide::adapters::modelhub::HuggingFaceFileDownloader>(
        envOrDefault("LOCALCODEIDE_HF_TOKEN")
    );

    auto hardwareProfileService = std::make_unique<ide::services::HardwareProfileService>(
        envOrDefault("LOCALCODEIDE_RAM_GIB"),
        envOrDefault("LOCALCODEIDE_VRAM_GIB")
    );

    ide::ui::viewmodels::MainViewModel mainViewModel(
        std::move(documentService),
        std::move(diagnosticService),
        std::move(codeIntelService),
        std::move(aiService),
        std::move(gitService),
        std::move(workspaceService),
        std::move(searchService),
        std::move(terminalService)
    );

    ide::ui::viewmodels::ModelHubViewModel modelHubViewModel(
        std::move(modelCatalogService),
        std::move(modelDownloader),
        std::move(hardwareProfileService),
        localRuntimePtr,
        std::move(llamaServerProcessService)
    );

    const QString workspaceFromEnv = envOrDefault("LOCALCODEIDE_WORKSPACE", QDir::currentPath());
    const QString modelHubAuthor = envOrDefault("LOCALCODEIDE_HF_AUTHOR", QStringLiteral("bartowski"));
    const QString modelHubDownloadDir = envOrDefault("LOCALCODEIDE_HF_DOWNLOAD_DIR");
    mainViewModel.setWorkspaceRootPath(workspaceFromEnv);
    modelHubViewModel.setAuthor(modelHubAuthor);
    if (!modelHubDownloadDir.isEmpty()) {
        modelHubViewModel.setTargetDownloadDir(modelHubDownloadDir);
    }
    modelHubViewModel.searchRepos();

    qmlRegisterType<ide::ui::highlighting::DocumentHighlighter>("LocalCodeIDE", 1, 0, "DocumentHighlighter");

    engine.rootContext()->setContextProperty("mainViewModel", &mainViewModel);
    engine.rootContext()->setContextProperty("modelHubViewModel", &modelHubViewModel);
    engine.loadFromModule("LocalCodeIDE", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
