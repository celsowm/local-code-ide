pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import "IconRegistry.js" as IconRegistry

ToolButton {
    id: root

    property string iconName: ""
    property string label: ""
    property string tooltipText: ""

    width: 24
    height: 24
    hoverEnabled: true
    icon.source: iconName.length > 0 ? IconRegistry.source(iconName) : ""
    icon.width: 14
    icon.height: 14
    icon.color: hovered ? "#f3f3f3" : "#9f9f9f"
    text: iconName.length > 0 ? "" : label
    font.pixelSize: 13
    padding: 0
    display: iconName.length > 0 ? AbstractButton.IconOnly : AbstractButton.TextOnly

    background: Rectangle {
        radius: 3
        color: root.hovered ? "#252526" : "transparent"
    }

    ToolTip.visible: hovered
    ToolTip.text: tooltipText
}
