import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    property var viewModel

    color: "#181818"
    border.color: "#252526"
    implicitHeight: 88

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        anchors.topMargin: 6
        anchors.bottomMargin: 8
        spacing: 8

        TextField {
            Layout.fillWidth: true
            text: root.viewModel ? root.viewModel.scmCommitMessage : ""
            enabled: root.viewModel && !root.viewModel.gitBusy
            placeholderText: "Message (Ctrl+Enter to commit staged changes)"
            color: "#d4d4d4"
            selectByMouse: true
            background: Rectangle {
                color: "#1f1f1f"
                radius: 4
                border.color: activeFocus ? "#007acc" : "#343434"
            }
            onTextChanged: if (root.viewModel) root.viewModel.scmCommitMessage = text
            onAccepted: {
                if (root.viewModel && commitButton.enabled) {
                    root.viewModel.commitGitChanges()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                id: commitButton
                text: root.viewModel && root.viewModel.gitBusy ? "Working..." : "Commit"
                enabled: root.viewModel && !root.viewModel.gitBusy && root.viewModel.gitStagedCount > 0 && root.viewModel.scmCommitMessage.trim().length > 0
                onClicked: if (root.viewModel) root.viewModel.commitGitChanges()
            }

            BusyIndicator {
                running: root.viewModel && root.viewModel.gitBusy
                visible: running
                Layout.preferredWidth: 20
                Layout.preferredHeight: 20
            }

            Label {
                text: root.viewModel && root.viewModel.gitStagedCount > 0
                    ? root.viewModel.gitStagedCount + " staged"
                    : "Stage changes to commit"
                color: root.viewModel && root.viewModel.gitStagedCount > 0 ? "#7f7f7f" : "#d19a66"
                font.pixelSize: 11
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
        }
    }
}
