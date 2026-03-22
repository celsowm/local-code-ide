
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#252526"
    border.color: "#2d2d30"
    property var viewModel

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            currentIndex: root.viewModel ? root.viewModel.secondaryAiTab : 0
            onCurrentIndexChanged: {
                if (root.viewModel && root.viewModel.secondaryAiTab !== currentIndex) {
                    root.viewModel.secondaryAiTab = currentIndex
                }
            }

            TabButton { text: "Assistant" }
            TabButton { text: "Models" }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            AssistantTab { }
            ModelHubPanel { }
        }
    }
}
