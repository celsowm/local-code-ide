import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    readonly property bool compact: width < 520
    readonly property bool discoverComplete: (modelHubViewModel.selectedRepoId || "").length > 0
    readonly property bool chooseComplete: (modelHubViewModel.selectedFilePath || "").length > 0
    readonly property bool downloadComplete: (modelHubViewModel.downloadedPath || "").length > 0 || !!modelHubViewModel.canUseSelectedAsCurrent
    readonly property bool activateComplete: !!modelHubViewModel.hasCurrentLocalModel

    QtObject {
        id: themeTokens

        readonly property int outerGap: 10
        readonly property int cardPadding: 10
        readonly property int radius: 8

        readonly property color pageColor: "#1f2024"
        readonly property color cardColor: "#23262b"
        readonly property color cardColorAlt: "#1f2227"
        readonly property color borderColor: "#343840"
        readonly property color textPrimary: "#eceff4"
        readonly property color textSecondary: "#b9c1cc"
        readonly property color textMuted: "#8b95a3"
        readonly property color accentColor: "#57a6ff"
        readonly property color successColor: "#7dce83"
        readonly property color warningColor: "#e0b45f"
    }

    function setStep(stepIndex) {
        modelHubViewModel.wizardStep = Math.max(0, Math.min(3, stepIndex))
    }

    function stepTitle(index) {
        if (index === 0)
            return "Discover"
        if (index === 1)
            return "Choose GGUF"
        if (index === 2)
            return "Download"
        return "Activate & Server"
    }

    function stepComplete(index) {
        if (index === 0)
            return discoverComplete
        if (index === 1)
            return chooseComplete
        if (index === 2)
            return downloadComplete
        return activateComplete
    }

    function stepWarning(index) {
        if (index === 1 && !discoverComplete)
            return "Pick a repository in Step 1 before choosing a GGUF file."
        if (index === 2 && !chooseComplete)
            return "Pick a GGUF file in Step 2 before starting a download."
        if (index === 3 && !downloadComplete)
            return "No local GGUF detected yet. Download one in Step 3 or select a file that already exists locally."
        return ""
    }

    readonly property Component activeStepComponent: {
        if (modelHubViewModel.wizardStep === 0)
            return discoverStep
        if (modelHubViewModel.wizardStep === 1)
            return chooseStep
        if (modelHubViewModel.wizardStep === 2)
            return downloadStep
        return activateServerStep
    }

    Rectangle {
        anchors.fill: parent
        color: themeTokens.pageColor
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: themeTokens.outerGap
        spacing: themeTokens.outerGap

        ModelHubCard {
            Layout.fillWidth: true
            cornerRadius: themeTokens.radius
            padding: themeTokens.cardPadding
            spacing: 6
            surfaceColor: themeTokens.cardColor
            outlineColor: themeTokens.borderColor

            Label {
                text: "MODEL HUB"
                color: themeTokens.textPrimary
                font.bold: true
            }

            Label {
                text: modelHubViewModel.providerName
                color: themeTokens.accentColor
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }

            Label {
                text: modelHubViewModel.statusMessage
                color: themeTokens.textSecondary
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }
        }

        Flow {
            id: chipsFlow
            Layout.fillWidth: true
            spacing: 8

            Repeater {
                model: 4

                delegate: ModelHubStepChip {
                    required property int index

                    stepIndex: index
                    title: root.stepTitle(index)
                    active: modelHubViewModel.wizardStep === index
                    done: root.stepComplete(index)
                    tokens: themeTokens
                    width: root.compact
                        ? chipsFlow.width
                        : Math.floor((chipsFlow.width - (chipsFlow.spacing * 3)) / 4)
                    onClicked: root.setStep(index)
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            visible: root.stepWarning(modelHubViewModel.wizardStep).length > 0
            color: "#3a3224"
            border.color: "#826232"
            radius: themeTokens.radius
            implicitHeight: warningText.implicitHeight + (themeTokens.cardPadding * 2)

            Label {
                id: warningText
                anchors.fill: parent
                anchors.margins: themeTokens.cardPadding
                text: root.stepWarning(modelHubViewModel.wizardStep)
                color: "#f2d7a2"
                wrapMode: Text.Wrap
            }
        }

        ScrollView {
            id: stepScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth

            Item {
                width: stepScroll.availableWidth
                implicitHeight: stepLoader.item ? stepLoader.item.implicitHeight : 0

                Loader {
                    id: stepLoader
                    anchors.left: parent.left
                    anchors.right: parent.right
                    sourceComponent: root.activeStepComponent
                }
            }
        }
    }

    Component {
        id: discoverStep

        ModelHubDiscoverStep {
            tokens: themeTokens
            compact: root.compact
        }
    }

    Component {
        id: chooseStep

        ModelHubChooseStep {
            tokens: themeTokens
            discoverComplete: root.discoverComplete
        }
    }

    Component {
        id: downloadStep

        ModelHubDownloadStep {
            tokens: themeTokens
            chooseComplete: root.chooseComplete
        }
    }

    Component {
        id: activateServerStep

        ModelHubActivateServerStep {
            tokens: themeTokens
        }
    }
}
