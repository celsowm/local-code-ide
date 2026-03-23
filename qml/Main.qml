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
    color: WorkbenchTheme.windowBackground

    Shortcut {
        sequence: StandardKey.Find
        onActivated: mainViewModel.primaryViewIndex = 1
    }
    Shortcut {
        sequence: "Ctrl+P"
        onActivated: quickOpen.open()
    }
    Shortcut {
        sequence: StandardKey.Open
        onActivated: mainViewModel.triggerOpenFileDialog()
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
            Action { text: "Open File..."; icon.source: IconRegistry.source("file"); onTriggered: mainViewModel.triggerOpenFileDialog() }
            Action { text: "Open Folder..."; icon.source: IconRegistry.source("folder"); onTriggered: mainViewModel.triggerOpenFolderDialog() }
            MenuSeparator { }
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
            Action { text: "Complete"; icon.source: IconRegistry.source("complete"); enabled: !mainViewModel.codeIntelBusy; onTriggered: mainViewModel.requestCompletionsAtCursor() }
            Action { text: "Hover"; icon.source: IconRegistry.source("hover"); enabled: !mainViewModel.codeIntelBusy; onTriggered: mainViewModel.requestHoverAtCursor() }
            Action {
                text: "Definition"
                icon.source: IconRegistry.source("definition")
                enabled: mainViewModel.definitionAvailable && !mainViewModel.codeIntelBusy
                onTriggered: mainViewModel.requestDefinitionAtCursor()
            }
            Action { text: "Extract patch preview"; icon.source: IconRegistry.source("diff"); onTriggered: mainViewModel.extractPatchPreview() }
            Action { text: "Open patch diff"; icon.source: IconRegistry.source("diff"); onTriggered: mainViewModel.openPatchPreviewDiff() }
            Action { text: "Apply assistant patch"; icon.source: IconRegistry.source("apply_patch"); onTriggered: mainViewModel.applyAssistantPatch() }
        }
        Menu {
            title: "Git"
            Action { text: "Refresh"; icon.source: IconRegistry.source("refresh"); enabled: !mainViewModel.gitBusy; onTriggered: mainViewModel.refreshGitChanges() }
            Action { text: "Commit"; icon.source: IconRegistry.source("apply_patch"); enabled: !mainViewModel.gitBusy; onTriggered: mainViewModel.commitGitChanges() }
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
        color: WorkbenchTheme.windowBackground

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

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
                        BottomPanel {
                            id: bottomPanel
                            SplitView.fillWidth: true
                            SplitView.preferredHeight: mainViewModel.bottomPanelHeight
                            SplitView.minimumHeight: WorkbenchTheme.minBottomPanelHeight
                            SplitView.maximumHeight: WorkbenchTheme.maxBottomPanelHeight
                            onHeightChanged: {
                                if (height > 0) {
                                    mainViewModel.bottomPanelHeight = Math.round(height)
                                }
                            }
                        }
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
                color: WorkbenchTheme.accent
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    Label { text: mainViewModel.statusMessage; color: "white"; elide: Text.ElideRight; Layout.fillWidth: true }
                    Label { text: mainViewModel.openEditorCount + " tabs"; color: "white" }
                    Label { text: mainViewModel.splitEditorVisible ? (mainViewModel.diffEditorVisible ? "diff" : "split") : "single"; color: "white" }
                    Label { text: mainViewModel.diagnosticsStatusLine; color: "white"; elide: Text.ElideRight }
                    Label { text: mainViewModel.gitChangeCount + " changes"; color: "white" }
                    Label { text: modelHubViewModel.hardwareSummary; color: "white"; elide: Text.ElideRight }
                    Label { text: mainViewModel.aiBackendName + " · " + modelHubViewModel.providerName; color: "white" }
                }
            }
        }

        Rectangle {
            anchors.fill: parent
            visible: mainViewModel.workspaceLoading
            color: "#000000"
            opacity: 0.38
            z: 900

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.AllButtons
                preventStealing: true
            }
        }

        Rectangle {
            visible: mainViewModel.workspaceLoading
            width: Math.min(parent.width * 0.6, 520)
            height: 132
            anchors.centerIn: parent
            radius: 8
            color: "#252526"
            border.color: "#4a4f57"
            z: 901

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 10

                Label {
                    text: "Opening folder"
                    color: "#f2f2f2"
                    font.pixelSize: 16
                    font.bold: true
                    Layout.fillWidth: true
                }

                Label {
                    text: mainViewModel.workspaceLoadingText.length > 0 ? mainViewModel.workspaceLoadingText : "Indexing workspace files..."
                    color: "#cccccc"
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                BusyIndicator {
                    running: mainViewModel.workspaceLoading
                    visible: running
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        Rectangle {
            id: toast
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: 16
            anchors.bottomMargin: 38
            visible: mainViewModel.toastVisible
            color: "#2f3136"
            border.color: "#5a5f69"
            radius: 6
            opacity: visible ? 0.96 : 0
            z: 1000
            implicitWidth: Math.min(parent.width * 0.45, toastLabel.implicitWidth + 28)
            implicitHeight: toastLabel.implicitHeight + 18

            Label {
                id: toastLabel
                anchors.fill: parent
                anchors.margins: 10
                text: mainViewModel.toastMessage
                color: "#f2f2f2"
                wrapMode: Text.Wrap
                maximumLineCount: 3
                elide: Text.ElideRight
            }
        }
    }

    Connections {
        target: mainViewModel
        function onQuickOpenRequested() { quickOpen.open() }
    }

    CommandPaletteDialog { id: commandPalette }
    QuickOpenDialog { id: quickOpen }

    Timer {
        id: toastTimer
        interval: 3600
        repeat: false
        onTriggered: mainViewModel.dismissToast()
    }

    Connections {
        target: mainViewModel
        function onToastChanged() {
            if (mainViewModel.toastVisible) {
                toastTimer.restart()
            } else {
                toastTimer.stop()
            }
        }
    }
}
