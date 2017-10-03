/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <qglobal.h>

#include <qrect.h>
#include <qpoint.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/document.h>
#include <AzFramework/Entity/EntityReference.h>

#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Types/SceneData.h>
#include <GraphCanvas/Types/SceneSerialization.h>

QT_FORWARD_DECLARE_CLASS(QKeyEvent);
QT_FORWARD_DECLARE_CLASS(QGraphicsScene);
QT_FORWARD_DECLARE_CLASS(QGraphicsSceneContextMenuEvent);
QT_FORWARD_DECLARE_CLASS(QGraphicsSceneMouseEvent);
QT_FORWARD_DECLARE_CLASS(QMimeData);

namespace GraphCanvas
{
    class SceneSerialization;

    enum DragSelectionType
    {
        // Items will be selected as they are dragged over.
        Realtime,

        // Items will be selected once a drag selection is complete.
        OnRelease
    };

    //! SceneRequests
    //! Requests that modify a scene.
    class SceneRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Get the grid entity (for setting grid pitch)
        virtual AZ::EntityId GetGrid() const = 0;

        //! Add a node to the scene.
        //! Nodes are owned by the scene and will follow the scene's entity life-cycle and be destroyed along with it.
        //! To avoid this, remove nodes before destroying the scene.
        //!
        //! Additionally, the node should not already be in another scene.
        //!
        //! # Parameters
        //! 1. The entity ID of the node to add.
        //! 2. A 2D vector indicating the position in scene space the node should initially have.
        virtual bool AddNode(const AZ::EntityId&, const AZ::Vector2&) = 0;

        //! Add a list of nodes to the scene.
        //! Nodes are owned by the scene and will follow the scene's entity life-cycle and be destroyed along with it.
        //! To avoid this, remove nodes before destroying the scene.
        //!
        //! Additionally, the node should not already be in another scene.
        //!
        //! # Parameters
        //! 1. The entity ID of the nodes to add.
        virtual void AddNodes(const AZStd::vector<AZ::EntityId>&) = 0;

        //! Remove a node from the scene.
        virtual bool RemoveNode(const AZ::EntityId&) = 0;

        //! Get the entity IDs of the nodes known to the scene.
        virtual AZStd::vector<AZ::EntityId> GetNodes() const = 0;

        //! Get the entity IDs of all selected nodes known to the scene.
        virtual AZStd::vector<AZ::EntityId> GetSelectedNodes() const = 0;

        //! Add a connection to the scene.
        //! The connection must be connected to two slots and both slots must be in the same scene.
        virtual bool AddConnection(const AZ::EntityId&) = 0;

        //! Add a list of connection to the scene.
        //! The connection must be connected to two slots and both slots must be in the same scene.
        virtual void AddConnections(const AZStd::vector<AZ::EntityId>&) = 0;

        //! Remove a connection from the scene.
        virtual bool RemoveConnection(const AZ::EntityId&) = 0;

        //! Get the entity IDs of the connections known to the scene.
        virtual AZStd::vector<AZ::EntityId> GetConnections() const = 0;

        //! Get the entity IDs of the selected connections known to the scene.
        virtual AZStd::vector<AZ::EntityId> GetSelectedConnections() const = 0;

        //! Returns whether or not the specified endpoint is connected to anything
        virtual bool IsEndpointConnected(const Endpoint& endpoint) const = 0;

        //! Get the entity IDs of the connections where one endpoint of the connection is the supplied endpoint
        virtual AZStd::vector<AZ::EntityId> GetConnectionsForEndpoint(const Endpoint& endpoint) const = 0;

        //! Get the IDs of the endpoint forming the other ends of all the connections this endpoint is apart of.
        virtual AZStd::vector<Endpoint> GetConnectedEndpoints(const Endpoint& endpoint) const = 0;

        //! Creates a connection using both endpoints
        virtual bool Connect(const Endpoint&, const Endpoint&) = 0;
        //! Destroys a connection using both endpoints to look up the connection
        virtual bool Disconnect(const Endpoint&, const Endpoint&) = 0;
        //! Destroys a connection using the supplied connection Id
        virtual bool DisconnectById(const AZ::EntityId& connectionId) = 0;
        //! Finds a connection using the specified endpoints
        //! A reference to the found connection is returned in the connectionEntity parameter
        virtual bool FindConnection(AZ::Entity*& connectionEntity, const Endpoint& firstEndpoint, const Endpoint& otherEndpoint) const = 0;
        
        //! Find connections in which either one or both ends of the connection are part of the selected node set
        //! \param nodeIds Entity Ids of nodes to find within the supplied connection set
        //! \param internalConnectionsOnly If true only connections between nodes in the node set will be considered when searching. 
        //! otherwise a node will only need to be in one of two endpoints of a connection in order for the connection to be considered found.
        virtual AZStd::unordered_set<AZ::EntityId> FindConnections(const AZStd::unordered_set<AZ::EntityId>& nodeIds, bool internalConnectionsOnly = false) const = 0;

