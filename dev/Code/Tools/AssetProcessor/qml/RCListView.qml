import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1

import AssetProcessor 1.0



ColumnLayout {
    id: outerContainer
    spacing: 4

    GroupBox {
        id: tableContainer
        title: qsTr("RC Jobs")
        Layout.fillWidth: true
        Layout.fillHeight: true

        TableView {
            id: tableView

            model: QMLProxyModel {
                source: rcQueueModel

                sortOrder: tableView.sortIndicatorOrder
                sortCaseSensitivity: Qt.CaseInsensitive
                sortRole: tableView.getColumn(tableView.sortIndicatorColumn).role

                filterString: searchBox.text === "" ? "" : "*" + searchBox.text + "*"
                filterSyntax: QMLProxyModel.Wildcard
                filterCaseSensitivity: Qt.CaseInsensitive

            }

            anchors.fill: parent

            sortIndicatorColumn: 2
            sortIndicatorOrder: Qt.DescendingOrder
            sortIndicatorVisible: true

            TableViewColumn {
                role: "jobID"
                title: qsTr("Job ID")
                width: 40
            }
            TableViewColumn {
                role: "state"
                title: qsTr("State")
                width: 60
            }
            TableViewColumn {
                role: "timeCreated"
                title: qsTr("Created")
                width: 80
            }
            TableViewColumn {
                role: "timeLaunched"
                title: qsTr("Launched")
                width: 80
            }
            TableViewColumn {
                role: "timeCompleted"
                title: qsTr("Completed")
                width: 80
            }
            TableViewColumn {
                role: "exitCode"
                title: qsTr("Exit Code")
                width: 60
            }
            TableViewColumn {
                role: "command"
                title: qsTr("Command")
                width: 220
            }
        }
    }

    RowLayout {
        id: buttonsContainer
        Layout.fillWidth: true
        spacing: 4

        TextField {
            id: searchBox

            Layout.fillWidth: true
            placeholderText: qsTr("Search...")
            //anchors.verticalCenter: parent.verticalCenter
            //anchors.right: parent.right
        }
    }
}
