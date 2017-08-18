import QtQuick 2.0
import QtQuick.Controls 1.2
import ExampleQMLViewPane 1.0

Rectangle {
    color: "#a8fffe"
    width: 200
    height: 200
    border.width: 2
    radius: 16

    // create a C++ object, then talk to it:
    property var myObject : ExampleQtObject {
        id: exampleObject
        onExampleSignal: {
            exampleLabel.text = "Signal Received"
        }

    }

    anchors.fill: parent
    Label {
        id: exampleLabel
        text: "<b>Embedded!</b>This is an example of an embedded resource<br>The file is being read out of the executable<br>"
        wrapMode: Label.WordWrap
        width: parent.width
        anchors.centerIn: parent
    }

    Label {
        id: fpsLabel
        text: exampleObject.fpsValue + "fps average"
        anchors.left: parent.left
        anchors.top: parent.top
        color: exampleObject.fpsValue < 10.0 ? "red" : "black"
        anchors.margins: 8
        Behavior on color {
            ColorAnimation {  }
        }


    }

    Button  {
        id: exampleButton
        text: "Example Button"
        anchors.top: exampleLabel.bottom
        anchors.horizontalCenter: exampleLabel.horizontalCenter
        onClicked: {
            exampleObject.exampleProperty = parent.width
            exampleObject.exampleSlot("123")
            exampleObject.exampleInvokableFunction(222)
        }
    }
    Image {
        anchors.bottom: exampleLabel.top
        width: 64
        height: 64
        anchors.horizontalCenter: exampleLabel.horizontalCenter
        source: "logo.png"
    }
    Label {
        id: propertywatcher
        anchors.top: exampleButton.bottom
        anchors.horizontalCenter: exampleButton.horizontalCenter
        text: exampleObject.exampleProperty
    }
}
