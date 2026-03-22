import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property int currentIndex: 0

    StackLayout {
        anchors.fill: parent
        currentIndex: root.currentIndex

        AssistantTab { }
        ModelHubPanel { }
    }
}
