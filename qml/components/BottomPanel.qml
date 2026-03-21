import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "#181818"
    border.color: "#2d2d30"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            color: "#252526"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                Label { text: "PROBLEMS / TERMINAL"; color: "#d4d4d4"; font.bold: true }
                Label { text: mainViewModel.diagnosticsCount + " diagnostics"; color: "#808080" }
                Item { Layout.fillWidth: true }
                Label { text: mainViewModel.terminalBackendName; color: "#808080" }
            }
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            Rectangle {
                SplitView.fillWidth: true
                color: "#1e1e1e"
                ListView {
                    anchors.fill: parent
                    anchors.margins: 8
                    model: mainViewModel.diagnosticsModel
                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 34
                        color: index % 2 === 0 ? "#1e1e1e" : "#1a1a1a"
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 10
                            Label { text: severity.toUpperCase(); color: severity === "error" ? "#f14c4c" : (severity === "warning" ? "#cca700" : "#75beff"); font.bold: true }
                            Label { text: "L" + line + ":" + column; color: "#808080" }
                            Label { text: message; color: "#d4d4d4"; Layout.fillWidth: true; elide: Text.ElideRight }
                        }
                        MouseArea { anchors.fill: parent; onClicked: mainViewModel.openSearchResult(mainViewModel.currentPath, line, column) }
                    }
                }
            }

            Rectangle {
                SplitView.fillWidth: true
                color: "#111111"
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    RowLayout {
                        Layout.fillWidth: true
                        TextField {
                            Layout.fillWidth: true
                            text: mainViewModel.terminalCommand
                            color: "#d4d4d4"
                            background: Rectangle { color: "#1e1e1e"; radius: 4; border.color: "#3c3c3c" }
                            onTextChanged: mainViewModel.terminalCommand = text
                            onAccepted: mainViewModel.runTerminalCommand()
                        }
                        Button { text: "Run"; onClicked: mainViewModel.runTerminalCommand() }
                    }
                    TextArea {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        readOnly: true
                        text: mainViewModel.terminalOutput
                        color: "#d4d4d4"
                        font.family: "monospace"
                        wrapMode: TextArea.Wrap
                        background: Rectangle { color: "#111111" }
                    }
                }
            }
        }
    }
}
