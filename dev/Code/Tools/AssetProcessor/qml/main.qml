import QtQuick 2.3
import QtQuick.Window 2.2
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1
import AssetProcessor 1.0

ApplicationWindow {
    id: appWindow
    visible: true
    width: 640
    height: 480
    title: qsTr("Asset Processor")
    property int anchorsMargin : 8
    signal quitRequested();
    minimumWidth: 800
    minimumHeight: 600

    menuBar: MenuBar {
        Menu {
            title: qsTr("File")
            MenuItem {
                text: qsTr("Exit")
                shortcut: "Ctrl+Q" // this helps us debug (Alt+f4 merely hides the window)
                onTriggered: {
                    screen.saveSettings()
                    quitRequested()
                }
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: anchorsMargin
        spacing: 10

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 200
            Layout.minimumWidth: 200

            Label {
                id: asssetProcessorListeningPortLabel;
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: 10
                text: qsTr("Asset Processor Port")
            }
            TextField{
                id:asssetProcessorListeningPortText;
                anchors.top:asssetProcessorListeningPortLabel.top
                anchors.right: parent.right
                text: applicationServer.getServerListeningPort()
                readOnly:true
                anchors.rightMargin: 30
            }

            Label {
                id: proxyInformation;
                anchors.top: asssetProcessorListeningPortLabel.bottom
                anchors.left: parent.left
                anchors.margins: 10
                text: qsTr("Proxy IP Address and Port")
            }

            TextField{
                id:proxyInformationText;
                anchors.top:proxyInformation.top
                anchors.right: parent.right
                anchors.rightMargin: 30
                text: connectionManager.proxyInformation

                onEditingFinished: {
                    connectionManager.proxyInformation = text
                }

                Connections {
                    target: connectionManager
                    onProxyInformationChanged:
                    {
                        proxyInformationText.text = Qt.binding( function() { return connectionManager.proxyInformation } )
                    }
                }
            }

            CheckBox {
                id: proxyConnectCheckbox
                anchors.top:proxyInformation.bottom
                anchors.topMargin: 10
                anchors.left: proxyInformation.left
                text: qsTr("Proxy Connect")

                onCheckedChanged: {
                    connectionManager.proxyConnect = proxyConnectCheckbox.checked
                    proxyConnectCheckbox.checked = Qt.binding( function() { return connectionManager.proxyConnect } )
                }

            }

            Button {
                id: addConnection
                text: qsTr("Add Connection")
                anchors.left: parent.left
                anchors.top: proxyConnectCheckbox.bottom
                anchors.topMargin:10
                onClicked:  connectionManager.addConnection();
            }

            ConnectionsTabView {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.top: addConnection.bottom
                anchors.topMargin: anchorsMargin
            }
        }

        TabView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Tab {
                title: qsTr("File Server")
                FileServerView {
                    anchors.fill: parent
                    anchors.margins: anchorsMargin
                }
            }

            Tab {
                title: qsTr("Resource Compiler")
                RCListView {
                    anchors.fill: parent
                    anchors.margins: anchorsMargin
                }
            }
            Tab {
                title: qsTr("Shader Compiler")
                ShaderCompilerView {
                    anchors.fill: parent
                    anchors.margins: anchorsMargin
                }
            }

        }
    }

    onClosing: {
        close.accepted = false
        appWindow.visible = false
    }


    WindowScreen
    {
        id : screen
        windowName: "Main Screen"
    }

    onWidthChanged: {
        screen.width = appWindow.width;
    }

    onHeightChanged: {
        screen.height = appWindow.height;
    }

    onXChanged: {
        screen.positionX = appWindow.x;
    }

    onYChanged: {
        screen.positionY = appWindow.y;
    }

    onVisibilityChanged: {
        if(appWindow.visibility != Window.Hidden )
        {
            screen.windowState = appWindow.visibility
        }
    }

    Component.onCompleted: {
        //Load screen values from storage
        screen.loadSettings(appWindow.width, appWindow.height, appWindow.minimumWidth, appWindow.minimumHeight);
        showWindow();
    }

    function showWindow() {
        appWindow.height = screen.height;
        appWindow.width = screen.width;
        appWindow.x = screen.positionX;
        appWindow.y = screen.positionY;
        appWindow.visibility = screen.windowState;
        appWindow.visible = true
    }

    function toggleWindow() {
        if(appWindow.visible)
        {
            hide();
        }
        else
        {
            show();
        }
    }

}
