pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var viewModel
    required property string nodeId
    required property string path
    required property string name
    required property int depth
    required property bool directory
    required property bool expanded
    required property bool hasChildren
    required property string iconSource
    required property string gitBadge
    required property string gitBadgeColor

    readonly property bool activeFile: !directory && viewModel && viewModel.currentPath === path
    readonly property bool hovered: rowHover.containsMouse

    color: activeFile ? "#21374d" : (hovered ? "#202122" : "transparent")
    radius: 3
    height: 21

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 5 + (root.depth * 11)
        anchors.rightMargin: 4
        spacing: 4

        Rectangle {
            Layout.preferredWidth: 2
            Layout.fillHeight: true
            radius: 1
            color: root.activeFile ? "#007acc" : "transparent"
        }

        Item {
            Layout.preferredWidth: 10
            Layout.preferredHeight: 12
            visible: root.directory && root.hasChildren

            Text {
                anchors.centerIn: parent
                text: root.expanded ? "▾" : "▸"
                color: root.hovered || root.activeFile ? "#d8d8d8" : "#7d7d7d"
                font.pixelSize: 10
            }

            MouseArea {
                anchors.fill: parent
                onClicked: root.viewModel.toggleWorkspaceFolder(root.nodeId)
            }
        }

        Item {
            Layout.preferredWidth: 10
            Layout.preferredHeight: 12
            visible: !(root.directory && root.hasChildren)
        }

        Image {
            source: root.iconSource
            sourceSize.width: 14
            sourceSize.height: 14
            Layout.preferredWidth: 14
            Layout.preferredHeight: 14
            opacity: root.activeFile ? 1.0 : 0.9
        }

        Label {
            text: root.name
            color: root.activeFile ? "#f3f3f3" : "#cccccc"
            font.pixelSize: 11
            Layout.fillWidth: true
            elide: Text.ElideRight
        }

        Text {
            visible: root.gitBadge.length > 0
            text: root.gitBadge
            color: root.gitBadgeColor.length > 0 ? Qt.color(root.gitBadgeColor) : "#9cdcfe"
            font.pixelSize: 10
            font.bold: true
            opacity: 0.95
            Layout.preferredWidth: 10
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        ToolButton {
            id: splitButton
            width: 30
            height: 17
            visible: !root.directory && root.hovered
            text: "Split"
            font.pixelSize: 10
            hoverEnabled: true

            contentItem: Text {
                text: splitButton.text
                color: splitButton.hovered ? "#f3f3f3" : "#9f9f9f"
                font: splitButton.font
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                radius: 3
                color: splitButton.hovered ? "#2f3133" : "#252526"
            }

            onClicked: root.viewModel.openWorkspaceFileInSplit(root.path)
        }
    }

    MouseArea {
        id: rowHover
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        z: -1
        onClicked: {
            if (root.directory) {
                root.viewModel.toggleWorkspaceFolder(root.nodeId)
            } else {
                root.viewModel.openWorkspaceFile(root.path)
            }
        }
    }
}