        //! Add an entity of any valid type to the scene.
        virtual bool Add(const AZ::EntityId&) = 0;

        //! Remove an entity of any valid type from the scene.
        virtual bool Remove(const AZ::EntityId&) = 0;

        //! Set the stylesheet for the scene from a string, it should be well-formed JSON.
        virtual void SetStyleSheet(const AZStd::string&) = 0;
        //! Set the stylesheet for the scene from a parsed JSON document.
        virtual void SetStyleSheet(const rapidjson::Document&) = 0;

        //! Clears the selection in the scene.
        virtual void ClearSelection() = 0;

        //! Set the selected area in the scene (within the rectangle between two points).
        //! Items within this area are selected immediately and can be retrieved with
        //! \ref GetSelectedItems
        virtual void SetSelectedArea(const AZ::Vector2&, const AZ::Vector2&) = 0;

        //! Whether or not there are selected items in the scene.
        virtual bool HasSelectedItems() const = 0;

        //! Get the selected items in the scene.
        virtual AZStd::vector<AZ::EntityId> GetSelectedItems() const = 0;

        //! Get the entities known to the scene at a given position in scene space.
        virtual AZStd::vector<AZ::EntityId> GetEntitiesAt(const AZ::Vector2&) const = 0;

        //! Get the entities known to the scene in the given rectangle.
        virtual AZStd::vector<AZ::EntityId> GetEntitiesInRect(const QRectF&) const = 0;

        //! Get the endpoints known to the scene in the given rectangle.
        virtual AZStd::vector<Endpoint> GetEndpointsInRect(const QRectF& rect) const = 0;

        //! Obtain the scene as a QGraphicsScene
        virtual QGraphicsScene* AsQGraphicsScene() = 0;

        //! Copies the selected nodes, connections and groups to the clipboard
        virtual void CopySelection() const = 0;

        //! Copies the specified entities to the clipboard
        virtual void Copy(const AZStd::vector<AZ::EntityId>&) const = 0;

        //! Serializes the specified entities to the given SceneSerializationHelper
        virtual void SerializeEntities(const AZStd::unordered_set<AZ::EntityId>& itemIds, SceneSerialization& serializationTarget) const = 0;

        //! Cuts the selected nodes, connections and groups to the clipboard
        virtual void CutSelection() = 0;

        //! Cuts the specified entities to the clipboard
        virtual void Cut(const AZStd::vector<AZ::EntityId>&) = 0;

        //! Paste nodes, connections and groups within the GraphCanvas clipboard to the scene
        virtual void Paste() = 0;

        //! Paste nodes, connections and groups within the GraphCanvas clipboard to the scene
        //! \param scenePos scene position where paste operation is to take place
        virtual void PasteAt(const QPointF& scenePos) = 0;

        //! Paste scene serialization at the given position.
        //! \param scenePos the position at which to deserialize the serialization.
        //! \param serializationSource is the data source from which data will be grabbed.
        virtual void DeserializeEntities(const QPointF& scenePos, const SceneSerialization& serializationSource) = 0;

        //! Duplicate the nodes, connections and groups currently selected to the scene
        virtual void DuplicateSelection() = 0;

        //! Duplicate the node, connections and group currently selected to the position given
        //! \param scenePos the position to duplicate the selections to
        virtual void DuplicateSelectionAt(const QPointF& scenePos) = 0;

        //! Duplicate the nodes, connections and groups specified by the input
        //! \param itemIds the id of the entity to be duplicated
        virtual void Duplicate(const AZStd::vector<AZ::EntityId>& itemIds) = 0;

        //! Duplicate the nodes, connections and groups to the position specified by the input
        //! \param itemIds the id of the entity to be duplicated
        //! \param scenePos the position to duplicate the selections to
        virtual void DuplicateAt(const AZStd::vector<AZ::EntityId>& itemIds, const QPointF& scenePos) = 0;

        //! Deletes the current selection from the scene.
        virtual void DeleteSelection() = 0;

        //! Delete nodes from supplied set that exist within the scene
        //! \param itemIds Set of ids to delete
        virtual void Delete(const AZStd::unordered_set<AZ::EntityId>&) = 0;

        virtual void FilterItems(const AZStd::unordered_set<AzFramework::EntityReference>&, AZStd::unordered_set<AzFramework::EntityReference>&,
            AZStd::unordered_set<AzFramework::EntityReference>&) const = 0;

        //! Stops the scene from allowing the next context menu's from being created.
        //! \param suppressed Whether or not context menu's should be allowed.
        virtual void SuppressNextContextMenu() = 0;

