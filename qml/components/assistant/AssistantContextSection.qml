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
        text: "CTX CONTROL"
        color: "#d4d4d4"
        font.bold: true
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        CheckBox {
            text: "document"
            checked: mainViewModel.aiIncludeDocument
            onToggled: mainViewModel.aiIncludeDocument = checked
        }
        CheckBox {
            text: "diagnostics"
            checked: mainViewModel.aiIncludeDiagnostics
            onToggled: mainViewModel.aiIncludeDiagnostics = checked
        }
        CheckBox {
            text: "git"
            checked: mainViewModel.aiIncludeGitSummary
            onToggled: mainViewModel.aiIncludeGitSummary = checked
        }
        CheckBox {
            text: "workspace"
            checked: mainViewModel.aiIncludeWorkspaceContext
            onToggled: mainViewModel.aiIncludeWorkspaceContext = checked
        }
        Item { Layout.fillWidth: true }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Label { text: "doc chars"; color: "#c5c5c5" }
        SpinBox {
            from: 512
            to: 200000
            stepSize: 512
            editable: true
            value: mainViewModel.aiDocumentContextChars
            onValueModified: mainViewModel.aiDocumentContextChars = value
        }

        Label { text: "ws chars"; color: "#c5c5c5" }
        SpinBox {
            from: 0
            to: 400000
            stepSize: 512
            editable: true
            value: mainViewModel.aiWorkspaceContextChars
            onValueModified: mainViewModel.aiWorkspaceContextChars = value
        }

        Label { text: "ws files"; color: "#c5c5c5" }
        SpinBox {
            from: 0
            to: 24
            editable: true
            value: mainViewModel.aiWorkspaceContextMaxFiles
            onValueModified: mainViewModel.aiWorkspaceContextMaxFiles = value
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Label { text: "max tokens"; color: "#c5c5c5" }
        SpinBox {
            from: 64
            to: 8192
            stepSize: 64
            editable: true
            value: mainViewModel.aiMaxOutputTokens
            onValueModified: mainViewModel.aiMaxOutputTokens = value
        }

        Label { text: "temp"; color: "#c5c5c5" }
        TextField {
            Layout.preferredWidth: 70
            text: Number(mainViewModel.aiTemperature).toFixed(2)
            color: "#d4d4d4"
            background: Rectangle { color: "#181818"; radius: 4; border.color: "#3c3c3c" }
            onEditingFinished: mainViewModel.aiTemperature = parseFloat(text)
        }

        Button { text: "Refresh context"; onClicked: mainViewModel.refreshRelevantContext() }
        Button { text: "Extract patch"; onClicked: mainViewModel.extractPatchPreview() }
        Button {
            text: "Apply patch"
            enabled: mainViewModel.canApplyPatchPreview
            onClicked: mainViewModel.applyAssistantPatch()
        }
    }

    TextArea {
        Layout.fillWidth: true
        Layout.preferredHeight: 72
        text: mainViewModel.aiSystemPrompt
        color: "#d4d4d4"
        wrapMode: TextArea.Wrap
        placeholderText: "System prompt for the local assistant"
        background: Rectangle { color: "#181818"; radius: 4; border.color: "#3c3c3c" }
        onTextChanged: mainViewModel.aiSystemPrompt = text
    }

    Label {
        text: mainViewModel.aiContextSummary
        color: "#808080"
        wrapMode: Text.WrapAnywhere
        Layout.fillWidth: true
    }
}
