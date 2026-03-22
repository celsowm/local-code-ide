import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property int stepIndex: 0
    property string title: ""
    property bool active: false
    property bool done: false
    property var tokens

    signal clicked()

    implicitHeight: 62
    radius: tokens ? tokens.radius : 8
    border.width: 1
    border.color: active
        ? tokens.accentColor
        : (done ? "#4f9b67" : tokens.borderColor)
    color: active
        ? "#1f3043"
        : (done ? "#1f2f27" : tokens.cardColorAlt)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 2

        Label {
            text: (root.stepIndex + 1) + ". " + root.title
            color: tokens.textPrimary
            font.bold: true
            Layout.fillWidth: true
            elide: Text.ElideRight
        }

        Label {
            text: root.done ? "Done" : (root.active ? "Current" : "Pending")
            color: root.done ? tokens.successColor : (root.active ? tokens.accentColor : tokens.textMuted)
            Layout.fillWidth: true
            elide: Text.ElideRight
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
