import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ModelHubCard {
    id: root

    property var tokens
    property bool compact: false

    cornerRadius: tokens.radius
    padding: tokens.cardPadding
    spacing: tokens.outerGap
    surfaceColor: tokens.cardColor
    outlineColor: tokens.borderColor

    Label {
        text: "Step 1: Discover"
        color: tokens.textPrimary
        font.bold: true
    }

    ModelHubCard {
        Layout.fillWidth: true
        cornerRadius: tokens.radius
        padding: tokens.cardPadding
        spacing: 8
        surfaceColor: tokens.cardColorAlt
        outlineColor: tokens.borderColor

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: "Local hardware"
                color: tokens.textPrimary
                font.bold: true
            }
            Item { Layout.fillWidth: true }
        }

        Flow {
            Layout.fillWidth: true
            spacing: 8

            IconButton {
                text: "Refresh"
                iconName: "refresh"
                onClicked: modelHubViewModel.refreshHardware()
            }

            IconButton {
                text: "Use auto profile"
                iconName: "auto_profile"
                onClicked: modelHubViewModel.applyDetectedProfile()
            }
        }

        Label {
            text: modelHubViewModel.hardwareSummary
            color: tokens.textSecondary
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }

        Label {
            text: modelHubViewModel.recommendationSummary
            color: tokens.successColor
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }
    }

    ModelHubCard {
        Layout.fillWidth: true
        cornerRadius: tokens.radius
        padding: tokens.cardPadding
        spacing: 8
        surfaceColor: tokens.cardColorAlt
        outlineColor: tokens.borderColor

        Label {
            text: "Repository search"
            color: tokens.textPrimary
            font.bold: true
        }

        Label {
            text: "Author"
            color: tokens.textMuted
        }

        TextField {
            Layout.fillWidth: true
            text: modelHubViewModel.author
            placeholderText: "bartowski"
            color: tokens.textPrimary
            background: Rectangle {
                color: "#171a1f"
                radius: 6
                border.color: tokens.borderColor
            }
            onTextChanged: modelHubViewModel.author = text
        }

        Label {
            text: "Search query"
            color: tokens.textMuted
        }

        TextField {
            Layout.fillWidth: true
            text: modelHubViewModel.searchQuery
            placeholderText: "Search GGUF repos (qwen, coder, phi, mistral...)"
            color: tokens.textPrimary
            background: Rectangle {
                color: "#171a1f"
                radius: 6
                border.color: tokens.borderColor
            }
            onTextChanged: modelHubViewModel.searchQuery = text
            onAccepted: modelHubViewModel.searchRepos()
        }

        IconButton {
            text: "Search"
            iconName: "search"
            onClicked: modelHubViewModel.searchRepos()
        }
    }

    ModelHubCard {
        Layout.fillWidth: true
        cornerRadius: tokens.radius
        padding: tokens.cardPadding
        spacing: 8
        surfaceColor: tokens.cardColorAlt
        outlineColor: tokens.borderColor

        Label {
            text: "Repositories (" + modelHubViewModel.repoCount + ")"
            color: tokens.textPrimary
            font.bold: true
        }

        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: 320
            clip: true
            model: modelHubViewModel.reposModel

            delegate: Rectangle {
                required property string repoId
                required property string pipelineTag
                required property int downloads
                required property int likes
                required property string lastModified

                width: ListView.view.width
                height: 76
                radius: 6

                readonly property bool selected: repoId === modelHubViewModel.selectedRepoId

                color: selected ? "#223244" : (hover.containsMouse ? "#2a2f36" : "transparent")
                border.color: selected ? tokens.accentColor : "transparent"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 2

                    Label {
                        text: repoId
                        color: tokens.accentColor
                        Layout.fillWidth: true
                        elide: Text.ElideMiddle
                    }

                    Label {
                        text: (pipelineTag || "unknown") + " | downloads " + downloads + " | likes " + likes
                        color: tokens.textMuted
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    Label {
                        text: lastModified
                        color: tokens.successColor
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                }

                MouseArea {
                    id: hover
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        modelHubViewModel.selectRepo(repoId)
                        if (modelHubViewModel.wizardStep < 1)
                            modelHubViewModel.wizardStep = 1
                    }
                }
            }
        }
    }
}
