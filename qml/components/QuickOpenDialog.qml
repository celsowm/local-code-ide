import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root
    modal: true
    focus: true
    width: 720
    height: 380
    anchors.horizontalCenter: Overlay.overlay.horizontalCenter
    y: 70
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: Rectangle { color: "#252526"; radius: 8; border.color: "#3c3c3c" }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        TextField {
            id: queryField
            Layout.fillWidth: true
            placeholderText: "Go to file"
            color: "#d4d4d4"
            background: Rectangle { color: "#1e1e1e"; radius: 4; border.color: "#3c3c3c" }
            onAccepted: {
                mainViewModel.openFirstMatchingWorkspaceFile(text)
                root.close()
            }
        }

        Label { text: "Press Enter to open the first matching file path."; color: "#808080" }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: mainViewModel.workspaceFilesModel
            delegate: Rectangle {
                visible: queryField.text.length === 0 || relativePath.toLowerCase().indexOf(queryField.text.toLowerCase()) !== -1
                width: ListView.view.width
                height: visible ? 30 : 0
                color: mouseArea.containsMouse ? "#2a2d2e" : "transparent"
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    text: relativePath
                    color: "#d4d4d4"
                    elide: Text.ElideMiddle
                    width: parent.width - 20
                }
                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        mainViewModel.openWorkspaceFile(path)
                        root.close()
                    }
                }
            }
        }
    }

    onOpened: queryField.forceActiveFocus()
}
