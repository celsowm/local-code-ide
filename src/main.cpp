#include "adapters/ai/AdaptiveAiBackend.hpp"
#include "adapters/ai/LlamaCppServerBackend.hpp"
#include "adapters/ai/MockAiBackend.hpp"
#include "adapters/codeintel/LanguageServerCodeIntelProvider.hpp"
#include "adapters/codeintel/MockCodeIntelProvider.hpp"
#include "adapters/diagnostics/LocalSyntaxDiagnosticProvider.hpp"
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
#include "services/DiagnosticCoordinator.hpp"
#include "services/DocumentService.hpp"
#include "services/GitService.hpp"
#include "services/HardwareProfileService.hpp"
#include "services/LanguagePackService.hpp"
#include "services/LanguageServerHub.hpp"
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

#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QMutex>
#include <QMutexLocker>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QtQuickControls2/QQuickStyle>
#include <QtQml>

#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <memory>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dbghelp.h>
#else
#include <execinfo.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#endif

namespace {
QString envOrDefault(const char* key, const QString& fallback = {}) {
    const QByteArray value = qgetenv(key);
    return value.isEmpty() ? fallback : QString::fromUtf8(value);
}

QStringList splitArgs(const QString& value) {
    return value.split(';', Qt::SkipEmptyParts);
}

QString utcTimestamp() {
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}

QString levelName(QtMsgType type) {
    switch (type) {
    case QtDebugMsg:
        return QStringLiteral("DEBUG");
    case QtInfoMsg:
        return QStringLiteral("INFO");
    case QtWarningMsg:
        return QStringLiteral("WARN");
    case QtCriticalMsg:
        return QStringLiteral("CRITICAL");
    case QtFatalMsg:
        return QStringLiteral("FATAL");
    }
    return QStringLiteral("UNKNOWN");
}

QMutex gLogMutex;
std::unique_ptr<QFile> gAppLogFile;
QString gAppLogPath;
QtMessageHandler gPreviousQtHandler = nullptr;
bool gQtHandlerInstalled = false;

#ifndef _WIN32
int gSignalLogFd = -1;
std::atomic_bool gSignalHandlerActive{false};
#endif

QString resolveSessionDir() {
    const QString sessionFromEnv = envOrDefault("LOCALCODEIDE_SESSION_DIR");
    if (!sessionFromEnv.isEmpty()) {
        return sessionFromEnv;
    }
    return QDir::current().filePath(QStringLiteral("build/logs/manual"));
}

void appendAppLogLine(const QString& message) {
    QMutexLocker locker(&gLogMutex);
    if (!gAppLogFile || !gAppLogFile->isOpen()) {
        return;
    }
    QTextStream stream(gAppLogFile.get());
    stream << message << Qt::endl;
    stream.flush();
    gAppLogFile->flush();
}

void ensureAppLoggingReady() {
    QMutexLocker locker(&gLogMutex);
    if (gAppLogFile && gAppLogFile->isOpen()) {
        return;
    }

    const QString sessionDir = resolveSessionDir();
    QDir().mkpath(sessionDir);

    gAppLogPath = envOrDefault("LOCALCODEIDE_APP_LOG_FILE");
    if (gAppLogPath.isEmpty()) {
        gAppLogPath = QDir(sessionDir).filePath(QStringLiteral("app.log"));
    }

    gAppLogFile = std::make_unique<QFile>(gAppLogPath);
    if (!gAppLogFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        const QByteArray error = QStringLiteral("Failed to open app log file: %1\n").arg(gAppLogPath).toUtf8();
        std::fwrite(error.constData(), 1, static_cast<size_t>(error.size()), stderr);
        std::fflush(stderr);
    }

#ifndef _WIN32
    if (gSignalLogFd >= 0) {
        ::close(gSignalLogFd);
        gSignalLogFd = -1;
    }
    const QByteArray logPathBytes = gAppLogPath.toUtf8();
    gSignalLogFd = ::open(logPathBytes.constData(), O_WRONLY | O_CREAT | O_APPEND, 0644);
#endif
}

void qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    QString formatted = QStringLiteral("%1 [%2] %3")
                            .arg(utcTimestamp(), levelName(type), message);
    if (context.file != nullptr) {
        formatted += QStringLiteral(" (%1:%2)")
                         .arg(QString::fromUtf8(context.file))
                         .arg(context.line);
    }

