import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    implicitHeight: outputColumn.implicitHeight

    ColumnLayout {
        id: outputColumn
        anchors.fill: parent
        spacing: 8

        Label {
            text: "ASSISTANT OUTPUT"
            color: "#d4d4d4"
            font.bold: true
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Vertical

            ScrollView {
                SplitView.fillWidth: true
                SplitView.fillHeight: true

                TextArea {
                    readOnly: true
                    text: mainViewModel.chatResponse
                    color: "#d4d4d4"
                    wrapMode: TextArea.Wrap
                    background: Rectangle { color: "#1e1e1e"; radius: 6 }
                }
            }

            AssistantCard {
                SplitView.fillWidth: true
                SplitView.preferredHeight: 170
                surfaceColor: "#181818"
                outlineColor: mainViewModel.canApplyPatchPreview ? "#6a9955" : "#2d2d30"
                cornerRadius: 6
                padding: 8
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: "PATCH PREVIEW"
                        color: "#d4d4d4"
                        font.bold: true
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: mainViewModel.patchSummary
                        color: mainViewModel.canApplyPatchPreview ? "#6a9955" : "#808080"
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignRight
                    }
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    TextArea {
                        readOnly: true
                        text: mainViewModel.patchPreviewText
                        color: "#d4d4d4"
                        font.family: "monospace"
                        wrapMode: TextArea.NoWrap
                        background: Rectangle { color: "#111111"; radius: 4 }
                    }
                }
            }
        }
    }
}
