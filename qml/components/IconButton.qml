import QtQuick
import QtQuick.Controls
import "IconRegistry.js" as IconRegistry

Button {
    id: root

    property string iconName: ""
    property int iconSize: 16

    icon.source: IconRegistry.source(iconName)
    icon.width: iconSize
    icon.height: iconSize
    icon.color: enabled ? WorkbenchTheme.textPrimary : WorkbenchTheme.textDim
    display: AbstractButton.TextBesideIcon
    spacing: 6
    padding: 6
    flat: true
    hoverEnabled: true

    background: Rectangle {
        radius: 3
        color: root.hovered ? "#2a2d2e" : "transparent"
    }
}
