import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    property var viewModel

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ScmHeader {
            Layout.fillWidth: true
            viewModel: root.viewModel
        }

        ScmCommitSection {
            Layout.fillWidth: true
            viewModel: root.viewModel
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Column {
                width: parent.width
                spacing: 10

                Item {
                    width: parent.width
                    height: (!root.viewModel || root.viewModel.gitChangeCount === 0) ? 92 : 0
                    visible: height > 0

                    Column {
                        anchors.centerIn: parent
                        spacing: 4

                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "No changes"
                            color: "#d4d4d4"
                            font.pixelSize: 14
                            font.bold: true
                        }

                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "Working tree is clean."
                            color: "#7f7f7f"
                            font.pixelSize: 11
                        }
                    }
                }

                Repeater {
                    model: root.viewModel ? root.viewModel.scmSectionsModel : null

                    delegate: ScmChangesSection {
                        width: parent.width
                        viewModel: root.viewModel
                        title: sectionTitle
                        count: sectionCount
                        sectionModel: sectionEntriesModel
                    }
                }

                ScmHistorySection {
                    width: parent.width
                    viewModel: root.viewModel
                }
            }
        }
    }
}
