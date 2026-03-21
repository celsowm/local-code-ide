import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Label { text: "SEARCH"; color: "#d4d4d4"; font.bold: true }

        RowLayout {
            Layout.fillWidth: true
            TextField {
                Layout.fillWidth: true
                text: mainViewModel.searchPattern
                placeholderText: "find in workspace"
                color: "#d4d4d4"
                background: Rectangle { color: "#1e1e1e"; radius: 4; border.color: "#3c3c3c" }
                onTextChanged: mainViewModel.searchPattern = text
                onAccepted: mainViewModel.runSearch()
            }
            Button { text: "Go"; onClicked: mainViewModel.runSearch() }
        }

        Label { text: mainViewModel.searchResultCount + " results"; color: "#808080" }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: mainViewModel.searchResultsModel
            delegate: Rectangle {
                width: ListView.view.width
                height: 52
                color: mouseArea.containsMouse ? "#2a2d2e" : "transparent"
                ColumnLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    anchors.topMargin: 6
                    anchors.bottomMargin: 6
                    Label { text: path + ":" + line; color: "#9cdcfe"; elide: Text.ElideMiddle; Layout.fillWidth: true }
                    Label { text: preview; color: "#d4d4d4"; elide: Text.ElideRight; Layout.fillWidth: true }
                }
                MouseArea { id: mouseArea; anchors.fill: parent; hoverEnabled: true; onClicked: mainViewModel.openSearchResult(path, line, column) }
            }
        }
    }
}
