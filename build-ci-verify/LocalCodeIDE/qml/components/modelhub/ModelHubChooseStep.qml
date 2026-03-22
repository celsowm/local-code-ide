import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ModelHubCard {
    id: root

    property var tokens
    property bool discoverComplete: false

    cornerRadius: tokens.radius
    padding: tokens.cardPadding
    spacing: tokens.outerGap
    surfaceColor: tokens.cardColor
    outlineColor: tokens.borderColor

    Label {
        text: "Step 2: Choose GGUF"
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
        enabled: root.discoverComplete

        Label {
            text: "Recommendation controls"
            color: tokens.textPrimary
            font.bold: true
        }

        Label {
            text: "Profile"
            color: tokens.textMuted
        }

        ComboBox {
            Layout.fillWidth: true
            model: ["balanced", "laptop", "quality", "coding"]
            currentIndex: Math.max(0, model.indexOf(modelHubViewModel.recommendationProfile))
            onActivated: modelHubViewModel.recommendationProfile = currentText
        }

        Label {
            text: "Quantization"
            color: tokens.textMuted
        }

        ComboBox {
            Layout.fillWidth: true
            model: modelHubViewModel.quantOptions
            currentIndex: {
                const idx = modelHubViewModel.quantOptions.indexOf(modelHubViewModel.quantFilter)
                return idx >= 0 ? idx : 0
            }
            onActivated: modelHubViewModel.quantFilter = currentText
        }

        IconButton {
            text: "Suggest"
            iconName: "suggest"
            onClicked: modelHubViewModel.suggestFile()
        }

        Label {
            text: modelHubViewModel.recommendationSummary
            color: tokens.successColor
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }

        Label {
            text: modelHubViewModel.helperText
            color: tokens.textMuted
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
            text: "GGUF files (" + modelHubViewModel.fileCount + ")"
            color: tokens.textPrimary
            font.bold: true
        }

        Label {
            text: modelHubViewModel.selectedRepoId.length > 0
                ? "Repository: " + modelHubViewModel.selectedRepoId
                : "No repository selected"
            color: tokens.textMuted
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }

        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: 360
            clip: true
            model: modelHubViewModel.filesModel
            enabled: root.discoverComplete

            delegate: Rectangle {
                required property string path
                required property string name
                required property string quantization
                required property string sizeLabel
                required property bool isSplit
                required property bool activeLocal
                required property bool recommended

                width: ListView.view.width
                height: 70
                radius: 6

                readonly property bool selected: path === modelHubViewModel.selectedFilePath

                color: selected ? "#223244" : (hover.containsMouse ? "#2a2f36" : "transparent")
                border.color: activeLocal
                    ? tokens.successColor
                    : (recommended ? tokens.warningColor : (selected ? tokens.accentColor : "transparent"))

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            text: name
                            color: tokens.textPrimary
                            Layout.fillWidth: true
                            elide: Text.ElideMiddle
                        }

                        Label {
                            text: quantization + " | " + sizeLabel + (isSplit ? " | split" : "") + (activeLocal ? " | active" : "")
                            color: activeLocal ? tokens.successColor : (recommended ? tokens.warningColor : tokens.textMuted)
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                    }

                    Button {
                        text: activeLocal ? "Active" : (recommended ? "Suggested" : "Select")
                        onClicked: {
                            modelHubViewModel.selectedFilePath = path
                            if (modelHubViewModel.wizardStep < 2)
                                modelHubViewModel.wizardStep = 2
                        }
                    }
                }

                MouseArea {
                    id: hover
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        modelHubViewModel.selectedFilePath = path
                        if (modelHubViewModel.wizardStep < 2)
                            modelHubViewModel.wizardStep = 2
                    }
                }
            }
        }
    }
}
