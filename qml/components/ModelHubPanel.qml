import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Label {
            text: "MODEL HUB"
            color: "#d4d4d4"
            font.bold: true
        }

        Label {
            text: modelHubViewModel.providerName
            color: "#9cdcfe"
        }

        Rectangle {
            Layout.fillWidth: true
            color: "#1e1e1e"
            radius: 6
            border.color: "#2d2d30"
            implicitHeight: hardwareColumn.implicitHeight + 16

            ColumnLayout {
                id: hardwareColumn
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "LOCAL HARDWARE"; color: "#d4d4d4"; font.bold: true }
                    Item { Layout.fillWidth: true }
                    Button { text: "Refresh"; onClicked: modelHubViewModel.refreshHardware() }
                    Button { text: "Use auto profile"; onClicked: modelHubViewModel.applyDetectedProfile() }
                }

                Label {
                    text: modelHubViewModel.hardwareSummary
                    color: "#c5c5c5"
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }

                Label {
                    text: modelHubViewModel.recommendationSummary
                    color: "#6a9955"
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            color: "#1e1e1e"
            radius: 6
            border.color: "#2d2d30"
            implicitHeight: serverColumn.implicitHeight + 16

            ColumnLayout {
                id: serverColumn
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "LOCAL LLAMA SERVER"; color: "#d4d4d4"; font.bold: true }
                    Item { Layout.fillWidth: true }
                    Button {
                        text: modelHubViewModel.serverRunning || modelHubViewModel.serverStarting ? "Stop" : "Start"
                        enabled: modelHubViewModel.serverRunning || modelHubViewModel.serverStarting || modelHubViewModel.canStartServer
                        onClicked: {
                            if (modelHubViewModel.serverRunning || modelHubViewModel.serverStarting) {
                                modelHubViewModel.stopServer()
                            } else {
                                modelHubViewModel.startServer()
                            }
                        }
                    }
                    Button { text: "Probe"; onClicked: modelHubViewModel.probeServer() }
                    Button { text: "Clear logs"; onClicked: modelHubViewModel.clearServerLogs() }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: modelHubViewModel.serverHealthy ? "online" : (modelHubViewModel.serverRunning || modelHubViewModel.serverStarting ? "starting" : "offline")
                        color: modelHubViewModel.serverHealthy ? "#6a9955" : ((modelHubViewModel.serverRunning || modelHubViewModel.serverStarting) ? "#cca700" : "#808080")
                        font.bold: true
                    }

                    Label {
                        text: modelHubViewModel.serverStatusLine
                        color: "#c5c5c5"
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    TextField {
                        Layout.preferredWidth: 170
                        text: modelHubViewModel.serverExecutablePath
                        placeholderText: "llama-server"
                        color: "#d4d4d4"
                        background: Rectangle { color: "#181818"; radius: 4; border.color: "#3c3c3c" }
                        onTextChanged: modelHubViewModel.serverExecutablePath = text
                    }

                    TextField {
                        Layout.preferredWidth: 180
                        text: modelHubViewModel.serverBaseUrl
                        placeholderText: "http://127.0.0.1:8080"
                        color: "#d4d4d4"
                        background: Rectangle { color: "#181818"; radius: 4; border.color: "#3c3c3c" }
                        onTextChanged: modelHubViewModel.serverBaseUrl = text
                    }

                    SpinBox {
                        from: 1024
                        to: 131072
                        stepSize: 1024
                        editable: true
                        value: modelHubViewModel.runtimeContextSize
                        onValueModified: modelHubViewModel.runtimeContextSize = value
                    }

                    TextField {
                        Layout.fillWidth: true
                        text: modelHubViewModel.serverExtraArguments
                        placeholderText: "--n-gpu-layers 999 --threads 8"
                        color: "#d4d4d4"
                        background: Rectangle { color: "#181818"; radius: 4; border.color: "#3c3c3c" }
                        onTextChanged: modelHubViewModel.serverExtraArguments = text
                    }
                }

                Label {
                    text: modelHubViewModel.currentLocalLaunchCommand
                    color: "#6a9955"
                    wrapMode: Text.WrapAnywhere
                    Layout.fillWidth: true
                }

                Label {
                    text: "ctx change takes effect on next server start"
                    color: "#808080"
                    wrapMode: Text.WrapAnywhere
                    Layout.fillWidth: true
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100

                    TextArea {
                        readOnly: true
                        text: modelHubViewModel.serverLogs
                        color: "#d4d4d4"
                        font.family: "monospace"
                        wrapMode: TextArea.WrapAnywhere
                        background: Rectangle { color: "#111111"; radius: 4 }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            TextField {
                Layout.preferredWidth: 140
                text: modelHubViewModel.author
                placeholderText: "author"
                color: "#d4d4d4"
                background: Rectangle { color: "#1e1e1e"; radius: 4; border.color: "#3c3c3c" }
                onTextChanged: modelHubViewModel.author = text
            }

            TextField {
                Layout.fillWidth: true
                text: modelHubViewModel.searchQuery
                placeholderText: "buscar repo GGUF (coder, qwen, phi, mistral...)"
                color: "#d4d4d4"
                background: Rectangle { color: "#1e1e1e"; radius: 4; border.color: "#3c3c3c" }
                onTextChanged: modelHubViewModel.searchQuery = text
                onAccepted: modelHubViewModel.searchRepos()
            }

            Button {
                text: "Search"
                onClicked: modelHubViewModel.searchRepos()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ComboBox {
                Layout.preferredWidth: 150
                model: ["balanced", "laptop", "quality", "coding"]
                currentIndex: Math.max(0, model.indexOf(modelHubViewModel.recommendationProfile))
                onActivated: modelHubViewModel.recommendationProfile = currentText
            }

            Label {
                text: "auto: " + modelHubViewModel.autoDetectedProfile
                color: "#808080"
            }

            Button {
                text: "Suggest"
                onClicked: modelHubViewModel.suggestFile()
            }

            Label {
                text: modelHubViewModel.helperText
                color: "#808080"
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        }

        Label {
            text: modelHubViewModel.statusMessage
            color: "#c5c5c5"
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            Rectangle {
                SplitView.preferredWidth: 260
                color: "#1e1e1e"
                border.color: "#2d2d30"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 6

                    Label {
                        text: "REPOSITORIES (" + modelHubViewModel.repoCount + ")"
                        color: "#d4d4d4"
                        font.bold: true
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: modelHubViewModel.reposModel

                        delegate: Rectangle {
                            width: ListView.view.width
                            height: 74
                            color: mouseArea.containsMouse ? "#2a2d2e" : (repoId === modelHubViewModel.selectedRepoId ? "#262b33" : "transparent")
                            border.color: repoId === modelHubViewModel.selectedRepoId ? "#007acc" : "transparent"
                            radius: 4

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 2

                                Label { text: repoId; color: "#9cdcfe"; Layout.fillWidth: true; elide: Text.ElideMiddle }
                                Label { text: (pipelineTag || "unknown") + " · ↓" + downloads + " · ♥" + likes; color: "#808080"; Layout.fillWidth: true; elide: Text.ElideRight }
                                Label { text: lastModified; color: "#6a9955"; Layout.fillWidth: true; elide: Text.ElideRight }
                            }

                            MouseArea {
                                id: mouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: modelHubViewModel.selectRepo(repoId)
                            }
                        }
                    }
                }
            }

            Rectangle {
                SplitView.fillWidth: true
                color: "#181818"
                border.color: "#2d2d30"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8

                    Label {
                        text: "GGUF FILES (" + modelHubViewModel.fileCount + ")"
                        color: "#d4d4d4"
                        font.bold: true
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: modelHubViewModel.filesModel

                        delegate: Rectangle {
                            width: ListView.view.width
                            height: 62
                            color: mouseArea2.containsMouse ? "#2a2d2e" : (path === modelHubViewModel.selectedFilePath ? "#262b33" : "transparent")
                            border.color: activeLocal ? "#6a9955" : (recommended ? "#cca700" : (path === modelHubViewModel.selectedFilePath ? "#007acc" : "transparent"))
                            radius: 4

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 10

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    Label { text: name; color: "#d4d4d4"; Layout.fillWidth: true; elide: Text.ElideMiddle }
                                    Label {
                                        text: quantization + " · " + sizeLabel + (isSplit ? " · split" : "") + (activeLocal ? " · active" : "")
                                        color: activeLocal ? "#6a9955" : (recommended ? "#ffd866" : "#808080")
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }
                                }

                                Button {
                                    text: activeLocal ? "Active" : (recommended ? "Suggested" : "Select")
                                    onClicked: modelHubViewModel.selectedFilePath = path
                                }
                            }

                            MouseArea {
                                id: mouseArea2
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: modelHubViewModel.selectedFilePath = path
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: "#2d2d30"
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        TextField {
                            Layout.fillWidth: true
                            text: modelHubViewModel.targetDownloadDir
                            color: "#d4d4d4"
                            background: Rectangle { color: "#1e1e1e"; radius: 4; border.color: "#3c3c3c" }
                            onTextChanged: modelHubViewModel.targetDownloadDir = text
                        }

                        Button {
                            text: "Download selected"
                            enabled: !modelHubViewModel.downloadActive
                            onClicked: modelHubViewModel.startDownloadSelected()
                        }

                        Button {
                            text: "Download suggested"
                            enabled: !modelHubViewModel.downloadActive
                            onClicked: modelHubViewModel.downloadSuggested()
                        }

                        Button {
                            text: "Cancel"
                            enabled: modelHubViewModel.downloadActive
                            onClicked: modelHubViewModel.cancelDownload()
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Button {
                            text: "Use selected as active"
                            enabled: modelHubViewModel.canUseSelectedAsCurrent
                            onClicked: modelHubViewModel.useSelectedAsCurrent()
                        }
                        Button {
                            text: "Use downloaded as active"
                            enabled: modelHubViewModel.downloadedPath.length > 0
                            onClicked: modelHubViewModel.useDownloadedAsCurrent()
                        }
                        Label {
                            text: modelHubViewModel.currentLocalModelSummary
                            color: modelHubViewModel.hasCurrentLocalModel ? "#6a9955" : "#808080"
                            wrapMode: Text.WrapAnywhere
                            Layout.fillWidth: true
                        }
                    }

                    ProgressBar {
                        Layout.fillWidth: true
                        from: 0
                        to: 1
                        value: modelHubViewModel.downloadProgress
                        indeterminate: modelHubViewModel.downloadActive && modelHubViewModel.downloadProgress <= 0
                    }

                    Label {
                        text: modelHubViewModel.downloadStatus
                        color: "#c5c5c5"
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                    }

                    Label {
                        text: modelHubViewModel.currentLocalLaunchCommand
                        color: "#6a9955"
                        Layout.fillWidth: true
                        wrapMode: Text.WrapAnywhere
                    }
                }
            }
        }
    }
}
