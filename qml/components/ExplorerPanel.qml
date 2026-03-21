import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Label { text: "EXPLORER"; color: "#d4d4d4"; font.bold: true }
        Label { text: mainViewModel.workspaceFileCount + " files"; color: "#808080" }

        Label { text: "OPEN EDITORS"; color: "#d4d4d4"; font.bold: true }
        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            clip: true
            model: mainViewModel.openEditorsModel
            delegate: Rectangle {
                width: ListView.view.width
                height: 28
                color: active ? "#2a2d2e" : (mouseArea.containsMouse ? "#222" : "transparent")
                MouseArea { id: mouseArea; anchors.fill: parent; hoverEnabled: true; z: -1; onClicked: mainViewModel.switchOpenEditor(path) }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    Label { text: dirty ? title + " •" : title; color: "#d4d4d4"; Layout.fillWidth: true; elide: Text.ElideRight }
                    Button { text: "×"; flat: true; onClicked: mainViewModel.closeOpenEditor(path) }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#2d2d30" }
        Label { text: "FOLDERS"; color: "#d4d4d4"; font.bold: true }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: mainViewModel.workspaceTreeModel
            delegate: Rectangle {
                width: ListView.view.width
                height: 28
                color: mouseArea.containsMouse ? "#2a2d2e" : "transparent"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 6 + depth * 14
                    anchors.rightMargin: 6
                    spacing: 6
                    Button {
                        text: directory && hasChildren ? (expanded ? "▾" : "▸") : ""
                        flat: true
                        enabled: directory && hasChildren
                        visible: directory && hasChildren
                        onClicked: mainViewModel.toggleWorkspaceFolder(nodeId)
                    }
                    Label {
                        text: directory ? "📁" : "📄"
                        color: directory ? "#c5c5c5" : "#808080"
                    }
                    Label {
                        text: name
                        color: directory ? "#dcdcaa" : "#d4d4d4"
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    Button {
                        visible: !directory && mouseArea.containsMouse
                        text: "Split"
                        flat: true
                        onClicked: mainViewModel.openWorkspaceFileInSplit(path)
                    }
                }
                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    z: -1
                    onClicked: {
                        if (directory) {
                            mainViewModel.toggleWorkspaceFolder(nodeId)
                        } else {
                            mainViewModel.openWorkspaceFile(path)
                        }
                    }
                }
            }
        }
    }
}
