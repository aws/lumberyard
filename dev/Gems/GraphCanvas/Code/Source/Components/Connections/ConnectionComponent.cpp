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
#include "precompiled.h"

#include <qcursor.h>
#include <qgraphicsscene.h>
#include <qgraphicsview.h>

#include <AzCore/Serialization/EditContext.h>

#include <Components/Connections/ConnectionComponent.h>

#include <Components/Connections/ConnectionLayerControllerComponent.h>
#include <Components/Connections/ConnectionVisualComponent.h>
#include <Components/StylingComponent.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>
#include <GraphCanvas/Utils/GraphUtils.h>
#include <Utils/ConversionUtils.h>

namespace GraphCanvas
{
    ///////////////////////////////
    // ConnectionEndpointAnimator
    ///////////////////////////////

    ConnectionComponent::ConnectionEndpointAnimator::ConnectionEndpointAnimator()
        : m_isAnimating(false)
        , m_timer(0.0f)
        , m_maxTime(0.25f)
    {

    }

    bool ConnectionComponent::ConnectionEndpointAnimator::IsAnimating() const
    {
        return m_isAnimating;
    }

    void ConnectionComponent::ConnectionEndpointAnimator::AnimateToEndpoint(const QPointF& startPoint, const Endpoint& endPoint, float maxTime)
    {
        if (m_isAnimating)
        {
            m_startPosition = m_currentPosition;
        }
        else
        {
            m_isAnimating = true;
            m_startPosition = startPoint;
        }

        m_targetEndpoint = endPoint;

        m_timer = 0.0f;
        m_maxTime = AZ::GetMax(maxTime, 0.001f);

        m_currentPosition = m_startPosition;
    }

    QPointF ConnectionComponent::ConnectionEndpointAnimator::GetAnimatedPosition() const
    {
        return m_currentPosition;
    }

    bool ConnectionComponent::ConnectionEndpointAnimator::Tick(float deltaTime)
    {
        m_timer += deltaTime;

        QPointF targetPosition;
        SlotUIRequestBus::EventResult(targetPosition, m_targetEndpoint.m_slotId, &SlotUIRequests::GetConnectionPoint);

        if (m_timer >= m_maxTime)
        {
            m_isAnimating = false;
            m_currentPosition = targetPosition;
        }
        else
        {
            float lerpPercent = (m_timer / m_maxTime);
            m_currentPosition.setX(AZ::Lerp(static_cast<float>(m_startPosition.x()), static_cast<float>(targetPosition.x()), lerpPercent));
            m_currentPosition.setY(AZ::Lerp(static_cast<float>(m_startPosition.y()), static_cast<float>(targetPosition.y()), lerpPercent));
        }

        return m_isAnimating;
    }

    ////////////////////////
    // ConnectionComponent
    ////////////////////////

