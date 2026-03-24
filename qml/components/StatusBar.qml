import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "IconRegistry.js" as IconRegistry

Rectangle {
    id: root
    Layout.fillWidth: true
    Layout.preferredHeight: 22
    color: WorkbenchTheme.accent

    property var mainVM
    property var hubVM

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Left section
        RowLayout {
            Layout.fillHeight: true
            spacing: 0

            StatusBarItem {
                iconName: "git"
                text: mainVM.gitBranchLabel
                visible: mainVM.gitBranchLabel !== ""
                onClicked: mainVM.primaryViewIndex = 2
            }

            StatusBarItem {
                text: mainVM.gitChangeCount + " changes"
                visible: mainVM.gitChangeCount > 0
                display: AbstractButton.TextOnly
                onClicked: mainVM.primaryViewIndex = 2
            }

            StatusBarItem {
                iconName: "analyze"
                text: mainVM.diagnosticsStatusLine
                visible: mainVM.diagnosticsStatusLine.length > 0
                onClicked: {
                    mainVM.bottomPanelTab = 0
                    mainVM.bottomPanelHeight = WorkbenchTheme.defaultBottomPanelHeight
                }
            }

            StatusBarItem {
                text: mainVM.statusMessage
                Layout.maximumWidth: 400
                display: AbstractButton.TextOnly
            }
        }

        Item { Layout.fillWidth: true }

        // Right section
        RowLayout {
            Layout.fillHeight: true
            spacing: 0

            StatusBarItem {
                text: "Ln " + mainVM.cursorLine + ", Col " + mainVM.cursorColumn
                visible: mainVM.currentPath !== ""
            }

            StatusBarItem {
                text: mainVM.openEditorCount + " tabs"
                display: AbstractButton.TextOnly
            }

            StatusBarItem {
                iconName: "split"
                text: mainVM.splitEditorVisible ? (mainVM.diffEditorVisible ? "diff" : "split") : "single"
            }

            StatusBarItem {
                iconName: "auto_profile"
                text: hubVM.hardwareSummary
                onClicked: mainVM.showModelsSidebar()
            }

            StatusBarItem {
                iconName: "assistant"
                text: mainVM.aiBackendName + " · " + hubVM.providerName
                onClicked: mainVM.showAssistantSidebar()
            }
        }
    }

    component StatusBarItem : Button {
        id: item
        property string iconName: ""

        Layout.fillHeight: true
        flat: true
        hoverEnabled: true
        leftPadding: 8
        rightPadding: 8
        topPadding: 0
        bottomPadding: 0
        display: AbstractButton.TextBesideIcon
        spacing: 4

        icon.source: iconName.length > 0 ? IconRegistry.source(iconName) : ""
        icon.width: 14
        icon.height: 14
        icon.color: "white"

        font.pixelSize: 11
        font.family: WorkbenchTheme.uiFont

        background: Rectangle {
            color: item.hovered ? Qt.rgba(1, 1, 1, 0.12) : "transparent"
        }

        contentItem: Row {
            spacing: item.spacing
            Image {
                source: item.icon.source
                visible: item.icon.source.toString().length > 0
                sourceSize.width: item.icon.width
                sourceSize.height: item.icon.height
                smooth: true
            }
            Label {
                text: item.text
                color: "white"
                font: item.font
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}
