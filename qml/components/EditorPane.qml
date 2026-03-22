import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LocalCodeIDE.Highlighting 1.0

Rectangle {
    color: "#1e1e1e"
    border.color: "#2d2d30"

    function cursorLine(text, pos) {
        var head = text.substring(0, pos)
        return head.split("\n").length
    }

    function cursorColumn(text, pos) {
        var head = text.substring(0, pos)
        var idx = head.lastIndexOf("\n")
        return idx === -1 ? head.length + 1 : head.length - idx
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: "#252526"

            ListView {
                anchors.fill: parent
                anchors.leftMargin: 6
                anchors.rightMargin: 6
                orientation: ListView.Horizontal
                spacing: 4
                clip: true
                model: mainViewModel.openEditorsModel

                delegate: Rectangle {
                    width: Math.max(120, titleLabel.implicitWidth + 48)
                    height: 30
                    radius: 4
                    color: active ? "#1e1e1e" : "#2d2d30"
                    border.color: active ? "#007acc" : "transparent"

                    MouseArea { anchors.fill: parent; z: -1; onClicked: mainViewModel.switchOpenEditor(path) }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 6
                        spacing: 6
                        Label { id: titleLabel; text: dirty ? title + " •" : title; color: "#d4d4d4"; elide: Text.ElideRight; Layout.fillWidth: true }
                        Button { text: "×"; flat: true; onClicked: mainViewModel.closeOpenEditor(path) }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            color: "#202020"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 8
                Label { text: mainViewModel.languageId.toUpperCase(); color: "#808080" }
                Label { text: mainViewModel.currentDocumentDirty ? "unsaved" : "saved"; color: mainViewModel.currentDocumentDirty ? "#f48771" : "#73c991" }
                Label { text: mainViewModel.splitEditorVisible ? (mainViewModel.diffEditorVisible ? "DIFF" : "SPLIT") : "SINGLE"; color: "#9cdcfe" }
                Item { Layout.fillWidth: true }
                IconButton { text: "Split Right"; iconName: "split"; onClicked: mainViewModel.openWorkspaceFileInSplit(mainViewModel.currentPath) }
                IconButton { text: "Patch Diff"; iconName: "diff"; enabled: mainViewModel.hasPatchPreview; onClicked: mainViewModel.openPatchPreviewDiff() }
                IconButton { text: "Save Right"; iconName: "apply_patch"; visible: mainViewModel.splitEditorVisible && !mainViewModel.diffEditorVisible; onClicked: mainViewModel.saveSecondaryEditor() }
                IconButton { text: "Close Split"; iconName: "split"; visible: mainViewModel.splitEditorVisible; onClicked: mainViewModel.closeSplitEditor() }
                IconButton { text: "Analyze"; iconName: "analyze"; onClicked: mainViewModel.analyzeNow() }
                IconButton { text: "Complete"; iconName: "complete"; onClicked: mainViewModel.requestCompletionsAtCursor() }
                IconButton { text: "Hover"; iconName: "hover"; onClicked: mainViewModel.requestHoverAtCursor() }
                IconButton { text: "Definition"; iconName: "definition"; onClicked: mainViewModel.requestDefinitionAtCursor() }
                IconButton { text: "Apply Patch"; iconName: "apply_patch"; enabled: mainViewModel.canApplyPatchPreview; onClicked: mainViewModel.applyAssistantPatch() }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 24
            visible: mainViewModel.splitEditorVisible
            spacing: 0
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                color: "#252526"
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    text: mainViewModel.diffEditorVisible ? ("ORIGINAL · " + mainViewModel.diffEditorTitle) : (mainViewModel.editorTabTitle + (mainViewModel.currentDocumentDirty ? " •" : ""))
                    color: "#c5c5c5"
                }
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                color: "#252526"
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    text: mainViewModel.diffEditorVisible ? ("MODIFIED · " + mainViewModel.diffEditorTitle) : (mainViewModel.secondaryEditorTitle + (mainViewModel.secondaryEditorDirty ? " •" : ""))
                    color: "#c5c5c5"
                }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: mainViewModel.diffEditorVisible ? 1 : 0

            Item {
                SplitView {
                    anchors.fill: parent
                    orientation: Qt.Horizontal

                    Rectangle {
                        SplitView.fillWidth: true
                        color: "#1e1e1e"
                        RowLayout {
                            anchors.fill: parent
                            spacing: 0
                            Rectangle {
                                Layout.preferredWidth: 60
                                Layout.fillHeight: true
                                color: "#181818"
                                Flickable {
                                    anchors.fill: parent
                                    contentHeight: lineColumn.implicitHeight
                                    clip: true
                                    Label {
                                        id: lineColumn
                                        width: parent.width
                                        padding: 8
                                        color: "#858585"
                                        font.family: "monospace"
                                        text: {
                                            var count = mainViewModel.editorText.split("\n").length
                                            var lines = []
                                            for (var i = 1; i <= count; ++i)
                                                lines.push(i)
                                            return lines.join("\n")
                                        }
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
                                    color: "#d4d4d4"
                                    selectionColor: "#264f78"
                                    selectedTextColor: "#ffffff"
                                    wrapMode: TextArea.NoWrap
                                    font.family: "monospace"
                                    font.pixelSize: 14
                                    persistentSelection: true
                                    background: Rectangle { color: "#1e1e1e" }
                                    onTextChanged: mainViewModel.editorText = text
                                    onCursorPositionChanged: {
                                        mainViewModel.setCursorPosition(cursorLine(text, cursorPosition), cursorColumn(text, cursorPosition))
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        visible: mainViewModel.splitEditorVisible
                        SplitView.preferredWidth: mainViewModel.splitEditorVisible ? width : 0
                        SplitView.fillWidth: mainViewModel.splitEditorVisible
                        color: "#1b1b1b"
                        ScrollView {
                            anchors.fill: parent
                            clip: true
                            TextArea {
                                id: secondaryEditor
                                text: mainViewModel.secondaryEditorText
                                color: "#d4d4d4"
                                selectionColor: "#264f78"
                                selectedTextColor: "#ffffff"
                                wrapMode: TextArea.NoWrap
                                font.family: "monospace"
                                font.pixelSize: 14
                                persistentSelection: true
                                background: Rectangle { color: "#1b1b1b" }
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
                            color: "#d4d4d4"
                            wrapMode: TextArea.NoWrap
                            font.family: "monospace"
                            font.pixelSize: 14
                            background: Rectangle { color: "#1b1b1b" }
                        }
                    }

                    ScrollView {
                        SplitView.fillWidth: true
                        clip: true
                        TextArea {
                            id: modifiedArea
                            readOnly: true
                            text: mainViewModel.diffModifiedText
                            color: "#d4d4d4"
                            wrapMode: TextArea.NoWrap
                            font.family: "monospace"
                            font.pixelSize: 14
                            background: Rectangle { color: "#1e1e1e" }
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
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 170
            color: "#181818"
            border.color: "#2d2d30"
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "COMPLETIONS (" + mainViewModel.completionCount + ")"; color: "#d4d4d4"; font.bold: true }
                    Item { Layout.fillWidth: true }
                    Label { text: mainViewModel.hoverText; color: "#808080"; elide: Text.ElideRight; Layout.preferredWidth: 320 }
                }
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: mainViewModel.completionModel
                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 42
                        color: mouseArea.containsMouse ? "#2a2d2e" : "transparent"
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            anchors.topMargin: 4
                            anchors.bottomMargin: 4
                            Label { text: label; color: "#c5e478"; Layout.fillWidth: true; elide: Text.ElideRight }
                            Label { text: detail; color: "#808080"; Layout.fillWidth: true; elide: Text.ElideRight }
                        }
                        MouseArea { id: mouseArea; anchors.fill: parent; hoverEnabled: true; onClicked: mainViewModel.applyCompletionInsert(insertText) }
                    }
                }
            }
        }
    }
}
