import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    property var viewModel

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Label { text: "SOURCE CONTROL"; color: "#d4d4d4"; font.bold: true }
        Label { text: root.viewModel.gitSummary; color: "#9cdcfe"; wrapMode: Text.Wrap }
        Label { text: root.viewModel.gitChangeCount + " changes · " + root.viewModel.gitStagedCount + " staged"; color: "#808080" }

        RowLayout {
            Layout.fillWidth: true
            TextField {
                Layout.fillWidth: true
                text: root.viewModel.scmCommitMessage
                placeholderText: "Commit message"
                color: "#d4d4d4"
                background: Rectangle { color: "#1e1e1e"; radius: 4; border.color: "#3c3c3c" }
                onTextChanged: root.viewModel.scmCommitMessage = text
                onAccepted: root.viewModel.commitGitChanges()
            }
            Button { text: "Commit"; onClicked: root.viewModel.commitGitChanges() }
            Button { text: "Refresh"; onClicked: root.viewModel.refreshGitChanges() }
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.viewModel.gitChangesModel
            delegate: Rectangle {
                width: ListView.view.width
                height: 82
                color: mouseArea.containsMouse ? "#2a2d2e" : (index % 2 === 0 ? "transparent" : "#1b1b1b")
                MouseArea { id: mouseArea; anchors.fill: parent; hoverEnabled: true; z: -1; onClicked: root.viewModel.openWorkspaceFile(root.viewModel.workspaceRootPath + "/" + path) }
                ColumnLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    anchors.topMargin: 6
                    anchors.bottomMargin: 6
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: statusCode; color: staged ? "#73c991" : "#f14c4c"; font.bold: true }
                        Label { text: path; color: "#d4d4d4"; Layout.fillWidth: true; elide: Text.ElideMiddle }
                        Label { text: statusText; color: "#808080" }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Button { text: staged ? "Unstage" : "Stage"; onClicked: staged ? root.viewModel.unstageGitPath(path) : root.viewModel.stageGitPath(path) }
                        Button { text: "Open"; onClicked: root.viewModel.openWorkspaceFile(root.viewModel.workspaceRootPath + "/" + path) }
                        Button { text: "Diff"; onClicked: root.viewModel.openGitDiff(path) }
                        Button { text: "Discard"; enabled: !untracked; onClicked: root.viewModel.discardGitPath(path) }
                    }
                }
            }
        }
    }
}
