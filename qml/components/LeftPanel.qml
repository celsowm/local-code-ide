import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "#181818"
    border.color: "#2d2d30"

    StackLayout {
        anchors.fill: parent
        anchors.margins: 8
        currentIndex: mainViewModel.primaryViewIndex

        ExplorerPanel { }
        SearchPanel { }
        SourceControlPanel { }
        Rectangle {
            color: "transparent"
            border.color: "transparent"
            ColumnLayout {
                anchors.fill: parent
                Label { text: "ASSISTANT"; color: "#d4d4d4"; font.bold: true }
                Label { text: "O painel completo fica à direita."; color: "#808080" }
                Label { text: mainViewModel.toolCatalogSummary; color: "#9cdcfe"; wrapMode: Text.Wrap }
            }
        }
    }
}
