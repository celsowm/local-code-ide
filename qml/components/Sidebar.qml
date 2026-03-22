pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "IconRegistry.js" as IconRegistry

Rectangle {
    id: root
    color: "#181818"
    border.color: "#2d2d30"
    property var viewModel

    property var items: [
        { iconName: "explorer", title: "Explorer" },
        { iconName: "search", title: "Search" },
        { iconName: "git", title: "Source Control" }
    ]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 8

        Repeater {
            model: root.items
            delegate: Rectangle {
                id: navItem
                required property int index
                required property var modelData
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                radius: 6
                color: index === root.viewModel.primaryViewIndex ? "#37373d" : "transparent"
                border.color: index === root.viewModel.primaryViewIndex ? "#007acc" : "transparent"

                ToolButton {
                    id: navButton
                    anchors.fill: parent
                    display: AbstractButton.IconOnly
                    icon.source: IconRegistry.source(navItem.modelData.iconName)
                    icon.width: 18
                    icon.height: 18
                    icon.color: navItem.index === root.viewModel.primaryViewIndex ? "#9cdcfe" : "#d4d4d4"
                    background: Rectangle { color: "transparent" }
                    hoverEnabled: true
                    onClicked: root.viewModel.primaryViewIndex = navItem.index
                }

                ToolTip.visible: navButton.hovered
                ToolTip.text: modelData.title
            }
        }

        Item { Layout.fillHeight: true }
    }
}
