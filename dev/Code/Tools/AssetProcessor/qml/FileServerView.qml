import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1
import AssetProcessor 1.0

ColumnLayout {
    id: outerContainer
    spacing: 4
    width: 400
    height: 400

    CheckBox {
        id: checkRealtimeMetrics
        width: 183
        height: 17
        text: qsTr("Realtime Metrics (Slower)")
        anchors.left: parent.left
        anchors.leftMargin: 100
        anchors.top: parent.top
        anchors.topMargin: 5
        clip: false
        checked: false
        onCheckedChanged: {
            fileServer.setRealTimeMetrics(checked)
        }
    }

    Text {
        id: rootFolderLabel;
        x: 21
        y: 50
        height: 20
        text: qsTr("Root Folder:")
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.top: parent.top
        anchors.topMargin: 27
        font.family: "Helvetica"
        font.pointSize: 8
    }

    Text {
        id:rootFolderText;
        height: 16
        color: "#000000"
        enabled: false
        text: fileServer.rootFolder
        anchors.verticalCenter: rootFolderLabel.verticalCenter
        font.italic: true
        font.pointSize: 10
        style: Text.Normal
        font.bold: false
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: rootFolderLabel.right
        anchors.leftMargin: 9
        anchors.top: parent.top
        anchors.topMargin: 24
        Layout.fillWidth: true
    }

    GroupBox {
        id: tableContainer
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 50
        clip: true
        title: qsTr("Server Metrics")
        Layout.fillWidth: true
        Layout.fillHeight: true

        Text {
            id: bytesSentLabel
            x: 188
            y: 7
            width: 79
            height: 20
            text: "Bytes Sent:"
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: bytesSent
            width: 130
            height: 20
            y:29
            text: fileServer.bytesSent
            anchors.verticalCenterOffset: 0
            anchors.verticalCenter: bytesSentLabel.verticalCenter
            horizontalAlignment: Text.AlignLeft
            anchors.left: bytesSentLabel.right
            anchors.leftMargin: 0
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: bytesReceivedLabel
            width: 102
            height: 20
            y:21
            text: qsTr("Bytes Received:")
            anchors.left: parent.left
            anchors.leftMargin: 0
            anchors.top: parent.top
            anchors.topMargin: 7
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: bytesReceived
            y: 29
            width: 94
            height: 20
            text: fileServer.bytesReceived
            anchors.verticalCenter: bytesReceivedLabel.verticalCenter
            anchors.left: bytesReceivedLabel.right
            anchors.leftMargin: 0
            horizontalAlignment: Text.AlignLeft
            font.family: "Helvetica"
            font.pointSize: 8
        }



        Text {
            id: readRequestsLabel
            width: 94
            height: 20
            text: qsTr("Read Requests:")
            anchors.top: openFileRequestsLabel.bottom
            anchors.topMargin: 6
            anchors.left: openFileRequestsLabel.right
            anchors.leftMargin: -121
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: readRequests
            width: 108
            height: 20
            y:50
            text: fileServer.numReadRequests
            anchors.verticalCenter: readRequestsLabel.verticalCenter
            horizontalAlignment: Text.AlignLeft
            anchors.left: readRequestsLabel.right
            anchors.leftMargin: 0
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: writeRequestsLabel
            width: 94
            height: 20
            y:50
            text: qsTr("Write Requests:")
            anchors.left: closeFileRequestsLabel.right
            anchors.leftMargin: -123
            anchors.top: closeFileRequestsLabel.bottom
            anchors.topMargin: 6
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: writeRequests
            width: 123
            height: 20
            y:50
            text: fileServer.numWriteRequests
            anchors.verticalCenter: writeRequestsLabel.verticalCenter
            anchors.left: writeRequestsLabel.right
            anchors.leftMargin: 1
            horizontalAlignment: Text.AlignLeft
            font.family: "Helvetica"
            font.pointSize: 8
        }



        Text {
            id: bytesWrittenLabel
            width: 85
            height: 20
            text: qsTr("Bytes Written:")
            anchors.top: bytesSentLabel.bottom
            anchors.topMargin: 6
            anchors.left: bytesSentLabel.right
            anchors.leftMargin: -79
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: bytesWritten
            width: 117
            height: 20
            y:50
            text: fileServer.bytesWritten
            anchors.verticalCenter: bytesWrittenLabel.verticalCenter
            horizontalAlignment: Text.AlignLeft
            anchors.left: bytesWrittenLabel.right
            anchors.leftMargin: 0
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: bytesReadLabel
            width: 78
            height: 20
            y:50
            text: qsTr("Bytes Read:")
            anchors.left: bytesReceivedLabel.right
            anchors.leftMargin: -102
            anchors.top: bytesReceivedLabel.bottom
            anchors.topMargin: 6
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: bytesRead
            width: 123
            height: 20
            text: fileServer.bytesRead
            anchors.verticalCenter: bytesReadLabel.verticalCenter
            y:50
            anchors.left: bytesReadLabel.right
            anchors.leftMargin: 1
            horizontalAlignment: Text.AlignLeft
            font.family: "Helvetica"
            font.pointSize: 8
        }



        Text {
            id: openedFilesLabel
            width: 85
            height: 20
            text: qsTr("Files Opened:")
            anchors.top: readRequestsLabel.bottom
            anchors.topMargin: 6
            anchors.left: readRequestsLabel.right
            anchors.leftMargin: -94
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: openedFiles
            width: 129
            height: 20
            y:50
            text: fileServer.numOpened
            anchors.verticalCenter: openedFilesLabel.verticalCenter
            horizontalAlignment: Text.AlignLeft
            anchors.left: openedFilesLabel.right
            anchors.leftMargin: 0
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: closedFilesLabel
            width: 78
            height: 20
            y:50
            text: qsTr("Files Closed:")
            anchors.left: writeRequestsLabel.right
            anchors.leftMargin: -94
            anchors.top: writeRequestsLabel.bottom
            anchors.topMargin: 6
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: closedFiles
            width: 123
            height: 20
            y:50
            text: fileServer.numClosed
            anchors.verticalCenter: closedFilesLabel.verticalCenter
            anchors.left: closedFilesLabel.right
            anchors.leftMargin: 1
            horizontalAlignment: Text.AlignLeft
            font.family: "Helvetica"
            font.pointSize: 8
        }



        Text {
            id: seekRequestLabel
            width: 97
            height: 20
            text: qsTr("Seek Requests:")
            anchors.top: openedFilesLabel.bottom
            anchors.topMargin: 32
            anchors.left: openedFilesLabel.right
            anchors.leftMargin: -85
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: seekRequests
            width: 105
            height: 20
            y:50
            text: fileServer.numSeekRequests
            anchors.verticalCenterOffset: 0
            anchors.verticalCenter: seekRequestLabel.verticalCenter
            horizontalAlignment: Text.AlignLeft
            anchors.left: seekRequestLabel.right
            anchors.leftMargin: 0
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: tellRequestsLabel
            width: 84
            height: 20
            y:50
            text: qsTr("Tell Requests:")
            anchors.left: openFilesLabel.right
            anchors.leftMargin: -67
            anchors.top: openFilesLabel.bottom
            anchors.topMargin: 6
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: tellRequests
            width: 123
            height: 20
            y:50
            text: fileServer.numTellRequests
            anchors.verticalCenter: tellRequestsLabel.verticalCenter
            anchors.left: tellRequestsLabel.right
            anchors.leftMargin: 1
            horizontalAlignment: Text.AlignLeft
            font.family: "Helvetica"
            font.pointSize: 8
        }



        Text {
            id: isReadOnlyRequestLabel
            width: 136
            height: 20
            text: qsTr("Is Read Only Requests:")
            anchors.top: seekRequestLabel.bottom
            anchors.topMargin: 6
            anchors.left: seekRequestLabel.right
            anchors.leftMargin: -97
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: isReadOnlyRequests
            width: 66
            height: 20
            y:50
            text: fileServer.numIsReadOnlyRequests
            anchors.verticalCenter: isReadOnlyRequestLabel.verticalCenter
            horizontalAlignment: Text.AlignLeft
            anchors.left: isReadOnlyRequestLabel.right
            anchors.leftMargin: 0
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: isDirectoryRequestsLabel
            width: 129
            height: 20
            y:50
            text: qsTr("Is Directory Requests:")
            anchors.left: tellRequestsLabel.right
            anchors.leftMargin: -84
            anchors.top: tellRequestsLabel.bottom
            anchors.topMargin: 6
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: isDirectoryRequests
            width: 123
            height: 20
            y:50
            text: fileServer.numIsDirectoryRequests
            anchors.verticalCenter: isDirectoryRequestsLabel.verticalCenter
            anchors.left: isDirectoryRequestsLabel.right
            anchors.leftMargin: 1
            horizontalAlignment: Text.AlignLeft
            font.family: "Helvetica"
            font.pointSize: 8
        }





        Text {
            id: sizeRequestLabel
            width: 88
            height: 20
            text: qsTr("Size Requests:")
            anchors.top: isReadOnlyRequestLabel.bottom
            anchors.topMargin: 6
            anchors.left: isReadOnlyRequestLabel.right
            anchors.leftMargin: -136
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: sizeRequests
            width: 114
            height: 20
            y:50
            text: fileServer.numSizeRequests
            anchors.verticalCenter: sizeRequestLabel.verticalCenter
            horizontalAlignment: Text.AlignLeft
            anchors.left: sizeRequestLabel.right
            anchors.leftMargin: 0
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: modtimeRequestsLabel
            width: 117
            height: 20
            y:50
            text: qsTr("ModTime Requests:")
            anchors.left: isDirectoryRequestsLabel.right
            anchors.leftMargin: -129
            anchors.top: isDirectoryRequestsLabel.bottom
            anchors.topMargin: 6
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: modtimeRequests
            width: 123
            height: 20
            y:50
            text: fileServer.numModificationTimeRequests
            anchors.verticalCenter: modtimeRequestsLabel.verticalCenter
            anchors.left: modtimeRequestsLabel.right
            anchors.leftMargin: 1
            horizontalAlignment: Text.AlignLeft
            font.family: "Helvetica"
            font.pointSize: 8
        }



        Text {
            id: eofRequestLabel
            width: 92
            height: 20
            text: qsTr("EOF Requests:")
            anchors.top: sizeRequestLabel.bottom
            anchors.topMargin: 6
            anchors.left: sizeRequestLabel.right
            anchors.leftMargin: -88
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: eofRequests
            width: 110
            height: 20
            y:50
            text: fileServer.numEofRequests
            anchors.verticalCenter: eofRequestLabel.verticalCenter
            horizontalAlignment: Text.AlignLeft
            anchors.left: eofRequestLabel.right
            anchors.leftMargin: 0
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: existsRequestsLabel
            width: 102
            height: 20
            y:50
            text: qsTr("Exists Requests:")
            anchors.left: modtimeRequestsLabel.right
            anchors.leftMargin: -117
            anchors.top: modtimeRequestsLabel.bottom
            anchors.topMargin: 6
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: existsRequests
            width: 123
            height: 20
            y:50
            text: fileServer.numExistsRequests
            anchors.verticalCenter: existsRequestsLabel.verticalCenter
            anchors.left: existsRequestsLabel.right
            anchors.leftMargin: 1
            horizontalAlignment: Text.AlignLeft
            font.family: "Helvetica"
            font.pointSize: 8
        }



        Text {
            id: flushRequestLabel
            width: 102
            height: 20
            y:50
            text: qsTr("Flush Requests:")
            anchors.left: existsRequestsLabel.right
            anchors.leftMargin: -102
            anchors.top: existsRequestsLabel.bottom
            anchors.topMargin: 6
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: flushRequests
            width: 129
            height: 20
            y:50
            text: fileServer.numFlushRequests
            anchors.verticalCenter: flushRequestLabel.verticalCenter
            horizontalAlignment: Text.AlignLeft
            anchors.left: flushRequestLabel.right
            anchors.leftMargin: 0
            font.family: "Helvetica"
            font.pointSize: 8
        }



        Text {
            id: createPathRequestsLabel
            width: 136
            height: 20
            text: qsTr("Create Path Requests:")
            anchors.top: eofRequestLabel.bottom
            anchors.topMargin: 32
            anchors.left: eofRequestLabel.right
            anchors.leftMargin: -92
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: createPathRequests
            width: 73
            height: 20
            y:50
            text: fileServer.numCreatePathRequests
            anchors.verticalCenter: createPathRequestsLabel.verticalCenter
            horizontalAlignment: Text.AlignLeft
            anchors.left: createPathRequestsLabel.right
            anchors.leftMargin: 0
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: destroyPathRequestsLabel
            width: 139
            height: 20
            y:50
            text: qsTr("Destroy Path Requests:")
            anchors.left: flushRequestLabel.right
            anchors.leftMargin: -102
            anchors.top: flushRequestLabel.bottom
            anchors.topMargin: 6
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: destroyPathRequests
            width: 123
            height: 20
            y:50
            text: fileServer.numDestroyPathRequests
            anchors.verticalCenter: destroyPathRequestsLabel.verticalCenter
            anchors.left: destroyPathRequestsLabel.right
            anchors.leftMargin: 0
            horizontalAlignment: Text.AlignLeft
            font.family: "Helvetica"
            font.pointSize: 8
        }




        Text {
            id: removeRequestsLabel
            width: 111
            height: 20
            text: qsTr("Remove Requests:")
            anchors.top: createPathRequestsLabel.bottom
            anchors.topMargin: 7
            anchors.left: createPathRequestsLabel.right
            anchors.leftMargin: -136
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: removeRequests
            width: 91
            height: 20
            y:50
            text: fileServer.numRemoveRequests
            anchors.verticalCenter: removeRequestsLabel.verticalCenter
            horizontalAlignment: Text.AlignLeft
            anchors.left: removeRequestsLabel.right
            anchors.leftMargin: 0
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: copyRequestsLabel
            width: 94
            height: 20
            y:50
            text: qsTr("Copy Requests:")
            anchors.left: destroyPathRequestsLabel.right
            anchors.leftMargin: -139
            anchors.top: destroyPathRequestsLabel.bottom
            anchors.topMargin: 7
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: copyRequests
            height: 20
            y:50
            width: 128
            text: fileServer.numCopyRequests
            anchors.left: copyRequestsLabel.right
            anchors.leftMargin: 0
            anchors.verticalCenterOffset: 0
            anchors.verticalCenter: copyRequestsLabel.verticalCenter
            horizontalAlignment: Text.AlignLeft
            font.family: "Helvetica"
            font.pointSize: 8
        }


        Text {
            id: renameRequestsLabel
            width: 112
            height: 20
            text: qsTr("Rename Requests:")
            anchors.top: removeRequestsLabel.bottom
            anchors.topMargin: 9
            anchors.left: removeRequestsLabel.right
            anchors.leftMargin: -111
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: renameRequests
            width: 91
            height: 20
            y:50
            text: fileServer.numRenameRequests
            anchors.verticalCenter: renameRequestsLabel.verticalCenter
            horizontalAlignment: Text.AlignLeft
            anchors.left: renameRequestsLabel.right
            anchors.leftMargin: -1
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: findFilesRequestsLabel
            width: 117
            height: 20
            y:50
            text: qsTr("Find Files Requests:")
            anchors.left: copyRequestsLabel.right
            anchors.leftMargin: -94
            anchors.top: copyRequestsLabel.bottom
            anchors.topMargin: 9
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: findFilesRequests
            width: 123
            height: 20
            y:50
            text: fileServer.numFindFileNamesRequests
            anchors.verticalCenter: findFilesRequestsLabel.verticalCenter
            anchors.left: findFilesRequestsLabel.right
            anchors.leftMargin: 0
            horizontalAlignment: Text.AlignLeft
            font.family: "Helvetica"
            font.pointSize: 8
        }


        Text {
            id: openFilesLabel
            width: 67
            height: 20
            text: qsTr("Open Files:")
            anchors.left: closedFilesLabel.right
            anchors.leftMargin: -78
            anchors.top: closedFilesLabel.bottom
            anchors.topMargin: 6
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: openFiles
            width: 123
            height: 20
            y:50
            text: fileServer.numOpenFiles
            anchors.verticalCenter: openFilesLabel.verticalCenter
            anchors.left: openFilesLabel.right
            anchors.leftMargin: 0
            horizontalAlignment: Text.AlignLeft
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: openFileRequestsLabel
            width: 121
            height: 20
            text: qsTr("Open File Requests:")
            anchors.left: bytesWrittenLabel.right
            anchors.leftMargin: -85
            anchors.top: bytesWrittenLabel.bottom
            anchors.topMargin: 6
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: openFileRequests
            y: 371
            width: 129
            height: 20
            text: fileServer.numOpenRequests
            anchors.left: openFileRequestsLabel.right
            anchors.leftMargin: 0
            anchors.verticalCenter: openFileRequestsLabel.verticalCenter
            font.family: "Helvetica"
            font.pointSize: 8
            horizontalAlignment: Text.AlignLeft
        }

        Text {
            id: closeFileRequestsLabel
            x: -7
            y: 43
            width: 123
            height: 20
            text: qsTr("Close File Requests:")
            anchors.left: bytesReadLabel.right
            anchors.leftMargin: -78
            anchors.top: bytesReadLabel.bottom
            anchors.topMargin: 6
            font.family: "Helvetica"
            font.pointSize: 8
        }

        Text {
            id: closeFileRequests
            y: 371
            width: 123
            height: 20
            text: fileServer.numCloseRequests
            anchors.left: closeFileRequestsLabel.right
            anchors.leftMargin: 0
            anchors.verticalCenter: closeFileRequestsLabel.verticalCenter
            font.family: "Helvetica"
            font.pointSize: 8
            horizontalAlignment: Text.AlignLeft
        }

    }




}
