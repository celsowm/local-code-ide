import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    implicitHeight: inputColumn.implicitHeight

    ColumnLayout {
        id: inputColumn
        anchors.fill: parent
        spacing: 8

        TextArea {
            Layout.fillWidth: true
            Layout.preferredHeight: 90
            text: mainViewModel.chatInput
            enabled: !mainViewModel.aiBusy
            placeholderText: "Explain an error, propose refactor, generate test, suggest diff, or ask to read/create/edit/search..."
            color: "#d4d4d4"
            wrapMode: TextArea.Wrap
            background: Rectangle { color: "#1e1e1e"; radius: 6 }
            onTextChanged: mainViewModel.chatInput = text
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: mainViewModel.aiBusy ? "Asking..." : "Ask"
                enabled: !mainViewModel.aiBusy
                onClicked: mainViewModel.askAssistant()
            }

            BusyIndicator {
                running: mainViewModel.aiBusy
                visible: running
                Layout.preferredWidth: 20
                Layout.preferredHeight: 20
            }

            Label {
                text: mainViewModel.relevantContextSummary
                color: "#808080"
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
        }

        AssistantCard {
            Layout.fillWidth: true
            Layout.preferredHeight: 140
            surfaceColor: "#181818"
            outlineColor: "#2d2d30"
            cornerRadius: 6
            padding: 8
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
                    color: hover.containsMouse ? "#2a2d2e" : "transparent"

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
                        id: hover
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: mainViewModel.openWorkspaceFile(path)
                    }
                }
            }
        }
    }
}
