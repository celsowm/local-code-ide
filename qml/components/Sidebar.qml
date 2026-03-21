import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "#181818"
    border.color: "#2d2d30"

    property var items: [
        { icon: "E", title: "Explorer" },
        { icon: "S", title: "Search" },
        { icon: "G", title: "Source Control" },
        { icon: "AI", title: "Assistant" }
    ]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 8

        Repeater {
            model: items
            delegate: Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                radius: 6
                color: index === mainViewModel.primaryViewIndex ? "#37373d" : "transparent"
                border.color: index === mainViewModel.primaryViewIndex ? "#007acc" : "transparent"

                Label {
                    anchors.centerIn: parent
                    text: modelData.icon
                    color: "#d4d4d4"
                    font.bold: true
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: mainViewModel.primaryViewIndex = index
                }

                ToolTip.visible: mouseArea.containsMouse
                ToolTip.text: modelData.title
            }
        }

        Item { Layout.fillHeight: true }
    }
}
