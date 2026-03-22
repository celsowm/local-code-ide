import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property int padding: 10
    property int spacing: 8
    property int cornerRadius: 8
    property color surfaceColor: "#23262b"
    property color outlineColor: "#343840"

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