        //! Get the string that the scene use to set the mime type of the clipboard object
        virtual const AZStd::string& GetCopyMimeType() const = 0;

        //! Set the mime type for the scene. Allows the generic event system to distinguish between
        //! different instance of GraphCanvas.
        virtual void SetMimeType(const char* mimeType) = 0;

        virtual void RegisterView(const AZ::EntityId& viewId) = 0;
        virtual void RemoveView(const AZ::EntityId& viewId) = 0;

        //! Retrieves the ViewId that this scene is registered with
        //! Returns registered ViewId if the Scene is registered with a view otherwise an invalid ViewId is returned
        virtual ViewId GetViewId() const = 0;

        //! Dispatches a Mime Drop Event to the this Scene
        virtual void DispatchSceneDropEvent(const AZ::Vector2& scenePos, const QMimeData* mimeData) = 0;

        //! Retrieves the user data associated with the SceneData structure stored in the Scene component
        //! If the user data is serializable then it will get serialized when the SceneData structure is serialized
        //! A pointer is returned instead of a reference, as EBuses cannot references. If this bus is listening the return value is never null
        virtual AZStd::any* GetUserData() = 0;
        virtual const AZStd::any* GetUserDataConst() const = 0;

        //! Retrieves the entity that the SceneRequests is addressed on
        virtual AZ::Entity* GetSceneEntity() const = 0;

        //! Returns a reference to the SceneData on the Scene, 
        //! The SceneRequests handler must be retrieved to invoke this method
        virtual SceneData* GetSceneData() = 0;
        virtual const SceneData* GetSceneDataConst() const = 0;

        //! Uses the supplied scene data to add nodes, connections, to the scene.
        //! \param sceneData structure containing data to add to the scene(nodes, connections, etc...)
        virtual bool AddSceneData(const SceneData&) = 0;
        //! Removes matching nodes, connections from the scene
        //! Note: User data is not modified
        //! \param sceneData structure containing data to remove from the scene
        virtual void RemoveSceneData(const SceneData&) = 0;
        //! Deletes matching nodes, connections from the scene
        //! \param sceneData structure containing data to delete from the scene
        virtual void DeleteSceneData(const SceneData&) = 0;

        //! Controls how drag selection is handled.
        //! Default value is OnRelease.
        virtual void SetDragSelectionType(DragSelectionType dragSelectionType) = 0;

