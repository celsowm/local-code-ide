import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Column {
    id: root

    property var viewModel

    spacing: 0
    visible: root.viewModel && root.viewModel.gitRecentCommitCount > 0

    ScmSectionHeader {
        width: parent.width
        title: "Recent Commits"
        count: root.viewModel ? root.viewModel.gitRecentCommitCount : 0
    }

    ListView {
        width: parent.width
        implicitHeight: contentHeight
        interactive: false
        spacing: 1
        model: root.viewModel ? root.viewModel.gitRecentCommitsModel : null

        delegate: Rectangle {
            required property string shortHash
            required property string subject
            required property string author
            required property string relativeTime
            required property bool head
            required property string refLabel

            width: root.width
            height: 44
            color: historyMouse.containsMouse ? "#1d1f21" : "transparent"
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 8

                Label {
                    text: "\u25cf"
                    color: head ? "#4fc1ff" : "#5a5a5a"
                    font.pixelSize: 10
                    Layout.alignment: Qt.AlignTop
                    Layout.topMargin: 4
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 1

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Label {
                            text: shortHash
                            color: "#7f7f7f"
                            font.pixelSize: 10
                        }

                        Label {
                            text: subject
                            color: "#d4d4d4"
                            font.pixelSize: 11
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Label {
                            text: relativeTime
                            color: "#6f6f6f"
                            font.pixelSize: 10
                        }
                    }

                    Label {
                        text: refLabel.length > 0 ? refLabel + " \u00b7 " + author : author
                        color: head ? "#9cdcfe" : "#7f7f7f"
                        font.pixelSize: 10
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
            }

            MouseArea {
                id: historyMouse
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
            }
        }
    }
}
