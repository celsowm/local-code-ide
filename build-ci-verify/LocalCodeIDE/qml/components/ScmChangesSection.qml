import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Column {
    id: root

    property var viewModel
    property string title: ""
    property int count: 0
    property var sectionModel

    spacing: 0

    ScmSectionHeader {
        width: parent.width
        title: root.title
        count: root.count
    }

    ListView {
        width: parent.width
        implicitHeight: contentHeight
        interactive: false
        spacing: 1
        model: root.sectionModel

        delegate: Item {
            required property string path
            required property string fileName
            required property string directory
            required property string statusCode
            required property string statusText
            required property string changeKind
            required property bool staged
            required property bool renamed
            required property bool untracked
            required property bool hasIndexChanges
            required property bool hasWorkingTreeChanges

            width: root.width
            height: row.implicitHeight

            ScmChangeRow {
                id: row
                anchors.left: parent.left
                anchors.right: parent.right
                viewModel: root.viewModel
                path: parent.path
                fileName: parent.fileName
                directory: parent.directory
                statusCode: parent.statusCode
                statusText: parent.statusText
                changeKind: parent.changeKind
                staged: parent.staged
                renamed: parent.renamed
                untracked: parent.untracked
                hasIndexChanges: parent.hasIndexChanges
                hasWorkingTreeChanges: parent.hasWorkingTreeChanges
            }
        }
    }
}
