import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var viewModel
    property string path: ""
    property string fileName: ""
    property string directory: ""
    property string statusCode: ""
    property string statusText: ""
    property string changeKind: ""
    property bool staged: false
    property bool renamed: false
    property bool untracked: false
    property bool hasIndexChanges: false
    property bool hasWorkingTreeChanges: false

    function badgeText() {
        if (untracked) return "U"
        if (renamed) return "R"
        const compact = statusCode.replace(/ /g, "")
        if (compact.length > 0) return compact.charAt(0)
        return "M"
    }

    function badgeColor() {
        if (untracked) return "#73c991"
        if (renamed) return "#c586c0"
        if (hasIndexChanges && hasWorkingTreeChanges) return "#4fc1ff"
        if (staged) return "#73c991"
        return "#e2c08d"
    }

    color: rowMouse.containsMouse ? "#202224" : "transparent"
    radius: 4
    implicitHeight: 44

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 6
        spacing: 6

        Label {
            text: root.badgeText()
            color: root.badgeColor()
            font.pixelSize: 12
            font.bold: true
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: 2
            Layout.preferredWidth: 14
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 1

            Label {
                text: root.fileName.length > 0 ? root.fileName : root.path
                color: "#d4d4d4"
                font.pixelSize: 12
                font.bold: rowMouse.containsMouse
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Label {
                text: root.directory.length > 0 ? root.directory : ""
                color: "#7f7f7f"
                font.pixelSize: 10
                elide: Text.ElideMiddle
                Layout.fillWidth: true
                visible: text.length > 0
            }
        }

        Label {
            text: root.statusText
            color: "#7f7f7f"
            font.pixelSize: 10
            visible: rowMouse.containsMouse && root.width > 300
        }

        RowLayout {
            spacing: 0
            visible: rowMouse.containsMouse

            ToolButton {
                text: root.staged ? "-" : "+"
                font.pixelSize: 12
                implicitWidth: 24
                implicitHeight: 24
                enabled: root.viewModel && !root.viewModel.gitBusy
                ToolTip.visible: hovered
                ToolTip.text: root.staged ? "Unstage" : "Stage"
                onClicked: root.staged ? root.viewModel.unstageGitPath(root.path) : root.viewModel.stageGitPath(root.path)
            }

            ToolButton {
                text: "O"
                font.pixelSize: 11
                implicitWidth: 24
                implicitHeight: 24
                ToolTip.visible: hovered
                ToolTip.text: "Open"
                onClicked: root.viewModel.openWorkspaceFile(root.viewModel.workspaceRootPath + "/" + root.path)
            }

            ToolButton {
                text: "D"
                font.pixelSize: 11
                implicitWidth: 24
                implicitHeight: 24
                ToolTip.visible: hovered
                ToolTip.text: "Diff"
                enabled: root.viewModel && !root.viewModel.gitDiffBusy
                onClicked: root.viewModel.openGitDiff(root.path)
            }

            ToolButton {
                text: "X"
                font.pixelSize: 11
                implicitWidth: 24
                implicitHeight: 24
                ToolTip.visible: hovered
                ToolTip.text: "Discard"
                enabled: !root.untracked && root.viewModel && !root.viewModel.gitBusy
                onClicked: root.viewModel.discardGitPath(root.path)
            }
        }
    }

    MouseArea {
        id: rowMouse
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        hoverEnabled: true
        onClicked: root.viewModel.openWorkspaceFile(root.viewModel.workspaceRootPath + "/" + root.path)
        z: -1
    }
}
