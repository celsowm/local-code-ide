import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LocalCodeIDE.Highlighting 1.0
import "IconRegistry.js" as IconRegistry

Rectangle {
    id: root
    color: WorkbenchTheme.editorBackground
    border.color: WorkbenchTheme.borderColor

    property int completionSelectedIndex: 0
    property bool completionPopupDismissed: false

    function cursorLine(text, pos) {
        const head = text.substring(0, pos)
        return head.split("\n").length
    }

    function cursorColumn(text, pos) {
        const head = text.substring(0, pos)
        const idx = head.lastIndexOf("\n")
        return idx === -1 ? head.length + 1 : head.length - idx
    }

    function lineNumbers(text) {
        const count = Math.max(1, text.split("\n").length)
        const lines = []
        for (let i = 1; i <= count; ++i) {
            lines.push(i)
        }
        return lines.join("\n")
    }

    function displayPath(path, rootPath) {
        if (!path || path.length === 0) {
            return "No file selected"
        }
        const normalizedPath = path.replace(/\\/g, "/")
        const normalizedRoot = (rootPath || "").replace(/\\/g, "/")
        if (normalizedRoot.length > 0 && normalizedPath.indexOf(normalizedRoot) === 0) {
            let relative = normalizedPath.substring(normalizedRoot.length)
            if (relative.startsWith("/")) {
                relative = relative.substring(1)
            }
            if (relative.length > 0) {
                return relative
            }
        }
        return normalizedPath
    }

    function dismissCompletionPopup() {
        completionPopupDismissed = true
        completionPopup.close()
    }

    function applyCurrentCompletion() {
        if (!completionPopup.visible || completionList.count <= 0) {
            return
        }
        const boundedIndex = Math.max(0, Math.min(completionSelectedIndex, completionList.count - 1))
        completionList.currentIndex = boundedIndex
        if (completionList.currentItem && completionList.currentItem.insertValue !== undefined) {
            mainViewModel.applyCompletionInsert(completionList.currentItem.insertValue)
            dismissCompletionPopup()
        }
    }

    Connections {
        target: mainViewModel

        function onCompletionsChanged() {
            completionPopupDismissed = false
            completionSelectedIndex = 0
            if (!mainViewModel.diffEditorVisible && mainViewModel.completionCount > 0) {
                completionPopup.open()
            } else {
                completionPopup.close()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: WorkbenchTheme.compactHeaderHeight
            color: WorkbenchTheme.editorHeaderBackground
            border.color: WorkbenchTheme.borderColor

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 8
                spacing: 10

                Label {
                    text: root.displayPath(mainViewModel.currentPath, mainViewModel.workspaceRootPath)
                    color: WorkbenchTheme.textPrimary
                    font.pixelSize: 12
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                }

                Label {
                    text: mainViewModel.languageId.toUpperCase()
                    color: WorkbenchTheme.textMuted
                    font.pixelSize: 11
                }

                Label {
                    text: mainViewModel.currentDocumentDirty ? "UNSAVED" : "SAVED"
                    color: mainViewModel.currentDocumentDirty ? WorkbenchTheme.warning : WorkbenchTheme.success
                    font.pixelSize: 11
                }

                ToolButton {
                    display: AbstractButton.IconOnly
                    icon.source: IconRegistry.source("complete")
                    icon.width: 16
                    icon.height: 16
                    icon.color: WorkbenchTheme.textPrimary
                    hoverEnabled: true
                    padding: 4
                    background: Rectangle { color: parent.hovered ? "#2a2d2e" : "transparent"; radius: 3 }
                    ToolTip.visible: hovered
                    ToolTip.text: "Trigger Completion"
                    onClicked: mainViewModel.requestCompletionsAtCursor()
                }

                ToolButton {
                    display: AbstractButton.IconOnly
                    icon.source: IconRegistry.source("split")
                    icon.width: 16
                    icon.height: 16
                    icon.color: WorkbenchTheme.textPrimary
                    hoverEnabled: true
                    padding: 4
                    background: Rectangle { color: parent.hovered ? "#2a2d2e" : "transparent"; radius: 3 }
                    ToolTip.visible: hovered
                    ToolTip.text: "Split Editor Right"
                    onClicked: mainViewModel.openWorkspaceFileInSplit(mainViewModel.currentPath)
                }

                ToolButton {
                    display: AbstractButton.IconOnly
                    visible: mainViewModel.splitEditorVisible && !mainViewModel.diffEditorVisible
                    icon.source: IconRegistry.source("apply_patch")
                    icon.width: 16
                    icon.height: 16
                    icon.color: WorkbenchTheme.textPrimary
                    hoverEnabled: true
                    padding: 4
                    background: Rectangle { color: parent.hovered ? "#2a2d2e" : "transparent"; radius: 3 }
                    ToolTip.visible: hovered
                    ToolTip.text: "Save Secondary Editor"
                    onClicked: mainViewModel.saveSecondaryEditor()
                }

                ToolButton {
                    display: AbstractButton.IconOnly
                    enabled: mainViewModel.hasPatchPreview
                    icon.source: IconRegistry.source("diff")
                    icon.width: 16
                    icon.height: 16
                    icon.color: enabled ? WorkbenchTheme.textPrimary : WorkbenchTheme.textDim
                    hoverEnabled: true
                    padding: 4
                    background: Rectangle { color: parent.hovered ? "#2a2d2e" : "transparent"; radius: 3 }
                    ToolTip.visible: hovered
                    ToolTip.text: "Open Patch Diff"
                    onClicked: mainViewModel.openPatchPreviewDiff()
                }

                ToolButton {
                    display: AbstractButton.IconOnly
                    enabled: mainViewModel.canApplyPatchPreview
                    icon.source: IconRegistry.source("apply_patch")
                    icon.width: 16
                    icon.height: 16
                    icon.color: enabled ? WorkbenchTheme.textPrimary : WorkbenchTheme.textDim
                    hoverEnabled: true
                    padding: 4
                    background: Rectangle { color: parent.hovered ? "#2a2d2e" : "transparent"; radius: 3 }
                    ToolTip.visible: hovered
                    ToolTip.text: "Apply Patch Preview"
                    onClicked: mainViewModel.applyAssistantPatch()
                }

                ToolButton {
                    display: AbstractButton.IconOnly
                    visible: mainViewModel.splitEditorVisible
                    icon.source: IconRegistry.source("clear_logs")
                    icon.width: 16
                    icon.height: 16
                    icon.color: WorkbenchTheme.textPrimary
                    hoverEnabled: true
                    padding: 4
                    background: Rectangle { color: parent.hovered ? "#2a2d2e" : "transparent"; radius: 3 }
                    ToolTip.visible: hovered
                    ToolTip.text: "Close Split"
                    onClicked: mainViewModel.closeSplitEditor()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: WorkbenchTheme.tabHeight
            visible: !mainViewModel.showWelcomeTab
            color: WorkbenchTheme.editorHeaderBackground
            border.color: WorkbenchTheme.borderColor

            ListView {
                anchors.fill: parent
                orientation: ListView.Horizontal
                spacing: 1
                clip: true
                model: mainViewModel.openEditorsModel

                delegate: Rectangle {
                    width: Math.max(120, titleLabel.implicitWidth + 44)
                    height: WorkbenchTheme.tabHeight
                    color: active ? WorkbenchTheme.editorTabActive : WorkbenchTheme.editorTabInactive
                    border.color: active ? WorkbenchTheme.accent : WorkbenchTheme.borderColor

                    MouseArea {
                        anchors.fill: parent
                        onClicked: mainViewModel.switchOpenEditor(path)
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 6
                        spacing: 6

                        Label {
                            id: titleLabel
                            text: dirty ? title + " •" : title
                            color: WorkbenchTheme.textPrimary
                            elide: Text.ElideRight
                            font.pixelSize: 12
                            Layout.fillWidth: true
                        }

                        ToolButton {
                            text: "x"
                            font.pixelSize: 11
                            hoverEnabled: true
                            padding: 2
                            background: Rectangle { color: parent.hovered ? "#3a3d41" : "transparent"; radius: 2 }
                            onClicked: mainViewModel.closeOpenEditor(path)
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 22
            visible: mainViewModel.splitEditorVisible && !mainViewModel.showWelcomeTab
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: WorkbenchTheme.editorHeaderBackground
                border.color: WorkbenchTheme.borderColor

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    text: mainViewModel.diffEditorVisible ? ("ORIGINAL · " + mainViewModel.diffEditorTitle) : (mainViewModel.editorTabTitle + (mainViewModel.currentDocumentDirty ? " •" : ""))
                    color: WorkbenchTheme.textMuted
                    font.pixelSize: 11
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: WorkbenchTheme.editorHeaderBackground
                border.color: WorkbenchTheme.borderColor

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    text: mainViewModel.diffEditorVisible ? ("MODIFIED · " + mainViewModel.diffEditorTitle) : (mainViewModel.secondaryEditorTitle + (mainViewModel.secondaryEditorDirty ? " •" : ""))
                    color: WorkbenchTheme.textMuted
                    font.pixelSize: 11
                }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: mainViewModel.showWelcomeTab ? 2 : (mainViewModel.diffEditorVisible ? 1 : 0)

            Item {
                SplitView {
                    anchors.fill: parent
                    orientation: Qt.Horizontal

                    Rectangle {
                        id: primaryEditorContainer
                        SplitView.fillWidth: true
                        color: WorkbenchTheme.editorBackground

                        RowLayout {
                            anchors.fill: parent
                            spacing: 0

                            Rectangle {
                                Layout.preferredWidth: WorkbenchTheme.gutterWidth
                                Layout.fillHeight: true
                                color: WorkbenchTheme.editorGutterBackground
                                border.color: WorkbenchTheme.borderColor

                                Flickable {
                                    anchors.fill: parent
                                    clip: true
                                    interactive: false
                                    boundsBehavior: Flickable.StopAtBounds
                                    contentHeight: lineNumberLabel.implicitHeight
                                    contentY: (editor && typeof editor.contentY === "number") ? editor.contentY : 0

                                    Label {
                                        id: lineNumberLabel
                                        width: parent.width
                                        text: root.lineNumbers(editor.text)
                                        color: WorkbenchTheme.editorGutterText
                                        font.family: WorkbenchTheme.monoFont
                                        font.pixelSize: 13
                                        horizontalAlignment: Text.AlignRight
                                        topPadding: 8
                                        rightPadding: 10
                                    }
                                }
                            }

                            ScrollView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true

                                TextArea {
                                    id: editor
                                    text: mainViewModel.editorText
                                    color: WorkbenchTheme.textPrimary
                                    selectionColor: WorkbenchTheme.selection
                                    selectedTextColor: "#ffffff"
                                    wrapMode: TextArea.NoWrap
                                    font.family: WorkbenchTheme.monoFont
                                    font.pixelSize: 14
                                    persistentSelection: true
                                    background: Rectangle { color: WorkbenchTheme.editorBackground }

                                    onTextChanged: mainViewModel.editorText = text
                                    onCursorPositionChanged: {
                                        mainViewModel.setCursorPosition(root.cursorLine(text, cursorPosition), root.cursorColumn(text, cursorPosition))
                                    }

                                    Keys.onPressed: function(event) {
                                        if (!completionPopup.visible) {
                                            return
                                        }
                                        if (event.key === Qt.Key_Down) {
                                            completionSelectedIndex = Math.min(completionList.count - 1, completionSelectedIndex + 1)
                                            completionList.currentIndex = completionSelectedIndex
                                            completionList.positionViewAtIndex(completionSelectedIndex, ListView.Visible)
                                            event.accepted = true
                                        } else if (event.key === Qt.Key_Up) {
                                            completionSelectedIndex = Math.max(0, completionSelectedIndex - 1)
                                            completionList.currentIndex = completionSelectedIndex
                                            completionList.positionViewAtIndex(completionSelectedIndex, ListView.Visible)
                                            event.accepted = true
                                        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Tab) {
                                            root.applyCurrentCompletion()
                                            event.accepted = true
                                        } else if (event.key === Qt.Key_Escape) {
                                            root.dismissCompletionPopup()
                                            event.accepted = true
                                        }
                                    }
                                }
                            }
                        }

                        Popup {
                            id: completionPopup
                            parent: primaryEditorContainer
                            x: WorkbenchTheme.gutterWidth + 8
                            y: Math.max(8, primaryEditorContainer.height - height - 10)
                            width: Math.max(260, Math.min(primaryEditorContainer.width - WorkbenchTheme.gutterWidth - 16, 560))
                            height: Math.min(240, completionList.contentHeight + 8)
                            modal: false
                            focus: true
                            padding: 0
                            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside | Popup.CloseOnPressOutsideParent

                            background: Rectangle {
                                color: WorkbenchTheme.elevatedBackground
                                border.color: WorkbenchTheme.subtleBorderColor
                                radius: 4
                            }

                            onClosed: completionSelectedIndex = 0

                            ListView {
                                id: completionList
                                anchors.fill: parent
                                anchors.margins: 4
                                clip: true
                                model: mainViewModel.completionModel
                                currentIndex: completionSelectedIndex
                                boundsBehavior: Flickable.StopAtBounds

                                delegate: Rectangle {
                                    required property string label
                                    required property string detail
                                    required property string insertText
                                    property string insertValue: insertText

                                    width: ListView.view.width
                                    height: 36
                                    color: ListView.isCurrentItem ? WorkbenchTheme.selection : (mouseArea.containsMouse ? "#2a2d2e" : "transparent")

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 8
                                        anchors.rightMargin: 8
                                        anchors.topMargin: 4
                                        anchors.bottomMargin: 4
                                        spacing: 1

                                        Label {
                                            text: label
                                            color: WorkbenchTheme.textPrimary
                                            font.pixelSize: 12
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }

                                        Label {
                                            text: detail
                                            color: WorkbenchTheme.textMuted
                                            font.pixelSize: 10
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                    }

                                    MouseArea {
                                        id: mouseArea
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: {
                                            completionSelectedIndex = index
                                            completionList.currentIndex = index
                                            mainViewModel.applyCompletionInsert(insertText)
                                            root.dismissCompletionPopup()
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        visible: mainViewModel.splitEditorVisible
                        SplitView.preferredWidth: mainViewModel.splitEditorVisible ? width : 0
                        SplitView.fillWidth: mainViewModel.splitEditorVisible
                        color: WorkbenchTheme.editorAltBackground

                        ScrollView {
                            anchors.fill: parent
                            clip: true

                            TextArea {
                                id: secondaryEditor
                                text: mainViewModel.secondaryEditorText
                                color: WorkbenchTheme.textPrimary
                                selectionColor: WorkbenchTheme.selection
                                selectedTextColor: "#ffffff"
                                wrapMode: TextArea.NoWrap
                                font.family: WorkbenchTheme.monoFont
                                font.pixelSize: 14
                                persistentSelection: true
                                background: Rectangle { color: WorkbenchTheme.editorAltBackground }
                                onTextChanged: mainViewModel.secondaryEditorText = text
                            }
                        }
                    }
                }

                DocumentHighlighter {
                    textDocument: editor.textDocument
                    language: mainViewModel.languageId
                }

                DocumentHighlighter {
                    textDocument: secondaryEditor.textDocument
                    language: mainViewModel.secondaryLanguageId
                }
            }

            Item {
                SplitView {
                    anchors.fill: parent
                    orientation: Qt.Horizontal

                    ScrollView {
                        SplitView.fillWidth: true
                        clip: true

                        TextArea {
                            id: originalArea
                            readOnly: true
                            text: mainViewModel.diffOriginalText
                            color: WorkbenchTheme.textPrimary
                            wrapMode: TextArea.NoWrap
                            font.family: WorkbenchTheme.monoFont
                            font.pixelSize: 14
                            background: Rectangle { color: WorkbenchTheme.editorAltBackground }
                        }
                    }

                    ScrollView {
                        SplitView.fillWidth: true
                        clip: true

                        TextArea {
                            id: modifiedArea
                            readOnly: true
                            text: mainViewModel.diffModifiedText
                            color: WorkbenchTheme.textPrimary
                            wrapMode: TextArea.NoWrap
                            font.family: WorkbenchTheme.monoFont
                            font.pixelSize: 14
                            background: Rectangle { color: WorkbenchTheme.editorBackground }
                        }
                    }
                }

                DocumentHighlighter {
                    textDocument: originalArea.textDocument
                    language: mainViewModel.languageId
                }

                DocumentHighlighter {
                    textDocument: modifiedArea.textDocument
                    language: mainViewModel.languageId
                }
            }

            Item {
                Rectangle {
                    anchors.fill: parent
                    color: WorkbenchTheme.editorBackground

                    Flickable {
                        anchors.fill: parent
                        contentWidth: width
                        contentHeight: welcomeColumn.implicitHeight + 48
                        clip: true

                        ColumnLayout {
                            id: welcomeColumn
                            width: Math.min(parent.width - 80, 920)
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.top: parent.top
                            anchors.topMargin: 40
                            spacing: 16

                            Label {
                                text: "Welcome to LocalCodeIDE"
                                color: WorkbenchTheme.textPrimary
                                font.pixelSize: 32
                                font.bold: true
                                Layout.fillWidth: true
                            }

                            Label {
                                text: "Open your project folder and start coding."
                                color: WorkbenchTheme.textMuted
                                font.pixelSize: 14
                                Layout.fillWidth: true
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10

                                Button {
                                    text: "Open Folder..."
                                    onClicked: mainViewModel.triggerOpenFolderDialog()
                                }

                                Button {
                                    text: "Open File..."
                                    onClicked: mainViewModel.triggerOpenFileDialog()
                                }

                                Button {
                                    text: "New File"
                                    onClicked: mainViewModel.createWorkspaceFile()
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10

                                Button {
                                    text: "Quick Open"
                                    onClicked: mainViewModel.triggerQuickOpen()
                                }

                                Button {
                                    text: "Open Sample C++"
                                    onClicked: mainViewModel.loadSampleCpp()
                                }

                                Button {
                                    text: "Open Sample Rust"
                                    onClicked: mainViewModel.loadSampleRust()
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: 1
                                color: WorkbenchTheme.borderColor
                            }

                            Label {
                                text: "Recent Folders"
                                color: WorkbenchTheme.textPrimary
                                font.pixelSize: 16
                                font.bold: true
                                Layout.fillWidth: true
                            }

                            Repeater {
                                model: mainViewModel.recentFolders

                                delegate: Rectangle {
                                    required property string modelData
                                    Layout.fillWidth: true
                                    implicitHeight: 38
                                    radius: 4
                                    color: recentFolderMouse.containsMouse ? WorkbenchTheme.editorTabInactive : "transparent"
                                    border.color: WorkbenchTheme.borderColor

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 10
                                        anchors.rightMargin: 10
                                        spacing: 8

                                        Label {
                                            text: modelData
                                            color: WorkbenchTheme.textPrimary
                                            elide: Text.ElideMiddle
                                            Layout.fillWidth: true
                                        }

                                        Button {
                                            text: "Open"
                                            onClicked: mainViewModel.reopenRecentFolder(modelData)
                                        }
                                    }

                                    MouseArea {
                                        id: recentFolderMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        z: -1
                                        onClicked: mainViewModel.reopenRecentFolder(modelData)
                                    }
                                }
                            }

                            Label {
                                visible: mainViewModel.recentFolders.length === 0
                                text: "No recent folders yet."
                                color: WorkbenchTheme.textMuted
                                font.pixelSize: 12
                            }
                        }
                    }
                }
            }
        }
    }
}
