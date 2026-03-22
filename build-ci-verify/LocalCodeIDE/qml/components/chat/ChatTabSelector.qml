import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property int currentIndex: 0
    signal currentIndexChangedByUser(int index)

    implicitHeight: tabBar.implicitHeight

    TabBar {
        id: tabBar
        anchors.fill: parent
        currentIndex: root.currentIndex
        onCurrentIndexChanged: {
            if (root.currentIndex !== currentIndex)
                root.currentIndexChangedByUser(currentIndex)
        }

        TabButton { text: "Assistant" }
        TabButton { text: "Models" }
    }
}
