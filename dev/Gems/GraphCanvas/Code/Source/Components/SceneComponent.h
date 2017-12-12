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

#include <QGraphicsScene>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Math/Vector2.h>

#include <AzFramework/Entity/EntityReference.h>

#include <Components/ColorPaletteManager/ColorPaletteManagerBus.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <GraphCanvas/Components/MimeDataHandlerBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>

class QAction;
class QMimeData;

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(QGraphicsScene, "{C27E4829-BA47-4BAA-B9FD-C549ACF316B7}");
}

class QGraphicsItem;

namespace GraphCanvas
{
    class GraphCanvasGraphicsScene;

    //! Separate class just to avoid over-cluttering the scene.
    class SceneMimeDelegate
        : protected SceneMimeDelegateHandlerRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(SceneMimeDelegate, AZ::SystemAllocator, 0);
        SceneMimeDelegate() = default;
        ~SceneMimeDelegate() = default;

        void Activate();
        void Deactivate();

        void SetSceneId(const AZ::EntityId& sceneId);
        void SetMimeType(const char* mimeType);

        // SceneMimeDelegateHandlerRequestBus
        bool IsInterestedInMimeData(const AZ::EntityId& sceneId, const QMimeData* dragEnterEvent) const override;
        void HandleDrop(const AZ::EntityId& sceneId, const QPointF& dropPoint, const QMimeData* mimeData) override;
        ////

