import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components/IconRegistry.js" as IconRegistry

ApplicationWindow {
    id: window
    required property var mainViewModel
    required property var modelHubViewModel
    visible: true
    width: 1540
    height: 940
    title: "LocalCodeIDE v3.3 Split + Diff"
    color: "#1e1e1e"

    Shortcut {
        sequence: StandardKey.Find
        onActivated: mainViewModel.primaryViewIndex = 1
    }
    Shortcut {
        sequence: "Ctrl+P"
        onActivated: quickOpen.open()
    }
    Shortcut {
        sequence: "Ctrl+Shift+P"
        onActivated: commandPalette.open()
    }
    Shortcut {
        sequence: "Ctrl+\\"
        onActivated: mainViewModel.openWorkspaceFileInSplit(mainViewModel.currentPath)
    }
    Shortcut {
        sequence: "Ctrl+Alt+A"
        onActivated: mainViewModel.toggleSecondaryAiSidebar()
    }

    menuBar: MenuBar {
        Menu {
            title: "File"
            Action { text: "Quick Open"; icon.source: IconRegistry.source("search"); onTriggered: quickOpen.open() }
            Action { text: "Load C++ Sample"; icon.source: IconRegistry.source("explorer"); onTriggered: mainViewModel.loadSampleCpp() }
            Action { text: "Load Rust Sample"; icon.source: IconRegistry.source("explorer"); onTriggered: mainViewModel.loadSampleRust() }
            Action { text: "Save"; icon.source: IconRegistry.source("apply_patch"); onTriggered: mainViewModel.saveCurrent() }
        }
        Menu {
            title: "View"
            Action { text: "Command Palette"; icon.source: IconRegistry.source("suggest"); onTriggered: commandPalette.open() }
            Action { text: "Split Editor Right"; icon.source: IconRegistry.source("split"); onTriggered: mainViewModel.openWorkspaceFileInSplit(mainViewModel.currentPath) }
            Action { text: "Close Split"; icon.source: IconRegistry.source("split"); onTriggered: mainViewModel.closeSplitEditor() }
            Action { text: "Explorer"; icon.source: IconRegistry.source("explorer"); onTriggered: mainViewModel.primaryViewIndex = 0 }
            Action { text: "Search"; icon.source: IconRegistry.source("search"); onTriggered: mainViewModel.primaryViewIndex = 1 }
            Action { text: "Source Control"; icon.source: IconRegistry.source("git"); onTriggered: mainViewModel.primaryViewIndex = 2 }
            MenuSeparator { }
            Action { text: "Toggle AI Sidebar"; icon.source: IconRegistry.source("assistant"); onTriggered: mainViewModel.toggleSecondaryAiSidebar() }
            Action { text: "Show Assistant"; icon.source: IconRegistry.source("assistant"); onTriggered: mainViewModel.showAssistantSidebar() }
            Action { text: "Show Models"; icon.source: IconRegistry.source("assistant"); onTriggered: mainViewModel.showModelsSidebar() }
        }
        Menu {
            title: "Code"
            Action { text: "Analyze"; icon.source: IconRegistry.source("analyze"); onTriggered: mainViewModel.analyzeNow() }
            Action { text: "Complete"; icon.source: IconRegistry.source("complete"); onTriggered: mainViewModel.requestCompletionsAtCursor() }
            Action { text: "Hover"; icon.source: IconRegistry.source("hover"); onTriggered: mainViewModel.requestHoverAtCursor() }
            Action { text: "Definition"; icon.source: IconRegistry.source("definition"); onTriggered: mainViewModel.requestDefinitionAtCursor() }
            Action { text: "Extract patch preview"; icon.source: IconRegistry.source("diff"); onTriggered: mainViewModel.extractPatchPreview() }
            Action { text: "Open patch diff"; icon.source: IconRegistry.source("diff"); onTriggered: mainViewModel.openPatchPreviewDiff() }
            Action { text: "Apply assistant patch"; icon.source: IconRegistry.source("apply_patch"); onTriggered: mainViewModel.applyAssistantPatch() }
        }
        Menu {
            title: "Git"
            Action { text: "Refresh"; icon.source: IconRegistry.source("refresh"); onTriggered: mainViewModel.refreshGitChanges() }
            Action { text: "Commit"; icon.source: IconRegistry.source("apply_patch"); onTriggered: mainViewModel.commitGitChanges() }
        }
        Menu {
            title: "Models"
            Action { text: "Refresh hardware"; icon.source: IconRegistry.source("refresh"); onTriggered: modelHubViewModel.refreshHardware() }
            Action { text: "Apply auto profile"; icon.source: IconRegistry.source("auto_profile"); onTriggered: modelHubViewModel.applyDetectedProfile() }
            Action { text: "Use downloaded as active"; icon.source: IconRegistry.source("use_active"); onTriggered: modelHubViewModel.useDownloadedAsCurrent() }
            Action { text: "Start local server"; icon.source: IconRegistry.source("start"); onTriggered: modelHubViewModel.startServer() }
            Action { text: "Stop local server"; icon.source: IconRegistry.source("stop"); onTriggered: modelHubViewModel.stopServer() }
            Action { text: "Probe local server"; icon.source: IconRegistry.source("probe"); onTriggered: modelHubViewModel.probeServer() }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#1e1e1e"

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                color: "#2d2d30"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 12
                    Label { text: "WORKSPACE"; color: "#c5c5c5"; font.bold: true }
                    TextField {
                        Layout.preferredWidth: 420
                        text: mainViewModel.workspaceRootPath
                        color: "#d4d4d4"
                        selectByMouse: true
                        background: Rectangle { color: "#1e1e1e"; radius: 4; border.color: "#3c3c3c" }
                        onAccepted: mainViewModel.workspaceRootPath = text
                    }
                    IconButton { text: "Scan"; iconName: "scan"; onClicked: mainViewModel.refreshWorkspace() }
                    IconButton {
                        text: mainViewModel.secondaryAiVisible ? "Hide AI" : "Show AI"
                        iconName: "assistant"
                        onClicked: mainViewModel.toggleSecondaryAiSidebar()
                    }
                    Item { Layout.fillWidth: true }
                    Label { text: mainViewModel.gitSummary; color: "#9cdcfe"; elide: Text.ElideRight }
                }
            }

            SplitView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                orientation: Qt.Horizontal

                Sidebar { viewModel: mainViewModel; SplitView.preferredWidth: 52 }
                LeftPanel { viewModel: mainViewModel; SplitView.preferredWidth: 320; SplitView.minimumWidth: 260 }

                SplitView {
                    SplitView.fillWidth: true
                    SplitView.fillHeight: true
                    orientation: Qt.Horizontal

                    SplitView {
                        SplitView.fillWidth: true
                        SplitView.fillHeight: true
                        orientation: Qt.Vertical
                        EditorPane { SplitView.fillWidth: true; SplitView.fillHeight: true }
                        BottomPanel { SplitView.fillWidth: true; SplitView.preferredHeight: 280; SplitView.minimumHeight: 220 }
                    }

                    ChatPanel {
                        viewModel: mainViewModel
                        visible: mainViewModel.secondaryAiVisible
                        SplitView.preferredWidth: mainViewModel.secondaryAiVisible ? mainViewModel.secondaryAiWidth : 0
                        SplitView.minimumWidth: mainViewModel.secondaryAiVisible ? 320 : 0
                        SplitView.maximumWidth: mainViewModel.secondaryAiVisible ? 720 : 0
                        SplitView.fillHeight: true
                        onWidthChanged: {
                            if (visible && width > 0) {
                                mainViewModel.secondaryAiWidth = width
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 26
                color: "#007acc"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    Label { text: mainViewModel.statusMessage; color: "white"; elide: Text.ElideRight; Layout.fillWidth: true }
                    Label { text: mainViewModel.openEditorCount + " tabs"; color: "white" }
                    Label { text: mainViewModel.splitEditorVisible ? (mainViewModel.diffEditorVisible ? "diff" : "split") : "single"; color: "white" }
                    Label { text: mainViewModel.gitChangeCount + " changes"; color: "white" }
                    Label { text: modelHubViewModel.hardwareSummary; color: "white"; elide: Text.ElideRight }
                    Label { text: mainViewModel.aiBackendName + " · " + modelHubViewModel.providerName; color: "white" }
                }
            }
        }
    }

    CommandPaletteDialog { id: commandPalette }
    QuickOpenDialog { id: quickOpen }
}