    const QByteArray lineBytes = (formatted + QStringLiteral("\n")).toUtf8();
    std::fwrite(lineBytes.constData(), 1, static_cast<size_t>(lineBytes.size()), stderr);
    std::fflush(stderr);

    appendAppLogLine(formatted);

    if (gPreviousQtHandler) {
        gPreviousQtHandler(type, context, message);
    }
    if (type == QtFatalMsg) {
        std::abort();
    }
}

void installQtLogging() {
    ensureAppLoggingReady();
    if (!gQtHandlerInstalled) {
        gPreviousQtHandler = qInstallMessageHandler(qtMessageHandler);
        gQtHandlerInstalled = true;
    }
}

[[noreturn]] void terminateHandler() {
    ensureAppLoggingReady();
    QString reason = QStringLiteral("std::terminate invoked.");
    if (const std::exception_ptr current = std::current_exception()) {
        try {
            std::rethrow_exception(current);
        } catch (const std::exception& ex) {
            reason = QStringLiteral("std::terminate invoked with exception: %1").arg(ex.what());
        } catch (...) {
            reason = QStringLiteral("std::terminate invoked with non-std exception.");
        }
    }
    const QString line = QStringLiteral("%1 [FATAL] %2").arg(utcTimestamp(), reason);
    appendAppLogLine(line);
    const QByteArray bytes = (line + QStringLiteral("\n")).toUtf8();
    std::fwrite(bytes.constData(), 1, static_cast<size_t>(bytes.size()), stderr);
    std::fflush(stderr);
    std::_Exit(EXIT_FAILURE);
}

#ifdef _WIN32
bool minidumpEnabled() {
    return envOrDefault("LOCALCODEIDE_ENABLE_MINIDUMP", "0") == "1";
}

LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo) {
    ensureAppLoggingReady();

    const DWORD code = exceptionInfo != nullptr && exceptionInfo->ExceptionRecord != nullptr
                           ? exceptionInfo->ExceptionRecord->ExceptionCode
                           : 0;
    const void* address = exceptionInfo != nullptr && exceptionInfo->ExceptionRecord != nullptr
                              ? exceptionInfo->ExceptionRecord->ExceptionAddress
                              : nullptr;

    const QString crashLine = QStringLiteral("%1 [FATAL] Unhandled Windows exception code=0x%2 address=%3")
                                  .arg(utcTimestamp())
                                  .arg(code, 8, 16, QLatin1Char('0'))
                                  .arg(reinterpret_cast<quintptr>(address), 0, 16);
    appendAppLogLine(crashLine);
    const QByteArray crashBytes = (crashLine + QStringLiteral("\n")).toUtf8();
    std::fwrite(crashBytes.constData(), 1, static_cast<size_t>(crashBytes.size()), stderr);
    std::fflush(stderr);

    if (minidumpEnabled() && exceptionInfo != nullptr) {
        const QString dumpPath = QDir(resolveSessionDir()).filePath(QStringLiteral("crash.dmp"));
        const HANDLE dumpFile = CreateFileW(reinterpret_cast<LPCWSTR>(dumpPath.utf16()),
                                            GENERIC_WRITE,
                                            0,
                                            nullptr,
                                            CREATE_ALWAYS,
                                            FILE_ATTRIBUTE_NORMAL,
                                            nullptr);
        if (dumpFile != INVALID_HANDLE_VALUE) {
            MINIDUMP_EXCEPTION_INFORMATION info{};
            info.ThreadId = GetCurrentThreadId();
            info.ExceptionPointers = exceptionInfo;
            info.ClientPointers = FALSE;
            const BOOL dumpOk = MiniDumpWriteDump(GetCurrentProcess(),
                                                  GetCurrentProcessId(),
                                                  dumpFile,
                                                  static_cast<MINIDUMP_TYPE>(MiniDumpWithThreadInfo | MiniDumpWithDataSegs),
                                                  &info,
                                                  nullptr,
                                                  nullptr);
            CloseHandle(dumpFile);

            const QString dumpLine = dumpOk
                                         ? QStringLiteral("%1 [INFO] Minidump written: %2").arg(utcTimestamp(), dumpPath)
                                         : QStringLiteral("%1 [CRITICAL] Minidump failed with error=%2")
                                               .arg(utcTimestamp())
                                               .arg(GetLastError());
            appendAppLogLine(dumpLine);
            const QByteArray dumpBytes = (dumpLine + QStringLiteral("\n")).toUtf8();
            std::fwrite(dumpBytes.constData(), 1, static_cast<size_t>(dumpBytes.size()), stderr);
            std::fflush(stderr);
        }
    }
    return EXCEPTION_EXECUTE_HANDLER;
}
#else
void writeSignalBytes(const char* bytes) {
    if (bytes == nullptr) {
        return;
    }
    const size_t len = std::strlen(bytes);
    if (len == 0) {
        return;
    }
    ::write(STDERR_FILENO, bytes, len);
    if (gSignalLogFd >= 0) {
        ::write(gSignalLogFd, bytes, len);
    }
}

