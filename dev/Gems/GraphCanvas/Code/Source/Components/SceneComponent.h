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
#include <QTimer>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Math/Vector2.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <GraphCanvas/Components/Bookmarks/BookmarkBus.h>
#include <GraphCanvas/Components/EntitySaveDataBus.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <GraphCanvas/Components/MimeDataHandlerBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Types/EntitySaveData.h>
#include <GraphCanvas/Utils/StateControllers/StateController.h>

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
        void SetEditorId(const EditorId& editorId);
        void SetMimeType(const char* mimeType);

        // SceneMimeDelegateHandlerRequestBus
        bool IsInterestedInMimeData(const AZ::EntityId& sceneId, const QMimeData* dragEnterEvent) override;
        void HandleMove(const AZ::EntityId& sceneId, const QPointF& dropPoint, const QMimeData* mimeData) override;
        void HandleDrop(const AZ::EntityId& sceneId, const QPointF& dropPoint, const QMimeData* mimeData) override;
        void HandleLeave(const AZ::EntityId& sceneId, const QMimeData* mimeData) override;
        ////

    private:

        void OnTrySplice();
        void CancelSplice();

        void PushUndoBlock();
        void PopUndoBlock();

        QTimer m_spliceTimer;

        QString m_mimeType;
        AZ::EntityId m_sceneId;
        EditorId     m_editorId;

        AZ::EntityId m_targetConnection;

        bool m_enableConnectionSplicing;
        QByteArray m_splicingData;

        AZ::EntityId m_splicingNode;
        QPainterPath m_splicingPath;
        AZ::Vector2  m_positionOffset;

        QPointF m_targetPosition;

        Endpoint m_spliceSource;
        Endpoint m_spliceTarget;

        bool m_pushedUndoBlock;
        
        StateSetter<RootGraphicsItemDisplayState> m_displayStateStateSetter;
    };

    struct SceneMemberBuckets
    {
        AZStd::unordered_set< AZ::EntityId > m_nodes;
        AZStd::unordered_set< AZ::EntityId > m_connections;
        AZStd::unordered_set< AZ::EntityId > m_bookmarkAnchors;
    };

    class SceneComponent
        : public GraphCanvasPropertyComponent
        , public AZ::EntityBus::Handler
        , public SceneRequestBus::Handler
        , public GeometryNotificationBus::MultiHandler
        , public VisualNotificationBus::MultiHandler
        , public ViewNotificationBus::Handler
        , public SceneMimeDelegateRequestBus::Handler
        , public EntitySaveDataRequestBus::Handler
        , public SceneBookmarkActionBus::Handler
        , public StyleManagerNotificationBus::Handler
    {
    private:
        friend class GraphCanvasGraphicsScene;
        friend class SceneComponentSaveData;

    public:
        AZ_COMPONENT(SceneComponent, "{3F71486C-3D51-431F-B904-DA070C7A0238}", GraphCanvasPropertyComponent);
        static void Reflect(AZ::ReflectContext* context);

        struct GraphCanvasConstructSaveData
        {
            AZ_CLASS_ALLOCATOR(GraphCanvasConstructSaveData, AZ::SystemAllocator, 0);
            AZ_RTTI(GraphCanvasConstructSaveData, "{C074944F-8218-4753-94EE-1C5CC02DE8E4}")
                enum class GraphCanvasConstructType
            {
                CommentNode,
                BlockCommentNode,
                BookmarkAnchor,

                Unknown
            };

            virtual ~GraphCanvasConstructSaveData() = default;

            GraphCanvasConstructType m_constructType = GraphCanvasConstructType::Unknown;
            EntitySaveDataContainer m_saveDataContainer;
        };

        class SceneComponentSaveData
            : public ComponentSaveData
        {
        public:
            AZ_RTTI(SceneComponentSaveData, "{5F84B500-8C45-40D1-8EFC-A5306B241444}", ComponentSaveData);
            AZ_CLASS_ALLOCATOR(SceneComponentSaveData, AZ::SystemAllocator, 0);

            SceneComponentSaveData()
                : m_bookmarkCounter(0)
            {
            }

            ~SceneComponentSaveData() = default;

            void ClearConstructData()
            {
                for (GraphCanvasConstructSaveData* saveData : m_constructs)
                {
                    delete saveData;
                }

                m_constructs.clear();
            }

            AZStd::vector< GraphCanvasConstructSaveData* > m_constructs;
            ViewParams                                     m_viewParams;
            AZ::u32                                        m_bookmarkCounter;
        };

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
        }

        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // EntityBus
        void OnEntityExists(const AZ::EntityId& entityId) override;
        ////

        // EntitySaveDataRequest
        void WriteSaveData(EntitySaveDataContainer& saveDataContainer) const override;
        void ReadSaveData(const EntitySaveDataContainer& saveDataContainer) override;
        ////

        SceneRequests* GetSceneRequests() { return this; }
        const SceneRequests* GetSceneRequestsConst() const { return this; }

        GraphData* GetGraphData() override { return &m_graphData; };
        const GraphData* GetGraphDataConst() const override { return &m_graphData; };

        // SceneRequestBus
        AZStd::any* GetUserData() override;
        const AZStd::any* GetUserDataConst() const override;
        AZ::Entity* GetSceneEntity() const override { return GetEntity(); }

        void SetEditorId(const EditorId& editorId) override;
        EditorId GetEditorId() const override;

        AZ::EntityId GetGrid() const override;

        AZ::EntityId CreatePulse(const AnimatedPulseConfiguration& pulseConfiguration) override;
        AZ::EntityId CreatePulseAroundSceneMember(const AZ::EntityId& memberId, int gridSteps, AnimatedPulseConfiguration configuration) override;
        AZ::EntityId CreateCircularPulse(const AZ::Vector2& centerPoint, float initialRadius, float finalRadius, AnimatedPulseConfiguration configuration) override;
        void CancelPulse(const AZ::EntityId& pulseId) override;

        bool AddNode(const AZ::EntityId& nodeId, const AZ::Vector2& position) override;
        void AddNodes(const AZStd::vector<AZ::EntityId>& nodeIds) override;
        bool RemoveNode(const AZ::EntityId& nodeId) override;

        AZStd::vector<AZ::EntityId> GetNodes() const override;

        AZStd::vector<AZ::EntityId> GetSelectedNodes() const override;

        bool TrySpliceNodeOntoConnection(const AZ::EntityId& node, const AZ::EntityId& connectionId) override;
        bool TrySpliceNodeTreeOntoConnection(const AZStd::unordered_set< NodeId >& entryNodes, const AZStd::unordered_set< NodeId >& exitNodes, const ConnectionId& connectionId) override;

        void DeleteNodeAndStitchConnections(const AZ::EntityId& node) override;

        bool TryCreateConnectionsBetween(const AZStd::vector< Endpoint >& endpoints, const AZ::EntityId& targetNode) override;

        AZ::EntityId CreateConnectionBetween(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) override;
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

        bool AddBookmarkAnchor(const AZ::EntityId& bookmarkAnchorId, const AZ::Vector2& position) override;
        bool RemoveBookmarkAnchor(const AZ::EntityId& bookmarkAnchorId) override;

        bool Add(const AZ::EntityId& entity) override;
        bool Remove(const AZ::EntityId& entity) override;

        void ClearSelection() override;

        void SetSelectedArea(const AZ::Vector2& topLeft, const AZ::Vector2& topRight) override;

        bool HasSelectedItems() const override;
        bool HasCopiableSelection() const override;

        bool HasEntitiesAt(const AZ::Vector2& scenePoint) const override;

        AZStd::vector<AZ::EntityId> GetSelectedItems() const override;

        QGraphicsScene* AsQGraphicsScene() override;

        void CopySelection() const override;
        void Copy(const AZStd::vector<AZ::EntityId>& itemIds) const override;

        void CutSelection() override;
        void Cut(const AZStd::vector<AZ::EntityId>& itemIds) override;

        void Paste() override;
        void PasteAt(const QPointF& scenePos) override;

        void SerializeEntities(const AZStd::unordered_set<AZ::EntityId>& itemIds, GraphSerialization& serializationTarget) const override;
        void DeserializeEntities(const QPointF& scenePos, const GraphSerialization& serializationTarget) override;

        void DuplicateSelection() override;
        void DuplicateSelectionAt(const QPointF& scenePos) override;
        void Duplicate(const AZStd::vector<AZ::EntityId>& itemIds) override;
        void DuplicateAt(const AZStd::vector<AZ::EntityId>& itemIds, const QPointF& scenePos) override;

        void DeleteSelection() override;
        void Delete(const AZStd::unordered_set<AZ::EntityId>& itemIds) override;
        void DeleteGraphData(const GraphData& graphData) override;

        void SuppressNextContextMenu() override;

        const AZStd::string& GetCopyMimeType() const override;
        void SetMimeType(const char* mimeType) override;

        AZStd::vector<AZ::EntityId> GetEntitiesAt(const AZ::Vector2& position) const override;
        AZStd::vector<AZ::EntityId> GetEntitiesInRect(const QRectF& rect, Qt::ItemSelectionMode mode) const override;

        AZStd::vector<Endpoint> GetEndpointsInRect(const QRectF& rect) const override;

        void RegisterView(const AZ::EntityId& viewId) override;
        void RemoveView(const AZ::EntityId& viewId) override;
        ViewId GetViewId() const override;

        void DispatchSceneDropEvent(const AZ::Vector2& scenePos, const QMimeData* mimeData) override;

        bool AddGraphData(const GraphData& graphData) override;
        void RemoveGraphData(const GraphData& graphData) override;

        void SetDragSelectionType(DragSelectionType dragSelectionType) override;

        void SignalDragSelectStart() override;
        void SignalDragSelectEnd() override;
        ////

        // VisualNotificationBus
        bool OnMousePress(const AZ::EntityId& sourceId, const QGraphicsSceneMouseEvent* mouseEvent) override;
        bool OnMouseRelease(const AZ::EntityId& sourceId, const QGraphicsSceneMouseEvent* mouseEvent) override;
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

        // SceneBookmarkActionBus
        AZ::u32 GetNewBookmarkCounter() override;
        ////

        // StyleManagerNotificationBus
        void OnStylesLoaded() override;
        ////

    protected:

        void OnSceneDragEnter(const QMimeData* mimeData);
        void OnSceneDragMoveEvent(const QPointF& scenePoint, const QMimeData* mimeData);
        void OnSceneDropEvent(const QPointF& scenePoint, const QMimeData* mimeData);
        void OnSceneDragExit(const QMimeData* mimeData);

        bool HasActiveMimeDelegates() const;

        bool IsConnectableNode(const AZ::EntityId& entityId);
        bool IsNodeOrWrapperSelected(const NodeId& entityId);
        bool IsSpliceableConnection(const AZ::EntityId& entityId);
        bool ParseSceneMembersIntoTree(const AZStd::vector<AZ::EntityId>& sourceSceneMembers, AZStd::unordered_set<NodeId>& entryNodes, AZStd::unordered_set<ConnectionId>& entryConnections, AZStd::unordered_set<NodeId>& exitNodes, AZStd::unordered_set<ConnectionId>& exitConnections, AZStd::unordered_set<NodeId>& internalNodes, AZStd::unordered_set<ConnectionId>& internalConnections);

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

        void RegisterSelectionItem(const AZ::EntityId& itemId);
        void UnregisterSelectionItem(const AZ::EntityId& itemId);

        void AddSceneMember(const AZ::EntityId& item, bool positionItem = false, const AZ::Vector2& position = AZ::Vector2());

        AZStd::unordered_set<AZ::EntityId> FindConnections(const AZStd::unordered_set<AZ::EntityId>& nodeIds, bool internalConnectionsOnly = false) const override;

        //! Validates that the node ids in the connection can be found in the supplied node set
        bool ValidateConnectionEndpoints(AZ::Entity* connectionRef, const AZStd::unordered_set<AZ::Entity*>& nodeRefs);

        //! Seives a set of entity id's into a node, connection and group entityId set based on if they are in the scene
        void SieveSceneMembers(const AZStd::unordered_set<AZ::EntityId>& itemIds, SceneMemberBuckets& buckets) const;

        QPointF GetViewCenterScenePoint() const;

        void OnCursorMoveForSplice();
        void OnTrySplice();

        void InitiateSpliceToNode(const NodeId& nodeId);
        void InitiateSpliceToConnection(const AZStd::vector<ConnectionId>& connectionIds);

        bool m_allowReset;
        QPointF m_pasteOffset;

        int m_deleteCount;
        AZStd::string m_copyMimeType;

        AZ::EntityId m_grid;
        AZStd::unordered_map<QGraphicsItem*, AZ::EntityId> m_itemLookup;

        ViewId m_viewId;
        ViewParams m_viewParams;

        SceneMimeDelegate m_mimeDelegate;

        GraphData m_graphData;

        AZStd::unordered_set<AZ::EntityId> m_delegates;
        AZStd::unordered_set<AZ::EntityId> m_activeDelegates;
        AZStd::unordered_set<AZ::EntityId> m_interestedDelegates;

        bool m_addingGraphData;

        AZStd::unique_ptr<GraphCanvasGraphicsScene> m_graphicsSceneUi;

        DragSelectionType m_dragSelectionType;

        bool m_activateScene;
        bool m_isDragSelecting;

        AZ::EntityId m_pressedEntity;
        AZ::Vector2  m_originalPosition;

        bool m_forceDragReleaseUndo;
        bool m_isDraggingEntity;

        // Elements for handling with the drag onto objects
        QTimer m_spliceTimer;
        bool m_enableSpliceTracking;
        
        bool m_enableNodeDragConnectionSpliceTracking;
        bool m_enableNodeDragCouplingTracking;

        bool m_enableNodeChainDragConnectionSpliceTracking;

        AZStd::unordered_set< NodeId > m_treeSpliceEntryNodes;
        AZStd::unordered_set< NodeId > m_treeSpliceExitNodes;
        AZStd::unordered_set< NodeId > m_treeSpliceInternalNodes;

        AZ::EntityId m_spliceTarget;

        StateSetter<RootGraphicsItemDisplayState> m_spliceTargetDisplayStateStateSetter;
        StateSetter<RootGraphicsItemDisplayState> m_pressedEntityDisplayStateStateSetter;
        ////

        AZ::u32 m_bookmarkCounter;

        EditorId m_editorId;
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
        void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
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
