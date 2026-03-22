pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    property var viewModel

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            color: "#181818"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 6
                anchors.rightMargin: 4
                spacing: 4

                Label {
                    text: "EXPLORER"
                    color: "#c8c8c8"
                    font.pixelSize: 11
                    font.bold: true
                    Layout.fillWidth: true
                }

                Label {
                    text: root.viewModel ? root.viewModel.workspaceFileCount + " files" : ""
                    color: "#787878"
                    font.pixelSize: 10
                }
            }
        }

        ExplorerRootRow {
            Layout.fillWidth: true
            viewModel: root.viewModel
            workspacePath: root.viewModel ? root.viewModel.workspaceRootPath : ""
        }

        ExplorerSectionHeader {
            Layout.fillWidth: true
            title: "FOLDERS"
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.viewModel ? root.viewModel.workspaceTreeModel : null
            boundsBehavior: Flickable.StopAtBounds
            spacing: 1

            delegate: ExplorerTreeRow {
                viewModel: root.viewModel
                width: ListView.view.width
            }
        }
    }
}
