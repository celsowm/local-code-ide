import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    property var viewModel

    function summaryLine() {
        if (!root.viewModel) {
            return ""
        }
        const parts = []
        if (root.viewModel.gitChangeCount > 0) {
            parts.push(root.viewModel.gitChangeCount + " changed")
        } else {
            parts.push("working tree clean")
        }
        if (root.viewModel.gitStagedCount > 0) {
            parts.push(root.viewModel.gitStagedCount + " staged")
        }
        if (root.viewModel.gitUntrackedCount > 0) {
            parts.push(root.viewModel.gitUntrackedCount + " untracked")
        }
        return parts.join(" · ")
    }

    color: "#181818"
    implicitHeight: 50

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 8

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Label {
                text: "SOURCE CONTROL"
                color: "#c8c8c8"
                font.pixelSize: 11
                font.bold: true
            }

            RowLayout {
                spacing: 6

                Label {
                    text: root.viewModel ? root.viewModel.gitBranchLabel : ""
                    color: "#9cdcfe"
                    font.pixelSize: 11
                    font.bold: true
                }

                Label {
                    text: root.summaryLine()
                    color: "#7f7f7f"
                    font.pixelSize: 10
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }

        ToolButton {
            text: "Refresh"
            font.pixelSize: 11
            enabled: !!root.viewModel
            onClicked: if (root.viewModel) root.viewModel.refreshGitChanges()
        }
    }
}
