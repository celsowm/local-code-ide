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
    icon.color: enabled ? "#d4d4d4" : "#707070"
    display: AbstractButton.TextBesideIcon
    spacing: 6
}

