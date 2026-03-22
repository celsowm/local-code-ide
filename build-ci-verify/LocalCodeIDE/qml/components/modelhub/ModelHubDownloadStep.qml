import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ModelHubCard {
    id: root

    property var tokens
    property bool chooseComplete: false

    cornerRadius: tokens.radius
    padding: tokens.cardPadding
    spacing: tokens.outerGap
    surfaceColor: tokens.cardColor
    outlineColor: tokens.borderColor

    Label {
        text: "Step 3: Download"
        color: tokens.textPrimary
        font.bold: true
    }

    ModelHubCard {
        Layout.fillWidth: true
        cornerRadius: tokens.radius
        padding: tokens.cardPadding
        spacing: 8
        surfaceColor: tokens.cardColorAlt
        outlineColor: tokens.borderColor
        enabled: root.chooseComplete

        Label {
            text: "Download target"
            color: tokens.textPrimary
            font.bold: true
        }

        TextField {
            Layout.fillWidth: true
            text: modelHubViewModel.targetDownloadDir
            placeholderText: "Target directory"
            color: tokens.textPrimary
            background: Rectangle {
                color: "#171a1f"
                radius: 6
                border.color: tokens.borderColor
            }
            onTextChanged: modelHubViewModel.targetDownloadDir = text
        }

        Flow {
            Layout.fillWidth: true
            spacing: 8

            IconButton {
                text: "Download selected"
                iconName: "download"
                enabled: !modelHubViewModel.downloadActive
                onClicked: modelHubViewModel.startDownloadSelected()
            }

            IconButton {
                text: "Download suggested"
                iconName: "download"
                enabled: !modelHubViewModel.downloadActive
                onClicked: modelHubViewModel.downloadSuggested()
            }

            IconButton {
                text: "Cancel"
                iconName: "stop"
                enabled: modelHubViewModel.downloadActive
                onClicked: modelHubViewModel.cancelDownload()
            }
        }

        Label {
            text: "Selected file: " + (modelHubViewModel.selectedFilePath.length > 0 ? modelHubViewModel.selectedFilePath : "none")
            color: tokens.textMuted
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }
    }

    ModelHubCard {
        Layout.fillWidth: true
        cornerRadius: tokens.radius
        padding: tokens.cardPadding
        spacing: 8
        surfaceColor: tokens.cardColorAlt
        outlineColor: tokens.borderColor

        Label {
            text: "Transfer status"
            color: tokens.textPrimary
            font.bold: true
        }

        ProgressBar {
            Layout.fillWidth: true
            from: 0
            to: 1
            value: modelHubViewModel.downloadProgress
            indeterminate: modelHubViewModel.downloadActive && modelHubViewModel.downloadProgress <= 0
        }

        Label {
            text: modelHubViewModel.downloadStatus
            color: tokens.textSecondary
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }

        Label {
            text: "phase " + modelHubViewModel.downloadPhase + " | speed " + modelHubViewModel.downloadSpeedText + " | eta " + modelHubViewModel.downloadEtaText
            color: tokens.textMuted
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }

        Label {
            text: modelHubViewModel.downloadProgressDetails
            color: tokens.successColor
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }
    }

    ModelHubCard {
        Layout.fillWidth: true
        cornerRadius: tokens.radius
        padding: tokens.cardPadding
        spacing: 8
        surfaceColor: tokens.cardColorAlt
        outlineColor: tokens.borderColor

        Label {
            text: "Diagnostics"
            color: tokens.textPrimary
            font.bold: true
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: 130
            clip: true

            TextArea {
                readOnly: true
                text: modelHubViewModel.downloadDiagnostics
                color: tokens.textSecondary
                font.family: "monospace"
                wrapMode: TextArea.WrapAnywhere
                background: Rectangle {
                    color: "#101317"
                    radius: 6
                }
            }
        }
    }
}
