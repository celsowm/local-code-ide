import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ModelHubCard {
    id: root

    property var tokens

    cornerRadius: tokens.radius
    padding: tokens.cardPadding
    spacing: tokens.outerGap
    surfaceColor: tokens.cardColor
    outlineColor: tokens.borderColor

    Label {
        text: "Step 4: Activate & Server"
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

        Label {
            text: "Active local model"
            color: tokens.textPrimary
            font.bold: true
        }

        Flow {
            Layout.fillWidth: true
            spacing: 8

            IconButton {
                text: "Use selected as active"
                iconName: "use_active"
                enabled: modelHubViewModel.canUseSelectedAsCurrent
                onClicked: modelHubViewModel.useSelectedAsCurrent()
            }

            IconButton {
                text: "Use downloaded as active"
                iconName: "use_active"
                enabled: modelHubViewModel.downloadedPath.length > 0
                onClicked: modelHubViewModel.useDownloadedAsCurrent()
            }
        }

        Label {
            text: modelHubViewModel.currentLocalModelSummary
            color: modelHubViewModel.hasCurrentLocalModel ? tokens.successColor : tokens.textMuted
            Layout.fillWidth: true
            wrapMode: Text.WrapAnywhere
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
            text: "Local llama server"
            color: tokens.textPrimary
            font.bold: true
        }

        Label {
            text: modelHubViewModel.serverHealthy
                ? "online"
                : ((modelHubViewModel.serverRunning || modelHubViewModel.serverStarting) ? "starting" : "offline")
            color: modelHubViewModel.serverHealthy
                ? tokens.successColor
                : ((modelHubViewModel.serverRunning || modelHubViewModel.serverStarting) ? tokens.warningColor : tokens.textMuted)
            font.bold: true
        }

        Label {
            text: modelHubViewModel.serverStatusLine
            color: tokens.textSecondary
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }

        Flow {
            Layout.fillWidth: true
            spacing: 8

            IconButton {
                text: modelHubViewModel.serverRunning || modelHubViewModel.serverStarting ? "Stop" : "Start"
                iconName: modelHubViewModel.serverRunning || modelHubViewModel.serverStarting ? "stop" : "start"
                enabled: modelHubViewModel.serverRunning || modelHubViewModel.serverStarting || modelHubViewModel.canStartServer
                onClicked: {
                    if (modelHubViewModel.serverRunning || modelHubViewModel.serverStarting)
                        modelHubViewModel.stopServer()
                    else
                        modelHubViewModel.startServer()
                }
            }

            IconButton {
                text: "Probe"
                iconName: "probe"
                onClicked: modelHubViewModel.probeServer()
            }

            IconButton {
                text: "Clear logs"
                iconName: "clear_logs"
                onClicked: modelHubViewModel.clearServerLogs()
            }
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
            text: "Runtime configuration"
            color: tokens.textPrimary
            font.bold: true
        }

        Label {
            text: "Executable"
            color: tokens.textMuted
        }

        TextField {
            Layout.fillWidth: true
            text: modelHubViewModel.serverExecutablePath
            placeholderText: "llama-server"
            color: tokens.textPrimary
            background: Rectangle {
                color: "#171a1f"
                radius: 6
                border.color: tokens.borderColor
            }
            onTextChanged: modelHubViewModel.serverExecutablePath = text
        }

        Label {
            text: "Base URL"
            color: tokens.textMuted
        }

        TextField {
            Layout.fillWidth: true
            text: modelHubViewModel.serverBaseUrl
            placeholderText: "http://127.0.0.1:8080"
            color: tokens.textPrimary
            background: Rectangle {
                color: "#171a1f"
                radius: 6
                border.color: tokens.borderColor
            }
            onTextChanged: modelHubViewModel.serverBaseUrl = text
        }

        Label {
            text: "Context size"
            color: tokens.textMuted
        }

        SpinBox {
            Layout.fillWidth: true
            from: 1024
            to: 131072
            stepSize: 1024
            editable: true
            value: modelHubViewModel.runtimeContextSize
            onValueModified: modelHubViewModel.runtimeContextSize = value
        }

        Label {
            text: "Extra arguments"
            color: tokens.textMuted
        }

        TextField {
            Layout.fillWidth: true
            text: modelHubViewModel.serverExtraArguments
            placeholderText: "--n-gpu-layers 999 --threads 8"
            color: tokens.textPrimary
            background: Rectangle {
                color: "#171a1f"
                radius: 6
                border.color: tokens.borderColor
            }
            onTextChanged: modelHubViewModel.serverExtraArguments = text
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
            text: "Launch command"
            color: tokens.textPrimary
            font.bold: true
        }

        Label {
            text: modelHubViewModel.currentLocalLaunchCommand
            color: tokens.successColor
            Layout.fillWidth: true
            wrapMode: Text.WrapAnywhere
        }

        Label {
            text: "Context size changes apply on next server start."
            color: tokens.textMuted
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }

        Label {
            text: "Server logs"
            color: tokens.textPrimary
            font.bold: true
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: 160

            TextArea {
                readOnly: true
                text: modelHubViewModel.serverLogs
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
