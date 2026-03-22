import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Label {
            text: "LOCAL ASSISTANT"
            color: "#d4d4d4"
            font.bold: true
        }

        Label {
            text: mainViewModel.aiBackendName
            color: "#9cdcfe"
        }

        Label {
            text: mainViewModel.aiBackendStatusLine
            color: (modelHubViewModel.serverHealthy
                ? "#6a9955"
                : ((modelHubViewModel.serverRunning || modelHubViewModel.serverStarting) ? "#cca700" : "#808080"))
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        AssistantRuntimeSection {
            Layout.fillWidth: true
        }

        AssistantContextSection {
            Layout.fillWidth: true
        }

        AssistantToolsSection {
            Layout.fillWidth: true
        }

        AssistantApprovalsSection {
            Layout.fillWidth: true
        }

        AssistantInputSection {
            Layout.fillWidth: true
        }

        AssistantOutputSection {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
