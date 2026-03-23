import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: WorkbenchTheme.sideBarBackground
    border.color: WorkbenchTheme.borderColor
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
