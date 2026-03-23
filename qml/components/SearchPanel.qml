pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    property var viewModel

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Label { text: "SEARCH"; color: "#d4d4d4"; font.bold: true }

        RowLayout {
            Layout.fillWidth: true
            TextField {
                Layout.fillWidth: true
                text: root.viewModel ? root.viewModel.searchPattern : ""
                enabled: root.viewModel && !root.viewModel.searchBusy
                placeholderText: "find in workspace"
                color: "#d4d4d4"
                background: Rectangle { color: "#1e1e1e"; radius: 4; border.color: "#3c3c3c" }
                onTextChanged: if (root.viewModel) root.viewModel.searchPattern = text
                onAccepted: if (root.viewModel) root.viewModel.runSearch()
            }
            Button { text: root.viewModel && root.viewModel.searchBusy ? "Searching..." : "Go"; enabled: !!root.viewModel && !root.viewModel.searchBusy; onClicked: if (root.viewModel) root.viewModel.runSearch() }
            BusyIndicator {
                running: root.viewModel && root.viewModel.searchBusy
                visible: running
                Layout.preferredWidth: 18
                Layout.preferredHeight: 18
            }
        }

        Label { text: (root.viewModel ? root.viewModel.searchResultCount : 0) + " results"; color: "#808080" }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.viewModel ? root.viewModel.searchResultsModel : null
            delegate: Rectangle {
                id: resultItem
                required property string path
                required property int line
                required property int column
                required property string preview
                width: ListView.view.width
                height: 52
                color: mouseArea.containsMouse ? "#2a2d2e" : "transparent"
                ColumnLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    anchors.topMargin: 6
                    anchors.bottomMargin: 6
                    Label { text: resultItem.path + ":" + resultItem.line; color: "#9cdcfe"; elide: Text.ElideMiddle; Layout.fillWidth: true }
                    Label { text: resultItem.preview; color: "#d4d4d4"; elide: Text.ElideRight; Layout.fillWidth: true }
                }
                MouseArea { id: mouseArea; anchors.fill: parent; hoverEnabled: true; onClicked: if (root.viewModel) root.viewModel.openSearchResult(resultItem.path, resultItem.line, resultItem.column) }
            }
        }
    }
}
