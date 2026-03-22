cmake_minimum_required(VERSION 3.21)
project(LocalCodeIDE VERSION 0.10.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Quick Qml QuickControls2 Network)
qt_standard_project_setup(REQUIRES 6.5)
qt_policy(SET QTP0004 NEW)

qt_add_executable(LocalCodeIDE
    src/main.cpp
    src/core/Document.cpp

    src/services/AiService.cpp
    src/services/CodeIntelService.cpp
    src/services/DiagnosticService.cpp
    src/services/DocumentService.cpp
    src/services/DiffApplyService.cpp
    src/services/GitService.cpp
    src/services/HardwareProfileService.cpp
    src/services/LlamaServerProcessService.cpp
    src/services/LocalModelRuntimeService.cpp
    src/services/ModelCatalogService.cpp
    src/services/SearchService.cpp
    src/services/TerminalService.cpp
    src/services/ToolCallingService.cpp
    src/services/WorkspaceContextService.cpp
    src/services/WorkspaceService.cpp

    src/adapters/ai/AdaptiveAiBackend.cpp
    src/adapters/ai/LlamaCppServerBackend.cpp
    src/adapters/ai/MockAiBackend.cpp

    src/adapters/codeintel/LspCodeIntelProvider.cpp
    src/adapters/codeintel/MockCodeIntelProvider.cpp

    src/adapters/diagnostics/HybridDiagnosticProvider.cpp
    src/adapters/diagnostics/LocalSyntaxDiagnosticProvider.cpp
    src/adapters/diagnostics/LspClient.cpp
    src/adapters/diagnostics/LspDiagnosticProvider.cpp
    src/adapters/diagnostics/MockDiagnosticProvider.cpp

    src/adapters/search/FileSystemSearchProvider.cpp
    src/adapters/modelhub/HuggingFaceFileDownloader.cpp
    src/adapters/modelhub/HuggingFaceModelCatalogProvider.cpp
    src/adapters/terminal/ProcessTerminalBackend.cpp
    src/adapters/workspace/FileSystemWorkspaceProvider.cpp

    src/adapters/tools/ApplyUnifiedDiffTool.cpp
    src/adapters/tools/CreateFileTool.cpp
    src/adapters/tools/EditFileTool.cpp
    src/adapters/tools/ListWorkspaceFilesTool.cpp
    src/adapters/tools/ReadFileTool.cpp
    src/adapters/tools/SearchWorkspaceTool.cpp

    src/adapters/vcs/GitCliProvider.cpp
    src/adapters/vcs/MockVersionControlProvider.cpp

    src/ui/highlighting/CppSyntaxHighlighter.cpp
    src/ui/highlighting/DocumentHighlighter.cpp

    src/ui/models/CommandPaletteListModel.cpp
    src/ui/models/CompletionListModel.cpp
    src/ui/models/DiagnosticListModel.cpp
    src/ui/models/GitChangeListModel.cpp
    src/ui/models/OpenEditorListModel.cpp
    src/ui/models/PendingToolApprovalListModel.cpp
    src/ui/models/ModelFileListModel.cpp
    src/ui/models/ModelRepoListModel.cpp
    src/ui/models/RelevantContextListModel.cpp
    src/ui/models/SearchResultListModel.cpp
    src/ui/models/WorkspaceFileListModel.cpp
    src/ui/models/WorkspaceTreeListModel.cpp

    src/ui/viewmodels/MainViewModel.cpp
    src/ui/viewmodels/ModelHubViewModel.cpp
)

qt_add_qml_module(LocalCodeIDE
    URI LocalCodeIDE
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/components/IconButton.qml
        qml/components/IconRegistry.js
        qml/components/AssistantTab.qml
        qml/components/BottomPanel.qml
        qml/components/ChatPanel.qml
        qml/components/CommandPaletteDialog.qml
        qml/components/EditorPane.qml
        qml/components/ExplorerPanel.qml
        qml/components/LeftPanel.qml
        qml/components/ModelHubPanel.qml
        qml/components/QuickOpenDialog.qml
        qml/components/SearchPanel.qml
        qml/components/Sidebar.qml
        qml/components/SourceControlPanel.qml
    RESOURCES
        qml/assets/icons/explorer.svg
        qml/assets/icons/search.svg
        qml/assets/icons/git.svg
        qml/assets/icons/assistant.svg
        qml/assets/icons/scan.svg
        qml/assets/icons/split.svg
        qml/assets/icons/diff.svg
        qml/assets/icons/analyze.svg
        qml/assets/icons/complete.svg
        qml/assets/icons/hover.svg
        qml/assets/icons/definition.svg
        qml/assets/icons/apply_patch.svg
        qml/assets/icons/refresh.svg
        qml/assets/icons/auto_profile.svg
        qml/assets/icons/start.svg
        qml/assets/icons/stop.svg
        qml/assets/icons/probe.svg
        qml/assets/icons/clear_logs.svg
        qml/assets/icons/suggest.svg
        qml/assets/icons/download.svg
        qml/assets/icons/use_active.svg
        qml/assets/icons/LICENSE.txt
)

target_include_directories(LocalCodeIDE PRIVATE src)

target_link_libraries(LocalCodeIDE PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickControls2
    Qt6::Network
)

# WiX Installer configuration (optional)
if(WIN32)
    find_program(WIX_PATH wix)
    if(WIX_PATH)
        message(STATUS "WiX Toolset found: ${WIX_PATH}")
        message(STATUS "To build installer: powershell -ExecutionPolicy Bypass -File ${CMAKE_SOURCE_DIR}/installer/build-installer.ps1")
    else()
        message(STATUS "WiX Toolset not found. To build installer, install from: https://wixtoolset.org/")
    endif()
endif()
