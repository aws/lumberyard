import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1

import AssetProcessor 1.0



Item
{
    id: rootElement

    Label {
        id: jobStartedLabel
        text: qsTr("Number of Job Started:")
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.top: parent.top
        anchors.topMargin: 10
        font.family: "Helvetica"
        font.pointSize: 8
    }

    Label {
        id: jobStartedNumber
        text: shaderCompilerManager.numberOfJobsStarted
        anchors.right:parent.right
        anchors.rightMargin: 100
        anchors.top: jobStartedLabel.top
        font.family: "Helvetica"
        font.pointSize: 8
    }

    Label {
        id: jobEndedLabel
        text: qsTr("Number of Job Ended:")
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.top: jobStartedLabel.bottom
        anchors.topMargin: 10
        font.family: "Helvetica"
        font.pointSize: 8
    }

    Label {
        id: jobEndedNumber
        text: shaderCompilerManager.numberOfJobsEnded
        anchors.right:parent.right
        anchors.rightMargin: 100
        anchors.top: jobEndedLabel.top
        font.family: "Helvetica"
        font.pointSize: 8
    }

    Label {
        id: jobErrorLabel
        text: qsTr("Number of Job Failed:")
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.top: jobEndedLabel.bottom
        anchors.topMargin: 10
        font.family: "Helvetica"
        font.pointSize: 8
    }

    Label {
        id: jobErrorNumber
        text: shaderCompilerManager.numberOfErrors
        anchors.right:parent.right
        anchors.rightMargin: 100
        anchors.top: jobErrorLabel.top
        font.family: "Helvetica"
        font.pointSize: 8
    }

    Label {
        id: activityLog
        color: "#333333"
        text: qsTr("Error Log")
        font.bold: true
        font.pointSize: 12
        font.family:"Arial"
        anchors.top: jobErrorLabel.bottom
        anchors.left: jobErrorLabel.left
        anchors.topMargin: 10

    }

    TableView {
       id: tableView
       anchors.top: activityLog.bottom
       anchors.topMargin: 10
       anchors.right: parent.right
       anchors.rightMargin: 10
       anchors.left: activityLog.left
       anchors.bottom: parent.bottom
       anchors.bottomMargin: 50
       TableViewColumn{
           id: firstColumn
           role: "timestamp"
           title: qsTr("TimeStamp")
           onWidthChanged: tableView.resizeWidthOfColumn()
           movable: false
       }
       TableViewColumn {
           id: secondColumn
           role: "server"
           title: qsTr("Server")
           onWidthChanged: tableView.resizeWidthOfColumn()
           movable: false
       }

       TableViewColumn {
           id: thirdColumn
           role: "error"
           title: qsTr("Error")
           movable: false

       }
       model: shaderCompilerModel
       selectionMode: SelectionMode.SingleSelection

       onWidthChanged: {
           resizeWidthOfColumn()
       }

       function resizeWidthOfColumn()
       {
           var finalWidth = width - (firstColumn.width + secondColumn.width + 2);
           if (finalWidth < 50)
               finalWidth = 50

           thirdColumn.width = finalWidth
       }
    }

    Button {
        id: copyToClipboardButton
        width: 200
        text:qsTr("Copy Shader Request To Clipboard")
        anchors.left: tableView.left
        anchors.top: tableView.bottom
        anchors.topMargin: 5
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 5
        visible: true
        onClicked: {
            tableView.selection.forEach( function(rowIndex) {shaderCompilerModel.copyToClipboard(rowIndex)});
        }
    }
}
