import QtQuick 2.3
import QtQuick.Controls 1.2
// we use StyleItem from controls.private, for now.
import QtQuick.Controls.Private 1.0
import QtQuick.Controls.Styles 1.2
import "."
import AssetProcessor 1.0


TabView {
    width: 500
    height: 500
    id: tabViewRoot
    signal tabCreated(int connId)

    ListModel {
        id:tabIndexToConnectionId
    }

    onCurrentIndexChanged: {
        // this causes the non-active tab to unload from memory.
        for (var idx = 0; idx < tabViewRoot.count; ++idx)
        {
            tabViewRoot.getTab(idx).active = (idx == currentIndex)
        }
    }

    // for now, we just use the default native style of the OS.
    style: TabViewStyle {
        frameOverlap: 1

        tab: Item {
            id:tabRectangle
            implicitWidth: styleData.availableWidth/(tabViewRoot.count+.5)
            implicitHeight: 20
            property string orientation: control.tabPosition === Qt.TopEdge ? "Top" : "Bottom"
            property string tabpos: control.count === 1 ? "only" : index === 0 ? "beginning" : index === control.count - 1 ? "end" : "middle"
            property string selectedpos: styleData.nextSelected ? "next" : styleData.previousSelected ? "previous" : ""
            // from / src/controls/Styles/Desktop/TabViewStyle.qml
            // note: A  "styleItem" is a component type that renders a native operating system widget depending on elementType
            // things available are tabs,  buttons, combos, checkboxes...
            StyleItem {
                elementType: "tab"
                paintMargins: style === "mac" ? 0 : 2
                anchors.fill: parent
                anchors.topMargin: style === "mac" ? 2 : 0
                anchors.rightMargin: -paintMargins
                anchors.bottomMargin: -1
                anchors.leftMargin: -paintMargins + (style === "mac" && selected ? -1 : 0)
                enabled: styleData.enabled
                selected: styleData.selected
                text: styleData.title
                hover: styleData.hovered
                hasFocus: selected
                properties: { "hasFrame" : true, "orientation": orientation, "tabpos": tabpos, "selectedpos": selectedpos }
                hints: control.styleHints
            }

            Rectangle{
                id:closeRectangle
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: 4
                implicitWidth:10
                implicitHeight:10
                radius: 2
                color:"red"
                border.color:"black"
                Text {
                    text: "X"
                    anchors.centerIn: closeRectangle
                    color: "gray"
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                             connectionManager.removeConnection(tabIndexToConnectionId.get(styleData.index).connectionId)
                        }
                    }
                }
            }
        }

        frame: StyleItem {
            id: styleitem
            anchors.fill: parent
            anchors.topMargin: 1//stack.baseOverlap
            z: style == "oxygen" ? 1 : 0
            elementType: "tabframe"

            maximum: width
            hints: control.styleHints
            border{
                top: 16
                bottom: 16
            }
            textureHeight: 64
        }
    }

    Component.onCompleted:
    {
        createTabs()
    }

    function createTabs()
    {
        tabIndexToConnectionId.clear()
        for(var idx =0 ;idx < connectionManager.getCount();idx++)
        {
            tabIndexToConnectionId.append({"connectionId":connectionManager.connectionMap()[idx].key()})
        }

        tabViewRoot.currentIndex = tabIndexToConnectionId.count-1

        for (idx = 0; idx < connectionManager.getCount(); ++idx)
        {
            createTab(idx)
        }
    }

    function createTab(index)
    {
        var newTab;
        var connectionObject = connectionManager.getConnection(tabIndexToConnectionId.get(index).connectionId)
        newTab = addTab(connectionObject.displayName);
        // do not allow the controls within this element to be created asynchronously
        // otherwise they appear 1 by 1 in a grid view, and flicker
        newTab.asynchronous = false
        // manually set the source of the tab, so that the "conn" property is ALWAYS PRESENT:
        newTab.setSource("ConnectionTab.qml", { "conn" : connectionObject } )
        // but start loading immediately (not when the tab is active)
        //newTab.active = true
        tabViewRoot.tabCreated(index);
        // bind the title of the tab to the display name.
        newTab.title = Qt.binding( function() { return connectionObject.displayName } )
        return newTab
    }

    Connections  {
        target: connectionManager
        onConnectionAdded:
        {
            tabIndexToConnectionId.append({"connectionId":connectionId})
            var index = tabIndexToConnectionId.count-1
            createTab(index)
            tabViewRoot.currentIndex = index
        }

        onBeforeConnectionRemoved:
        {
            var idx =0 ;
            for(idx =0 ;idx < tabIndexToConnectionId.count;idx++)
            {
                if(tabIndexToConnectionId.get(idx).connectionId === connectionId )
                {
                    tabIndexToConnectionId.remove(idx)
                    tabViewRoot.getTab(idx).active = false // destroy the view.  This takes care of focus.
                    tabViewRoot.removeTab(idx)
                    return
                }
            }

        }
    }

}

