import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#181818"
    border.color: "#2d2d30"
    property var viewModel

    StackLayout {
        anchors.fill: parent
        anchors.margins: 8
        currentIndex: root.viewModel ? root.viewModel.primaryViewIndex : 0

        ExplorerPanel { viewModel: root.viewModel }
        SearchPanel { viewModel: root.viewModel }
        SourceControlPanel { viewModel: root.viewModel }
    }
}
