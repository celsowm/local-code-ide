import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LocalCodeIDE 1.0

ApplicationWindow {
    id: window
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

    menuBar: MenuBar {
        Menu {
            title: "File"
            Action { text: "Quick Open"; onTriggered: quickOpen.open() }
            Action { text: "Load C++ Sample"; onTriggered: mainViewModel.loadSampleCpp() }
            Action { text: "Load Rust Sample"; onTriggered: mainViewModel.loadSampleRust() }
            Action { text: "Save"; onTriggered: mainViewModel.saveCurrent() }
        }
        Menu {
            title: "View"
            Action { text: "Command Palette"; onTriggered: commandPalette.open() }
            Action { text: "Split Editor Right"; onTriggered: mainViewModel.openWorkspaceFileInSplit(mainViewModel.currentPath) }
            Action { text: "Close Split"; onTriggered: mainViewModel.closeSplitEditor() }
            Action { text: "Explorer"; onTriggered: mainViewModel.primaryViewIndex = 0 }
            Action { text: "Search"; onTriggered: mainViewModel.primaryViewIndex = 1 }
            Action { text: "Source Control"; onTriggered: mainViewModel.primaryViewIndex = 2 }
            Action { text: "Assistant"; onTriggered: mainViewModel.primaryViewIndex = 3 }
        }
        Menu {
            title: "Code"
            Action { text: "Analyze"; onTriggered: mainViewModel.analyzeNow() }
            Action { text: "Complete"; onTriggered: mainViewModel.requestCompletionsAtCursor() }
            Action { text: "Hover"; onTriggered: mainViewModel.requestHoverAtCursor() }
            Action { text: "Definition"; onTriggered: mainViewModel.requestDefinitionAtCursor() }
            Action { text: "Extract patch preview"; onTriggered: mainViewModel.extractPatchPreview() }
            Action { text: "Open patch diff"; onTriggered: mainViewModel.openPatchPreviewDiff() }
            Action { text: "Apply assistant patch"; onTriggered: mainViewModel.applyAssistantPatch() }
        }
        Menu {
            title: "Git"
            Action { text: "Refresh"; onTriggered: mainViewModel.refreshGitChanges() }
            Action { text: "Commit"; onTriggered: mainViewModel.commitGitChanges() }
        }
        Menu {
            title: "Models"
            Action { text: "Refresh hardware"; onTriggered: modelHubViewModel.refreshHardware() }
            Action { text: "Apply auto profile"; onTriggered: modelHubViewModel.applyDetectedProfile() }
            Action { text: "Use downloaded as active"; onTriggered: modelHubViewModel.useDownloadedAsCurrent() }
            Action { text: "Start local server"; onTriggered: modelHubViewModel.startServer() }
            Action { text: "Stop local server"; onTriggered: modelHubViewModel.stopServer() }
            Action { text: "Probe local server"; onTriggered: modelHubViewModel.probeServer() }
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
                    Button { text: "Scan"; onClicked: mainViewModel.refreshWorkspace() }
                    Item { Layout.fillWidth: true }
                    Label { text: mainViewModel.gitSummary; color: "#9cdcfe"; elide: Text.ElideRight }
                }
            }

            SplitView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                orientation: Qt.Horizontal

                Sidebar { SplitView.preferredWidth: 52 }
                LeftPanel { SplitView.preferredWidth: 320; SplitView.minimumWidth: 260 }

                SplitView {
                    SplitView.fillWidth: true
                    SplitView.fillHeight: true
                    orientation: Qt.Vertical

                    SplitView {
                        SplitView.fillWidth: true
                        SplitView.fillHeight: true
                        orientation: Qt.Horizontal
                        EditorPane { SplitView.fillWidth: true; SplitView.fillHeight: true }
                        ChatPanel { SplitView.preferredWidth: 390; SplitView.minimumWidth: 320 }
                    }

                    BottomPanel { SplitView.fillWidth: true; SplitView.preferredHeight: 280; SplitView.minimumHeight: 220 }
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
