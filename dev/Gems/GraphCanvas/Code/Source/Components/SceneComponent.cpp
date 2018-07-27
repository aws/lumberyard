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
#include <precompiled.h>

#include <functional>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QGraphicsItem>
#include <QGraphicsSceneEvent>
#include <QGraphicsView>
#include <QMimeData>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/sort.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

#include <Components/SceneComponent.h>

#include <Components/BookmarkManagerComponent.h>
#include <Components/BookmarkAnchor/BookmarkAnchorComponent.h>
#include <Components/Connections/ConnectionComponent.h>
#include <Components/GridComponent.h>
#include <Components/Nodes/NodeComponent.h>
#include <Components/SceneMemberComponent.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/Nodes/NodeUIBus.h>
#include <GraphCanvas/Components/Nodes/Wrapper/WrapperNodeBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Editor/GraphCanvasProfiler.h>
#include <GraphCanvas/Editor/GraphModelBus.h>
#include <GraphCanvas/Styling/Parser.h>
#include <GraphCanvas/tools.h>
#include <GraphCanvas/Types/GraphCanvasGraphSerialization.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>
#include <GraphCanvas/Widgets/GraphCanvasMimeContainer.h>
#include <GraphCanvas/Widgets/MimeEvents/CreateSplicingNodeMimeEvent.h>
#include <GraphCanvas/Utils/QtVectorMath.h>
#include <Utils/ConversionUtils.h>

namespace GraphCanvas
{
    //////////////////////
    // SceneMimeDelegate
    //////////////////////

    void SceneMimeDelegate::Activate()
    {
        m_pushedUndoBlock = false;
        m_enableConnectionSplicing = false;

        m_spliceTimer.setInterval(500);
        m_spliceTimer.setSingleShot(true);

        QObject::connect(&m_spliceTimer, &QTimer::timeout, [this]() { OnTrySplice(); });

        SceneMimeDelegateHandlerRequestBus::Handler::BusConnect(m_sceneId);
        SceneMimeDelegateRequestBus::Event(m_sceneId, &SceneMimeDelegateRequests::AddDelegate, m_sceneId);
    }

    void SceneMimeDelegate::Deactivate()
    {
        SceneMimeDelegateRequestBus::Event(m_sceneId, &SceneMimeDelegateRequests::RemoveDelegate, m_sceneId);
        SceneMimeDelegateHandlerRequestBus::Handler::BusDisconnect();
    }

    void SceneMimeDelegate::SetSceneId(const AZ::EntityId& sceneId)
    {
        m_sceneId = sceneId;
    }

    void SceneMimeDelegate::SetEditorId(const EditorId& editorId)
    {
        m_editorId = editorId;
    }

    void SceneMimeDelegate::SetMimeType(const char* mimeType)
    {
        m_mimeType = mimeType;
    }

    bool SceneMimeDelegate::IsInterestedInMimeData(const AZ::EntityId&, const QMimeData* mimeData)
    {
        bool isInterested = !m_mimeType.isEmpty() && mimeData->hasFormat(m_mimeType);
        m_enableConnectionSplicing = false;

        if (isInterested)
        {
            // Need a copy since we are going to try to use
            // this event not in response to a movement, but in response to a timeout.
            m_splicingData = mimeData->data(m_mimeType);

            GraphCanvasMimeContainer mimeContainer;

            // Splicing only makes sense when we have a single node.
            if (mimeContainer.FromBuffer(m_splicingData.constData(), m_splicingData.size()) 
                && mimeContainer.m_mimeEvents.size() == 1
                && azrtti_istypeof<CreateSplicingNodeMimeEvent>(mimeContainer.m_mimeEvents.front()))
            {
                m_enableConnectionSplicing = true;

                AZ_Error("GraphCanvas", !m_splicingNode.IsValid(), "Splicing node not properly invalidated in between interest calls.");
                m_splicingNode.SetInvalid();

                m_splicingPath = QPainterPath();
            }
            else
            {
                m_splicingData.clear();
            }
        }

        return isInterested;
    }    

    void SceneMimeDelegate::HandleMove(const AZ::EntityId& sceneId, const QPointF& dragPoint, const QMimeData* mimeData)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        bool enableSplicing = false;
        AssetEditorSettingsRequestBus::EventResult(enableSplicing, m_editorId, &AssetEditorSettingsRequests::IsDropConnectionSpliceEnabled);

        if (!enableSplicing || !m_enableConnectionSplicing)
        {
            return;
        }

        if (m_splicingNode.IsValid() || !m_splicingPath.isEmpty())
        {
            if (!m_splicingPath.contains(dragPoint))
            {
                CancelSplice();
            }
            else if (m_splicingNode.IsValid())
            {
                PushUndoBlock();
                GeometryRequestBus::Event(m_splicingNode, &GeometryRequests::SetPosition, ConversionUtils::QPointToVector(dragPoint) + m_positionOffset);
                PopUndoBlock();
            }
        }

