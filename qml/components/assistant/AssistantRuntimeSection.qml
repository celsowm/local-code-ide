import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

AssistantCard {
    Layout.fillWidth: true
    surfaceColor: "#1e1e1e"
    outlineColor: "#2d2d30"
    cornerRadius: 6
    padding: 8
    spacing: 6

    Label {
        text: "RUNTIME LINK"
        color: "#d4d4d4"
        font.bold: true
    }

    Label {
        text: modelHubViewModel.currentLocalModelSummary
        color: modelHubViewModel.hasCurrentLocalModel ? "#6a9955" : "#808080"
        wrapMode: Text.WrapAnywhere
        Layout.fillWidth: true
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Label { text: "ctx"; color: "#c5c5c5" }
        SpinBox {
            from: 1024
            to: 131072
            stepSize: 1024
            editable: true
            value: modelHubViewModel.runtimeContextSize
            onValueModified: modelHubViewModel.runtimeContextSize = value
        }

        Label { text: "server"; color: "#c5c5c5" }
        TextField {
            Layout.fillWidth: true
            text: modelHubViewModel.serverBaseUrl
            color: "#d4d4d4"
            background: Rectangle { color: "#181818"; radius: 4; border.color: "#3c3c3c" }
            onTextChanged: modelHubViewModel.serverBaseUrl = text
        }

        Button {
            text: modelHubViewModel.serverRunning || modelHubViewModel.serverStarting ? "Stop" : "Start"
            enabled: modelHubViewModel.serverRunning || modelHubViewModel.serverStarting || modelHubViewModel.canStartServer
            onClicked: {
                if (modelHubViewModel.serverRunning || modelHubViewModel.serverStarting)
                    modelHubViewModel.stopServer()
                else
                    modelHubViewModel.startServer()
            }
        }
    }

    Label {
        text: modelHubViewModel.currentLocalLaunchCommand
        color: "#808080"
        wrapMode: Text.WrapAnywhere
        Layout.fillWidth: true
    }
}
