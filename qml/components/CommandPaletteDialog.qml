import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root
    modal: true
    focus: true
    width: 720
    height: 420
    anchors.centerIn: Overlay.overlay
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: Rectangle { color: "#252526"; radius: 8; border.color: "#3c3c3c" }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        TextField {
            id: queryField
            Layout.fillWidth: true
            placeholderText: "Type a command"
            color: "#d4d4d4"
            background: Rectangle { color: "#1e1e1e"; radius: 4; border.color: "#3c3c3c" }
            onTextChanged: mainViewModel.refreshCommandPalette(text)
            onAccepted: if (mainViewModel.commandPaletteCount > 0) list.currentIndex = 0
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: mainViewModel.commandPaletteModel
            delegate: Rectangle {
                width: ListView.view.width
                height: 50
                color: ListView.isCurrentItem ? "#094771" : (mouseArea.containsMouse ? "#2a2d2e" : "transparent")
                ColumnLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    anchors.topMargin: 6
                    anchors.bottomMargin: 6
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: title; color: "#d4d4d4"; Layout.fillWidth: true; elide: Text.ElideRight }
                        Label { text: category; color: "#808080" }
                    }
                    Label { text: commandId + " · " + hint; color: "#808080"; Layout.fillWidth: true; elide: Text.ElideRight }
                }
                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        mainViewModel.runCommandPaletteCommand(commandId)
                        root.close()
                    }
                }
            }
        }
    }

    onOpened: {
        queryField.text = ""
        mainViewModel.refreshCommandPalette("")
        queryField.forceActiveFocus()
    }
}