    private:
        QString m_mimeType;
        AZ::EntityId m_sceneId;
    };

    //! This is the Scene that represents a GraphCanvas.
    //! It is a an entity component; users can create new scenes by calling
    //!
    //! AZ::EntityId sceneId = Scene::CreateScene()
    //!
    //! From that moment on, users can interact with the scene through the SceneRequestBus, For example adding
    //! a node:
    //!
    //! SceneRequestBus::Broadcast(sceneId, &SceneRequests::AddNode, AzTypeInfo<MyNodeType>::Uuid());
    //!
    //! This will create a new Entity with a MyNodeType component.
    //!
    //! Recall how we can specify a list of component descriptors for any given Node. This will in turn add
    //! the specified components to the Node entity. Similarly, we can do the same for connections:
    //!
    //! SceneRequestBus::Broadcast(sceneId, &SceneRequests::AddConnection, sourceNodeId, sourceSlotId, targetNodeId, targetSlotId);
    //!
    //! This will create a connection Entity that will be added to the list of entities in the Scene component,
    //! with a Connection component, the connection component will hold the sourceNodeId, sourceSlotId,
    //! targetNodeId & targetSlotId.
    //!
    class SceneComponent
        : public GraphCanvasPropertyComponent
        , public AZ::EntityBus::Handler
        , public SceneRequestBus::Handler
        , public GeometryNotificationBus::MultiHandler
        , public ViewNotificationBus::Handler
        , public SceneMimeDelegateRequestBus::Handler
    {
    private:
        friend class GraphCanvasGraphicsScene;

    public:
        AZ_COMPONENT(SceneComponent, "{3F71486C-3D51-431F-B904-DA070C7A0238}");
        static void Reflect(AZ::ReflectContext* context);

        SceneComponent();
        ~SceneComponent() override;

        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("GraphCanvas_SceneService", 0x8ec010e7));
            provided.push_back(AZ_CRC("GraphCanvas_MimeDataHandlerService", 0x7a6beb5a));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incombatible)
        {
            incombatible.push_back(AZ_CRC("GraphCanvas_SceneService", 0x8ec010e7));
            incombatible.push_back(AZ_CRC("GraphCanvas_MimeDataHandlerService", 0x7a6beb5a));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("GraphCanvas_StyleService", 0x1a69884f));
            required.push_back(ColorPaletteManagerServiceCrc);
        }

        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // SceneRequestBus
        AZStd::any* GetUserData() override;
        const AZStd::any* GetUserDataConst() const override;
        AZ::Entity* GetSceneEntity() const override { return GetEntity(); }
        ////

        SceneRequests* GetSceneRequests() { return this; }
        const SceneRequests* GetSceneRequestsConst() const { return this; }

        SceneData* GetSceneData() override { return &m_sceneData; };
        const SceneData* GetSceneDataConst() const override { return &m_sceneData; };
    
        // SceneRequestBus
        AZ::EntityId GetGrid() const override;

        bool AddNode(const AZ::EntityId& nodeId, const AZ::Vector2& position) override;
        void AddNodes(const AZStd::vector<AZ::EntityId>& nodeIds) override;
        bool RemoveNode(const AZ::EntityId& nodeId) override;

        AZStd::vector<AZ::EntityId> GetNodes() const override;

        AZStd::vector<AZ::EntityId> GetSelectedNodes() const override;

        bool AddConnection(const AZ::EntityId& connectionId) override;
        void AddConnections(const AZStd::vector<AZ::EntityId>& connectionIds) override;
        bool RemoveConnection(const AZ::EntityId& connectionId) override;

        AZStd::vector<AZ::EntityId> GetConnections() const override;
        AZStd::vector<AZ::EntityId> GetSelectedConnections() const override;
        AZStd::vector<AZ::EntityId> GetConnectionsForEndpoint(const Endpoint& firstEndpoint) const override;

        bool IsEndpointConnected(const Endpoint& endpoint) const override;
        AZStd::vector<Endpoint> GetConnectedEndpoints(const Endpoint& firstEndpoint) const override;

        bool Connect(const Endpoint&, const Endpoint&) override;
        bool Disconnect(const Endpoint&, const Endpoint&) override;
        bool DisconnectById(const AZ::EntityId& connectionId) override;
        bool FindConnection(AZ::Entity*& connectionEntity, const Endpoint& firstEndpoint, const Endpoint& otherEndpoint) const override;

        bool Add(const AZ::EntityId& entity) override;
        bool Remove(const AZ::EntityId& entity) override;

        void SetStyleSheet(const AZStd::string& json) override;
        void SetStyleSheet(const rapidjson::Document& json) override;

        void ClearSelection() override;

        void SetSelectedArea(const AZ::Vector2& topLeft, const AZ::Vector2& topRight) override;

        bool HasSelectedItems() const override;
        AZStd::vector<AZ::EntityId> GetSelectedItems() const override;

        QGraphicsScene* AsQGraphicsScene() override;

        void CopySelection() const override;
        void Copy(const AZStd::vector<AZ::EntityId>& itemIds) const override;

        void CutSelection() override;
        void Cut(const AZStd::vector<AZ::EntityId>& itemIds) override;

        void Paste() override;
        void PasteAt(const QPointF& scenePos) override;

        void SerializeEntities(const AZStd::unordered_set<AZ::EntityId>& itemIds, SceneSerialization& serializationTarget) const override;
        void DeserializeEntities(const QPointF& scenePos, const SceneSerialization& serializationTarget) override;

        void DuplicateSelection() override;
        void DuplicateSelectionAt(const QPointF& scenePos) override;
        void Duplicate(const AZStd::vector<AZ::EntityId>& itemIds) override;
        void DuplicateAt(const AZStd::vector<AZ::EntityId>& itemIds, const QPointF& scenePos) override;

        void DeleteSelection() override;
        void Delete(const AZStd::unordered_set<AZ::EntityId>& itemIds) override;
        void DeleteSceneData(const SceneData& sceneData) override;

        void SuppressNextContextMenu() override;

        const AZStd::string& GetCopyMimeType() const override;
        void SetMimeType(const char* mimeType) override;

        AZStd::vector<AZ::EntityId> GetEntitiesAt(const AZ::Vector2& position) const override;
        AZStd::vector<AZ::EntityId> GetEntitiesInRect(const QRectF& rect) const override;

        AZStd::vector<Endpoint> GetEndpointsInRect(const QRectF& rect) const override;

        void RegisterView(const AZ::EntityId& viewId) override;
        void RemoveView(const AZ::EntityId& viewId) override;
        ViewId GetViewId() const override;

        void DispatchSceneDropEvent(const AZ::Vector2& scenePos, const QMimeData* mimeData) override;

        bool AddSceneData(const SceneData& sceneData) override;
        void RemoveSceneData(const SceneData& sceneData) override;

        void SetDragSelectionType(DragSelectionType dragSelectionType) override;

        void SignalDragSelectStart() override;
        void SignalDragSelectEnd() override;
        ////

        // GeometryNotificationBus
        void OnPositionChanged(const AZ::EntityId& itemId, const AZ::Vector2& position) override;
        ////

        // ViewNotificationBus
        void OnViewParamsChanged(const ViewParams& viewParams) override;
        ////

        // MimeDelegateRequestBus
        void AddDelegate(const AZ::EntityId& delegateId) override;
        void RemoveDelegate(const AZ::EntityId& delegateId) override;
        ////

    protected:

        void OnSceneDragEnter(const QMimeData* mimeData);
        void OnSceneDropEvent(const QPointF& scenePoint, const QMimeData* mimeData);
        void OnSceneDragExit();

        bool HasActiveMimeDelegates() const;

    private:
        SceneComponent(const SceneComponent&) = delete;

        template<typename Container>
        void InitItems(const Container& entities) const;

        template<typename Container>
        void ActivateItems(const Container& entities);

        template<typename Container>
        void DeactivateItems(const Container& entities);

        template<typename Container>
        void DestroyItems(const Container& entities) const;

        void InitConnections();
        void NotifyConnectedSlots();
        void OnSelectionChanged();

        AZStd::unordered_set<AZ::EntityId> FindConnections(const AZStd::unordered_set<AZ::EntityId>& nodeIds, bool internalConnectionsOnly = false) const override;

        //! Validates that the node ids in the connection can be found in the supplied node set
        bool ValidateConnectionEndpoints(AZ::Entity* connectionRef, const AZStd::unordered_set<AZ::Entity*>& nodeRefs);

        //! Filter a set of entity refs into a node, connection and group entityReferences set based on if they are in the scene
        void FilterItems(const AZStd::unordered_set<AzFramework::EntityReference>& itemIds, AZStd::unordered_set<AzFramework::EntityReference>& nodeIds,
            AZStd::unordered_set<AzFramework::EntityReference>& connectionIds) const override;

        //! Filter a set of entity id's into a node, connection and group entityId set based on if they are in the scene
        void FilterItems(const AZStd::unordered_set<AZ::EntityId>& itemIds, AZStd::unordered_set<AZ::EntityId>& nodeIds, AZStd::unordered_set<AZ::EntityId>& connectionIds) const;

        QPointF GetViewCenterScenePoint() const;

        int m_deleteCount;
        AZStd::string m_copyMimeType;

        AZ::EntityId m_grid;
        AZStd::unordered_map<QGraphicsItem*, AZ::EntityId> m_itemLookup;

        ViewId m_viewId;
        ViewParams m_viewParams;

        SceneMimeDelegate m_mimeDelegate;

        SceneData m_sceneData;

        AZStd::unordered_set<AZ::EntityId> m_delegates;
        AZStd::unordered_set<AZ::EntityId> m_activeDelegates;
        AZStd::unordered_set<AZ::EntityId> m_interestedDelegates;

        AZStd::unique_ptr<GraphCanvasGraphicsScene> m_graphicsSceneUi;

        DragSelectionType m_dragSelectionType;

        bool m_activateScene;
        bool m_isDragSelecting;
        bool m_ignoreSelectionChanges;
    };

    //! This is the is Qt Ui QGraphicsScene elements that is stored in the GraphCanvas SceneComponent
    //! This class should NOT be serialized
    class GraphCanvasGraphicsScene
        : public QGraphicsScene
    {
    public:
        AZ_TYPE_INFO(Scene, "{48C47083-2CF2-4BB5-8058-FF25084FC2AA}");
        AZ_CLASS_ALLOCATOR(GraphCanvasGraphicsScene, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext*) = delete;

        GraphCanvasGraphicsScene(SceneComponent& scene);
        ~GraphCanvasGraphicsScene() = default;

        // Do not allow serialization
        AZ::EntityId GetEntityId() const;
        void SuppressNextContextMenu();

    protected:
        // QGraphicsScene
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;
        void contextMenuEvent(QGraphicsSceneContextMenuEvent* contextMenuEvent) override;
        QList<QGraphicsItem*> itemsAtPosition(const QPoint& screenPos, const QPointF& scenePos, QWidget* widget) const;
        void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

        void dragEnterEvent(QGraphicsSceneDragDropEvent * event) override;
        void dragLeaveEvent(QGraphicsSceneDragDropEvent* event) override;
        void dragMoveEvent(QGraphicsSceneDragDropEvent* event) override;
        void dropEvent(QGraphicsSceneDragDropEvent* event) override;
        ////

    private:
        GraphCanvasGraphicsScene(const GraphCanvasGraphicsScene&) = delete;

        SceneComponent& m_scene;
        bool m_suppressContextMenu;
    };
}
