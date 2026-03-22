import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property int padding: 8
    property int spacing: 8
    property int cornerRadius: 6
    property color surfaceColor: "#1e1e1e"
    property color outlineColor: "#2d2d30"

    default property alias contentData: contentColumn.data

    radius: cornerRadius
    color: surfaceColor
    border.color: outlineColor
    implicitHeight: contentColumn.implicitHeight + (padding * 2)

    ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        anchors.margins: root.padding
        spacing: root.spacing
    }
}