        if (!m_splicingNode.IsValid() && m_splicingPath.isEmpty())
        {
            AZ::EntityId targetId;
            AZ::Vector2 targetVector(dragPoint.x(), dragPoint.y());

            AZStd::vector< AZ::EntityId > connectionEntities;
            SceneRequestBus::EventResult(connectionEntities, m_sceneId, &SceneRequests::GetEntitiesAt, targetVector);

            int hitCounter = 0;

            for (AZ::EntityId currentEntity : connectionEntities)
            {
                if (ConnectionRequestBus::FindFirstHandler(currentEntity) != nullptr)
                {
                    ++hitCounter;
                    QGraphicsItem* connectionObject = nullptr;
                    VisualRequestBus::EventResult(connectionObject, currentEntity, &VisualRequests::AsGraphicsItem);

                    m_splicingPath = connectionObject->shape();

                    targetId = currentEntity;
                }
            }

            // Only want to do the splicing if it's unambiguous which thing they are over.
            if (hitCounter == 1)
            {
                if (m_targetConnection != targetId)
                {
                    m_targetConnection = targetId;
                    m_targetPosition = dragPoint;

                    StateController<RootGraphicsItemDisplayState>* stateController = nullptr;
                    RootGraphicsItemRequestBus::EventResult(stateController, m_targetConnection, &RootGraphicsItemRequests::GetDisplayStateStateController);

                    if (stateController)
                    {
                        m_displayStateStateSetter.AddStateController(stateController);
                        m_displayStateStateSetter.SetState(RootGraphicsItemDisplayState::Preview);
                    }

                    AZStd::chrono::milliseconds spliceDuration(500);
                    AssetEditorSettingsRequestBus::EventResult(spliceDuration, m_editorId, &AssetEditorSettingsRequests::GetDropConnectionSpliceTime);

                    m_spliceTimer.stop();
                    m_spliceTimer.setInterval(spliceDuration.count());
                    m_spliceTimer.start();
                }
            }
            else
            {
                if (m_targetConnection.IsValid())
                {
                    m_displayStateStateSetter.ResetStateSetter();

                    m_targetConnection.SetInvalid();
                    m_spliceTimer.stop();
                }

                if (hitCounter > 0)
                {
                    m_splicingPath = QPainterPath();
                }
            }
        }        
    }

    void SceneMimeDelegate::HandleDrop(const AZ::EntityId& sceneId, const QPointF& dropPoint, const QMimeData* mimeData)
    {
        GRAPH_CANVAS_PROFILE_FUNCTION();

        m_spliceTimer.stop();

        m_displayStateStateSetter.ResetStateSetter();

        if (m_splicingNode.IsValid())
        {
            // Once we finalize the node, we want to release the undo state, and push a new undo.
            GraphModelRequestBus::Event(m_sceneId, &GraphModelRequests::RequestUndoPoint);

            SceneRequestBus::Event(m_sceneId, &SceneRequests::ClearSelection);
            SceneMemberUIRequestBus::Event(m_splicingNode, &SceneMemberUIRequests::SetSelected, true);

            m_splicingData.clear();
            m_splicingNode.SetInvalid();

            m_targetConnection.SetInvalid();

            m_splicingPath = QPainterPath();

            return;
        }

        if (!mimeData->hasFormat(m_mimeType))
        {
            AZ_Error("SceneMimeDelegate", false, "Handling an event that does not meet our Mime requirements");
            return;
        }

        QByteArray arrayData = mimeData->data(m_mimeType);

        GraphCanvasMimeContainer mimeContainer;

        if (!mimeContainer.FromBuffer(arrayData.constData(), arrayData.size()) || mimeContainer.m_mimeEvents.empty())
        {
            return;
        }

        bool success = false;

        AZ::Vector2 sceneMousePoint = AZ::Vector2(dropPoint.x(), dropPoint.y());
        AZ::Vector2 sceneDropPoint = sceneMousePoint;
        
        QGraphicsScene* graphicsScene = nullptr;
        SceneRequestBus::EventResult(graphicsScene, m_sceneId, &SceneRequests::AsQGraphicsScene);
 
        if (graphicsScene)
        {
            graphicsScene->blockSignals(true);
        }

        SceneRequestBus::Event(m_sceneId, &SceneRequests::ClearSelection);

        for (GraphCanvasMimeEvent* mimeEvent : mimeContainer.m_mimeEvents)
        {
            if (mimeEvent->ExecuteEvent(sceneMousePoint, sceneDropPoint, m_sceneId))
            {
                success = true;
            }
        }

        if (success)
        {
            SceneNotificationBus::Event(m_sceneId, &SceneNotifications::PostCreationEvent);
        }
        
        if (graphicsScene)
        {
            graphicsScene->blockSignals(false);
            emit graphicsScene->selectionChanged();
        }
    }

    void SceneMimeDelegate::HandleLeave(const AZ::EntityId& sceneId, const QMimeData* mimeData)
    {
        CancelSplice();
    }

    void SceneMimeDelegate::OnTrySplice()
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        if (m_targetConnection.IsValid())
        {
            GraphCanvasMimeContainer mimeContainer;

            if (mimeContainer.FromBuffer(m_splicingData.constData(), m_splicingData.size()))
            {
                if (mimeContainer.m_mimeEvents.empty())
                {
                    return;
                }

                ConnectionRequestBus::EventResult(m_spliceSource, m_targetConnection, &ConnectionRequests::GetSourceEndpoint);
                ConnectionRequestBus::EventResult(m_spliceTarget, m_targetConnection, &ConnectionRequests::GetTargetEndpoint);

                CreateSplicingNodeMimeEvent* mimeEvent = azrtti_cast<CreateSplicingNodeMimeEvent*>(mimeContainer.m_mimeEvents.front());

                if (mimeEvent)
                {
                    PushUndoBlock();

                    m_splicingNode = mimeEvent->CreateSplicingNode(m_sceneId);

                    if (m_splicingNode.IsValid())
                    {
                        SceneRequestBus::Event(m_sceneId, &SceneRequests::AddNode, m_splicingNode, ConversionUtils::QPointToVector(m_targetPosition));

                        bool allowNode = false;
                        SceneRequestBus::EventResult(allowNode, m_sceneId, &SceneRequests::TrySpliceNodeOntoConnection, m_splicingNode, m_targetConnection);

                        if (!allowNode)
                        {
                            AZStd::unordered_set<AZ::EntityId> deleteIds = { m_splicingNode };
                            SceneRequestBus::Event(m_sceneId, &SceneRequests::Delete, deleteIds);

                            m_splicingNode.SetInvalid();
                        }
                        else
                        {
                            m_displayStateStateSetter.ResetStateSetter();

                            StateController<RootGraphicsItemDisplayState>* stateController = nullptr;

                            RootGraphicsItemRequestBus::EventResult(stateController, m_splicingNode, &RootGraphicsItemRequests::GetDisplayStateStateController);
                            m_displayStateStateSetter.AddStateController(stateController);

                            AZStd::vector< AZ::EntityId > slotIds;
                            NodeRequestBus::EventResult(slotIds, m_splicingNode, &NodeRequests::GetSlotIds);

                            AZ::Vector2 centerPoint(0,0);
                            int totalSamples = 0;

                            for (const AZ::EntityId& slotId : slotIds)
                            {
                                AZStd::vector< AZ::EntityId > connectionIds;
                                SlotRequestBus::EventResult(connectionIds, slotId, &SlotRequests::GetConnections);

                                if (!connectionIds.empty())
                                {
                                    QPointF slotPosition;
                                    SlotUIRequestBus::EventResult(slotPosition, slotId, &SlotUIRequests::GetConnectionPoint);

                                    centerPoint += ConversionUtils::QPointToVector(slotPosition);
                                    ++totalSamples;

                                    for (const AZ::EntityId& connectionId : connectionIds)
                                    {
                                        stateController = nullptr;
                                        RootGraphicsItemRequestBus::EventResult(stateController, connectionId, &RootGraphicsItemRequests::GetDisplayStateStateController);

                                        m_displayStateStateSetter.AddStateController(stateController);
                                    }
                                }
                            }

                            if (totalSamples > 0)
                            {
                                centerPoint /= totalSamples;
                            }

                            m_positionOffset = ConversionUtils::QPointToVector(m_targetPosition) - centerPoint;
                            GeometryRequestBus::Event(m_splicingNode, &GeometryRequests::SetPosition, m_positionOffset + ConversionUtils::QPointToVector(m_targetPosition));

                            m_displayStateStateSetter.SetState(RootGraphicsItemDisplayState::Preview);

                            AnimatedPulseConfiguration pulseConfiguration;

                            pulseConfiguration.m_drawColor = QColor(255, 255, 255);
                            pulseConfiguration.m_durationSec = 0.35f;
                            pulseConfiguration.m_enableGradient = true;

                            QGraphicsItem* item = nullptr;
                            SceneMemberUIRequestBus::EventResult(item, m_splicingNode, &SceneMemberUIRequests::GetRootGraphicsItem);

                            if (item)
                            {
                                pulseConfiguration.m_zValue = item->zValue() - 1;
                            }

                            const int k_squaresToPulse = 4;

                            SceneRequestBus::Event(m_sceneId, &SceneRequests::CreatePulseAroundSceneMember, m_splicingNode, k_squaresToPulse, pulseConfiguration);
                        }
                    }

                    PopUndoBlock();
                }
            }
        }
    }

    void SceneMimeDelegate::CancelSplice()
    {
        m_displayStateStateSetter.ResetStateSetter();
        m_spliceTimer.stop();
        m_splicingPath = QPainterPath();

        if (m_splicingNode.IsValid())
        {
            PushUndoBlock();
            AZStd::unordered_set< AZ::EntityId > deleteIds = { m_splicingNode };
            SceneRequestBus::Event(m_sceneId, &SceneRequests::Delete, deleteIds);

            m_splicingNode.SetInvalid();

            AZ::EntityId connectionId;
            SlotRequestBus::EventResult(connectionId, m_spliceSource.GetSlotId(), &SlotRequests::CreateConnectionWithEndpoint, m_spliceTarget);

            GraphModelRequestBus::Event(m_sceneId, &GraphModelRequests::CreateConnection, connectionId, m_spliceSource, m_spliceTarget);

            AZ_Error("GraphCanvas", connectionId.IsValid(), "Failed to recreate a connection after unsplicing a spliced node.");

            PopUndoBlock();
        }
    }

    void SceneMimeDelegate::PushUndoBlock()
    {
        if (!m_pushedUndoBlock)
        {
            GraphModelRequestBus::Event(m_sceneId, &GraphModelRequests::RequestPushPreventUndoStateUpdate);

            m_pushedUndoBlock = true;
        }
    }

    void SceneMimeDelegate::PopUndoBlock()
    {
        if (m_pushedUndoBlock)
        {
            m_pushedUndoBlock = false;

            GraphModelRequestBus::Event(m_sceneId, &GraphModelRequests::RequestPopPreventUndoStateUpdate);
        }
    }

    ///////////////
    // Copy Utils
    ///////////////

    void SerializeToBuffer(const GraphSerialization& serializationTarget, AZStd::vector<char>& buffer)
    {
        AZ::SerializeContext* serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();

        AZ::IO::ByteContainerStream<AZStd::vector<char>> stream(&buffer);
        AZ::Utils::SaveObjectToStream(stream, AZ::DataStream::ST_BINARY, &serializationTarget, serializeContext);
    }

    void SerializeToClipboard(const GraphSerialization& serializationTarget)
    {
        AZ_Error("Graph Canvas", !serializationTarget.GetSerializationKey().empty(), "Serialization Key not server for scene serialization. Cannot push to clipboard.");
        if (serializationTarget.GetSerializationKey().empty())
        {
            return;
        }

        AZ::SerializeContext* serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();
        
        AZStd::vector<char> buffer;
        SerializeToBuffer(serializationTarget, buffer);

        QMimeData* mime = new QMimeData();
        mime->setData(serializationTarget.GetSerializationKey().c_str(), QByteArray(buffer.cbegin(), static_cast<int>(buffer.size())));
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setMimeData(mime);
    }

    ///////////////////
    // SceneComponent
    ///////////////////

    void BuildEndpointMap(GraphData& graphData)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        graphData.m_endpointMap.clear();
        for (auto& connectionEntity : graphData.m_connections)
        {
            auto* connection = connectionEntity ? AZ::EntityUtils::FindFirstDerivedComponent<ConnectionComponent>(connectionEntity) : nullptr;
            if (connection)
            {
                graphData.m_endpointMap.emplace(connection->GetSourceEndpoint(), connection->GetTargetEndpoint());
                graphData.m_endpointMap.emplace(connection->GetTargetEndpoint(), connection->GetSourceEndpoint());
            }
        }
    }

    class GraphCanvasSceneDataEventHandler : public AZ::SerializeContext::IEventHandler
    {
    public:
        /// Called to rebuild the Endpoint map
        void OnWriteEnd(void* classPtr) override
        {
            auto* sceneData = reinterpret_cast<GraphData*>(classPtr);

            BuildEndpointMap((*sceneData));
        }
    };

    ///////////////////
    // SceneComponent
    ///////////////////

    static const char* k_copyPasteKey = "GraphCanvasScene";

    void SceneComponent::Reflect(AZ::ReflectContext* context)
    {
        GraphSerialization::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }
            
        serializeContext->Class<GraphData>()
            ->Version(2)
            ->EventHandler<GraphCanvasSceneDataEventHandler>()
            ->Field("m_nodes", &GraphData::m_nodes)
            ->Field("m_connections", &GraphData::m_connections)
            ->Field("m_userData", &GraphData::m_userData)
            ->Field("m_bookmarkAnchors", &GraphData::m_bookmarkAnchors)
        ;

        serializeContext->Class<GraphCanvasConstructSaveData>()
            ->Version(1)
            ->Field("Type", &GraphCanvasConstructSaveData::m_constructType)
            ->Field("DataContainer", &GraphCanvasConstructSaveData::m_saveDataContainer)
        ;

        serializeContext->Class<ViewParams>()
            ->Version(1)
            ->Field("Scale", &ViewParams::m_scale)
            ->Field("AnchorX", &ViewParams::m_anchorPointX)
            ->Field("AnchorY", &ViewParams::m_anchorPointY)
            ;

        serializeContext->Class<SceneComponentSaveData>()
            ->Version(3)
            ->Field("Constructs", &SceneComponentSaveData::m_constructs)
            ->Field("ViewParams", &SceneComponentSaveData::m_viewParams)
            ->Field("BookmarkCounter", &SceneComponentSaveData::m_bookmarkCounter)
        ;

        serializeContext->Class<SceneComponent, GraphCanvasPropertyComponent>()
            ->Version(3)
            ->Field("SceneData", &SceneComponent::m_graphData)
            ->Field("ViewParams", &SceneComponent::m_viewParams)
        ;
    }

    SceneComponent::SceneComponent()
        : m_allowReset(false)
        , m_deleteCount(0)
        , m_dragSelectionType(DragSelectionType::OnRelease)
        , m_activateScene(true)
        , m_isDragSelecting(false)
        , m_originalPosition(0,0)
        , m_forceDragReleaseUndo(false)
        , m_isDraggingEntity(false)
        , m_enableSpliceTracking(false)
        , m_enableNodeDragConnectionSpliceTracking(false)
        , m_enableNodeDragCouplingTracking(false)
        , m_bookmarkCounter(0)
    {
        m_spliceTimer.setInterval(500);
        m_spliceTimer.setSingleShot(true);

        QObject::connect(&m_spliceTimer, &QTimer::timeout, [this]() { OnTrySplice(); });
    }

    SceneComponent::~SceneComponent()
    {
        DestroyItems(m_graphData.m_nodes);
        DestroyItems(m_graphData.m_connections);
        DestroyItems(m_graphData.m_bookmarkAnchors);
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, m_grid);
    }

    void SceneComponent::Init()
    {        
        GRAPH_CANVAS_PROFILE_FUNCTION();
        GraphCanvasPropertyComponent::Init();

        // Make the QGraphicsScene Ui element for managing Qt scene items
        m_graphicsSceneUi = AZStd::make_unique<GraphCanvasGraphicsScene>(*this);

        AZ::EntityBus::Handler::BusConnect(GetEntityId());

        AZ::Entity* gridEntity = GridComponent::CreateDefaultEntity();
        m_grid = gridEntity->GetId();

        InitItems(m_graphData.m_nodes);
        InitConnections();
        InitItems(m_graphData.m_bookmarkAnchors);

        // Grids needs to be active for the save information parsing to work correctly
        ActivateItems(AZStd::initializer_list<AZ::Entity*>{ AzToolsFramework::GetEntity(m_grid) });
        ////

        EntitySaveDataRequestBus::Handler::BusConnect(GetEntityId());
    }

    void SceneComponent::Activate()
    {
        GRAPH_CANVAS_PROFILE_FUNCTION();
        GraphCanvasPropertyComponent::Activate();

        // Need to register this before activating saved nodes. Otherwise data is not properly setup.
        SceneRequestBus::Handler::BusConnect(GetEntityId());
        SceneMimeDelegateRequestBus::Handler::BusConnect(GetEntityId());
        SceneBookmarkActionBus::Handler::BusConnect(GetEntityId());
        
        // Only want to activate the scene if we have something to activate
        // Otherwise elements may be repeatedly activated/registered to the scene.
        m_activateScene = !m_graphData.m_nodes.empty() || !m_graphData.m_bookmarkAnchors.empty();

        m_mimeDelegate.SetSceneId(GetEntityId());

        m_mimeDelegate.Activate();
    }

    void SceneComponent::Deactivate()
    {
        GRAPH_CANVAS_PROFILE_FUNCTION();
        m_mimeDelegate.Deactivate();

        GraphCanvasPropertyComponent::Deactivate();
        
        SceneBookmarkActionBus::Handler::BusDisconnect();
        SceneMimeDelegateRequestBus::Handler::BusDisconnect();
        SceneRequestBus::Handler::BusDisconnect();

        m_activeDelegates.clear();

        DeactivateItems(m_graphData.m_connections);
        DeactivateItems(m_graphData.m_nodes);
        DeactivateItems(m_graphData.m_bookmarkAnchors);
        DeactivateItems(AZStd::initializer_list<AZ::Entity*>{ AzToolsFramework::GetEntity(m_grid) });
        SceneMemberRequestBus::Event(m_grid, &SceneMemberRequests::ClearScene, GetEntityId());
    }

    void SceneComponent::OnEntityExists(const AZ::EntityId& entityId)
    {
        AZ::Entity* entity = GetEntity();

        // A less then ideal way of doing version control on the scenes
        BookmarkManagerComponent* bookmarkComponent = entity->FindComponent<BookmarkManagerComponent>();

        if (bookmarkComponent == nullptr)
        {
            entity->CreateComponent<BookmarkManagerComponent>();
        }

        AZ::EntityBus::Handler::BusDisconnect(GetEntityId());
    }

    void SceneComponent::WriteSaveData(EntitySaveDataContainer& saveDataContainer) const
    {
        GRAPH_CANVAS_PROFILE_FUNCTION();
        SceneComponentSaveData* saveData = saveDataContainer.FindCreateSaveData<SceneComponentSaveData>();        
        saveData->ClearConstructData();

        for (AZ::Entity* currentEntity : m_graphData.m_nodes)
        {
            if (CommentRequestBus::FindFirstHandler(currentEntity->GetId()) != nullptr)
            {
                GraphCanvasConstructSaveData* constructSaveData = aznew GraphCanvasConstructSaveData();

                if (BlockCommentRequestBus::FindFirstHandler(currentEntity->GetId()) == nullptr)
                {
                    constructSaveData->m_constructType = GraphCanvasConstructSaveData::GraphCanvasConstructType::CommentNode;
                }
                else
                {
                    constructSaveData->m_constructType = GraphCanvasConstructSaveData::GraphCanvasConstructType::BlockCommentNode;
                }

                EntitySaveDataRequestBus::Event(currentEntity->GetId(), &EntitySaveDataRequests::WriteSaveData, constructSaveData->m_saveDataContainer);

                saveData->m_constructs.push_back(constructSaveData);
            }
        }

        saveData->m_constructs.reserve(saveData->m_constructs.size() + m_graphData.m_bookmarkAnchors.size());

        for (AZ::Entity* currentEntity : m_graphData.m_bookmarkAnchors)
        {
            GraphCanvasConstructSaveData* constructSaveData = aznew GraphCanvasConstructSaveData();

            constructSaveData->m_constructType = GraphCanvasConstructSaveData::GraphCanvasConstructType::BookmarkAnchor;
            EntitySaveDataRequestBus::Event(currentEntity->GetId(), &EntitySaveDataRequests::WriteSaveData, constructSaveData->m_saveDataContainer);

            saveData->m_constructs.push_back(constructSaveData);
        }

        saveData->m_viewParams = m_viewParams;
        saveData->m_bookmarkCounter = m_bookmarkCounter;
    }

    void SceneComponent::ReadSaveData(const EntitySaveDataContainer& saveDataContainer)
    {
        GRAPH_CANVAS_PROFILE_FUNCTION();
        const SceneComponentSaveData* saveData = saveDataContainer.FindSaveDataAs<SceneComponentSaveData>();

        for (const GraphCanvasConstructSaveData* currentConstruct : saveData->m_constructs)
        {
            AZ::Entity* constructEntity = nullptr;
            switch (currentConstruct->m_constructType)
            {
            case GraphCanvasConstructSaveData::GraphCanvasConstructType::CommentNode:
                GraphCanvasRequestBus::BroadcastResult(constructEntity, &GraphCanvasRequests::CreateCommentNode);
                break;
            case GraphCanvasConstructSaveData::GraphCanvasConstructType::BlockCommentNode:
                GraphCanvasRequestBus::BroadcastResult(constructEntity, &GraphCanvasRequests::CreateBlockCommentNode);
                break;
            case GraphCanvasConstructSaveData::GraphCanvasConstructType::BookmarkAnchor:
                GraphCanvasRequestBus::BroadcastResult(constructEntity, &GraphCanvasRequests::CreateBookmarkAnchor);
                break;
            default:
                break;
            }

            if (constructEntity)
            {
                constructEntity->Init();
                constructEntity->Activate();

                EntitySaveDataRequestBus::Event(constructEntity->GetId(), &EntitySaveDataRequests::ReadSaveData, currentConstruct->m_saveDataContainer);

                Add(constructEntity->GetId());
            }
        }

        m_viewParams = saveData->m_viewParams;
        m_bookmarkCounter = saveData->m_bookmarkCounter;
    }

    AZStd::any* SceneComponent::GetUserData()
    {
        return &m_graphData.m_userData;
    }

    const AZStd::any* SceneComponent::GetUserDataConst() const
    {
        return &m_graphData.m_userData;
    }

    void SceneComponent::SetEditorId(const EditorId& editorId)
    {
        if (m_editorId != editorId)
        {
            m_editorId = editorId;
            m_mimeDelegate.SetEditorId(editorId);

            StyleManagerNotificationBus::Handler::BusConnect(m_editorId);
            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnStylesChanged);
            StyleNotificationBus::Event(m_grid, &StyleNotifications::OnStyleChanged);
        }
    }

    EditorId SceneComponent::GetEditorId() const
    {
        return m_editorId;
    }

    AZ::EntityId SceneComponent::GetGrid() const
    {
        return m_grid;
    }

    AZ::EntityId SceneComponent::CreatePulse(const AnimatedPulseConfiguration& pulseConfiguration)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AnimatedPulse* animatedPulse = aznew AnimatedPulse(pulseConfiguration);
        m_graphicsSceneUi->addItem(animatedPulse);
        return animatedPulse->GetId();
    }

    AZ::EntityId SceneComponent::CreatePulseAroundSceneMember(const AZ::EntityId& memberId, int gridSteps, AnimatedPulseConfiguration pulseConfiguration)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZ::EntityId gridId = GetGrid();

        AZ::Vector2 minorPitch(0, 0);
        GridRequestBus::EventResult(minorPitch, gridId, &GridRequests::GetMinorPitch);

        QGraphicsItem* graphicsItem = nullptr;
        SceneMemberUIRequestBus::EventResult(graphicsItem, memberId, &SceneMemberUIRequests::GetRootGraphicsItem);

        if (graphicsItem)
        {
            QRectF target = graphicsItem->sceneBoundingRect();

            pulseConfiguration.m_controlPoints.reserve(4);

            for (QPointF currentPoint : { target.topLeft(), target.topRight(), target.bottomRight(), target.bottomLeft()})
            {
                QPointF directionVector = currentPoint - target.center();

                directionVector = QtVectorMath::Normalize(directionVector);

                QPointF finalPoint(currentPoint.x() + directionVector.x() * minorPitch.GetX() * gridSteps, currentPoint.y() + directionVector.y() * minorPitch.GetY() * gridSteps);

                pulseConfiguration.m_controlPoints.emplace_back(currentPoint, finalPoint);
            }

            return CreatePulse(pulseConfiguration);
        }

        return AZ::EntityId();
    }

    AZ::EntityId SceneComponent::CreateCircularPulse(const AZ::Vector2& point, float initialRadius, float finalRadius, AnimatedPulseConfiguration pulseConfiguration)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        static const int k_circleSegemnts = 9;

        pulseConfiguration.m_controlPoints.clear();
        pulseConfiguration.m_controlPoints.reserve(k_circleSegemnts);

        float step = AZ::Constants::TwoPi / static_cast<float>(k_circleSegemnts);

        // Start it at some random offset just to hide the staticness of it.
        float currentAngle = AZ::Constants::TwoPi * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX));

        for (int i = 0; i < k_circleSegemnts; ++i)
        {
            QPointF outerPoint;
            outerPoint.setX(point.GetX() + initialRadius * std::sin(currentAngle));
            outerPoint.setY(point.GetY() + initialRadius * std::cos(currentAngle));

            QPointF innerPoint;
            innerPoint.setX(point.GetX() + finalRadius * std::sin(currentAngle));
            innerPoint.setY(point.GetY() + finalRadius * std::cos(currentAngle));

            currentAngle += step;

            if (currentAngle > AZ::Constants::TwoPi)
            {
                currentAngle -= AZ::Constants::TwoPi;
            }

            pulseConfiguration.m_controlPoints.emplace_back(outerPoint, innerPoint);
        }

        return CreatePulse(pulseConfiguration);
    }

    void SceneComponent::CancelPulse(const AZ::EntityId& pulseId)
    {
        AnimatedPulse* pulse = nullptr;
        PulseRequestBus::EventResult(pulse, pulseId, &PulseRequests::GetPulse);

        if (pulse)
        {
            PulseNotificationBus::Event(pulse->GetId(), &PulseNotifications::OnPulseCanceled);
            m_graphicsSceneUi->removeItem(pulse);
            delete pulse;
        }
    }

    bool SceneComponent::AddNode(const AZ::EntityId& nodeId, const AZ::Vector2& position)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZ::Entity* nodeEntity(AzToolsFramework::GetEntity(nodeId));
        AZ_Assert(nodeEntity, "Node (ID: %s) Entity not found!", nodeId.ToString().data());
        AZ_Assert(nodeEntity->GetState() == AZ::Entity::ES_ACTIVE, "Only active node entities can be added to a scene");

        QGraphicsItem* item = nullptr;
        SceneMemberUIRequestBus::EventResult(item, nodeId, &SceneMemberUIRequests::GetRootGraphicsItem);
        AZ_Assert(item && !item->parentItem(), "Nodes must have a \"root\", unparented visual/QGraphicsItem");

        auto foundIt = AZStd::find_if(m_graphData.m_nodes.begin(), m_graphData.m_nodes.end(), [&nodeEntity](const AZ::Entity* node) { return node->GetId() == nodeEntity->GetId(); });
        if (foundIt == m_graphData.m_nodes.end())
        {
            m_graphData.m_nodes.emplace(nodeEntity);

            AddSceneMember(nodeId, true, position);
            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnNodeAdded, nodeId);

            return true;
        }

        return false;
    }

    void SceneComponent::AddNodes(const AZStd::vector<AZ::EntityId>& nodeIds)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        for (const auto& nodeId : nodeIds)
        {
            AZ::Vector2 position;
            GeometryRequestBus::EventResult(position, nodeId, &GeometryRequests::GetPosition);
            AddNode(nodeId, position);
        }
    }

    bool SceneComponent::RemoveNode(const AZ::EntityId& nodeId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        auto foundIt = AZStd::find_if(m_graphData.m_nodes.begin(), m_graphData.m_nodes.end(), [&nodeId](const AZ::Entity* node) { return node->GetId() == nodeId; });
        if (foundIt != m_graphData.m_nodes.end())
        {
            GeometryNotificationBus::MultiHandler::BusDisconnect(nodeId);
            m_graphData.m_nodes.erase(foundIt);

            QGraphicsItem* item = nullptr;
            SceneMemberUIRequestBus::EventResult(item, nodeId, &SceneMemberUIRequests::GetRootGraphicsItem);

            if (item && item->scene() == m_graphicsSceneUi.get())
            {
                m_graphicsSceneUi->removeItem(item);
            }

            UnregisterSelectionItem(nodeId);
            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnNodeRemoved, nodeId);
            SceneMemberRequestBus::Event(nodeId, &SceneMemberRequests::ClearScene, GetEntityId());

            return true;
        }

        return false;
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetNodes() const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> result;

        for (const auto& nodeRef : m_graphData.m_nodes)
        {
            result.push_back(nodeRef->GetId());
        }

        return result;
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetSelectedNodes() const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> result;
        if (m_graphicsSceneUi)
        {
            const QList<QGraphicsItem*> selected(m_graphicsSceneUi->selectedItems());
            result.reserve(selected.size());

            for (QGraphicsItem* item : selected)
            {
                auto entry = m_itemLookup.find(item);
                if (entry != m_itemLookup.end())
                {
                    AZ::Entity* entity = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entry->second);

                    if (m_graphData.m_nodes.count(entity) > 0)
                    {
                        result.push_back(entity->GetId());
                    }
                }
            }
        }

        return result;
    }

    bool SceneComponent::TrySpliceNodeOntoConnection(const AZ::EntityId& node, const AZ::EntityId& connectionId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        bool allowNode = false;
        if (node.IsValid() && connectionId.IsValid())
        {
            GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestPushPreventUndoStateUpdate);

            Endpoint connectionSourceEndpoint;
            ConnectionRequestBus::EventResult(connectionSourceEndpoint, connectionId, &ConnectionRequests::GetSourceEndpoint);

            Endpoint connectionTargetEndpoint;
            ConnectionRequestBus::EventResult(connectionTargetEndpoint, connectionId, &ConnectionRequests::GetTargetEndpoint);

            if (connectionSourceEndpoint.IsValid()
                && connectionTargetEndpoint.IsValid())
            {
                Delete({ connectionId });

                allowNode = TryCreateConnectionsBetween({ connectionSourceEndpoint, connectionTargetEndpoint }, node);                

                if (!allowNode)
                {
                    CreateConnectionBetween(connectionSourceEndpoint, connectionTargetEndpoint);
                }
            }

            GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestPopPreventUndoStateUpdate);

            if (allowNode)
            {
                GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestUndoPoint);
            }
        }

        return allowNode;
    }

    bool SceneComponent::TrySpliceNodeTreeOntoConnection(const AZStd::unordered_set< NodeId >& entryNodes, const AZStd::unordered_set< NodeId >& exitNodes, const ConnectionId& connectionId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        bool handledSplice = false;

        GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestPushPreventUndoStateUpdate);

        Endpoint sourceEndpoint;
        ConnectionRequestBus::EventResult(sourceEndpoint, connectionId, &ConnectionRequests::GetSourceEndpoint);

        Endpoint targetEndpoint;
        ConnectionRequestBus::EventResult(targetEndpoint, connectionId, &ConnectionRequests::GetTargetEndpoint);

        Delete({ connectionId });

        bool createdEntry = false;
        AZStd::unordered_set< AZ::EntityId > createdConnections;

        for (const AZ::EntityId& entryNode : entryNodes)
        {
            AZStd::vector< AZ::EntityId > slotIds;
            NodeRequestBus::EventResult(slotIds, entryNode, &NodeRequests::GetSlotIds);

            for (const AZ::EntityId& currentSlotId : slotIds)
            {
                SlotGroup slotGroup = SlotGroups::Invalid;
                SlotRequestBus::EventResult(slotGroup, currentSlotId, &SlotRequests::GetSlotGroup);

                bool isVisible = false;
                SlotLayoutRequestBus::EventResult(isVisible, entryNode, &SlotLayoutRequests::IsSlotGroupVisible, slotGroup);

                if (isVisible)
                {
                    Endpoint testTargetEndpoint(entryNode, currentSlotId);

                    AZ::EntityId createdConnectionId = CreateConnectionBetween(sourceEndpoint, testTargetEndpoint);
                    if (createdConnectionId.IsValid())
                    {
                        createdConnections.insert(createdConnectionId);
                        createdEntry = true;
                    }
                }
            }
        }

        bool createdExit = false;

        for (const AZ::EntityId& exitNode : exitNodes)
        {
            AZStd::vector< AZ::EntityId > slotIds;
            NodeRequestBus::EventResult(slotIds, exitNode, &NodeRequests::GetSlotIds);

            for (const AZ::EntityId& currentSlotId : slotIds)
            {
                Endpoint testSourceEndpoint(exitNode, currentSlotId);

                AZ::EntityId createdConnectionId = CreateConnectionBetween(testSourceEndpoint, targetEndpoint);
                if (createdConnectionId.IsValid())
                {
                    createdConnections.insert(createdConnectionId);
                    createdExit = true;
                }
            }
        }

        if (!createdEntry || !createdExit)
        {
            handledSplice = false;
            CreateConnectionBetween(sourceEndpoint, targetEndpoint);

            Delete(createdConnections);
        }

        GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestPopPreventUndoStateUpdate);

        if (handledSplice)
        {
            GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestUndoPoint);
        }

        return handledSplice;
    }

    void SceneComponent::DeleteNodeAndStitchConnections(const AZ::EntityId& node)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        if (node.IsValid())
        {
            GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestPushPreventUndoStateUpdate);
            AZStd::vector< AZ::EntityId > slotIds;
            NodeRequestBus::EventResult(slotIds, node, &NodeRequests::GetSlotIds);

            AZStd::vector< Endpoint > sourceEndpoints;
            AZStd::vector< Endpoint > targetEndpoints;

            for (const AZ::EntityId& slotId : slotIds)
            {
                AZStd::vector< AZ::EntityId > connectionIds;
                SlotRequestBus::EventResult(connectionIds, slotId, &SlotRequests::GetConnections);

                for (const AZ::EntityId& connectionId : connectionIds)
                {
                    Endpoint sourceEndpoint;
                    ConnectionRequestBus::EventResult(sourceEndpoint, connectionId, &ConnectionRequests::GetSourceEndpoint);

                    if (sourceEndpoint.m_nodeId != node)
                    {
                        sourceEndpoints.push_back(sourceEndpoint);
                    }

                    Endpoint targetEndpoint;
                    ConnectionRequestBus::EventResult(targetEndpoint, connectionId, &ConnectionRequests::GetTargetEndpoint);

                    if (targetEndpoint.m_nodeId != node)
                    {
                        targetEndpoints.push_back(targetEndpoint);
                    }
                }
            }

            Delete({ node });

            for (const Endpoint& sourceEndpoint : sourceEndpoints)
            {
                for (const Endpoint& targetEndpoint : targetEndpoints)
                {                    
                    CreateConnectionBetween(sourceEndpoint, targetEndpoint);
                }
            }

            GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestPopPreventUndoStateUpdate);
            GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestUndoPoint);
        }
    }

    bool SceneComponent::TryCreateConnectionsBetween(const AZStd::vector< Endpoint >& endpoints, const AZ::EntityId& targetNode)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        bool allowNode = false;
        AZStd::vector< AZ::EntityId > slotIds;
        NodeRequestBus::EventResult(slotIds, targetNode, &NodeRequests::GetSlotIds);

        for (const AZ::EntityId& slotId : slotIds)
        {
            Endpoint currentEndpoint(targetNode, slotId);

            SlotGroup slotGroup = SlotGroups::Invalid;
            SlotRequestBus::EventResult(slotGroup, slotId, &SlotRequests::GetSlotGroup);

            bool isVisible = false;
            SlotLayoutRequestBus::EventResult(isVisible, targetNode, &SlotLayoutRequests::IsSlotGroupVisible, slotGroup);

            if (isVisible)
            {
                for (const Endpoint& testEndpoint : endpoints)
                {
                    // Could be a source or a target endpoint, or possibly an omni-direction one.
                    // So try both combinations to see which one fits.
                    // Will currently fail with uni-direction connections
                    if (CreateConnectionBetween(testEndpoint, currentEndpoint).IsValid() || CreateConnectionBetween(currentEndpoint, testEndpoint).IsValid())
                    {
                        allowNode = true;
                    }
                }
            }
        }

        return allowNode;
    }

    AZ::EntityId SceneComponent::CreateConnectionBetween(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        if (!sourceEndpoint.IsValid() || !targetEndpoint.IsValid())
        {
            return AZ::EntityId();
        }

        AZ::EntityId connectionId;

        bool isValidConnection = false;
        GraphModelRequestBus::EventResult(isValidConnection, GetEntityId(), &GraphModelRequests::IsValidConnection, sourceEndpoint, targetEndpoint);

        if (isValidConnection)
        {
            SlotRequestBus::EventResult(connectionId, sourceEndpoint.GetSlotId(), &SlotRequests::CreateConnectionWithEndpoint, targetEndpoint);

            GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::CreateConnection, connectionId, sourceEndpoint, targetEndpoint);
        }

        return connectionId;
    }

    bool SceneComponent::AddConnection(const AZ::EntityId& connectionId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZ_Assert(connectionId.IsValid(), "Connection ID %s is not valid!", connectionId.ToString().data());

        AZ::Entity* connectionEntity(AzToolsFramework::GetEntity(connectionId));
        auto connection = connectionEntity ? AZ::EntityUtils::FindFirstDerivedComponent<ConnectionComponent>(connectionEntity) : nullptr;
        AZ_Warning("Graph Canvas", connection->GetEntity()->GetState() == AZ::Entity::ES_ACTIVE, "Only active connection entities can be added to a scene");
        AZ_Warning("Graph Canvas", connection, "Couldn't find the connection's component (ID: %s)!", connectionId.ToString().data());

        if (connection)
        {
            auto foundIt = AZStd::find_if(m_graphData.m_connections.begin(), m_graphData.m_connections.end(), [&connection](const AZ::Entity* connectionEntity) { return connectionEntity->GetId() == connection->GetEntityId(); });
            if (foundIt == m_graphData.m_connections.end())
            {
                AddSceneMember(connectionId);

                m_graphData.m_connections.emplace(connection->GetEntity());
                Endpoint sourceEndpoint;
                ConnectionRequestBus::EventResult(sourceEndpoint, connectionId, &ConnectionRequests::GetSourceEndpoint);
                Endpoint targetEndpoint;
                ConnectionRequestBus::EventResult(targetEndpoint, connectionId, &ConnectionRequests::GetTargetEndpoint);
                m_graphData.m_endpointMap.emplace(sourceEndpoint, targetEndpoint);
                m_graphData.m_endpointMap.emplace(targetEndpoint, sourceEndpoint);

                SlotRequestBus::Event(sourceEndpoint.GetSlotId(), &SlotRequests::AddConnectionId, connectionId, targetEndpoint);
                SlotRequestBus::Event(targetEndpoint.GetSlotId(), &SlotRequests::AddConnectionId, connectionId, sourceEndpoint);
                SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnConnectionAdded, connectionId);

                return true;
            }
        }

        return false;
    }

    void SceneComponent::AddConnections(const AZStd::vector<AZ::EntityId>& connectionIds)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        for (const auto& connectionId : connectionIds)
        {
            AddConnection(connectionId);
        }
    }

    bool SceneComponent::RemoveConnection(const AZ::EntityId& connectionId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZ_Assert(connectionId.IsValid(), "Connection ID %s is not valid!", connectionId.ToString().data());

        auto foundIt = AZStd::find_if(m_graphData.m_connections.begin(), m_graphData.m_connections.end(), [&connectionId](const AZ::Entity* connection) { return connection->GetId() == connectionId; });
        if (foundIt != m_graphData.m_connections.end())
        {
            GeometryNotificationBus::MultiHandler::BusDisconnect(connectionId);

            Endpoint sourceEndpoint;
            ConnectionRequestBus::EventResult(sourceEndpoint, connectionId, &ConnectionRequests::GetSourceEndpoint);
            Endpoint targetEndpoint;
            ConnectionRequestBus::EventResult(targetEndpoint, connectionId, &ConnectionRequests::GetTargetEndpoint);
            m_graphData.m_endpointMap.erase(sourceEndpoint);
            m_graphData.m_endpointMap.erase(targetEndpoint);
            m_graphData.m_connections.erase(foundIt);

            QGraphicsItem* item{};
            SceneMemberUIRequestBus::EventResult(item, connectionId, &SceneMemberUIRequests::GetRootGraphicsItem);
            AZ_Assert(item, "Connections must have a visual/QGraphicsItem");
            if (m_graphicsSceneUi)
            {
                m_graphicsSceneUi->removeItem(item);
            }

            UnregisterSelectionItem(connectionId);

            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnConnectionRemoved, connectionId);
            SlotRequestBus::Event(targetEndpoint.GetSlotId(), &SlotRequests::RemoveConnectionId, connectionId, sourceEndpoint);
            SlotRequestBus::Event(sourceEndpoint.GetSlotId(), &SlotRequests::RemoveConnectionId, connectionId, targetEndpoint);

            return true;
        }

        return false;
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetConnections() const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> result;

        for (const auto& connection : m_graphData.m_connections)
        {
            result.push_back(connection->GetId());
        }

        return result;
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetSelectedConnections() const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> result;
        if (m_graphicsSceneUi)
        {
            const QList<QGraphicsItem*> selected(m_graphicsSceneUi->selectedItems());
            result.reserve(selected.size());

            for (QGraphicsItem* item : selected)
            {
                auto entry = m_itemLookup.find(item);
                if (entry != m_itemLookup.end())
                {
                    AZ::Entity* entity{};
                    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entry->second);
                    if (entity && AZ::EntityUtils::FindFirstDerivedComponent<ConnectionComponent>(entity))
                    {
                        result.push_back(entity->GetId());
                    }
                }
            }
        }

        return result;
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetConnectionsForEndpoint(const Endpoint& firstEndpoint) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> result;
        for (const auto& connection : m_graphData.m_connections)
        {
            Endpoint sourceEndpoint;
            ConnectionRequestBus::EventResult(sourceEndpoint, connection->GetId(), &ConnectionRequests::GetSourceEndpoint);
            Endpoint targetEndpoint;
            ConnectionRequestBus::EventResult(targetEndpoint, connection->GetId(), &ConnectionRequests::GetTargetEndpoint);

            if (sourceEndpoint == firstEndpoint || targetEndpoint == firstEndpoint)
            {
                result.push_back(connection->GetId());
            }
        }

        return result;
    }

    bool SceneComponent::IsEndpointConnected(const Endpoint& endpoint) const
    {
        return m_graphData.m_endpointMap.count(endpoint) > 0;
    }

    AZStd::vector<Endpoint> SceneComponent::GetConnectedEndpoints(const Endpoint& firstEndpoint) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<Endpoint> connectedEndpoints;
        auto otherEndpointsRange = m_graphData.m_endpointMap.equal_range(firstEndpoint);
        for (auto otherIt = otherEndpointsRange.first; otherIt != otherEndpointsRange.second; ++otherIt)
        {
            connectedEndpoints.push_back(otherIt->second);
        }
        return connectedEndpoints;
    }

    bool SceneComponent::Connect(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZ::Entity* foundEntity = nullptr;
        if (FindConnection(foundEntity, sourceEndpoint, targetEndpoint))
        {
            AZ_Warning("Graph Canvas", false, "Attempting to create duplicate connection between source endpoint (%s, %s) and target endpoint(%s, %s)",
                sourceEndpoint.GetNodeId().ToString().data(), sourceEndpoint.GetSlotId().ToString().data(),
                targetEndpoint.GetNodeId().ToString().data(), targetEndpoint.GetSlotId().ToString().data());
            return false;
        }

        auto entry = AZStd::find_if(m_graphData.m_nodes.begin(), m_graphData.m_nodes.end(), [&sourceEndpoint](const AZ::Entity* node) { return node->GetId() == sourceEndpoint.GetNodeId(); });
        if (entry == m_graphData.m_nodes.end())
        {
            AZ_Error("Scene", false, "The source node with id %s is not in this scene, a connection cannot be made", sourceEndpoint.GetNodeId().ToString().data());
            return false;
        }

        entry = AZStd::find_if(m_graphData.m_nodes.begin(), m_graphData.m_nodes.end(), [&targetEndpoint](const AZ::Entity* node) { return node->GetId() == targetEndpoint.GetNodeId(); });
        if (entry == m_graphData.m_nodes.end())
        {
            AZ_Error("Scene", false, "The target node with id %s is not in this scene, a connection cannot be made", targetEndpoint.GetNodeId().ToString().data());
            return false;
        }

        AZ::EntityId connectionEntity;
        SlotRequestBus::EventResult(connectionEntity, sourceEndpoint.GetSlotId(), &SlotRequests::CreateConnectionWithEndpoint, targetEndpoint);

        return true;
    }

    bool SceneComponent::Disconnect(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint)
    {
        AZ::Entity* connectionEntity{};
        if (FindConnection(connectionEntity, sourceEndpoint, targetEndpoint) && RemoveConnection(connectionEntity->GetId()))
        {
            delete connectionEntity;
            return true;
        }
        return false;
    }

    bool SceneComponent::DisconnectById(const AZ::EntityId& connectionId)
    {
        if (RemoveConnection(connectionId))
        {
            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, connectionId);
            return true;
        }

        return false;
    }

    bool SceneComponent::FindConnection(AZ::Entity*& connectionEntity, const Endpoint& firstEndpoint, const Endpoint& otherEndpoint) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZ::Entity* foundEntity = nullptr;
        for (auto connectionIt = m_graphData.m_connections.begin(); connectionIt != m_graphData.m_connections.end(); ++connectionIt)
        {
            Endpoint sourceEndpoint;
            ConnectionRequestBus::EventResult(sourceEndpoint, (*connectionIt)->GetId(), &ConnectionRequests::GetSourceEndpoint);
            Endpoint targetEndpoint;
            ConnectionRequestBus::EventResult(targetEndpoint, (*connectionIt)->GetId(), &ConnectionRequests::GetTargetEndpoint);

            if ((sourceEndpoint == firstEndpoint && targetEndpoint == otherEndpoint)
                || (sourceEndpoint == otherEndpoint && targetEndpoint == firstEndpoint))
            {
                foundEntity = *connectionIt;
                break;
            }
        }

        if (foundEntity)
        {
            connectionEntity = foundEntity;
            return true;
        }

        return false;
    }

    bool SceneComponent::AddBookmarkAnchor(const AZ::EntityId& bookmarkAnchorId, const AZ::Vector2& position)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZ::Entity* anchorEntity(AzToolsFramework::GetEntity(bookmarkAnchorId));
        AZ_Assert(anchorEntity, "BookmarkAnchor (ID: %s) Entity not found!", bookmarkAnchorId.ToString().data());
        AZ_Assert(anchorEntity->GetState() == AZ::Entity::ES_ACTIVE, "Only active node entities can be added to a scene");

        QGraphicsItem* item = nullptr;
        SceneMemberUIRequestBus::EventResult(item, bookmarkAnchorId, &SceneMemberUIRequests::GetRootGraphicsItem);
        AZ_Assert(item && !item->parentItem(), "BookmarkAnchors must have a \"root\", unparented visual/QGraphicsItem");

        auto foundIt = AZStd::find_if(m_graphData.m_bookmarkAnchors.begin(), m_graphData.m_bookmarkAnchors.end(), [&anchorEntity](const AZ::Entity* entity) { return entity->GetId() == anchorEntity->GetId(); });
        if (foundIt == m_graphData.m_bookmarkAnchors.end())
        {
            m_graphData.m_bookmarkAnchors.emplace(anchorEntity);
            AddSceneMember(bookmarkAnchorId, true, position);
            return true;
        }

        return false;
    }

    bool SceneComponent::RemoveBookmarkAnchor(const AZ::EntityId& bookmarkAnchorId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        auto foundIt = AZStd::find_if(m_graphData.m_bookmarkAnchors.begin(), m_graphData.m_bookmarkAnchors.end(), [&bookmarkAnchorId](const AZ::Entity* anchorEntity) { return anchorEntity->GetId() == bookmarkAnchorId; });
        if (foundIt != m_graphData.m_bookmarkAnchors.end())
        {
            GeometryNotificationBus::MultiHandler::BusDisconnect(bookmarkAnchorId);
            m_graphData.m_bookmarkAnchors.erase(foundIt);

            QGraphicsItem* item = nullptr;
            SceneMemberUIRequestBus::EventResult(item, bookmarkAnchorId, &SceneMemberUIRequests::GetRootGraphicsItem);

            if (item && item->scene() == m_graphicsSceneUi.get())
            {
                m_graphicsSceneUi->removeItem(item);
            }

            UnregisterSelectionItem(bookmarkAnchorId);
            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnSceneMemberRemoved, bookmarkAnchorId);
            SceneMemberRequestBus::Event(bookmarkAnchorId, &SceneMemberRequests::ClearScene, GetEntityId());

            return true;
        }

        return false;
    }

    bool SceneComponent::Add(const AZ::EntityId& entityId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZ::Entity* actual = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(actual, &AZ::ComponentApplicationRequests::FindEntity, entityId);
        if (!actual)
        {
            return false;
        }

        if (AZ::EntityUtils::FindFirstDerivedComponent<NodeComponent>(actual))
        {
            AZ::Vector2 position;
            GeometryRequestBus::EventResult(position, entityId, &GeometryRequests::GetPosition);
            return AddNode(entityId, position);
        }
        else if (AZ::EntityUtils::FindFirstDerivedComponent<ConnectionComponent>(actual))
        {
            return AddConnection(entityId);
        }
        else if (AZ::EntityUtils::FindFirstDerivedComponent<BookmarkAnchorComponent>(actual))
        {
            AZ::Vector2 position;
            GeometryRequestBus::EventResult(position, entityId, &GeometryRequests::GetPosition);
            return AddBookmarkAnchor(entityId, position);
        }
        else
        {
            AZ_Error("Scene", false, "The entity does not appear to be a valid scene membership candidate (ID: %s)", entityId.ToString().data());
        }

        return false;
    }

    bool SceneComponent::Remove(const AZ::EntityId& entityId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZ::Entity* actual = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(actual, &AZ::ComponentApplicationRequests::FindEntity, entityId);
        if (!actual)
        {
            return false;
        }

        if (AZ::EntityUtils::FindFirstDerivedComponent<NodeComponent>(actual))
        {
            return RemoveNode(entityId);
        }
        else if (AZ::EntityUtils::FindFirstDerivedComponent<ConnectionComponent>(actual))
        {
            return RemoveConnection(entityId);
        }
        else if (AZ::EntityUtils::FindFirstDerivedComponent<BookmarkAnchorComponent>(actual))
        {
            return RemoveBookmarkAnchor(entityId);
        }
        else
        {
            AZ_Error("Scene", false, "The entity does not appear to be a valid scene membership candidate (ID: %s)", entityId.ToString().data());
        }

        return false;
    }

    void SceneComponent::ClearSelection()
    {
        if (m_graphicsSceneUi)
        {
            m_graphicsSceneUi->clearSelection();
        }
    }

    void SceneComponent::SetSelectedArea(const AZ::Vector2& topLeft, const AZ::Vector2& topRight)
    {
        if (m_graphicsSceneUi)
        {
            QPainterPath path;
            path.addRect(QRectF{ QPointF {topLeft.GetX(), topLeft.GetY()}, QPointF { topRight.GetX(), topRight.GetY() } });
            m_graphicsSceneUi->setSelectionArea(path);
        }
    }

    bool SceneComponent::HasSelectedItems() const
    {
        return m_graphicsSceneUi ? !m_graphicsSceneUi->selectedItems().isEmpty() : false;
    }

    bool SceneComponent::HasCopiableSelection() const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        bool hasCopiableSelection = false;
        if (m_graphicsSceneUi)
        {
            const QList<QGraphicsItem*> selected(m_graphicsSceneUi->selectedItems());

            for (QGraphicsItem* item : selected)
            {
                auto entry = m_itemLookup.find(item);
                if (entry != m_itemLookup.end())
                {
                    AZ::Entity* entity = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entry->second);

                    if (m_graphData.m_nodes.count(entity) > 0)
                    {
                        hasCopiableSelection = true;
                        break;
                    }
                    else if (m_graphData.m_bookmarkAnchors.count(entity) > 0)
                    {
                        hasCopiableSelection = true;
                        break;
                    }
                }
            }
        }

        return hasCopiableSelection;
    }

    bool SceneComponent::HasEntitiesAt(const AZ::Vector2& scenePoint) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        bool retVal = false;

        if (m_graphicsSceneUi)
        {
            QList<QGraphicsItem*> itemsThere = m_graphicsSceneUi->items(QPointF(scenePoint.GetX(), scenePoint.GetY()));
            for (QGraphicsItem* item : itemsThere)
            {
                auto entry = m_itemLookup.find(item);
                if (entry != m_itemLookup.end() && entry->second != m_grid)
                {
                    retVal = true;
                    break;
                }
            }
        }

        return retVal;
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetSelectedItems() const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> result;
        if (m_graphicsSceneUi)
        {
            const QList<QGraphicsItem*> selected(m_graphicsSceneUi->selectedItems());
            result.reserve(selected.size());

            for (QGraphicsItem* item : selected)
            {
                auto entry = m_itemLookup.find(item);
                if (entry != m_itemLookup.end())
                {
                    result.push_back(entry->second);
                }
            }
        }
        return result;
    }

    QGraphicsScene* SceneComponent::AsQGraphicsScene()
    {
        return m_graphicsSceneUi.get();
    }

    void SceneComponent::CopySelection() const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> entities = GetSelectedItems();

        Copy(entities);
    }

    void SceneComponent::Copy(const AZStd::vector<AZ::EntityId>& selectedItems) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnCopyBegin);

        GraphSerialization serializationTarget(m_copyMimeType);
        SerializeEntities({ selectedItems.begin(), selectedItems.end() }, serializationTarget);
        SerializeToClipboard(serializationTarget);

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnCopyEnd);
    }

    void SceneComponent::CutSelection()
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> entities = GetSelectedItems();
        Cut(entities);
    }

    void SceneComponent::Cut(const AZStd::vector<AZ::EntityId>& selectedItems)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        Copy(selectedItems);

        AZStd::unordered_set<AZ::EntityId> deletedItems(selectedItems.begin(), selectedItems.end());
        Delete(deletedItems);
    }

    void SceneComponent::Paste()
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        m_allowReset = false;
        QPointF pasteCenter = GetViewCenterScenePoint() + m_pasteOffset;
        PasteAt(pasteCenter);

        AZ::Vector2 minorPitch;
        GridRequestBus::EventResult(minorPitch, m_grid, &GridRequests::GetMinorPitch);

        // Don't want to shift it diagonally, because we also shift things diagonally when we drag/drop in stuff
        // So we'll just move it straight down.
        m_pasteOffset += QPointF(0, minorPitch.GetY() * 2);
        m_allowReset = true;
    }

    void SceneComponent::PasteAt(const QPointF& scenePos)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnPasteBegin);

        if (m_graphicsSceneUi)
        {
            m_graphicsSceneUi->blockSignals(true);
            m_graphicsSceneUi->clearSelection();
        }

        AZ::Vector2 pastePos{ static_cast<float>(scenePos.x()), static_cast<float>(scenePos.y()) };
        QClipboard* clipboard = QApplication::clipboard();

        // Trying to paste unknown data into our scene.
        if (!clipboard->mimeData()->hasFormat(m_copyMimeType.c_str()))
        {
            return;
        }

        QByteArray byteArray = clipboard->mimeData()->data(m_copyMimeType.c_str());
        GraphSerialization serializationSource(byteArray);
        DeserializeEntities(scenePos, serializationSource);

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::PostCreationEvent);
        if (m_graphicsSceneUi)
        {
            m_graphicsSceneUi->blockSignals(false);
        }
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnPasteEnd);

        OnSelectionChanged();
    }

    void SceneComponent::SerializeEntities(const AZStd::unordered_set<AZ::EntityId>& itemIds, GraphSerialization& serializationTarget) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        GraphData& serializedEntities = serializationTarget.GetGraphData();

        SceneMemberBuckets sceneMembers;
        SieveSceneMembers(itemIds, sceneMembers);

        // If we have nothing to copy. Don't copy.
        if (sceneMembers.m_nodes.empty()
            && sceneMembers.m_bookmarkAnchors.empty())
        {
            return;
        }

        // Calculate the position of selected items relative to the position from either the context menu mouse point or the scene center
        AZ::Vector2 aggregatePos = AZ::Vector2::CreateZero();
        for (const auto& nodeRef : sceneMembers.m_nodes)
        {
            NodeNotificationBus::Event(nodeRef, &NodeNotifications::OnNodeAboutToSerialize, serializationTarget);
            serializedEntities.m_nodes.emplace(AzToolsFramework::GetEntity(nodeRef));
        }

        AZStd::unordered_set< AZ::EntityId > serializedNodes;

        // Can't do this with the above listing. Because when nodes get serialized, they may add other nodes to the list.
        // So once we are fully added in, we can figure out our positions.
        for (AZ::Entity* entity : serializedEntities.m_nodes)
        {
            serializedNodes.insert(entity->GetId());

            QGraphicsItem* graphicsItem = nullptr;
            SceneMemberUIRequestBus::EventResult(graphicsItem, entity->GetId(), &SceneMemberUIRequests::GetRootGraphicsItem);

            AZ::Vector2 itemPos = AZ::Vector2::CreateZero();

            if (graphicsItem)
            {
                QPointF scenePosition = graphicsItem->scenePos();
                itemPos.SetX(scenePosition.x());
                itemPos.SetY(scenePosition.y());
            }
            
            aggregatePos += itemPos;
        }

        for (const auto& anchorRef : sceneMembers.m_bookmarkAnchors)
        {
            serializedEntities.m_bookmarkAnchors.emplace(AzToolsFramework::GetEntity(anchorRef));
        }

        for (AZ::Entity* entity : serializedEntities.m_bookmarkAnchors)
        {
            QGraphicsItem* graphicsItem = nullptr;
            SceneMemberUIRequestBus::EventResult(graphicsItem, entity->GetId(), &SceneMemberUIRequests::GetRootGraphicsItem);

            AZ::Vector2 itemPos = AZ::Vector2::CreateZero();

            if (graphicsItem)
            {
                QPointF scenePosition = graphicsItem->scenePos();
                itemPos.SetX(scenePosition.x());
                itemPos.SetY(scenePosition.y());
            }

            aggregatePos += itemPos;
        }

        // This copies only connections among nodes in the copied node set
        AZStd::unordered_set< AZ::EntityId > connections = FindConnections(serializedNodes, true);

        for (const auto& connection : connections)
        {
            serializedEntities.m_connections.emplace(AzToolsFramework::GetEntity(connection));
        }

        AZ::Vector2 averagePos = aggregatePos / (serializedEntities.m_nodes.size() + serializedEntities.m_bookmarkAnchors.size());
        serializationTarget.SetAveragePosition(averagePos);

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnEntitiesSerialized, serializationTarget);
    }

    void SceneComponent::DeserializeEntities(const QPointF& scenePoint, const GraphSerialization& serializationSource)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZ::Vector2 deserializePoint = AZ::Vector2(scenePoint.x(), scenePoint.y());
        const AZ::Vector2& averagePos = serializationSource.GetAveragePosition();

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnEntitiesDeserialized, serializationSource);

        const GraphData& pasteSceneData = serializationSource.GetGraphData();

        for (auto& nodeRef : pasteSceneData.m_nodes)
        {
            AZStd::unique_ptr<AZ::Entity> entity(nodeRef);
            entity->Init();
            entity->Activate();

            AZ::Vector2 prevNodePos;
            GeometryRequestBus::EventResult(prevNodePos, entity->GetId(), &GeometryRequests::GetPosition);
            GeometryRequestBus::Event(entity->GetId(), &GeometryRequests::SetPosition, (prevNodePos - averagePos) + deserializePoint);
            SceneMemberNotificationBus::Event(entity->GetId(), &SceneMemberNotifications::OnSceneMemberDeserializedForGraph, GetEntityId());
            NodeNotificationBus::Event(entity->GetId(), &NodeNotifications::OnNodeDeserialized, GetEntityId(), serializationSource);

            SceneMemberUIRequestBus::Event(entity->GetId(), &SceneMemberUIRequests::SetSelected, true);

            if (Add(entity->GetId()))
            {
                entity.release();
            }
        }

        for (auto& bookmarkRef : pasteSceneData.m_bookmarkAnchors)
        {
            AZStd::unique_ptr<AZ::Entity> entity(bookmarkRef);
            entity->Init();
            entity->Activate();

            AZ::Vector2 prevPos;
            GeometryRequestBus::EventResult(prevPos, entity->GetId(), &GeometryRequests::GetPosition);
            GeometryRequestBus::Event(entity->GetId(), &GeometryRequests::SetPosition, (prevPos - averagePos) + deserializePoint);
            SceneMemberNotificationBus::Event(entity->GetId(), &SceneMemberNotifications::OnSceneMemberDeserializedForGraph, GetEntityId());

            SceneMemberUIRequestBus::Event(entity->GetId(), &SceneMemberUIRequests::SetSelected, true);

            if (Add(entity->GetId()))
            {
                entity.release();
            }
        }

        for (auto& connection : pasteSceneData.m_connections)
        {
            AZStd::unique_ptr<AZ::Entity> entity(connection);
            entity->Init();
            entity->Activate();

            if (ValidateConnectionEndpoints(connection, m_graphData.m_nodes) && Add(entity->GetId()))
            {
                entity.release();
            }
        }
    }

    void SceneComponent::DuplicateSelection()
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> entities = GetSelectedItems();
        Duplicate(entities);
    }

    void SceneComponent::DuplicateSelectionAt(const QPointF& scenePos)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> entities = GetSelectedItems();
        DuplicateAt(entities, scenePos);
    }

    void SceneComponent::Duplicate(const AZStd::vector<AZ::EntityId>& itemIds)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        QPointF duplicateCenter = GetViewCenterScenePoint() + m_pasteOffset;
        DuplicateAt(itemIds, duplicateCenter);

        AZ::Vector2 minorPitch;
        GridRequestBus::EventResult(minorPitch, m_grid, &GridRequests::GetMinorPitch);

        // Don't want to shift it diagonally, because we also shift things diagonally when we drag/drop in stuff
        // So we'll just move it straight down.
        m_pasteOffset += QPointF(0, minorPitch.GetY() * 2);
    }

    void SceneComponent::DuplicateAt(const AZStd::vector<AZ::EntityId>& itemIds, const QPointF& scenePos)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnDuplicateBegin);

        if (m_graphicsSceneUi)
        {
            m_graphicsSceneUi->blockSignals(true);
            m_graphicsSceneUi->clearSelection();
        }

        GraphSerialization serializationTarget;
        SerializeEntities({ itemIds.begin(), itemIds.end() }, serializationTarget);

        AZStd::vector<char> buffer;
        SerializeToBuffer(serializationTarget, buffer);
        GraphSerialization deserializationTarget(QByteArray(buffer.cbegin(), static_cast<int>(buffer.size())));

        DeserializeEntities(scenePos, deserializationTarget);

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::PostCreationEvent);

        if (m_graphicsSceneUi)
        {
            m_graphicsSceneUi->blockSignals(false);
        }
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnDuplicateEnd);

        OnSelectionChanged();
    }

    void SceneComponent::DeleteSelection()
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        if (m_graphicsSceneUi)
        {
            const QList<QGraphicsItem*> selected(m_graphicsSceneUi->selectedItems());

            AZStd::unordered_set<AZ::EntityId> toDelete;

            for (QGraphicsItem* item : selected)
            {
                auto entry = m_itemLookup.find(item);
                if (entry != m_itemLookup.end())
                {
                    toDelete.insert(entry->second);
                }
            }

            Delete(toDelete);
        }
    }

    void SceneComponent::Delete(const AZStd::unordered_set<AZ::EntityId>& itemIds)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        if (itemIds.empty())
        {
            return;
        }

        // Need to deal with recursive deleting because of Wrapper Nodes
        ++m_deleteCount;

        SceneMemberBuckets sceneMembers;

        SieveSceneMembers(itemIds, sceneMembers);

        auto nodeConnections = FindConnections(sceneMembers.m_nodes);
        sceneMembers.m_connections.insert(nodeConnections.begin(), nodeConnections.end());

        // Block the graphics scene from sending selection update events as we remove items.
        if (m_graphicsSceneUi)
        {
            m_graphicsSceneUi->blockSignals(true);
        }

        for (const auto& connection : sceneMembers.m_connections)
        {
            if (Remove(connection))
            {
                SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnPreConnectionDeleted, connection);
                AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, connection);
            }
        }

        for (const auto& node : sceneMembers.m_nodes)
        {
            if (Remove(node))
            {
                SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnPreNodeDeleted, node);
                NodeNotificationBus::Event(node, &NodeNotifications::OnRemovedFromScene, GetEntityId());

                bool isWrappedNode = false;
                NodeRequestBus::EventResult(isWrappedNode, node, &NodeRequests::IsWrapped);
                
                QGraphicsItem* item = nullptr;
                if (!isWrappedNode)
                {
                    SceneMemberUIRequestBus::EventResult(item, node, &SceneMemberUIRequests::GetRootGraphicsItem);
                }

                AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, node);
            }
        }

        for (const auto& bookmarkAnchor : sceneMembers.m_bookmarkAnchors)
        {
            if (Remove(bookmarkAnchor))
            {
                AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, bookmarkAnchor);
            }
        }

        if (m_graphicsSceneUi)
        {
            m_graphicsSceneUi->blockSignals(false);

            // Once complete, signal selection is changed
            emit m_graphicsSceneUi->selectionChanged();
        }

        --m_deleteCount;

        if (m_deleteCount == 0)
        {
            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::PostDeletionEvent);
        }
    }

    void SceneComponent::DeleteGraphData(const GraphData& graphData)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::unordered_set<AZ::EntityId> itemIds;
        graphData.CollectItemIds(itemIds);        

        Delete(itemIds);
    }

    void SceneComponent::SuppressNextContextMenu()
    {
        if (m_graphicsSceneUi)
        {
            m_graphicsSceneUi->SuppressNextContextMenu();
        }
    }

    const AZStd::string& SceneComponent::GetCopyMimeType() const
    {
        return m_copyMimeType;
    }

    void SceneComponent::SetMimeType(const char* mimeType)
    {
        m_mimeDelegate.SetMimeType(mimeType);

        m_copyMimeType = AZStd::string::format("%s::copy", mimeType);
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetEntitiesAt(const AZ::Vector2& position) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> result;

        if (m_graphicsSceneUi)
        {
            QList<QGraphicsItem*> itemsThere = m_graphicsSceneUi->items(QPointF(position.GetX(), position.GetY()));
            for (QGraphicsItem* item : itemsThere)
            {
                auto entry = m_itemLookup.find(item);
                if (entry != m_itemLookup.end() && entry->second != m_grid)
                {
                    result.emplace_back(AZStd::move(entry->second));
                }
            }
        }

        return result;
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetEntitiesInRect(const QRectF& rect, Qt::ItemSelectionMode mode) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<AZ::EntityId> result;

        if (m_graphicsSceneUi)
        {
            QList<QGraphicsItem*> itemsThere = m_graphicsSceneUi->items(rect, mode);
            for (QGraphicsItem* item : itemsThere)
            {
                auto entry = m_itemLookup.find(item);
                if (entry != m_itemLookup.end() && entry->second != m_grid)
                {
                    result.emplace_back(AZStd::move(entry->second));
                }
            }
        }

        return result;
    }

    AZStd::vector<Endpoint> SceneComponent::GetEndpointsInRect(const QRectF& rect) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::vector<Endpoint> result;

        AZStd::vector<AZ::EntityId> entitiesThere = GetEntitiesInRect(rect, Qt::ItemSelectionMode::IntersectsItemShape);
        for (AZ::EntityId nodeId : entitiesThere)
        {
            if (NodeRequestBus::FindFirstHandler(nodeId) != nullptr)
            {
                AZStd::vector<AZ::EntityId> slotIds;
                NodeRequestBus::EventResult(slotIds, nodeId, &NodeRequestBus::Events::GetSlotIds);
                for (AZ::EntityId slotId : slotIds)
                {
                    QPointF point;
                    SlotUIRequestBus::EventResult(point, slotId, &SlotUIRequestBus::Events::GetConnectionPoint);
                    if (rect.contains(point))
                    {
                        result.emplace_back(Endpoint(nodeId, slotId));
                    }
                }
            }
        }

        AZStd::sort(result.begin(), result.end(), [&rect](Endpoint a, Endpoint b) {
            QPointF pointA;
            SlotUIRequestBus::EventResult(pointA, a.GetSlotId(), &SlotUIRequestBus::Events::GetConnectionPoint);
            QPointF pointB;
            SlotUIRequestBus::EventResult(pointB, b.GetSlotId(), &SlotUIRequestBus::Events::GetConnectionPoint);

            return (rect.center() - pointA).manhattanLength() < (rect.center() - pointB).manhattanLength();
        });

        return result;
    }

    void SceneComponent::RegisterView(const AZ::EntityId& viewId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        if (m_activateScene)
        {
            m_activateScene = false;

            GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestPushPreventUndoStateUpdate);
            
            ActivateItems(m_graphData.m_nodes);
            ActivateItems(m_graphData.m_connections);
            ActivateItems(m_graphData.m_bookmarkAnchors);
            NotifyConnectedSlots();

            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnStylesChanged); // Forces activated elements to refresh their visual elements.

            GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestPopPreventUndoStateUpdate);
        }

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnViewRegistered);

        if (!m_viewId.IsValid() || m_viewId == viewId)
        {
            ViewRequestBus::EventResult(m_editorId, viewId, &ViewRequests::GetEditorId);

            m_viewId = viewId;
            ViewNotificationBus::Handler::BusConnect(m_viewId);
            ViewRequestBus::Event(m_viewId, &ViewRequests::SetViewParams, m_viewParams);
        }
        else
        {
            AZ_Error("Scene", false, "Trying to register scene to two different views.");
        }
    }

    void SceneComponent::RemoveView(const AZ::EntityId& viewId)
    {
        if (m_viewId == viewId)
        {
            m_editorId = EditorId();
            m_viewId.SetInvalid();
            ViewNotificationBus::Handler::BusDisconnect();
        }
        else
        {
            AZ_Error("Scene", !m_viewId.IsValid(), "Trying to unregister scene from the wrong view.");
        }
    }

    ViewId SceneComponent::GetViewId() const
    {
        return m_viewId;
    }

    void SceneComponent::DispatchSceneDropEvent(const AZ::Vector2& scenePos, const QMimeData* mimeData)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        QPointF scenePoint(scenePos.GetX(), scenePos.GetY());

        for (const AZ::EntityId& delegateId : m_delegates)
        {
            bool isInterested;
            SceneMimeDelegateHandlerRequestBus::EventResult(isInterested, delegateId, &SceneMimeDelegateHandlerRequests::IsInterestedInMimeData, GetEntityId(), mimeData);

            if (isInterested)
            {
                SceneMimeDelegateHandlerRequestBus::Event(delegateId, &SceneMimeDelegateHandlerRequests::HandleDrop, GetEntityId(), scenePoint, mimeData);
            }
        }

        // Force the focus onto the GraphicsScene after a drop.
        AZ::EntityId viewId = GetViewId();

        QTimer::singleShot(0, [viewId]()
        {
            GraphCanvasGraphicsView* graphicsView = nullptr;
            ViewRequestBus::EventResult(graphicsView, viewId, &ViewRequests::AsGraphicsView);

            if (graphicsView)
            {
                graphicsView->setFocus(Qt::FocusReason::MouseFocusReason);
            }
        });
    }

    bool SceneComponent::AddGraphData(const GraphData& graphData)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        bool success = true;

        for (const AZStd::unordered_set<AZ::Entity*>& entitySet : { graphData.m_nodes, graphData.m_bookmarkAnchors, graphData.m_connections })
        {
            for (AZ::Entity* itemRef : entitySet)
            {
                if (itemRef->GetState() == AZ::Entity::ES_INIT)
                {
                    itemRef->Activate();
                }

                success = Add(itemRef->GetId()) && success;
            }
        }

        return success;
    }

    void SceneComponent::RemoveGraphData(const GraphData& graphData)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::unordered_set<AZ::EntityId> itemIds;
        graphData.CollectItemIds(itemIds);

        for (const AZ::EntityId& itemId : itemIds)
        {
            Remove(itemId);
        }
    }

    void SceneComponent::SetDragSelectionType(DragSelectionType selectionType)
    {
        m_dragSelectionType = selectionType;
    }

    void SceneComponent::SignalDragSelectStart()
    {
        m_isDragSelecting = true;
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnDragSelectStart);
    }

    void SceneComponent::SignalDragSelectEnd()
    {
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnDragSelectEnd);
        m_isDragSelecting = false;
    }

    bool SceneComponent::OnMousePress(const AZ::EntityId& sourceId, const QGraphicsSceneMouseEvent* event)
    {
        if (event->button() == Qt::LeftButton && sourceId != m_grid)
        {
            m_enableSpliceTracking = false;
            m_enableNodeDragConnectionSpliceTracking = false;
            m_enableNodeDragCouplingTracking = false;
            m_enableNodeChainDragConnectionSpliceTracking = false;
            m_spliceTarget.SetInvalid();

            m_pressedEntity = sourceId;
            GeometryRequestBus::EventResult(m_originalPosition, m_pressedEntity, &GeometryRequests::GetPosition);
        }

        return false;
    }

    bool SceneComponent::OnMouseRelease(const AZ::EntityId& sourceId, const QGraphicsSceneMouseEvent* event)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        if (m_isDraggingEntity)
        {
            m_isDraggingEntity = false;

            AZ::Vector2 finalPosition;
            GeometryRequestBus::EventResult(finalPosition, m_pressedEntity, &GeometryRequests::GetPosition);
        
            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnSceneMemberDragComplete);

            if (m_forceDragReleaseUndo || !finalPosition.IsClose(m_originalPosition))
            {
                m_forceDragReleaseUndo = false;
                GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestUndoPoint);
            }
        }

        m_enableSpliceTracking = false;
        m_spliceTimer.stop();
        m_spliceTarget.SetInvalid();
        m_spliceTargetDisplayStateStateSetter.ResetStateSetter();
        m_pressedEntityDisplayStateStateSetter.ResetStateSetter();

        m_pressedEntity.SetInvalid();
        
        return false;
    }

    void SceneComponent::OnPositionChanged(const AZ::EntityId& itemId, const AZ::Vector2& position)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        if (m_pressedEntity.IsValid())
        {
            if (!m_isDraggingEntity)
            {
                m_isDraggingEntity = true;
                SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnSceneMemberDragBegin);

                AssetEditorSettingsRequestBus::EventResult(m_enableNodeDragConnectionSpliceTracking, GetEditorId(), &AssetEditorSettingsRequests::IsDragConnectionSpliceEnabled);
                AssetEditorSettingsRequestBus::EventResult(m_enableNodeDragCouplingTracking, GetEditorId(), &AssetEditorSettingsRequests::IsDragNodeCouplingEnabled);

                if (IsConnectableNode(m_pressedEntity) && (m_enableNodeDragConnectionSpliceTracking || m_enableNodeDragCouplingTracking))
                {
                    AZStd::vector< AZ::EntityId > selectedEntities = GetSelectedNodes();

                    m_treeSpliceEntryNodes.clear();
                    m_treeSpliceExitNodes.clear();
                    m_treeSpliceInternalNodes.clear();

                    AZStd::unordered_set<ConnectionId> entryConnections;
                    AZStd::unordered_set<ConnectionId> exitConnections;
                    AZStd::unordered_set<ConnectionId> internalConnections;

                    m_enableNodeChainDragConnectionSpliceTracking = ParseSceneMembersIntoTree(selectedEntities, m_treeSpliceEntryNodes, entryConnections, m_treeSpliceExitNodes, exitConnections, m_treeSpliceInternalNodes, internalConnections);

                    AZStd::unordered_set< AZ::EntityId > chainedEntities(selectedEntities.begin(), selectedEntities.end());
                    
                    if (m_enableNodeDragCouplingTracking && selectedEntities.size() > 1)
                    {
                        m_enableNodeDragCouplingTracking = false;
                    }

                    m_enableSpliceTracking = m_enableNodeChainDragConnectionSpliceTracking || m_enableNodeDragCouplingTracking;

                    if (m_enableSpliceTracking)
                    {
                        m_pressedEntityDisplayStateStateSetter.ResetStateSetter();

                        StateController<RootGraphicsItemDisplayState>* stateController;
                        RootGraphicsItemRequestBus::EventResult(stateController, m_pressedEntity, &RootGraphicsItemRequests::GetDisplayStateStateController);

                        m_pressedEntityDisplayStateStateSetter.AddStateController(stateController);
                    }
                    else
                    {
                        m_treeSpliceEntryNodes.clear();
                        m_treeSpliceExitNodes.clear();
                        m_treeSpliceInternalNodes.clear();
                    }
                }
            }
        }
        

        if (ConnectionRequestBus::FindFirstHandler(itemId) != nullptr)
        {
            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnNodePositionChanged, itemId, position);

            if (m_allowReset)
            {
                m_pasteOffset.setX(0);
                m_pasteOffset.setY(0);
            }
        }

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnSceneMemberPositionChanged, itemId, position);
        if (m_graphicsSceneUi)
        {
            m_graphicsSceneUi->update();
        }
    }

    void SceneComponent::OnViewParamsChanged(const ViewParams& viewParams)
    {
        m_pasteOffset.setX(0);
        m_pasteOffset.setY(0);
        
        m_viewParams = viewParams;
    }

    void SceneComponent::AddDelegate(const AZ::EntityId& delegateId)
    {
        m_delegates.insert(delegateId);
    }

    void SceneComponent::RemoveDelegate(const AZ::EntityId& delegateId)
    {
        m_delegates.erase(delegateId);
    }

    AZ::u32 SceneComponent::GetNewBookmarkCounter()
    {
        return ++m_bookmarkCounter;
    }

    void SceneComponent::OnStylesLoaded()
    {
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnStylesChanged);
    }

    void SceneComponent::OnSceneDragEnter(const QMimeData* mimeData)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        m_activeDelegates.clear();

        for (const AZ::EntityId& delegateId : m_delegates)
        {
            bool isInterested = false;
            SceneMimeDelegateHandlerRequestBus::EventResult(isInterested, delegateId, &SceneMimeDelegateHandlerRequests::IsInterestedInMimeData, GetEntityId(), mimeData);

            if (isInterested)
            {
                m_activeDelegates.insert(delegateId);
            }
        }
    }

    void SceneComponent::OnSceneDragMoveEvent(const QPointF& scenePoint, const QMimeData* mimeData)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        for (const AZ::EntityId& delegateId : m_activeDelegates)
        {
            SceneMimeDelegateHandlerRequestBus::Event(delegateId, &SceneMimeDelegateHandlerRequests::HandleMove, GetEntityId(), scenePoint, mimeData);
        }
    }

    void SceneComponent::OnSceneDropEvent(const QPointF& scenePoint, const QMimeData* mimeData)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        for (const AZ::EntityId& dropHandler : m_activeDelegates)
        {
            SceneMimeDelegateHandlerRequestBus::Event(dropHandler, &SceneMimeDelegateHandlerRequests::HandleDrop, GetEntityId(), scenePoint, mimeData);
        }

        AZ::EntityId viewId = GetViewId();

        // Force the focus onto the GraphicsView after a drop.
        QTimer::singleShot(0, [viewId]()
        {
            GraphCanvasGraphicsView* graphicsView = nullptr;
            ViewRequestBus::EventResult(graphicsView, viewId, &ViewRequests::AsGraphicsView);

            if (graphicsView)
            {
                graphicsView->setFocus(Qt::FocusReason::MouseFocusReason);
            }
        });
    }

    void SceneComponent::OnSceneDragExit(const QMimeData* mimeData)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        for (const AZ::EntityId& dropHandler : m_activeDelegates)
        {
            SceneMimeDelegateHandlerRequestBus::Event(dropHandler, &SceneMimeDelegateHandlerRequests::HandleLeave, GetEntityId(), mimeData);
        }

        m_activeDelegates.clear();
    }

    bool SceneComponent::HasActiveMimeDelegates() const
    {
        return !m_activeDelegates.empty();
    }

    bool SceneComponent::IsConnectableNode(const AZ::EntityId& entityId)
    {
        return NodeRequestBus::FindFirstHandler(entityId) != nullptr && CommentRequestBus::FindFirstHandler(entityId) == nullptr;
    }

    bool SceneComponent::IsNodeOrWrapperSelected(const NodeId& nodeId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        bool isSelected = false;

        NodeId currentNode = nodeId;
        
        do
        {
            SceneMemberUIRequestBus::EventResult(isSelected, currentNode, &SceneMemberUIRequests::IsSelected);

            bool isWrapped = false;
            NodeRequestBus::EventResult(isWrapped, currentNode, &NodeRequests::IsWrapped);

            if (isWrapped)
            {
                NodeRequestBus::EventResult(currentNode, currentNode, &NodeRequests::GetWrappingNode);
            }
            else
            {
                break;
            }

        }
        while (!isSelected);

        return isSelected;
    }

    bool SceneComponent::IsSpliceableConnection(const AZ::EntityId& connectionId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        if (ConnectionRequestBus::FindFirstHandler(connectionId) != nullptr)
        {
            AZ::EntityId testId;
            ConnectionRequestBus::EventResult(testId, connectionId, &ConnectionRequests::GetSourceNodeId);

            bool isSelected = false;
            SceneMemberUIRequestBus::EventResult(isSelected, testId, &SceneMemberUIRequests::IsSelected);

            if (isSelected)
            {
                return false;
            }

            isSelected = false;
            testId.SetInvalid();
            ConnectionRequestBus::EventResult(testId, connectionId, &ConnectionRequests::GetTargetNodeId);
            

            if (isSelected)
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        return true;
    }

    bool SceneComponent::ParseSceneMembersIntoTree(const AZStd::vector<AZ::EntityId>& sourceSceneMembers, AZStd::unordered_set<NodeId>& entryNodes, AZStd::unordered_set<ConnectionId>& entryConnections, AZStd::unordered_set<NodeId>& exitNodes, AZStd::unordered_set<ConnectionId>& exitConnections, AZStd::unordered_set<NodeId>& internalNodes, AZStd::unordered_set<ConnectionId>& internalConnections)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        if (sourceSceneMembers.empty())
        {
            return false;
        }

        bool isTree = true;
        AZStd::unordered_set< AZ::EntityId > unexploredEntities(sourceSceneMembers.begin(), sourceSceneMembers.end());

        AZStd::set< AZ::EntityId > exploredEntities;

        auto unexploredIter = unexploredEntities.begin();
        AZ::EntityId rootEntity = (*unexploredIter);

        bool isWrapped = false;
        NodeRequestBus::EventResult(isWrapped, rootEntity, &NodeRequests::IsWrapped);

        while (!IsConnectableNode(rootEntity) || isWrapped)
        {
            // Skip over Wrapped nodes, but keep them in the unexplored list for use later on.
            if (isWrapped)
            {
                ++unexploredIter;
            }
            else
            {
                unexploredIter = unexploredEntities.erase(unexploredIter);
            }

            if (unexploredIter == unexploredEntities.end())
            {
                isTree = false;
                break;
            }

            rootEntity = (*unexploredIter);
            NodeRequestBus::EventResult(isWrapped, rootEntity, &NodeRequests::IsWrapped);
        }

        AZStd::set< AZ::EntityId > searchableEntities;
        searchableEntities.insert(rootEntity);

        while (!searchableEntities.empty())
        {
            AZ::EntityId currentEntity = (*searchableEntities.begin());
            exploredEntities.insert(currentEntity);
            searchableEntities.erase(currentEntity);

            unexploredEntities.erase(currentEntity);

            if (!IsConnectableNode(currentEntity))
            {
                continue;
            }

            bool isWrapped = false;
            NodeRequestBus::EventResult(isWrapped, currentEntity, &NodeRequests::IsWrapped);
            
            // If a node is Wrapped, we don't want to allow it to invalidate the tree, but we also don't want to parse it
            // for Entrance/Exit.
            //
            // Going to skip over it, but re-insert it into the unexploredEntities, and then let the final check determine if it's
            // apart of the tree or not.
            if (isWrapped)
            {
                exploredEntities.erase(currentEntity);
                unexploredEntities.insert(currentEntity);
                continue;
            }

            AZStd::vector< AZ::EntityId> slotIds;
            NodeRequestBus::EventResult(slotIds, currentEntity, &NodeRequests::GetSlotIds);

            bool hasInternalOutput = false;
            bool isExit = false;

            bool hasInternalInput = false;
            bool isEntrance = false;

            AZStd::vector< ConnectionId > nodeConnectionIds;

            for (const AZ::EntityId& testSlot : slotIds)
            {
                bool hasConnection = false;
                SlotRequestBus::EventResult(hasConnection, testSlot, &SlotRequests::HasConnections);

                ConnectionType connectionType = ConnectionType::CT_Invalid;
                SlotRequestBus::EventResult(connectionType, testSlot, &SlotRequests::GetConnectionType);

                switch (connectionType)
                {
                case ConnectionType::CT_Input:
                    isEntrance = true;
                    break;
                case ConnectionType::CT_Output:
                    isExit = true;
                    break;
                default:
                    break;
                }

                if (hasConnection)
                {
                    AZStd::vector< ConnectionId > connectionIds;
                    SlotRequestBus::EventResult(connectionIds, testSlot, &SlotRequests::GetConnections);

                    nodeConnectionIds.insert(nodeConnectionIds.end(), connectionIds.begin(), connectionIds.end());

                    for (const AZ::EntityId& connectionId : connectionIds)
                    {
                        NodeId expansionNode;

                        if (connectionType == ConnectionType::CT_Input)
                        {
                            ConnectionRequestBus::EventResult(expansionNode, connectionId, &ConnectionRequests::GetSourceNodeId);
                        }
                        else if (connectionType == ConnectionType::CT_Output)
                        {
                            ConnectionRequestBus::EventResult(expansionNode, connectionId, &ConnectionRequests::GetTargetNodeId);
                        }

                        if (expansionNode.IsValid())
                        {
                            if (exploredEntities.find(expansionNode) != exploredEntities.end()
                                || unexploredEntities.find(expansionNode) != unexploredEntities.end())
                            {
                                switch (connectionType)
                                {
                                case ConnectionType::CT_Input:
                                    hasInternalInput = true;
                                    break;
                                case ConnectionType::CT_Output:
                                    hasInternalOutput = true;
                                    break;
                                default:
                                    break;
                                }
                            }

                            if (exploredEntities.find(expansionNode) == exploredEntities.end()
                                && unexploredEntities.find(expansionNode) != unexploredEntities.end())
                            {
                                searchableEntities.insert(expansionNode);
                            }
                        }
                    }
                }
            } // </Slots>

            // Need to process all of our wrapped elements
            // to avoid trying to snap to connections coming from things we may wrap.
            if (WrapperNodeRequestBus::FindFirstHandler(currentEntity) != nullptr)
            {
                AZStd::vector< NodeId > wrappedNodes;
                WrapperNodeRequestBus::EventResult(wrappedNodes, currentEntity, &WrapperNodeRequests::GetWrappedNodeIds);

                for (const NodeId& wrappedNodeId : wrappedNodes)
                {
                    internalNodes.insert(currentEntity);

                    AZStd::vector< AZ::EntityId> wrappedSlotIds;
                    NodeRequestBus::EventResult(wrappedSlotIds, currentEntity, &NodeRequests::GetSlotIds);

                    for (const AZ::EntityId& wrappedSlotId : wrappedSlotIds)
                    {
                        AZStd::vector< ConnectionId > wrappedConnectionIds;
                        SlotRequestBus::EventResult(wrappedConnectionIds, wrappedSlotId, &SlotRequests::GetConnections);

                        for (const ConnectionId& wrappedConnectionId : wrappedConnectionIds)
                        {
                            internalConnections.insert(wrappedConnectionId);
                        }
                    }
                }
            }

            bool isLeafNode = false;

            if (isEntrance && !hasInternalInput)
            {
                isLeafNode = true;
                entryNodes.insert(currentEntity);

                for (const ConnectionId& connectionId : nodeConnectionIds)
                {
                    entryConnections.insert(connectionId);
                }
            }

            if (isExit && !hasInternalOutput)
            {
                isLeafNode = true;
                exitNodes.insert(currentEntity);

                for (const ConnectionId& connectionId : nodeConnectionIds)
                {
                    exitConnections.insert(connectionId);
                }
            }

            if (!isLeafNode)
            {
                internalNodes.insert(currentEntity);

                for (const ConnectionId& connectionId : nodeConnectionIds)
                {
                    internalConnections.insert(connectionId);
                }
            }
        } // </Breadth First Search>

        for (const AZ::EntityId& currentEntity : unexploredEntities)
        {
            if (IsConnectableNode(currentEntity))
            {
                // Check against the outermost wrapper, which will be the actual element within the tree
                NodeId outerWrapper = currentEntity;

                bool isWrapped = false;
                NodeRequestBus::EventResult(isWrapped, currentEntity, &NodeRequests::IsWrapped);
                
                while (isWrapped)
                {
                    NodeId wrapperNode;
                    NodeRequestBus::EventResult(outerWrapper, outerWrapper, &NodeRequests::GetWrappingNode);

                    NodeRequestBus::EventResult(isWrapped, outerWrapper, &NodeRequests::IsWrapped);
                }

                if (exploredEntities.find(outerWrapper) != exploredEntities.end())
                {
                    continue;
                }

                isTree = false;
                break;
            }
        }

        return isTree;
    }

    template<typename Container>
    void SceneComponent::InitItems(const Container& entities) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        for (const auto& entityRef : entities)
        {
            AZ::Entity* entity = entityRef;
            if (entity)
            {
                AZ::Entity::State state = entity->GetState();
                if (entity->GetState() == AZ::Entity::ES_CONSTRUCTED)
                {
                    entity->Init();
                }
            }
        }
    }

    template<typename Container>
    void SceneComponent::ActivateItems(const Container& entities)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        for (const auto& entityRef : entities)
        {
            AZ::Entity* entity = entityRef;
            if (entity)
            {
                if (entity->GetState() == AZ::Entity::ES_INIT)
                {
                    entity->Activate();
                }

                AddSceneMember(entity->GetId());
            }
        }
    }

    template<typename Container>
    void SceneComponent::DeactivateItems(const Container& entities)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        for (const auto& entityRef : entities)
        {
            AZ::Entity* entity = entityRef;
            if (entity)
            {
                if (entity->GetState() == AZ::Entity::ES_ACTIVE)
                {
                    GeometryNotificationBus::MultiHandler::BusDisconnect(entity->GetId());
                    QGraphicsItem* item = nullptr;
                    SceneMemberUIRequestBus::EventResult(item, entity->GetId(), &SceneMemberUIRequests::GetRootGraphicsItem);
                    SceneMemberRequestBus::Event(entity->GetId(), &SceneMemberRequests::ClearScene, GetEntityId());
                    if (m_graphicsSceneUi)
                    {
                        m_graphicsSceneUi->removeItem(item);
                    }
                    entity->Deactivate();
                }
            }
        }
    }

    template<typename Container>
    void SceneComponent::DestroyItems(const Container& entities) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        for (auto& entityRef : entities)
        {
            delete entityRef;
        }
    }

    void SceneComponent::InitConnections()
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        BuildEndpointMap(m_graphData);
        InitItems(m_graphData.m_connections);
    }

    void SceneComponent::NotifyConnectedSlots()
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        for (auto& connection : m_graphData.m_connections)
        {
            AZ::Entity* entity = connection;
            auto* connectionEntity = entity ? AZ::EntityUtils::FindFirstDerivedComponent<ConnectionComponent>(entity) : nullptr;
            if (connectionEntity)
            {
                SlotRequestBus::Event(connectionEntity->GetSourceEndpoint().GetSlotId(), &SlotRequests::AddConnectionId, connectionEntity->GetEntityId(), connectionEntity->GetTargetEndpoint());
                SlotRequestBus::Event(connectionEntity->GetTargetEndpoint().GetSlotId(), &SlotRequests::AddConnectionId, connectionEntity->GetEntityId(), connectionEntity->GetSourceEndpoint());
            }
        }
    }

    void SceneComponent::OnSelectionChanged()
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        if((m_isDragSelecting && (m_dragSelectionType != DragSelectionType::Realtime)))
        {
            // Nothing to do.
            return;
        }

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnSelectionChanged);
    }

    void SceneComponent::RegisterSelectionItem(const AZ::EntityId& itemId)
    {
        QGraphicsItem* selectionItem = nullptr;
        SceneMemberUIRequestBus::EventResult(selectionItem, itemId, &SceneMemberUIRequests::GetSelectionItem);

        m_itemLookup[selectionItem] = itemId;
    }

    void SceneComponent::UnregisterSelectionItem(const AZ::EntityId& itemId)
    {
        QGraphicsItem* selectionItem = nullptr;
        SceneMemberUIRequestBus::EventResult(selectionItem, itemId, &SceneMemberUIRequests::GetSelectionItem);

        m_itemLookup.erase(selectionItem);
    }

    void SceneComponent::AddSceneMember(const AZ::EntityId& sceneMemberId, bool positionItem, const AZ::Vector2& position)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        QGraphicsItem* graphicsItem = nullptr;
        SceneMemberUIRequestBus::EventResult(graphicsItem, sceneMemberId, &SceneMemberUIRequests::GetRootGraphicsItem);

        if (graphicsItem)
        {
            if (m_graphicsSceneUi)
            {
                m_graphicsSceneUi->addItem(graphicsItem);
            }

            RegisterSelectionItem(sceneMemberId);
            
            SceneMemberRequestBus::Event(sceneMemberId, &SceneMemberRequests::SetScene, GetEntityId());

            if (positionItem)
            {
                GeometryRequestBus::Event(sceneMemberId, &GeometryRequests::SetPosition, position);
            }

            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnSceneMemberAdded, sceneMemberId);
            GeometryNotificationBus::MultiHandler::BusConnect(sceneMemberId);
            VisualNotificationBus::MultiHandler::BusConnect(sceneMemberId);

            SceneMemberRequestBus::Event(sceneMemberId, &SceneMemberRequests::SignalMemberSetupComplete);
        }
    }

    AZStd::unordered_set<AZ::EntityId> SceneComponent::FindConnections(const AZStd::unordered_set<AZ::EntityId>& nodeIds, bool internalConnectionsOnly) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        AZStd::unordered_set<AZ::EntityId> foundConnections;
        for (auto& connection : m_graphData.m_connections)
        {
            AZ::EntityId source;
            ConnectionRequestBus::EventResult(source, connection->GetId(), &ConnectionRequests::GetSourceNodeId);
            AZ::EntityId target;
            ConnectionRequestBus::EventResult(target, connection->GetId(), &ConnectionRequests::GetTargetNodeId);

            if (internalConnectionsOnly)
            {
                // Both sides of the connection must be in the nodeIds set for internal connections
                if (nodeIds.count(source) != 0 && nodeIds.count(target) != 0)
                {
                    foundConnections.emplace(connection->GetId());
                }
            }
            else
            {
                if (nodeIds.count(source) != 0 || nodeIds.count(target) != 0)
                {
                    foundConnections.emplace(connection->GetId());
                }
            }
        }

        return foundConnections;
    }

    bool SceneComponent::ValidateConnectionEndpoints(AZ::Entity* connection, const AZStd::unordered_set<AZ::Entity*>& nodeRefs)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        Endpoint source;
        ConnectionRequestBus::EventResult(source, connection->GetId(), &ConnectionRequests::GetSourceEndpoint);
        Endpoint target;
        ConnectionRequestBus::EventResult(target, connection->GetId(), &ConnectionRequests::GetTargetEndpoint);

        auto sourceFound = AZStd::find_if(nodeRefs.begin(), nodeRefs.end(), [&source](const AZ::Entity* node) { return node->GetId() == source.GetNodeId(); }) != nodeRefs.end();
        auto targetFound = AZStd::find_if(nodeRefs.begin(), nodeRefs.end(), [&target](const AZ::Entity* node) { return node->GetId() == target.GetNodeId(); }) != nodeRefs.end();
        return sourceFound != 0 && targetFound != 0;
    }

    void SceneComponent::SieveSceneMembers(const AZStd::unordered_set<AZ::EntityId>& itemIds, SceneMemberBuckets& sceneMembers) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        AZStd::unordered_set< AZ::EntityId > wrapperNodes;

        for (const auto& node : m_graphData.m_nodes)
        {
            if (itemIds.find(node->GetId()) != itemIds.end())
            {
                sceneMembers.m_nodes.insert(node->GetId());

                if (WrapperNodeRequestBus::FindFirstHandler(node->GetId()) != nullptr)
                {
                    wrapperNodes.insert(node->GetId());
                }
            }
        }

        // Wrapper nodes handle copying/deleting everything internal to themselves. 
        // So we need to sanitize our filtering to avoid things that are wrapped when the wrapper
        // is also copied.
        for (const auto& wrapperNode : wrapperNodes)
        {
            AZStd::vector< AZ::EntityId > wrappedNodes;
            WrapperNodeRequestBus::EventResult(wrappedNodes, wrapperNode, &WrapperNodeRequests::GetWrappedNodeIds);

            for (const auto& wrappedNode : wrappedNodes)
            {
                sceneMembers.m_nodes.erase(wrappedNode);
            }
        }
        
        for (const auto& connection : m_graphData.m_connections)
        {
            if (itemIds.find(connection->GetId()) != itemIds.end())
            {
                sceneMembers.m_connections.insert(connection->GetId());
            }
        }

        for (const auto& bookmarkAnchors : m_graphData.m_bookmarkAnchors)
        {
            if (itemIds.find(bookmarkAnchors->GetId()) != itemIds.end())
            {
                sceneMembers.m_bookmarkAnchors.insert(bookmarkAnchors->GetId());
            }
        }
    }

    QPointF SceneComponent::GetViewCenterScenePoint() const
    {
        AZ::Vector2 viewCenter(0,0);

        ViewId viewId = GetViewId();

        ViewRequestBus::EventResult(viewCenter, viewId, &ViewRequests::GetViewSceneCenter);

        return QPointF(viewCenter.GetX(), viewCenter.GetY());
    }

    void SceneComponent::OnCursorMoveForSplice()
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        QGraphicsItem* graphicsItem = nullptr;
        SceneMemberUIRequestBus::EventResult(graphicsItem, m_pressedEntity, &SceneMemberUIRequests::GetRootGraphicsItem);

        if (graphicsItem)
        {
            // We'll use the bounding rect to determine visibility.
            // But we'll use the cursor position to determine snapping
            QRectF boundingRect = graphicsItem->sceneBoundingRect();

            AZStd::vector< AZ::EntityId > sceneEntities = GetEntitiesInRect(boundingRect, Qt::ItemSelectionMode::IntersectsItemShape);

            if (!sceneEntities.empty())
            {
                bool ambiguousNode = false;
                AZ::EntityId hoveredNode;

                bool ambiguousConnection = false;
                AZStd::vector< AZ::EntityId > ambiguousConnections;

                for (const AZ::EntityId& currentEntity : sceneEntities)
                {
                    if (currentEntity == m_pressedEntity)
                    {
                        continue;
                    }

                    if (IsSpliceableConnection(currentEntity))
                    {
                        ambiguousConnections.push_back(currentEntity);
                    }
                    else if (IsConnectableNode(currentEntity))
                    {
                        bool isWrapped = false;
                        NodeRequestBus::EventResult(isWrapped, currentEntity, &NodeRequests::IsWrapped);

                        if (isWrapped)
                        {
                            AZ::EntityId parentId;
                            NodeRequestBus::EventResult(parentId, currentEntity, &NodeRequests::GetWrappingNode);

                            while (parentId.IsValid())
                            {
                                if (parentId == m_pressedEntity)
                                {
                                    break;
                                }

                                NodeRequestBus::EventResult(isWrapped, currentEntity, &NodeRequests::IsWrapped);

                                if (isWrapped)
                                {
                                    NodeRequestBus::EventResult(parentId, parentId, &NodeRequests::GetWrappingNode);
                                }
                                else
                                {
                                    break;
                                }
                            }

                            if (parentId == m_pressedEntity)
                            {
                                continue;
                            }
                        }

                        if (hoveredNode.IsValid())
                        {
                            ambiguousNode = true;
                        }

                        hoveredNode = currentEntity;
                    }
                }

                AZStd::chrono::milliseconds spliceTime(500);

                if (m_enableNodeDragCouplingTracking && !ambiguousNode && hoveredNode.IsValid())
                {
                    InitiateSpliceToNode(hoveredNode);
                    AssetEditorSettingsRequestBus::EventResult(spliceTime, GetEditorId(), &AssetEditorSettingsRequests::GetDragCouplingTime);
                }
                else if (m_enableNodeDragConnectionSpliceTracking)
                {
                    InitiateSpliceToConnection(ambiguousConnections);
                    AssetEditorSettingsRequestBus::EventResult(spliceTime, GetEditorId(), &AssetEditorSettingsRequests::GetDragConnectionSpliceTime);
                }
                else
                {
                    m_spliceTarget.SetInvalid();
                }

                // If we move, no matter what. Restart the timer, so long as we have a valid target.
                m_spliceTimer.stop();

                if (m_spliceTarget.IsValid())
                {
                    int spliceTimeout = 0;

                    m_spliceTimer.setInterval(spliceTime.count());
                    m_spliceTimer.start();
                }
            }
            else
            {
                m_spliceTargetDisplayStateStateSetter.ResetStateSetter();
                m_pressedEntityDisplayStateStateSetter.ReleaseState();
                m_spliceTimer.stop();
            }
        }
    }

    void SceneComponent::OnTrySplice()
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestPushPreventUndoStateUpdate);

        m_spliceTargetDisplayStateStateSetter.ResetStateSetter();
        m_pressedEntityDisplayStateStateSetter.ReleaseState();

        if (m_enableSpliceTracking && m_spliceTarget.IsValid() && m_pressedEntity.IsValid())
        {
            AnimatedPulseConfiguration pulseConfiguration;

            pulseConfiguration.m_durationSec = 0.35f;
            pulseConfiguration.m_enableGradient = true;

            QGraphicsItem* item = nullptr;
            SceneMemberUIRequestBus::EventResult(item, m_pressedEntity, &SceneMemberUIRequests::GetRootGraphicsItem);

            if (item)
            {
                pulseConfiguration.m_zValue = item->zValue() - 1;
            }

            if (ConnectionRequestBus::FindFirstHandler(m_spliceTarget) != nullptr)
            {
                if (m_enableNodeChainDragConnectionSpliceTracking)
                {
                    if (TrySpliceNodeTreeOntoConnection(m_treeSpliceEntryNodes, m_treeSpliceExitNodes, m_spliceTarget))
                    {
                        m_forceDragReleaseUndo = true;
                        pulseConfiguration.m_drawColor = QColor(255, 255, 255);
                    }
                    else
                    {
                        pulseConfiguration.m_drawColor = QColor(255, 0, 0);
                    }
                }
                else
                {
                    if (TrySpliceNodeOntoConnection(m_pressedEntity, m_spliceTarget))
                    {
                        m_forceDragReleaseUndo = true;
                        pulseConfiguration.m_drawColor = QColor(255, 255, 255);
                    }
                    else
                    {
                        pulseConfiguration.m_drawColor = QColor(255, 0, 0);
                    }
                }
            }
            else if (NodeRequestBus::FindFirstHandler(m_spliceTarget) != nullptr
                    && CommentRequestBus::FindFirstHandler(m_spliceTarget) == nullptr)
            {
                QGraphicsItem* targetItem = nullptr;
                SceneMemberUIRequestBus::EventResult(targetItem, m_spliceTarget, &SceneMemberUIRequests::GetRootGraphicsItem);

                QGraphicsItem* draggingEntity = nullptr;
                SceneMemberUIRequestBus::EventResult(draggingEntity, m_pressedEntity, &SceneMemberUIRequests::GetRootGraphicsItem);

                if (targetItem)
                {
                    QRectF targetRect = targetItem->sceneBoundingRect();
                    QRectF draggingRect = draggingEntity->sceneBoundingRect();

                    GraphCanvas::ConnectionType allowedType = GraphCanvas::ConnectionType::CT_None;

                    // Reference point is we are gathering slots from the pressed node.
                    // So we want to determine which side is offset from, and grabs the nodes from the other side.
                    if (draggingRect.x() > targetRect.x())
                    {
                        allowedType = GraphCanvas::ConnectionType::CT_Input;
                    }
                    else
                    {
                        allowedType = GraphCanvas::ConnectionType::CT_Output;
                    }

                    AZStd::vector< AZ::EntityId > slotIds;
                    NodeRequestBus::EventResult(slotIds, m_pressedEntity, &NodeRequests::GetSlotIds);

                    AZStd::vector< Endpoint > connectableEndpoints;

                    for (const AZ::EntityId& testSlotId : slotIds)
                    {
                        GraphCanvas::ConnectionType connectionType = GraphCanvas::ConnectionType::CT_Invalid;
                        SlotRequestBus::EventResult(connectionType, testSlotId, &SlotRequests::GetConnectionType);

                        if (connectionType == allowedType)
                        {
                            connectableEndpoints.emplace_back(m_pressedEntity, testSlotId);
                        }
                    }

                    if (TryCreateConnectionsBetween(connectableEndpoints, m_spliceTarget))
                    {
                        m_forceDragReleaseUndo = true;
                        pulseConfiguration.m_drawColor = QColor(255, 255, 255);
                    }
                    else
                    {
                        pulseConfiguration.m_drawColor = QColor(255, 0, 0);
                    }
                }
            }

            CreatePulseAroundSceneMember(m_pressedEntity, 3, pulseConfiguration);
            m_spliceTarget.SetInvalid();
        }

        GraphModelRequestBus::Event(GetEntityId(), &GraphModelRequests::RequestPopPreventUndoStateUpdate);
    }

    void SceneComponent::InitiateSpliceToNode(const NodeId& nodeId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        if (m_spliceTarget != nodeId)
        {
            m_spliceTargetDisplayStateStateSetter.ResetStateSetter();

            m_spliceTarget = nodeId;

            if (m_spliceTarget.IsValid())
            {
                m_pressedEntityDisplayStateStateSetter.SetState(RootGraphicsItemDisplayState::InspectionTransparent);

                StateController<RootGraphicsItemDisplayState>* stateController;
                RootGraphicsItemRequestBus::EventResult(stateController, m_spliceTarget, &RootGraphicsItemRequests::GetDisplayStateStateController);

                m_spliceTargetDisplayStateStateSetter.AddStateController(stateController);
                m_spliceTargetDisplayStateStateSetter.SetState(RootGraphicsItemDisplayState::Preview);
            }
            else
            {
                m_pressedEntityDisplayStateStateSetter.ReleaseState();
            }
        }
    }

    void SceneComponent::InitiateSpliceToConnection(const AZStd::vector<ConnectionId>& connectionIds)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        m_spliceTarget.SetInvalid();
        m_spliceTargetDisplayStateStateSetter.ResetStateSetter();

        if (!connectionIds.empty())
        {
            m_pressedEntityDisplayStateStateSetter.SetState(RootGraphicsItemDisplayState::InspectionTransparent);
        }
        else if (m_pressedEntityDisplayStateStateSetter.HasState())
        {
            m_pressedEntityDisplayStateStateSetter.ReleaseState();
        }

        GraphCanvasGraphicsView* graphicsView = nullptr;
        ViewRequestBus::EventResult(graphicsView, m_viewId, &ViewRequests::AsGraphicsView);

        QPointF scenePoint;

        if (graphicsView)
        {
            QPointF cursorPoint = QCursor::pos();
            QPointF viewPoint = graphicsView->mapFromGlobal(cursorPoint.toPoint());
            scenePoint = graphicsView->mapToScene(viewPoint.toPoint());
        }        

        for (const ConnectionId& connectionId : connectionIds)
        {
            bool containsCursor = false;

            QGraphicsItem* spliceTargetItem = nullptr;
            SceneMemberUIRequestBus::EventResult(spliceTargetItem, connectionId, &SceneMemberUIRequests::GetRootGraphicsItem);

            if (spliceTargetItem)
            {
                containsCursor = spliceTargetItem->contains(scenePoint);
            }

            if (containsCursor)
            {                
                m_spliceTarget = connectionId;

                StateController<RootGraphicsItemDisplayState>* stateController;
                RootGraphicsItemRequestBus::EventResult(stateController, m_spliceTarget, &RootGraphicsItemRequests::GetDisplayStateStateController);

                m_spliceTargetDisplayStateStateSetter.AddStateController(stateController);
                m_spliceTargetDisplayStateStateSetter.SetState(RootGraphicsItemDisplayState::Preview);
            }
        }
    }

    //////////////////////////////
    // GraphCanvasGraphicsScenes
    //////////////////////////////

    GraphCanvasGraphicsScene::GraphCanvasGraphicsScene(SceneComponent& scene)
        : m_scene(scene)
        , m_suppressContextMenu(false)
    {
        setMinimumRenderSize(2.0f);
        connect(this, &QGraphicsScene::selectionChanged, this, [this]() { m_scene.OnSelectionChanged(); });
        setSceneRect(-100000, -100000, 200000, 200000);
    }

    AZ::EntityId GraphCanvasGraphicsScene::GetEntityId() const
    {
        return m_scene.GetEntityId();
    }

    void GraphCanvasGraphicsScene::SuppressNextContextMenu()
    {
        m_suppressContextMenu = true;
    }

    void GraphCanvasGraphicsScene::keyPressEvent(QKeyEvent* event)
    {
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnKeyPressed, event);

        QGraphicsScene::keyPressEvent(event);
    }

    void GraphCanvasGraphicsScene::keyReleaseEvent(QKeyEvent* event)
    {
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnKeyReleased, event);

        QGraphicsScene::keyPressEvent(event);
    }

    void GraphCanvasGraphicsScene::contextMenuEvent(QGraphicsSceneContextMenuEvent* contextMenuEvent)
    {
        if (!m_suppressContextMenu)
        {
            contextMenuEvent->ignore();

            // Send the event to all items at this position until one item accepts the event.
            for (QGraphicsItem* item : itemsAtPosition(contextMenuEvent->screenPos(), contextMenuEvent->scenePos(), contextMenuEvent->widget()))
            {
                contextMenuEvent->setPos(item->mapFromScene(contextMenuEvent->scenePos()));
                contextMenuEvent->accept();
                if (!sendEvent(item, contextMenuEvent))
                {
                    break;
                }

                if (contextMenuEvent->isAccepted())
                {
                    break;
                }
            }

            if (!contextMenuEvent->isAccepted())
            {
                SceneUIRequestBus::Event(GetEntityId(), &SceneUIRequests::OnSceneContextMenuEvent, GetEntityId(), contextMenuEvent);
            }
        }
        else
        {
            m_suppressContextMenu = false;
        }
    }

    QList<QGraphicsItem*> GraphCanvasGraphicsScene::itemsAtPosition(const QPoint& screenPos, const QPointF& scenePos, QWidget* widget) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        QGraphicsView* view = widget ? qobject_cast<QGraphicsView*>(widget->parentWidget()) : nullptr;
        if (!view)
        {
            return items(scenePos, Qt::IntersectsItemShape, Qt::DescendingOrder, QTransform());
        }

        const QRectF pointRect(QPointF(widget->mapFromGlobal(screenPos)), QSizeF(1, 1));
        if (!view->isTransformed())
        {
            return items(pointRect, Qt::IntersectsItemShape, Qt::DescendingOrder);
        }

        const QTransform viewTransform = view->viewportTransform();
        if (viewTransform.type() <= QTransform::TxScale)
        {
            return items(viewTransform.inverted().mapRect(pointRect), Qt::IntersectsItemShape,
                Qt::DescendingOrder, viewTransform);
        }
        return items(viewTransform.inverted().map(pointRect), Qt::IntersectsItemShape,
            Qt::DescendingOrder, viewTransform);
    }

    void GraphCanvasGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
    {
        if (event->button() == Qt::RightButton)
        {
            // IMPORTANT: When the user right-clicks on the scene,
            // and there are NO items at the click position, the
            // current selection is lost. See documentation:
            //
            // "If there is no item at the given position on the scene,
            // the selection area is reset, any focus item loses its
            // input focus, and the event is then ignored."
            // http://doc.qt.io/qt-5/qgraphicsscene.html#mousePressEvent
            //
            // This ISN'T the behavior we want. We want to preserve
            // the current selection to allow scene interactions. To get around
            // this behavior, we'll accept the event and by-pass its
            // default implementation.

            event->accept();
            return;
        }

        QGraphicsScene::mousePressEvent(event);
    }

    void GraphCanvasGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
    {
        m_scene.OnSelectionChanged();

        QGraphicsScene::mouseReleaseEvent(event);
    }

    void GraphCanvasGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
    {
        QGraphicsScene::mouseMoveEvent(event);

        if (m_scene.m_enableSpliceTracking)
        {
            m_scene.OnCursorMoveForSplice();
        }
    }

    void GraphCanvasGraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
    {
        SceneUIRequestBus::Event(GetEntityId(), &SceneUIRequests::OnSceneDoubleClickEvent, GetEntityId(), event);

        QGraphicsScene::mouseDoubleClickEvent(event);
    }

    void GraphCanvasGraphicsScene::dragEnterEvent(QGraphicsSceneDragDropEvent * event)
    {
        QGraphicsScene::dragEnterEvent(event);

        m_scene.OnSceneDragEnter(event->mimeData());

        if (m_scene.HasActiveMimeDelegates())
        {
            event->accept();
            event->acceptProposedAction();
        }
    }

    void GraphCanvasGraphicsScene::dragLeaveEvent(QGraphicsSceneDragDropEvent* event)
    {
        QGraphicsScene::dragLeaveEvent(event);

        m_scene.OnSceneDragExit(event->mimeData());
    }

    void GraphCanvasGraphicsScene::dragMoveEvent(QGraphicsSceneDragDropEvent* event)
    {
        QGraphicsScene::dragMoveEvent(event);

        m_scene.OnSceneDragMoveEvent(event->scenePos(), event->mimeData());

        if (m_scene.HasActiveMimeDelegates())
        {
            event->accept();
            event->acceptProposedAction();
        }
    }

    void GraphCanvasGraphicsScene::dropEvent(QGraphicsSceneDragDropEvent* event)
    {
        bool accepted = event->isAccepted();
        event->setAccepted(false);

        QGraphicsScene::dropEvent(event);

        if (!event->isAccepted() && m_scene.HasActiveMimeDelegates())
        {
            event->accept();
            m_scene.OnSceneDropEvent(event->scenePos(), event->mimeData());
        }
        else
        {
            event->setAccepted(accepted);
        }
    }
}
