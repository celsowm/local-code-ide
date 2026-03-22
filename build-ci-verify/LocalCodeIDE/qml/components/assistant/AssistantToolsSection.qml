import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

AssistantCard {
    Layout.fillWidth: true
    surfaceColor: "#1e1e1e"
    outlineColor: "#2d2d30"
    cornerRadius: 6
    padding: 8
    spacing: 8

    Label {
        text: "TOOLS"
        color: "#d4d4d4"
        font.bold: true
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        CheckBox {
            text: "enable tool calling"
            checked: mainViewModel.aiEnableTools
            onToggled: mainViewModel.aiEnableTools = checked
        }

        CheckBox {
            text: "confirm destructive"
            checked: mainViewModel.aiRequireApprovalForDestructive
            onToggled: mainViewModel.aiRequireApprovalForDestructive = checked
        }

        Label { text: "rounds"; color: "#c5c5c5" }
        SpinBox {
            from: 0
            to: 12
            editable: true
            value: mainViewModel.aiMaxToolRounds
            onValueModified: mainViewModel.aiMaxToolRounds = value
        }

        Label {
            text: "calls: " + mainViewModel.toolCallCount
            color: mainViewModel.toolCallCount > 0 ? "#6a9955" : "#808080"
        }

        Item { Layout.fillWidth: true }
    }

    Label {
        text: mainViewModel.toolCatalogSummary
        color: "#808080"
        wrapMode: Text.WrapAnywhere
        Layout.fillWidth: true
    }

    Label {
        text: "pending: " + mainViewModel.pendingApprovalCount + " · " + mainViewModel.pendingApprovalSummary
        color: mainViewModel.pendingApprovalCount > 0 ? "#d7ba7d" : "#808080"
        wrapMode: Text.WrapAnywhere
        Layout.fillWidth: true
    }

    ScrollView {
        Layout.fillWidth: true
        Layout.preferredHeight: 96

        TextArea {
            readOnly: true
            text: mainViewModel.toolCallLog
            color: "#d4d4d4"
            placeholderText: "Tool execution log will appear here."
            font.family: "monospace"
            wrapMode: TextArea.WrapAnywhere
            background: Rectangle { color: "#181818"; radius: 4; border.color: "#3c3c3c" }
        }
    }
}
