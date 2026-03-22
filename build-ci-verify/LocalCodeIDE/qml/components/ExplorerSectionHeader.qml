import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property string title: ""

    color: "#181818"
    implicitHeight: 22

    Label {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 8
        text: root.title
        color: "#858585"
        font.pixelSize: 10
        font.bold: true
    }
}
