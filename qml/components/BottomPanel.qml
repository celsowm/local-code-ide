import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: WorkbenchTheme.panelBackground
    border.color: WorkbenchTheme.borderColor

    property int selectedTab: 0

    Component.onCompleted: selectedTab = mainViewModel.bottomPanelTab

    Connections {
        target: mainViewModel

        function onBottomPanelChanged() {
            if (root.selectedTab !== mainViewModel.bottomPanelTab) {
                root.selectedTab = mainViewModel.bottomPanelTab
            }
        }
    }

    onSelectedTabChanged: {
        if (mainViewModel.bottomPanelTab !== selectedTab) {
            mainViewModel.bottomPanelTab = selectedTab
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: WorkbenchTheme.panelHeaderHeight
            color: WorkbenchTheme.panelHeaderBackground
            border.color: WorkbenchTheme.borderColor

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 6
                anchors.rightMargin: 12
                spacing: 2

                Repeater {
                    model: [
                        { title: "PROBLEMS", count: mainViewModel.diagnosticsCount },
                        { title: "TERMINAL", count: -1 }
                    ]

                    delegate: ToolButton {
                        required property int index
                        required property var modelData

                        text: modelData.count >= 0 ? (modelData.title + " (" + modelData.count + ")") : modelData.title
                        font.pixelSize: 11
                        padding: 8
                        hoverEnabled: true
                        onClicked: root.selectedTab = index

                        background: Rectangle {
                            color: root.selectedTab === index ? WorkbenchTheme.editorTabActive : (parent.hovered ? "#2b2b2d" : "transparent")
                            border.color: root.selectedTab === index ? WorkbenchTheme.accent : "transparent"
                            radius: 0
                        }

                        contentItem: Label {
                            text: parent.text
                            color: root.selectedTab === index ? WorkbenchTheme.textPrimary : WorkbenchTheme.textMuted
                            font.pixelSize: parent.font.pixelSize
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                Label {
                    text: root.selectedTab === 0
                        ? (mainViewModel.diagnosticsCount + " diagnostics · " + mainViewModel.diagnosticsProviderName)
                        : mainViewModel.terminalBackendName
                    color: WorkbenchTheme.textMuted
                    font.pixelSize: 11
                }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.selectedTab

            Rectangle {
                color: WorkbenchTheme.editorBackground

                ListView {
                    anchors.fill: parent
                    anchors.margins: 8
                    clip: true
                    model: mainViewModel.diagnosticsModel
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 30
                        color: mouseArea.containsMouse ? "#2a2d2e" : "transparent"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 10

                            Label {
                                text: severity.toUpperCase()
                                color: severity === "error" ? WorkbenchTheme.danger : (severity === "warning" ? WorkbenchTheme.warning : WorkbenchTheme.info)
                                font.bold: true
                                font.pixelSize: 11
                                Layout.preferredWidth: 58
                            }

                            Label {
                                text: "L" + line + ":" + column
                                color: WorkbenchTheme.textMuted
                                font.pixelSize: 11
                            }

                            Label {
                                text: message
                                color: WorkbenchTheme.textPrimary
                                font.pixelSize: 11
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Label {
                                text: source
                                color: WorkbenchTheme.textMuted
                                font.pixelSize: 10
                                Layout.preferredWidth: 120
                                horizontalAlignment: Text.AlignRight
                                elide: Text.ElideLeft
                            }
                        }

                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: mainViewModel.openSearchResult(
                                           filePath && filePath.length > 0 ? filePath : mainViewModel.currentPath,
                                           line,
                                           column)
                        }
                    }
                }
            }

            Rectangle {
                color: WorkbenchTheme.terminalBackground

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        TextField {
                            Layout.fillWidth: true
                            text: mainViewModel.terminalCommand
                            color: WorkbenchTheme.textPrimary
                            placeholderText: "Enter a command"
                            onTextChanged: mainViewModel.terminalCommand = text
                            onAccepted: mainViewModel.runTerminalCommand()

                            background: Rectangle {
                                color: WorkbenchTheme.editorBackground
                                border.color: WorkbenchTheme.subtleBorderColor
                                radius: 2
                            }
                        }

                        Button {
                            text: "Run"
                            onClicked: mainViewModel.runTerminalCommand()
                        }
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        TextArea {
                            readOnly: true
                            text: mainViewModel.terminalOutput
                            color: WorkbenchTheme.textPrimary
                            font.family: WorkbenchTheme.monoFont
                            font.pixelSize: 12
                            wrapMode: TextArea.Wrap
                            background: Rectangle { color: WorkbenchTheme.terminalBackground }
                        }
                    }
                }
            }
        }
    }
}