void fatalSignalHandler(int signalNumber, siginfo_t* info, void* /*unused*/) {
    if (gSignalHandlerActive.exchange(true)) {
        _exit(128 + signalNumber);
    }

    char header[256];
    const int signalCode = info != nullptr ? info->si_code : 0;
    const int headerLen = std::snprintf(
        header,
        sizeof(header),
        "%s [FATAL] Received signal %d (code=%d)\n",
        utcTimestamp().toUtf8().constData(),
        signalNumber,
        signalCode
    );
    if (headerLen > 0) {
        writeSignalBytes(header);
    }

    void* frames[64];
    const int frameCount = backtrace(frames, 64);
    if (frameCount > 0) {
        writeSignalBytes("Stacktrace (best effort):\n");
        backtrace_symbols_fd(frames, frameCount, STDERR_FILENO);
        if (gSignalLogFd >= 0) {
            backtrace_symbols_fd(frames, frameCount, gSignalLogFd);
        }
        writeSignalBytes("\n");
    }

    std::signal(signalNumber, SIG_DFL);
    raise(signalNumber);
}

void installFatalSignalHandlers() {
    ensureAppLoggingReady();
    struct sigaction action {};
    action.sa_sigaction = fatalSignalHandler;
    action.sa_flags = SA_SIGINFO | SA_RESETHAND;
    sigemptyset(&action.sa_mask);

    const int fatalSignals[] = {SIGSEGV, SIGABRT, SIGILL, SIGFPE, SIGBUS, SIGTERM};
    for (const int sig : fatalSignals) {
        sigaction(sig, &action, nullptr);
    }
}
#endif

