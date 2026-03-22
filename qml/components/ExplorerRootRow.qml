pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "IconRegistry.js" as IconRegistry

Rectangle {
    id: root

    property var viewModel
    property string workspacePath: ""

    function baseName(path) {
        if (!path || path.length === 0) {
            return "Workspace"
        }
        const normalized = path.replace(/\\/g, "/")
        const parts = normalized.split("/")
        return parts.length > 0 && parts[parts.length - 1].length > 0 ? parts[parts.length - 1] : normalized
    }

    color: "#1d1d1d"
    border.color: "#232323"
    implicitHeight: 28

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 4
        spacing: 6

        Image {
            source: IconRegistry.source("folder")
            sourceSize.width: 14
            sourceSize.height: 14
            Layout.preferredWidth: 14
            Layout.preferredHeight: 14
        }

        Label {
            text: root.baseName(root.workspacePath)
            color: "#d7d7d7"
            font.pixelSize: 12
            font.bold: true
            Layout.fillWidth: true
            elide: Text.ElideRight
        }

        ExplorerToolbarButton {
            iconName: "new_file"
            tooltipText: "New File"
            onClicked: root.viewModel.createWorkspaceFile()
        }

        ExplorerToolbarButton {
            iconName: "new_folder"
            tooltipText: "New Folder"
            onClicked: root.viewModel.createWorkspaceFolder()
        }

        ExplorerToolbarButton {
            iconName: "refresh"
            tooltipText: "Refresh Explorer"
            onClicked: root.viewModel.refreshWorkspace()
        }

        ExplorerToolbarButton {
            iconName: "collapse_all"
            tooltipText: "Collapse All"
            onClicked: root.viewModel.collapseWorkspaceFolders()
        }
    }
}