        virtual void SignalDragSelectStart() = 0;
        virtual void SignalDragSelectEnd() = 0;
    };

    using SceneRequestBus = AZ::EBus<SceneRequests>;


    //! SceneNotifications
    //! Notifications about changes to the state of scenes.
    class SceneNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! A node has been added to the scene.
        virtual void OnNodeAdded(const AZ::EntityId& /*nodeId*/) {}
        //! A node has been removed from the scene.
        virtual void OnNodeRemoved(const AZ::EntityId& /*nodeId*/) {}
        //! A node in the scene has been selected
        virtual void OnNodeSelected(const AZ::EntityId& /*nodeId*/, bool /*selected*/) {}

        //! A node in the scene has been moved
        virtual void OnNodePositionChanged(const AZ::EntityId& /*nodeId*/, const AZ::Vector2& /*position*/) {}

        //! A node in the scene has received a mouse release event
        virtual void OnItemMouseMoveComplete(const AZ::EntityId& /*itemId*/, const QGraphicsSceneMouseEvent* /*event*/) {}

        //! A node in the scene has been deleted
        virtual void OnPreNodeDeleted(const AZ::EntityId& /*nodeId*/) {}

        //! A connection has been added to the scene.
        virtual void OnConnectionAdded(const AZ::EntityId& /*connectionId*/) {}

        //! A connection has been removed from the scene.
        virtual void OnConnectionRemoved(const AZ::EntityId& /*connectionId*/) {}

        //! A connection in the scene has been selected
        virtual void OnConnectionSelected(const AZ::EntityId& /*connectionId*/, bool /*selected*/) {}

        //! A connection in the scene has been deleted
        virtual void OnPreConnectionDeleted(const AZ::EntityId& /*connectionId*/) {}

        //! Selected nodes, connections and groups have been serialized to the target serialization.
        virtual void OnEntitiesSerialized(SceneSerialization&) {}

        //! GraphCanvas nodes, connections and groups have been pasted from the clipboard
        //! The userData map contains any custom data serialized in from a copy operation.
        virtual void OnEntitiesDeserialized(const SceneSerialization&) {}

        //! Sent when a duplicate command begins
        virtual void OnDuplicateBegin() {}

        //! Sent when a duplicate command ends
        virtual void OnDuplicateEnd() {}

        //! Sent When a paste command begins
        virtual void OnPasteBegin() {}

        //! Sent when a paste command finishes
        virtual void OnPasteEnd() {}

        //! Sent when a copy branch begins
        virtual void OnCopyBegin() {}

        //! Sent after a scene has complete a copy branch.
        virtual void OnCopyEnd() {}

        //! Sent after a scene has completed a deletion batch
        virtual void PostDeletionEvent() {}

        //! Sent after the scene has successfully handled a creation event.
        virtual void PostCreationEvent() {}

        //! The scene's stylesheet was changed.
        virtual void OnStyleSheetChanged() {}

        //! The selection in the scene has changed.
        virtual void OnSelectionChanged() {}

        //! Position of an item in the scene
        virtual void OnPositionChanged(const AZ::EntityId& /*itemId*/, const AZ::Vector2& /*position*/) {}

        //! A key was pressed in the scene
        virtual void OnKeyPressed(QKeyEvent*) {}
        //! A key was released in the scene
        virtual void OnKeyReleased(QKeyEvent*) {}
    };

    using SceneNotificationBus = AZ::EBus<SceneNotifications>;

    //! SceneNotifications
    //! Notifications about changes to the state of scenes.
    class SceneUIRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Hook into the QGraphicsScene context menu event.
        //! This event is only sent when none of the QGraphicsItem on the QGraphicScene accepts this event
        virtual void OnSceneContextMenuEvent(const AZ::EntityId& sceneId, QGraphicsSceneContextMenuEvent* /*event*/) = 0;

        //! Hook into the QGraphicsScene double click menu event.
        virtual void OnSceneDoubleClickEvent(const AZ::EntityId& sceneId, QGraphicsSceneMouseEvent* /*event*/) = 0;

        //! Hook into the GraphicsItem context menu event
        //! Specifically this is sent when a Node receives the context menu event
        virtual void OnNodeContextMenuEvent(const AZ::EntityId& nodeId, QGraphicsSceneContextMenuEvent* /*event*/) = 0;

        //! Hook into the GraphicsItem context menu event
        //! Specifically this is sent when a Slot receives the context menu event
        virtual void OnSlotContextMenuEvent(const AZ::EntityId& slotId, QGraphicsSceneContextMenuEvent* /*event*/) = 0;

        //! Hook into the GraphicsItem context menu event
        //! Specifically this is sent when a Connection receives the context menu event
        virtual void OnConnectionContextMenuEvent(const AZ::EntityId& connectionId, QGraphicsSceneContextMenuEvent* /*event*/) = 0;

        //! This is sent when a Connection has no target.
        //! Returns the EntityId of the node create, if any.
        virtual Endpoint CreateNodeForProposal(const AZ::EntityId& connectionId, const Endpoint& endpoint, const QPointF& scenePosition, const QPoint& screenPosition) = 0;

        //! Callback for the Wrapper node action widgets
       virtual void OnWrapperNodeActionWidgetClicked(const AZ::EntityId& wrapperNode, const QRect& actionWidgetBoundingRect, const QPointF& scenePosition, const QPoint& screenPosition) = 0;

       //! Callback for requesting an Undo Point to be posted.
       virtual void RequestUndoPoint() = 0;

       //! Callback for requesting the incrementation of the value of the ignore undo point tracker
       virtual void RequestPushPreventUndoStateUpdate() = 0;

       //! Callback for requesting the decrementation of the value of the ignore undo point tracker
       virtual void RequestPopPreventUndoStateUpdate() = 0;
    };

    using SceneUIRequestBus = AZ::EBus<SceneUIRequests>;

    //! SceneMemberRequests
    //! An interface that allows generic retrieval of the scene entities belong to.
    //! This is used in, for example, the styling code.
    class SceneMemberRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Set the scene an entity is in.
        virtual void SetScene(const AZ::EntityId&) = 0;
        //! Remove this entity from any scene it's in.
        virtual void ClearScene(const AZ::EntityId&) = 0;

        //! When the entity is being copied. Provides a hook for copying extra information.
        virtual void PrepareExtraCopyData(AZStd::unordered_set< AZ::EntityId >& /*contextualCopies*/) { };

        //! Get the scene that the entity belongs to (directly or indirectly), if any.
        virtual AZ::EntityId GetScene() const = 0;
    };

    using SceneMemberRequestBus = AZ::EBus<SceneMemberRequests>;

    //! SceneMemberNotifications
    //! Notifications about changes to the scene membership of entities
    class SceneMemberNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! When the entity is added to a scene, this event is emitted.
        virtual void OnSceneSet(const AZ::EntityId&) {}
        //! When the entity is removed from a scene, this event is emitted.
        //! Includes the previously-set scene ID.
        virtual void OnSceneCleared(const AZ::EntityId&) {}

        //! Signal sent once the scene is fully configured and ready to be displayed.
        virtual void OnSceneReady() {}        
    };

    using SceneMemberNotificationBus = AZ::EBus<SceneMemberNotifications>;
}