int runApplication(int argc, char* argv[]) {
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("Fusion"));
    }

    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(QStringLiteral(":/qt/qml/LocalCodeIDE/assets/local-code-ide-logo-normalized.png")));
    QQmlApplicationEngine engine;

    auto documentService = std::make_unique<ide::services::DocumentService>();

    const QString lspCommand = envOrDefault("LOCALCODEIDE_LSP_COMMAND");
    const QStringList lspArgs = splitArgs(envOrDefault("LOCALCODEIDE_LSP_ARGS"));
    const bool allowPathFallback = envOrDefault("LOCALCODEIDE_ALLOW_DEV_LSP_PATH", "0") == "1";
    const bool allowDevInstallerPath = envOrDefault("LOCALCODEIDE_ALLOW_DEV_LSP_INSTALL", "0") == "1"
        || envOrDefault("LOCALCODEIDE_ALLOW_EXTERNAL_PACK_INSTALL", "0") == "1";
    const QString codeIntelProviderMode = envOrDefault("LOCALCODEIDE_CODEINTEL_PROVIDER", "hub").trimmed().toLower();

    auto languagePackService = std::make_unique<ide::services::LanguagePackService>(allowDevInstallerPath, allowPathFallback);
    auto languageServerHub = std::make_unique<ide::services::LanguageServerHub>(
        languagePackService.get(),
        lspCommand,
        lspArgs,
        allowDevInstallerPath
    );
    auto diagnosticCoordinator = std::make_unique<ide::services::DiagnosticCoordinator>(
        std::make_unique<ide::adapters::diagnostics::LocalSyntaxDiagnosticProvider>(),
        languageServerHub.get()
    );

    std::unique_ptr<ide::services::interfaces::ICodeIntelProvider> codeIntelProvider;
    if (codeIntelProviderMode == "mock") {
        codeIntelProvider = std::make_unique<ide::adapters::codeintel::MockCodeIntelProvider>();
    } else {
        codeIntelProvider = std::make_unique<ide::adapters::codeintel::LanguageServerCodeIntelProvider>(languageServerHub.get());
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

    auto mainViewModel = std::make_unique<ide::ui::viewmodels::MainViewModel>(
        std::move(documentService),
        std::move(diagnosticCoordinator),
        std::move(codeIntelService),
        std::move(aiService),
        std::move(gitService),
        std::move(workspaceService),
        std::move(searchService),
        std::move(terminalService)
    );

    auto modelHubViewModel = std::make_unique<ide::ui::viewmodels::ModelHubViewModel>(
        std::move(modelCatalogService),
        std::move(modelDownloader),
        std::move(hardwareProfileService),
        localRuntimePtr,
        std::move(llamaServerProcessService)
    );

    const QString workspaceFromEnv = envOrDefault("LOCALCODEIDE_WORKSPACE", QDir::currentPath());
    const QString modelHubAuthor = envOrDefault("LOCALCODEIDE_HF_AUTHOR", QStringLiteral("bartowski"));
    const QString modelHubDownloadDir = envOrDefault("LOCALCODEIDE_HF_DOWNLOAD_DIR");

    qmlRegisterType<ide::ui::highlighting::DocumentHighlighter>("LocalCodeIDE.Highlighting", 1, 0, "DocumentHighlighter");

    QQmlEngine::setObjectOwnership(mainViewModel.get(), QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(modelHubViewModel.get(), QQmlEngine::CppOwnership);
    engine.rootContext()->setContextProperty("mainViewModel", mainViewModel.get());
    engine.rootContext()->setContextProperty("modelHubViewModel", modelHubViewModel.get());
    engine.setInitialProperties({
        {QStringLiteral("mainViewModel"), QVariant::fromValue(mainViewModel.get())},
        {QStringLiteral("modelHubViewModel"), QVariant::fromValue(modelHubViewModel.get())}
    });

    // Load root component from the compiled QML module.
    engine.loadFromModule(QStringLiteral("LocalCodeIDE"), QStringLiteral("Main"));

    if (engine.rootObjects().isEmpty()) {
        qCritical("Failed to load QML!");
        return -1;
    }

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
        if (languageServerHub) {
            languageServerHub->shutdown();
        }
    });

    // Defer startup-heavy work until the UI is already up.
    QTimer::singleShot(0, &app, [mainVm = mainViewModel.get(),
                                 modelHubVm = modelHubViewModel.get(),
                                 workspaceFromEnv,
                                 modelHubAuthor,
                                 modelHubDownloadDir]() {
        if (mainVm) {
            mainVm->setWorkspaceRootPath(workspaceFromEnv);
        }
        if (modelHubVm) {
            modelHubVm->setAuthor(modelHubAuthor);
            if (!modelHubDownloadDir.isEmpty()) {
                modelHubVm->setTargetDownloadDir(modelHubDownloadDir);
            }
            modelHubVm->searchRepos();
        }
    });

    return app.exec();
}
} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication::setOrganizationName(QStringLiteral("LocalCodeIDE"));
    QCoreApplication::setApplicationName(QStringLiteral("LocalCodeIDE"));

    ensureAppLoggingReady();
    installQtLogging();
    std::set_terminate(terminateHandler);
#ifdef _WIN32
    SetUnhandledExceptionFilter(unhandledExceptionFilter);
#else
    installFatalSignalHandlers();
#endif

    try {
        return runApplication(argc, argv);
    } catch (const std::exception& ex) {
        const QString line = QStringLiteral("%1 [FATAL] Unhandled std::exception at app boundary: %2")
                                 .arg(utcTimestamp(), ex.what());
        appendAppLogLine(line);
        const QByteArray bytes = (line + QStringLiteral("\n")).toUtf8();
        std::fwrite(bytes.constData(), 1, static_cast<size_t>(bytes.size()), stderr);
        std::fflush(stderr);
    } catch (...) {
        const QString line = QStringLiteral("%1 [FATAL] Unhandled non-std exception at app boundary.")
                                 .arg(utcTimestamp());
        appendAppLogLine(line);
        const QByteArray bytes = (line + QStringLiteral("\n")).toUtf8();
        std::fwrite(bytes.constData(), 1, static_cast<size_t>(bytes.size()), stderr);
        std::fflush(stderr);
    }
    return EXIT_FAILURE;
}