    void ConnectionComponent::Reflect(AZ::ReflectContext* context)
    {
        Endpoint::Reflect(context);
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<ConnectionComponent, AZ::Component>()
            ->Version(3)
            ->Field("Source", &ConnectionComponent::m_sourceEndpoint)
            ->Field("Target", &ConnectionComponent::m_targetEndpoint)
            ->Field("Tooltip", &ConnectionComponent::m_tooltip)
            ->Field("UserData", &ConnectionComponent::m_userData)
            ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<ConnectionComponent>("Position", "The connection's position in the scene")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "Connection's class attributes")
            ->DataElement(AZ::Edit::UIHandlers::Default, &ConnectionComponent::m_tooltip, "Tooltip", "The connection's tooltip")
            ;
    }

    AZ::Entity* ConnectionComponent::CreateBaseConnectionEntity(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint, bool createModelConnection, const AZStd::string& selectorClass)
    {
        // Create this Connection's entity.
        AZ::Entity* entity = aznew AZ::Entity("Connection");

        entity->CreateComponent<ConnectionComponent>(sourceEndpoint, targetEndpoint, createModelConnection);
        entity->CreateComponent<StylingComponent>(Styling::Elements::Connection, AZ::EntityId(), selectorClass);
        entity->CreateComponent<ConnectionLayerControllerComponent>();

        return entity;
    }

    AZ::Entity* ConnectionComponent::CreateGeneralConnection(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint, bool createModelConnection, const AZStd::string& substyle)
    {
        AZ::Entity* entity = CreateBaseConnectionEntity(sourceEndpoint, targetEndpoint, createModelConnection, substyle);
        entity->CreateComponent<ConnectionVisualComponent>();

        return entity;
    }    

    ConnectionComponent::ConnectionComponent()
        : m_dragContext(DragContext::Unknown)
    {
    }

    ConnectionComponent::ConnectionComponent(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint, bool createModelConnection)
        : AZ::Component()
    {
        m_sourceEndpoint = sourceEndpoint;
        m_targetEndpoint = targetEndpoint;

        AZ_Warning("GraphCanvas", m_targetEndpoint.IsValid() || m_sourceEndpoint.IsValid(), "Either source or target endpoint must be valid when creating a connection.");

        if (createModelConnection && m_sourceEndpoint.IsValid() && m_targetEndpoint.IsValid())
        {
            m_dragContext = DragContext::TryConnection;
        }
        else
        {
            m_dragContext = DragContext::Unknown;
        }
    }

    void ConnectionComponent::Activate()
    {
        ConnectionRequestBus::Handler::BusConnect(GetEntityId());
        SceneMemberRequestBus::Handler::BusConnect(GetEntityId());
        
        if (m_sourceEndpoint.IsValid() && m_targetEndpoint.IsValid() && m_dragContext != DragContext::TryConnection)
        {
            m_dragContext = DragContext::Connected;
        }

        StateController<RootGraphicsItemDisplayState>* displayStateController = nullptr;
        RootGraphicsItemRequestBus::EventResult(displayStateController, GetEntityId(), &RootGraphicsItemRequests::GetDisplayStateStateController);

        m_connectionStateStateSetter.AddStateController(displayStateController);
    }

    void ConnectionComponent::Deactivate()
    {
        StopMove();

        SceneMemberRequestBus::Handler::BusDisconnect();
        ConnectionRequestBus::Handler::BusDisconnect();
    }

    AZ::EntityId ConnectionComponent::GetSourceSlotId() const
    {
        return m_sourceEndpoint.GetSlotId();
    }

    AZ::EntityId ConnectionComponent::GetSourceNodeId() const
    {
        return m_sourceEndpoint.GetNodeId();
    }

    Endpoint ConnectionComponent::GetSourceEndpoint() const
    {
        return m_sourceEndpoint;
    }

    QPointF ConnectionComponent::GetSourcePosition() const
    {
        if (m_sourceAnimator.IsAnimating())
        {
            return m_sourceAnimator.GetAnimatedPosition();
        }
        else if (m_sourceEndpoint.IsValid())
        {
            QPointF connectionPoint;
            SlotUIRequestBus::EventResult(connectionPoint, m_sourceEndpoint.m_slotId, &SlotUIRequests::GetConnectionPoint);

            return connectionPoint;
        }
        else
        {
            return m_mousePoint;
        }
    }

    void ConnectionComponent::StartSourceMove()
    {
        SlotRequestBus::Event(GetSourceEndpoint().GetSlotId(), &SlotRequests::RemoveConnectionId, GetEntityId(), GetTargetEndpoint());
        m_previousEndPoint = m_sourceEndpoint;
        m_sourceEndpoint = Endpoint();

        m_dragContext = DragContext::MoveSource;

        StartMove();
    }

    void ConnectionComponent::SnapSourceDisplayTo(const Endpoint& sourceEndpoint)
    {
        if (!sourceEndpoint.IsValid())
        {
            AZ_Error("GraphCanvas", false, "Trying to display a connection to an unknown source Endpoint");
            return;
        }

        bool canDisplaySource = false;
        SlotRequestBus::EventResult(canDisplaySource, m_targetEndpoint.GetSlotId(), &SlotRequests::CanDisplayConnectionTo, sourceEndpoint);

        if (!canDisplaySource)
        {
            return;
        }

        if (m_sourceEndpoint.IsValid())
        {
            SlotRequestBus::Event(m_sourceEndpoint.GetSlotId(), &SlotRequests::RemoveConnectionId, GetEntityId(), m_targetEndpoint);
        }

        Endpoint oldEndpoint = m_sourceEndpoint;
        m_sourceEndpoint = sourceEndpoint;

        ConnectionNotificationBus::Event(GetEntityId(), &ConnectionNotifications::OnSourceSlotIdChanged, oldEndpoint.GetSlotId(), m_sourceEndpoint.GetSlotId());
        SlotRequestBus::Event(m_sourceEndpoint.GetSlotId(), &SlotRequests::AddConnectionId, GetEntityId(), m_targetEndpoint);
    }

    void ConnectionComponent::AnimateSourceDisplayTo(const Endpoint& sourceEndpoint, float connectionTime)
    {
        QPointF startPosition = GetSourcePosition();
        SnapSourceDisplayTo(sourceEndpoint);        
        m_sourceAnimator.AnimateToEndpoint(startPosition, sourceEndpoint, connectionTime);
            
        if (!AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusConnect();
        }        
    }    

    AZ::EntityId ConnectionComponent::GetTargetSlotId() const
    {
        return m_targetEndpoint.GetSlotId();
    }

    AZ::EntityId ConnectionComponent::GetTargetNodeId() const
    {
        return m_targetEndpoint.GetNodeId();
    }

    Endpoint ConnectionComponent::GetTargetEndpoint() const
    {
        return m_targetEndpoint;
    }

    QPointF ConnectionComponent::GetTargetPosition() const
    {
        if (m_targetAnimator.IsAnimating())
        {
            return m_targetAnimator.GetAnimatedPosition();
        }
        else if (m_targetEndpoint.IsValid())
        {
            QPointF connectionPoint;
            SlotUIRequestBus::EventResult(connectionPoint, m_targetEndpoint.m_slotId, &SlotUIRequests::GetConnectionPoint);

            return connectionPoint;
        }
        else
        {
            return m_mousePoint;
        }
    }

    void ConnectionComponent::StartTargetMove()
    {
        SlotRequestBus::Event(GetTargetEndpoint().GetSlotId(), &SlotRequests::RemoveConnectionId, GetEntityId(), GetSourceEndpoint());

        m_previousEndPoint = m_targetEndpoint;
        m_targetEndpoint = Endpoint();
        m_dragContext = DragContext::MoveTarget;

        StartMove();
    }

    void ConnectionComponent::SnapTargetDisplayTo(const Endpoint& targetEndpoint)
    {
        if (!targetEndpoint.IsValid())
        {
            AZ_Error("GraphCanvas", false, "Trying to display a connection to an unknown source Endpoint");
            return;
        }

        bool canDisplayTarget = false;
        SlotRequestBus::EventResult(canDisplayTarget, m_sourceEndpoint.GetSlotId(), &SlotRequests::CanDisplayConnectionTo, targetEndpoint);

        if (!canDisplayTarget)
        {
            return;
        }        

        if (m_sourceEndpoint.IsValid())
        {
            SlotRequestBus::Event(m_targetEndpoint.GetSlotId(), &SlotRequests::RemoveConnectionId, GetEntityId(), m_sourceEndpoint);
        }

        Endpoint oldTarget = m_targetEndpoint;

        m_targetEndpoint = targetEndpoint;

        ConnectionNotificationBus::Event(GetEntityId(), &ConnectionNotifications::OnTargetSlotIdChanged, oldTarget.GetSlotId(), m_targetEndpoint.GetSlotId());
        SlotRequestBus::Event(m_targetEndpoint.GetSlotId(), &SlotRequests::AddConnectionId, GetEntityId(), m_sourceEndpoint);        
    }

    void ConnectionComponent::AnimateTargetDisplayTo(const Endpoint& targetEndpoint, float connectionTime)
    {
        QPointF startPosition = GetTargetPosition();

        SnapTargetDisplayTo(targetEndpoint);

        m_targetAnimator.AnimateToEndpoint(startPosition, targetEndpoint, connectionTime);

        if (!AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusConnect();
        }
    }

    bool ConnectionComponent::ContainsEndpoint(const Endpoint& endpoint) const
    {
        bool containsEndpoint = false;

        if (m_sourceEndpoint == endpoint)
        {
            containsEndpoint = (m_dragContext != DragContext::MoveSource);
        }
        else if (m_targetEndpoint == endpoint)
        {
            containsEndpoint = (m_dragContext != DragContext::MoveTarget);
        }

        return containsEndpoint;
    }

    void ConnectionComponent::ChainProposalCreation(const QPointF& scenePos, const QPoint& screenPos)
    {
        UpdateMovePosition(scenePos);

        const bool chainAddition = true;
        FinalizeMove(scenePos, screenPos, chainAddition);
    }

    void ConnectionComponent::OnEscape()
    {
        StopMove();

        bool keepConnection = OnConnectionMoveCancelled();

        if (!keepConnection)
        {
            AZStd::unordered_set<AZ::EntityId> deletion;
            deletion.insert(GetEntityId());

            SceneRequestBus::Event(m_graphId, &SceneRequests::Delete, deletion);
        }
    }

    void ConnectionComponent::SetScene(const GraphId& graphId)
    {
        m_graphId = graphId;

        if (!m_sourceEndpoint.IsValid())
        {
            StartSourceMove();
        }
        else if (!m_targetEndpoint.IsValid())
        {
            StartTargetMove();
        }
        else if (m_dragContext == DragContext::TryConnection)
        {
            OnConnectionMoveComplete(QPointF(), QPoint());
        }

        SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnSceneSet, m_graphId);
    }

    void ConnectionComponent::ClearScene(const AZ::EntityId& /*oldSceneId*/)
    {
        AZ_Warning("Graph Canvas", m_graphId.IsValid(), "This connection (ID: %s) is not in a scene (ID: %s)!", GetEntityId().ToString().data(), m_graphId.ToString().data());
        AZ_Warning("Graph Canvas", GetEntityId().IsValid(), "This connection (ID: %s) doesn't have an Entity!", GetEntityId().ToString().data());

        if (!m_graphId.IsValid() || !GetEntityId().IsValid())
        {
            return;
        }

        SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnRemovedFromScene, m_graphId);
        m_graphId.SetInvalid();
    }

    void ConnectionComponent::SignalMemberSetupComplete()
    {
        SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnMemberSetupComplete);
    }

    AZ::EntityId ConnectionComponent::GetScene() const
    {
        return m_graphId;
    }

    bool ConnectionComponent::LockForExternalMovement(const AZ::EntityId& sceneMemberId)
    {
        if (!m_lockingSceneMember.IsValid())
        {
            m_lockingSceneMember = sceneMemberId;
        }
        
        return m_lockingSceneMember == sceneMemberId;
    }

    void ConnectionComponent::UnlockForExternalMovement(const AZ::EntityId& sceneMemberId)
    {
        if (m_lockingSceneMember == sceneMemberId)
        {
            m_lockingSceneMember.SetInvalid();
        }
    }

    AZStd::string ConnectionComponent::GetTooltip() const
    {
        return m_tooltip;
    }

    void ConnectionComponent::SetTooltip(const AZStd::string& tooltip)
    {
        m_tooltip = tooltip;
    }

    AZStd::any* ConnectionComponent::GetUserData()
    {
        return &m_userData;
    }

    void ConnectionComponent::OnTick(float deltaTime, AZ::ScriptTimePoint)
    {
        bool sourceAnimating = m_sourceAnimator.IsAnimating() && m_sourceAnimator.Tick(deltaTime);
        bool targetAnimating = m_targetAnimator.IsAnimating() && m_targetAnimator.Tick(deltaTime);

        ConnectionUIRequestBus::Event(GetEntityId(), &ConnectionUIRequests::UpdateConnectionPath);

        if (!sourceAnimating && !targetAnimating)
        {
            AZ::TickBus::Handler::BusDisconnect();
        }
    }

    void ConnectionComponent::FinalizeMove()
    {
        DragContext dragContext = m_dragContext;
        m_dragContext = DragContext::Connected;

        if (dragContext == DragContext::MoveSource)
        {
            ConnectionNotificationBus::Event(GetEntityId(), &ConnectionNotifications::OnSourceSlotIdChanged, m_previousEndPoint.GetSlotId(), m_sourceEndpoint.GetSlotId());
            SlotRequestBus::Event(GetSourceEndpoint().GetSlotId(), &SlotRequests::AddConnectionId, GetEntityId(), GetTargetEndpoint());
        }
        else
        {
            ConnectionNotificationBus::Event(GetEntityId(), &ConnectionNotifications::OnTargetSlotIdChanged, m_previousEndPoint.GetSlotId(), m_targetEndpoint.GetSlotId());
            SlotRequestBus::Event(GetTargetEndpoint().GetSlotId(), &SlotRequests::AddConnectionId, GetEntityId(), GetSourceEndpoint());
        }

        ConnectionNotificationBus::Event(GetEntityId(), &ConnectionNotifications::OnMoveComplete);
    }

    void ConnectionComponent::OnConnectionMoveStart()
    {
        SceneRequestBus::Event(m_graphId, &SceneRequests::SignalConnectionDragBegin);
        ConnectionNotificationBus::Event(GetEntityId(), &ConnectionNotifications::OnMoveBegin);
        GraphModelRequestBus::Event(m_graphId, &GraphModelRequests::DisconnectConnection, GetEntityId());
    }

    bool ConnectionComponent::OnConnectionMoveCancelled()
    {
        bool keepConnection = false;

        if (m_previousEndPoint.IsValid())
        {
            if (m_dragContext == DragContext::MoveSource)
            {
                m_sourceEndpoint = m_previousEndPoint;
            }
            else
            {
                m_targetEndpoint = m_previousEndPoint;
            }

            bool acceptConnection = GraphUtils::CreateModelConnection(m_graphId, GetEntityId(), m_sourceEndpoint, m_targetEndpoint);

            AZ_Error("GraphCanvas", acceptConnection, "Cancelled a move, and was unable to reconnect to the previous connection.");
            if (acceptConnection)
            {
                keepConnection = true;
                FinalizeMove();
            }
        }

        return keepConnection;
    }

    ConnectionComponent::ConnectionMoveResult ConnectionComponent::OnConnectionMoveComplete(const QPointF& scenePos, const QPoint& screenPos)
    {
        ConnectionMoveResult connectionResult = ConnectionMoveResult::DeleteConnection;

        bool acceptConnection = GraphUtils::CreateModelConnection(m_graphId, GetEntityId(), m_sourceEndpoint, m_targetEndpoint);

        if (acceptConnection)
        {
            connectionResult = ConnectionMoveResult::ConnectionMove;
        }
        else if (!acceptConnection && !m_previousEndPoint.IsValid() && m_dragContext != DragContext::TryConnection)
        {
            Endpoint knownEndpoint = m_sourceEndpoint;

            if (!knownEndpoint.IsValid())
            {
                knownEndpoint = m_targetEndpoint;
            }

            GraphCanvas::Endpoint otherEndpoint;

            SceneUIRequestBus::EventResult(otherEndpoint, m_graphId, &SceneUIRequests::CreateNodeForProposal, GetEntityId(), knownEndpoint, scenePos, screenPos);

            if (otherEndpoint.IsValid())
            {
                if (!m_sourceEndpoint.IsValid())
                {
                    m_sourceEndpoint = otherEndpoint;
                }
                else if (!m_targetEndpoint.IsValid())
                {
                    m_targetEndpoint = otherEndpoint;
                }

                acceptConnection = GraphUtils::CreateModelConnection(m_graphId, GetEntityId(), m_sourceEndpoint, m_targetEndpoint);

                if (acceptConnection)
                {
                    connectionResult = ConnectionMoveResult::NodeCreation;
                }
            }
        }

        return connectionResult;
    }

    void ConnectionComponent::StartMove()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();

        QGraphicsItem* connectionGraphicsItem = nullptr;
        SceneMemberUIRequestBus::EventResult(connectionGraphicsItem, GetEntityId(), &SceneMemberUIRequests::GetRootGraphicsItem);

        if (connectionGraphicsItem)
        {
            connectionGraphicsItem->setSelected(false);
            connectionGraphicsItem->setOpacity(0.5f);

            m_eventFilter = aznew ConnectionEventFilter((*this));

            QGraphicsScene* graphicsScene = nullptr;
            SceneRequestBus::EventResult(graphicsScene, m_graphId, &SceneRequests::AsQGraphicsScene);

            if (graphicsScene)
            {
                graphicsScene->addItem(m_eventFilter);

                if (!graphicsScene->views().empty())
                {
                    QGraphicsView* view = graphicsScene->views().front();
                    m_mousePoint = view->mapToScene(view->mapFromGlobal(QCursor::pos()));
                }
            }

            connectionGraphicsItem->installSceneEventFilter(m_eventFilter);
            connectionGraphicsItem->grabMouse();

            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::AddSelectorState, Styling::States::Dragging);

            AZ::EntityId nodeId;
            StateController<RootGraphicsItemDisplayState>* stateController = nullptr;

            if (m_dragContext == DragContext::MoveSource)
            {
                nodeId = GetTargetEndpoint().GetNodeId();
            }
            else
            {
                nodeId = GetSourceEndpoint().GetNodeId();
            }

            RootGraphicsItemRequestBus::EventResult(stateController, nodeId, &RootGraphicsItemRequests::GetDisplayStateStateController);

            m_nodeDisplayStateStateSetter.AddStateController(stateController);
            m_nodeDisplayStateStateSetter.SetState(RootGraphicsItemDisplayState::Inspection);

            OnConnectionMoveStart();
        }
    }

    void ConnectionComponent::StopMove()
    {
        QGraphicsItem* connectionGraphicsItem = nullptr;
        SceneMemberUIRequestBus::EventResult(connectionGraphicsItem, GetEntityId(), &SceneMemberUIRequests::GetRootGraphicsItem);

        if (connectionGraphicsItem)
        {
            connectionGraphicsItem->removeSceneEventFilter(m_eventFilter);
            delete m_eventFilter;

            connectionGraphicsItem->setOpacity(1.0f);

            connectionGraphicsItem->ungrabMouse();
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::RemoveSelectorState, Styling::States::Dragging);
        }

        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();

        if (m_dragContext == DragContext::MoveSource)
        {
            SlotRequestBus::Event(GetSourceEndpoint().GetSlotId(), &SlotRequests::RemoveProposedConnection, GetEntityId(), GetTargetEndpoint());
            StyledEntityRequestBus::Event(GetSourceEndpoint().GetSlotId(), &StyledEntityRequests::RemoveSelectorState, Styling::States::ValidDrop);
        }
        else
        {
            SlotRequestBus::Event(GetTargetEndpoint().GetSlotId(), &SlotRequests::RemoveProposedConnection, GetEntityId(), GetSourceEndpoint());
            StyledEntityRequestBus::Event(GetTargetEndpoint().GetSlotId(), &StyledEntityRequests::RemoveSelectorState, Styling::States::ValidDrop);
        }

        m_nodeDisplayStateStateSetter.ResetStateSetter();
        SceneRequestBus::Event(m_graphId, &SceneRequests::SignalConnectionDragEnd);
    }

    bool ConnectionComponent::UpdateProposal(Endpoint& activePoint, const Endpoint& proposalPoint, AZStd::function< void(const AZ::EntityId&, const AZ::EntityId&)> endpointChangedFunctor)
    {
        bool retVal = false;

        if (activePoint != proposalPoint)
        {
            retVal = true;

            QGraphicsItem* connectionGraphicsItem = nullptr;
            SceneMemberUIRequestBus::EventResult(connectionGraphicsItem, GetEntityId(), &SceneMemberUIRequests::GetRootGraphicsItem);

            if (activePoint.IsValid())
            {
                StateController<RootGraphicsItemDisplayState>* stateController = nullptr;
                RootGraphicsItemRequestBus::EventResult(stateController, activePoint.GetNodeId(), &RootGraphicsItemRequests::GetDisplayStateStateController);

                m_nodeDisplayStateStateSetter.RemoveStateController(stateController);

                StyledEntityRequestBus::Event(activePoint.m_slotId, &StyledEntityRequests::RemoveSelectorState, Styling::States::ValidDrop);
                if (connectionGraphicsItem)
                {
                    connectionGraphicsItem->setOpacity(0.5f);
                }
            }

            AZ::EntityId oldId = activePoint.GetSlotId();

            activePoint = proposalPoint;
            endpointChangedFunctor(oldId, activePoint.GetSlotId());

            if (activePoint.IsValid())
            {
                StateController<RootGraphicsItemDisplayState>* stateController = nullptr;
                RootGraphicsItemRequestBus::EventResult(stateController, activePoint.GetNodeId(), &RootGraphicsItemRequests::GetDisplayStateStateController);

                m_nodeDisplayStateStateSetter.AddStateController(stateController);

                StyledEntityRequestBus::Event(activePoint.m_slotId, &StyledEntityRequests::AddSelectorState, Styling::States::ValidDrop);
                if (connectionGraphicsItem)
                {
                    connectionGraphicsItem->setOpacity(1.0f);
                }
            }
        }

        return retVal || !activePoint.IsValid();
    }

    Endpoint ConnectionComponent::FindConnectionCandidateAt(const QPointF& scenePos) const
    {
        Endpoint knownEndpoint = m_targetEndpoint;

        if (m_dragContext == DragContext::MoveTarget)
        {
            knownEndpoint = m_sourceEndpoint;
        }

        EditorId editorId;
        SceneRequestBus::EventResult(editorId, m_graphId, &SceneRequests::GetEditorId);

        Endpoint retVal;

        double snapDist = 10.0;
        AssetEditorSettingsRequestBus::EventResult(snapDist, editorId, &AssetEditorSettingsRequests::GetSnapDistance);

        QPointF dist(snapDist, snapDist);
        QRectF rect(scenePos - dist, scenePos + dist);

        // These are returnned sorted. So we just need to return the first match we find.
        AZStd::vector<Endpoint> endpoints;
        SceneRequestBus::EventResult(endpoints, m_graphId, &SceneRequests::GetEndpointsInRect, rect);

        for (Endpoint endpoint : endpoints)
        {
            bool canCreateConnection = false;

            if (m_dragContext == DragContext::MoveSource && endpoint == m_sourceEndpoint)
            {
                canCreateConnection = true;
            }            
            else if (m_dragContext == DragContext::MoveTarget && endpoint == m_targetEndpoint)
            {
                canCreateConnection = true;
            }
            else if (m_dragContext == DragContext::MoveTarget && endpoint == m_sourceEndpoint
                     || m_dragContext == DragContext::MoveSource && endpoint == m_targetEndpoint)
            {
                continue;
            }
            else
            {
                SlotRequestBus::EventResult(canCreateConnection, endpoint.GetSlotId(), &SlotRequests::CanCreateConnectionTo, knownEndpoint);
            }

            if (canCreateConnection)
            {
                retVal = endpoint;
                break;
            }
        }

        return retVal;
    }

    void ConnectionComponent::UpdateMovePosition(const QPointF& position)
    {
        if (m_dragContext == DragContext::MoveSource
            || m_dragContext == DragContext::MoveTarget)
        {
            m_mousePoint = position;

            Endpoint newProposal = FindConnectionCandidateAt(m_mousePoint);

            bool updateConnection = false;
            if (m_dragContext == DragContext::MoveSource)
            {
                AZStd::function<void(const AZ::EntityId&, const AZ::EntityId)> updateFunction = [this](const AZ::EntityId& oldId, const AZ::EntityId& newId)
                {
                    SlotRequestBus::Event(oldId, &SlotRequests::RemoveProposedConnection, this->GetEntityId(), this->GetTargetEndpoint());
                    SlotRequestBus::Event(newId, &SlotRequests::DisplayProposedConnection, this->GetEntityId(), this->GetTargetEndpoint());
                    ConnectionNotificationBus::Event(this->GetEntityId(), &ConnectionNotifications::OnSourceSlotIdChanged, oldId, newId);
                };

                updateConnection = UpdateProposal(m_sourceEndpoint, newProposal, updateFunction);
            }
            else
            {
                AZStd::function<void(const AZ::EntityId&, const AZ::EntityId)> updateFunction = [this](const AZ::EntityId& oldId, const AZ::EntityId& newId)
                {
                    SlotRequestBus::Event(oldId, &SlotRequests::RemoveProposedConnection, this->GetEntityId(), this->GetSourceEndpoint());
                    SlotRequestBus::Event(newId, &SlotRequests::DisplayProposedConnection, this->GetEntityId(), this->GetSourceEndpoint());
                    ConnectionNotificationBus::Event(this->GetEntityId(), &ConnectionNotifications::OnTargetSlotIdChanged, oldId, newId);
                };
                updateConnection = UpdateProposal(m_targetEndpoint, newProposal, updateFunction);
            }

            if (updateConnection)
            {
                ConnectionUIRequestBus::Event(GetEntityId(), &ConnectionUIRequests::UpdateConnectionPath);
            }
        }
    }

    void ConnectionComponent::FinalizeMove(const QPointF& scenePos, const QPoint& screenPos, bool chainAddition)
    {
        if (m_dragContext == DragContext::Connected)
        {
            AZ_Error("Graph Canvas", false, "Receiving MouseRelease event in invalid drag state.");
            return;
        }

        DragContext chainContext = m_dragContext;

        StopMove();

        // Have to copy the GraphId here because deletion of the Entity this component is attached to deletes this component.
        GraphId graphId = m_graphId;

        ConnectionMoveResult connectionResult = OnConnectionMoveComplete(scenePos, screenPos);

        if (connectionResult == ConnectionMoveResult::DeleteConnection)
        {
            // If the previous endpoint is not valid, this implies a new connection is being created
            bool preventUndoState = !m_previousEndPoint.IsValid();
            if (preventUndoState)
            {
                GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestPushPreventUndoStateUpdate);
            }

            AZStd::unordered_set<AZ::EntityId> deletion;
            deletion.insert(GetEntityId());

            // The SceneRequests::Delete will delete the Entity this component is attached.
            // Therefore it is invalid to access the members of this component after the call.
            SceneRequestBus::Event(graphId, &SceneRequests::Delete, deletion);
            if (preventUndoState)
            {
                GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestPopPreventUndoStateUpdate);
            }
        }
        else
        {
            FinalizeMove();
            GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestUndoPoint);

            if (chainAddition && connectionResult == ConnectionMoveResult::NodeCreation)
            {
                AZ::EntityId graphId;
                SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &SceneMemberRequests::GetScene);

                AZ::EntityId nodeId;
                SlotType slotType = SlotTypes::Invalid;
                ConnectionType connectionType = CT_Invalid;

                if (chainContext == DragContext::MoveSource)
                {
                    nodeId = GetSourceNodeId();
                    slotType = SlotTypes::ExecutionSlot;
                    connectionType = CT_Input;
                }
                else if (chainContext == DragContext::MoveTarget)
                {
                    nodeId = GetTargetNodeId();
                    slotType = SlotTypes::ExecutionSlot;
                    connectionType = CT_Output;
                }

                SceneRequestBus::Event(graphId, &SceneRequests::HandleProposalDaisyChain, nodeId, slotType, connectionType, screenPos, scenePos);
            }
        }
    }
}