import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

AssistantCard {
    Layout.fillWidth: true
    surfaceColor: "#1e1e1e"
    outlineColor: mainViewModel.pendingApprovalCount > 0 ? "#d7ba7d" : "#2d2d30"
    cornerRadius: 6
    padding: 8
    spacing: 8

    RowLayout {
        Layout.fillWidth: true
        Label {
            text: "PENDING APPROVALS (" + mainViewModel.pendingApprovalCount + ")"
            color: "#d4d4d4"
            font.bold: true
        }
        Item { Layout.fillWidth: true }
        Button {
            text: "Approve all"
            enabled: mainViewModel.pendingApprovalCount > 0
            onClicked: mainViewModel.approveAllPendingTools()
        }
        Button {
            text: "Clear"
            enabled: mainViewModel.pendingApprovalCount > 0
            onClicked: mainViewModel.clearPendingTools()
        }
    }

    ListView {
        Layout.fillWidth: true
        Layout.preferredHeight: Math.max(84, Math.min(contentHeight, 180))
        clip: true
        model: mainViewModel.pendingApprovalModel

        delegate: Rectangle {
            width: ListView.view.width
            height: approvalContent.implicitHeight + 10
            color: "#181818"
            radius: 4
            border.color: destructive ? "#d7ba7d" : "#3c3c3c"

            ColumnLayout {
                id: approvalContent
                anchors.fill: parent
                anchors.margins: 6
                spacing: 4

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: toolName + (destructive ? " *" : "")
                        color: "#ce9178"
                        font.bold: true
                    }
                    Label {
                        text: pathHints
                        color: "#808080"
                        Layout.fillWidth: true
                        elide: Text.ElideMiddle
                    }
                }

                Label {
                    text: summary
                    color: "#d4d4d4"
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }

                TextArea {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 72
                    readOnly: true
                    text: argumentsText
                    color: "#d4d4d4"
                    font.family: "monospace"
                    wrapMode: TextArea.WrapAnywhere
                    background: Rectangle { color: "#111111"; radius: 4 }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Item { Layout.fillWidth: true }
                    Button { text: "Reject"; onClicked: mainViewModel.rejectPendingTool(approvalId) }
                    Button { text: "Approve"; onClicked: mainViewModel.approvePendingTool(approvalId) }
                }
            }
        }
    }
}
