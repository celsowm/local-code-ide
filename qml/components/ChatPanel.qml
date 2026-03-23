import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var viewModel

    color: WorkbenchTheme.elevatedBackground
    border.color: WorkbenchTheme.borderColor

    readonly property int selectedTab: root.viewModel ? root.viewModel.secondaryAiTab : 0

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ChatTabSelector {
            Layout.fillWidth: true
            currentIndex: root.selectedTab
            onCurrentIndexChangedByUser: function(index) {
                if (root.viewModel && root.viewModel.secondaryAiTab !== index)
                    root.viewModel.secondaryAiTab = index
            }
        }

        ChatContentStack {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.selectedTab
        }
    }
}
