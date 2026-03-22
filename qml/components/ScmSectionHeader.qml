import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property string title: ""
    property int count: 0

    color: "#181818"
    implicitHeight: 24

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 6

        Label {
            text: root.title
            color: "#9d9d9d"
            font.pixelSize: 10
            font.bold: true
            Layout.fillWidth: true
        }

        Label {
            text: root.count
            color: "#6f6f6f"
            font.pixelSize: 10
        }
    }
}
