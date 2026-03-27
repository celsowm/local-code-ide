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
    readonly property bool hasMainVM: !!mainVM
    readonly property bool hasHubVM: !!hubVM

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Left section
        RowLayout {
            Layout.fillHeight: true
            spacing: 0

            StatusBarItem {
                iconName: "git"
                text: hasMainVM ? mainVM.gitBranchLabel : ""
                visible: hasMainVM && mainVM.gitBranchLabel !== ""
                onClicked: if (hasMainVM) mainVM.primaryViewIndex = 2
            }

            StatusBarItem {
                text: hasMainVM ? (mainVM.gitChangeCount + " changes") : ""
                visible: hasMainVM && mainVM.gitChangeCount > 0
                display: AbstractButton.TextOnly
                onClicked: if (hasMainVM) mainVM.primaryViewIndex = 2
            }

            StatusBarItem {
                iconName: "analyze"
                text: hasMainVM ? mainVM.diagnosticsStatusLine : ""
                visible: hasMainVM && mainVM.diagnosticsStatusLine.length > 0
                onClicked: {
                    if (!hasMainVM) return
                    mainVM.bottomPanelTab = 0
                    mainVM.bottomPanelHeight = WorkbenchTheme.defaultBottomPanelHeight
                }
            }

            StatusBarItem {
                text: hasMainVM ? mainVM.statusMessage : ""
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
                text: hasMainVM ? ("Ln " + mainVM.cursorLine + ", Col " + mainVM.cursorColumn) : ""
                visible: hasMainVM && mainVM.currentPath !== ""
            }

            StatusBarItem {
                text: hasMainVM ? (mainVM.openEditorCount + " tabs") : "0 tabs"
                display: AbstractButton.TextOnly
            }

            StatusBarItem {
                iconName: "split"
                text: hasMainVM
                    ? (mainVM.splitEditorVisible ? (mainVM.diffEditorVisible ? "diff" : "split") : "single")
                    : "single"
            }

            StatusBarItem {
                iconName: "auto_profile"
                text: hasHubVM ? hubVM.hardwareSummary : ""
                onClicked: if (hasMainVM) mainVM.showModelsSidebar()
            }

            StatusBarItem {
                iconName: "assistant"
                text: hasMainVM && hasHubVM ? (mainVM.aiBackendName + " · " + hubVM.providerName) : ""
                onClicked: if (hasMainVM) mainVM.showAssistantSidebar()
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
