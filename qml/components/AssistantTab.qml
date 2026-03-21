import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Label {
            text: "LOCAL ASSISTANT"
            color: "#d4d4d4"
            font.bold: true
        }

        Label {
            text: mainViewModel.aiBackendName
            color: "#9cdcfe"
        }

        Label {
            text: mainViewModel.aiBackendStatusLine
            color: (modelHubViewModel.serverHealthy ? "#6a9955" : ((modelHubViewModel.serverRunning || modelHubViewModel.serverStarting) ? "#cca700" : "#808080"))
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        Rectangle {
            Layout.fillWidth: true
            color: "#1e1e1e"
            radius: 6
            border.color: "#2d2d30"
            implicitHeight: runtimeColumn.implicitHeight + 16

            ColumnLayout {
                id: runtimeColumn
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                Label {
                    text: "RUNTIME LINK"
                    color: "#d4d4d4"
                    font.bold: true
                }

                Label {
                    text: modelHubViewModel.currentLocalModelSummary
                    color: modelHubViewModel.hasCurrentLocalModel ? "#6a9955" : "#808080"
                    wrapMode: Text.WrapAnywhere
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label { text: "ctx"; color: "#c5c5c5" }
                    SpinBox {
                        from: 1024
                        to: 131072
                        stepSize: 1024
                        editable: true
                        value: modelHubViewModel.runtimeContextSize
                        onValueModified: modelHubViewModel.runtimeContextSize = value
                    }

                    Label { text: "server"; color: "#c5c5c5" }
                    TextField {
                        Layout.fillWidth: true
                        text: modelHubViewModel.serverBaseUrl
                        color: "#d4d4d4"
                        background: Rectangle { color: "#181818"; radius: 4; border.color: "#3c3c3c" }
                        onTextChanged: modelHubViewModel.serverBaseUrl = text
                    }

                    Button {
                        text: modelHubViewModel.serverRunning || modelHubViewModel.serverStarting ? "Stop" : "Start"
                        enabled: modelHubViewModel.serverRunning || modelHubViewModel.serverStarting || modelHubViewModel.canStartServer
                        onClicked: {
                            if (modelHubViewModel.serverRunning || modelHubViewModel.serverStarting)
                                modelHubViewModel.stopServer()
                            else
                                modelHubViewModel.startServer()
                        }
                    }
                }

                Label {
                    text: modelHubViewModel.currentLocalLaunchCommand
                    color: "#808080"
                    wrapMode: Text.WrapAnywhere
                    Layout.fillWidth: true
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            color: "#1e1e1e"
            radius: 6
            border.color: "#2d2d30"
            implicitHeight: settingsColumn.implicitHeight + 16

            ColumnLayout {
                id: settingsColumn
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                Label {
                    text: "CTX CONTROL"
                    color: "#d4d4d4"
                    font.bold: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    CheckBox {
                        text: "document"
                        checked: mainViewModel.aiIncludeDocument
                        onToggled: mainViewModel.aiIncludeDocument = checked
                    }
                    CheckBox {
                        text: "diagnostics"
                        checked: mainViewModel.aiIncludeDiagnostics
                        onToggled: mainViewModel.aiIncludeDiagnostics = checked
                    }
                    CheckBox {
                        text: "git"
                        checked: mainViewModel.aiIncludeGitSummary
                        onToggled: mainViewModel.aiIncludeGitSummary = checked
                    }
                    CheckBox {
                        text: "workspace"
                        checked: mainViewModel.aiIncludeWorkspaceContext
                        onToggled: mainViewModel.aiIncludeWorkspaceContext = checked
                    }
                    Item { Layout.fillWidth: true }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label { text: "doc chars"; color: "#c5c5c5" }
                    SpinBox {
                        from: 512
                        to: 200000
                        stepSize: 512
                        editable: true
                        value: mainViewModel.aiDocumentContextChars
                        onValueModified: mainViewModel.aiDocumentContextChars = value
                    }

                    Label { text: "ws chars"; color: "#c5c5c5" }
                    SpinBox {
                        from: 0
                        to: 400000
                        stepSize: 512
                        editable: true
                        value: mainViewModel.aiWorkspaceContextChars
                        onValueModified: mainViewModel.aiWorkspaceContextChars = value
                    }

                    Label { text: "ws files"; color: "#c5c5c5" }
                    SpinBox {
                        from: 0
                        to: 24
                        editable: true
                        value: mainViewModel.aiWorkspaceContextMaxFiles
                        onValueModified: mainViewModel.aiWorkspaceContextMaxFiles = value
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label { text: "max tokens"; color: "#c5c5c5" }
                    SpinBox {
                        from: 64
                        to: 8192
                        stepSize: 64
                        editable: true
                        value: mainViewModel.aiMaxOutputTokens
                        onValueModified: mainViewModel.aiMaxOutputTokens = value
                    }

                    Label { text: "temp"; color: "#c5c5c5" }
                    TextField {
                        Layout.preferredWidth: 70
                        text: Number(mainViewModel.aiTemperature).toFixed(2)
                        color: "#d4d4d4"
                        background: Rectangle { color: "#181818"; radius: 4; border.color: "#3c3c3c" }
                        onEditingFinished: mainViewModel.aiTemperature = parseFloat(text)
                    }

                    Button { text: "Refresh context"; onClicked: mainViewModel.refreshRelevantContext() }
                    Button { text: "Extract patch"; onClicked: mainViewModel.extractPatchPreview() }
                    Button {
                        text: "Apply patch"
                        enabled: mainViewModel.canApplyPatchPreview
                        onClicked: mainViewModel.applyAssistantPatch()
                    }
                }

                TextArea {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 72
                    text: mainViewModel.aiSystemPrompt
                    color: "#d4d4d4"
                    wrapMode: TextArea.Wrap
                    placeholderText: "System prompt for the local assistant"
                    background: Rectangle { color: "#181818"; radius: 4; border.color: "#3c3c3c" }
                    onTextChanged: mainViewModel.aiSystemPrompt = text
                }

                Label {
                    text: mainViewModel.aiContextSummary
                    color: "#808080"
                    wrapMode: Text.WrapAnywhere
                    Layout.fillWidth: true
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            color: "#1e1e1e"
            radius: 6
            border.color: "#2d2d30"
            implicitHeight: toolsColumn.implicitHeight + 16

            ColumnLayout {
                id: toolsColumn
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                Label {
                    text: "TOOLS"
                    color: "#d4d4d4"
                    font.bold: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    CheckBox {
                        text: "enable tool calling"
                        checked: mainViewModel.aiEnableTools
                        onToggled: mainViewModel.aiEnableTools = checked
                    }

                    CheckBox {
                        text: "confirm destructive"
                        checked: mainViewModel.aiRequireApprovalForDestructive
                        onToggled: mainViewModel.aiRequireApprovalForDestructive = checked
                    }

                    Label { text: "rounds"; color: "#c5c5c5" }
                    SpinBox {
                        from: 0
                        to: 12
                        editable: true
                        value: mainViewModel.aiMaxToolRounds
                        onValueModified: mainViewModel.aiMaxToolRounds = value
                    }

                    Label {
                        text: "calls: " + mainViewModel.toolCallCount
                        color: mainViewModel.toolCallCount > 0 ? "#6a9955" : "#808080"
                    }

                    Item { Layout.fillWidth: true }
                }

                Label {
                    text: mainViewModel.toolCatalogSummary
                    color: "#808080"
                    wrapMode: Text.WrapAnywhere
                    Layout.fillWidth: true
                }

                Label {
                    text: "pending: " + mainViewModel.pendingApprovalCount + " · " + mainViewModel.pendingApprovalSummary
                    color: mainViewModel.pendingApprovalCount > 0 ? "#d7ba7d" : "#808080"
                    wrapMode: Text.WrapAnywhere
                    Layout.fillWidth: true
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 96

                    TextArea {
                        readOnly: true
                        text: mainViewModel.toolCallLog
                        color: "#d4d4d4"
                        placeholderText: "Tool execution log will appear here."
                        font.family: "monospace"
                        wrapMode: TextArea.WrapAnywhere
                        background: Rectangle { color: "#181818"; radius: 4; border.color: "#3c3c3c" }
                    }
                }
            }
        }


        Rectangle {
            Layout.fillWidth: true
            color: "#1e1e1e"
            radius: 6
            border.color: mainViewModel.pendingApprovalCount > 0 ? "#d7ba7d" : "#2d2d30"
            implicitHeight: approvalsColumn.implicitHeight + 16

            ColumnLayout {
                id: approvalsColumn
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: "PENDING APPROVALS (" + mainViewModel.pendingApprovalCount + ")"
                        color: "#d4d4d4"
                        font.bold: true
                    }
                    Item { Layout.fillWidth: true }
                    Button {
                        text: "Approve all"
                        enabled: mainViewModel.pendingApprovalCount > 0
                        onClicked: mainViewModel.approveAllPendingTools()
                    }
                    Button {
                        text: "Clear"
                        enabled: mainViewModel.pendingApprovalCount > 0
                        onClicked: mainViewModel.clearPendingTools()
                    }
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.max(84, Math.min(contentHeight, 180))
                    clip: true
                    model: mainViewModel.pendingApprovalModel

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: approvalContent.implicitHeight + 10
                        color: "#181818"
                        radius: 4
                        border.color: destructive ? "#d7ba7d" : "#3c3c3c"

                        ColumnLayout {
                            id: approvalContent
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 4

                            RowLayout {
                                Layout.fillWidth: true
                                Label {
                                    text: toolName + (destructive ? " *" : "")
                                    color: "#ce9178"
                                    font.bold: true
                                }
                                Label {
                                    text: pathHints
                                    color: "#808080"
                                    Layout.fillWidth: true
                                    elide: Text.ElideMiddle
                                }
                            }

                            Label {
                                text: summary
                                color: "#d4d4d4"
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                            }

                            TextArea {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 72
                                readOnly: true
                                text: argumentsText
                                color: "#d4d4d4"
                                font.family: "monospace"
                                wrapMode: TextArea.WrapAnywhere
                                background: Rectangle { color: "#111111"; radius: 4 }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Item { Layout.fillWidth: true }
                                Button { text: "Reject"; onClicked: mainViewModel.rejectPendingTool(approvalId) }
                                Button { text: "Approve"; onClicked: mainViewModel.approvePendingTool(approvalId) }
                            }
                        }
                    }
                }
            }
        }

        TextArea {
            Layout.fillWidth: true
            Layout.preferredHeight: 90
            text: mainViewModel.chatInput
            placeholderText: "Explique este erro, proponha refactor, gere teste, sugira diff, ou peça para ler/criar/editar/pesquisar..."
            color: "#d4d4d4"
            wrapMode: TextArea.Wrap
            background: Rectangle { color: "#1e1e1e"; radius: 6 }
            onTextChanged: mainViewModel.chatInput = text
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: "Ask"
                onClicked: mainViewModel.askAssistant()
            }

            Label {
                text: mainViewModel.relevantContextSummary
                color: "#808080"
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 140
            color: "#181818"
            radius: 6
            border.color: "#2d2d30"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                Label {
                    text: "RELEVANT WORKSPACE FILES (" + mainViewModel.relevantContextCount + ")"
                    color: "#d4d4d4"
                    font.bold: true
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: mainViewModel.relevantContextModel

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 56
                        color: mouseArea.containsMouse ? "#2a2d2e" : "transparent"

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            anchors.topMargin: 4
                            anchors.bottomMargin: 4
                            spacing: 2

                            Label {
                                text: relativePath + "  [score=" + score + "]"
                                color: "#9cdcfe"
                                Layout.fillWidth: true
                                elide: Text.ElideMiddle
                            }
                            Label {
                                text: reason
                                color: "#808080"
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                            Label {
                                text: excerpt
                                color: "#d4d4d4"
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                        }

                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: mainViewModel.openWorkspaceFile(path)
                        }
                    }
                }
            }
        }

        Label {
            text: "ASSISTANT OUTPUT"
            color: "#d4d4d4"
            font.bold: true
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Vertical

            ScrollView {
                SplitView.fillWidth: true
                SplitView.fillHeight: true

                TextArea {
                    readOnly: true
                    text: mainViewModel.chatResponse
                    color: "#d4d4d4"
                    wrapMode: TextArea.Wrap
                    background: Rectangle { color: "#1e1e1e"; radius: 6 }
                }
            }

            Rectangle {
                SplitView.fillWidth: true
                SplitView.preferredHeight: 170
                color: "#181818"
                radius: 6
                border.color: mainViewModel.canApplyPatchPreview ? "#6a9955" : "#2d2d30"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: "PATCH PREVIEW"
                            color: "#d4d4d4"
                            font.bold: true
                        }
                        Item { Layout.fillWidth: true }
                        Label {
                            text: mainViewModel.patchSummary
                            color: mainViewModel.canApplyPatchPreview ? "#6a9955" : "#808080"
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                        }
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        TextArea {
                            readOnly: true
                            text: mainViewModel.patchPreviewText
                            color: "#d4d4d4"
                            font.family: "monospace"
                            wrapMode: TextArea.NoWrap
                            background: Rectangle { color: "#111111"; radius: 4 }
                        }
                    }
                }
            }
        }
    }
}
