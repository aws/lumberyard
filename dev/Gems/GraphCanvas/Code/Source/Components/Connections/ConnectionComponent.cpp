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

#include <Components/Connections/ConnectionVisualComponent.h>
#include <Components/StylingComponent.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>

namespace GraphCanvas
{
    ////////////////////
    // ScopedUndoBlock
    ////////////////////

    class ScopedUndoBlock
    {
    public:
        ScopedUndoBlock(const AZ::EntityId& sceneId)
            : m_sceneId(sceneId)
        {
            SceneUIRequestBus::Event(m_sceneId, &SceneUIRequests::RequestPushPreventUndoStateUpdate);
        }

        ~ScopedUndoBlock()
        {
            SceneUIRequestBus::Event(m_sceneId, &SceneUIRequests::RequestPopPreventUndoStateUpdate);
        }

    private:

        AZ::EntityId m_sceneId;
    };

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

        serializeContext->Class<ConnectionComponent>()
            ->Version(2)
            ->Field("Scene", &ConnectionComponent::m_sceneId)
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

    AZ::Entity* ConnectionComponent::CreateBaseConnectionEntity(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint, const AZStd::string& selectorClass)
    {
        // Create this Connection's entity.
        AZ::Entity* entity = aznew AZ::Entity("Connection");

        entity->CreateComponent<ConnectionComponent>(sourceEndpoint, targetEndpoint);
        entity->CreateComponent<StylingComponent>(Styling::Elements::Connection, AZ::EntityId(), selectorClass);

        return entity;
    }

    AZ::Entity* ConnectionComponent::CreateGeneralConnection(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint, const AZStd::string& substyle)
    {
        AZ::Entity* entity = CreateBaseConnectionEntity(sourceEndpoint, targetEndpoint, substyle);
        entity->CreateComponent<ConnectionVisualComponent>();

        entity->Init();
        entity->Activate();

        return entity;
    }

    ConnectionComponent::ConnectionComponent()
        : m_dragContext(DragContext::Unknown)
    {
    }

    ConnectionComponent::ConnectionComponent(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint)
        : AZ::Component()
    {
        m_sourceEndpoint = sourceEndpoint;
        m_targetEndpoint = targetEndpoint;

        AZ_Warning("GraphCanvas", m_targetEndpoint.IsValid() || m_sourceEndpoint.IsValid(), "Either source or target endpoint must be valid when creating a connection.");        

        if (m_sourceEndpoint.IsValid() && m_targetEndpoint.IsValid())
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
        if (m_sourceEndpoint.IsValid())
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
        if (m_targetEndpoint.IsValid())
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
	
    void ConnectionComponent::OnEscape()
    {
        StopMove();

        bool keepConnection = OnConnectionMoveCancelled();

        if (!keepConnection)
        {
            AZStd::unordered_set<AZ::EntityId> deletion;
            deletion.insert(GetEntityId());

            SceneRequestBus::Event(m_sceneId, &SceneRequests::Delete, deletion);
        }
    }
	
    void ConnectionComponent::SetScene(const AZ::EntityId& sceneId)
    {
        m_sceneId = sceneId;

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

        SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnSceneSet, m_sceneId);
    }

    void ConnectionComponent::ClearScene(const AZ::EntityId& oldSceneId)
    {
        AZ_Warning("Graph Canvas", m_sceneId.IsValid(), "This connection (ID: %s) is not in a scene (ID: %s)!", GetEntityId().ToString().data(), m_sceneId.ToString().data());
        AZ_Warning("Graph Canvas", GetEntityId().IsValid(), "This connection (ID: %s) doesn't have an Entity!", GetEntityId().ToString().data());

        if (!m_sceneId.IsValid() || !GetEntityId().IsValid())
        {
            return;
        }

        SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnSceneCleared, m_sceneId);
        m_sceneId.SetInvalid();
    }

    AZ::EntityId ConnectionComponent::GetScene() const
    {
        return m_sceneId;
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
    }

    void ConnectionComponent::OnConnectionMoveStart()
    {
        ConnectionSceneRequestBus::Event(m_sceneId, &ConnectionSceneRequests::DisconnectConnection, GetEntityId());
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

            bool acceptConnection = false;
            ConnectionSceneRequestBus::EventResult(acceptConnection, m_sceneId, &ConnectionSceneRequests::CreateConnection, GetEntityId(), m_sourceEndpoint, m_targetEndpoint);

            AZ_Error("GraphCanvas", acceptConnection, "Cancelled a move, and was unable to reconnect to the previous connection.");
            if (acceptConnection)
            {
                keepConnection = true;
                FinalizeMove();
            }
        }

        return keepConnection;
    }

    bool ConnectionComponent::OnConnectionMoveComplete(const QPointF& scenePos, const QPoint& screenPos)
    {
        bool acceptConnection = false;
        ConnectionSceneRequestBus::EventResult(acceptConnection, m_sceneId, &ConnectionSceneRequests::CreateConnection, GetEntityId(), m_sourceEndpoint, m_targetEndpoint);

        if (!acceptConnection && !m_previousEndPoint.IsValid() && m_dragContext != DragContext::TryConnection)
        {
            Endpoint knownEndpoint = m_sourceEndpoint;

            if (!knownEndpoint.IsValid())
            {
                knownEndpoint = m_targetEndpoint;
            }

            GraphCanvas::Endpoint otherEndpoint;

            SceneUIRequestBus::EventResult(otherEndpoint, m_sceneId, &SceneUIRequests::CreateNodeForProposal, GetEntityId(), knownEndpoint, scenePos, screenPos);

            if (otherEndpoint.IsValid())
            {
                acceptConnection = true;

                if (!m_sourceEndpoint.IsValid())
                {
                    m_sourceEndpoint = otherEndpoint;
                }
                else if (!m_targetEndpoint.IsValid())
                {
                    m_targetEndpoint = otherEndpoint;
                }
            }
        }

        return acceptConnection;
    }
	
	void ConnectionComponent::StartMove()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        
        QGraphicsItem* connectionGraphicsItem = nullptr;
        RootVisualRequestBus::EventResult(connectionGraphicsItem, GetEntityId(), &RootVisualRequests::GetRootGraphicsItem);

        if (connectionGraphicsItem)
        {
            connectionGraphicsItem->setSelected(false);
            connectionGraphicsItem->setOpacity(0.5f);

            m_eventFilter = aznew ConnectionEventFilter((*this));

            QGraphicsScene* graphicsScene = nullptr;
            SceneRequestBus::EventResult(graphicsScene, m_sceneId, &SceneRequests::AsQGraphicsScene);

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

            OnConnectionMoveStart();
        }
    }

    void ConnectionComponent::StopMove()
    {
        QGraphicsItem* connectionGraphicsItem = nullptr;
        RootVisualRequestBus::EventResult(connectionGraphicsItem, GetEntityId(), &RootVisualRequests::GetRootGraphicsItem);

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
    }

    bool ConnectionComponent::UpdateProposal(Endpoint& activePoint, const Endpoint& proposalPoint, AZStd::function< void(const AZ::EntityId&, const AZ::EntityId&)> endpointChangedFunctor)
    {
        bool retVal = false;

        if (activePoint != proposalPoint)
        {
            retVal = true;

            QGraphicsItem* connectionGraphicsItem = nullptr;
            RootVisualRequestBus::EventResult(connectionGraphicsItem, GetEntityId(), &RootVisualRequests::GetRootGraphicsItem);

            if (activePoint.IsValid())
            {
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

        Endpoint retVal;

        double snapDist = 10.0;
        ConnectionSceneRequestBus::EventResult(snapDist, m_sceneId, &ConnectionSceneRequestBus::Events::GetSnapDistance);

        QPointF dist(snapDist, snapDist);
        QRectF rect(scenePos - dist, scenePos + dist);

        AZStd::vector<Endpoint> endpoints;
        SceneRequestBus::EventResult(endpoints, m_sceneId, &SceneRequestBus::Events::GetEndpointsInRect, rect);

        for (Endpoint endpoint : endpoints)
        {
            Connectability result;
            ConnectableObjectRequestBus::EventResult(result, endpoint.GetSlotId(), &ConnectableObjectRequests::CanConnectWith, knownEndpoint.GetSlotId());

            if (result.entity.IsValid() && result.status == Connectability::Connectable)
            {
                retVal.m_slotId = result.entity;
                SlotRequestBus::EventResult(retVal.m_nodeId, retVal.m_slotId, &SlotRequests::GetNode);
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
	
    void ConnectionComponent::FinalizeMove(const QPointF& scenePos, const QPoint& screenPos)
    {
        if (m_dragContext == DragContext::Connected)
        {
            AZ_Error("Graph Canvas", false, "Receiving MouseRelease event in invalid drag state.");
            return;
        }

        StopMove();

        bool acceptConnection = OnConnectionMoveComplete(scenePos, screenPos);

        if (!acceptConnection)
        {
            // If the previous endpoint is not valid, this applies a new connection is being created
            bool preventUndoState = !m_previousEndPoint.IsValid();
            if (preventUndoState)
            {
                SceneUIRequestBus::Event(m_sceneId, &SceneUIRequests::RequestPushPreventUndoStateUpdate);
            }

            AZStd::unordered_set<AZ::EntityId> deletion;
            deletion.insert(GetEntityId());

            SceneRequestBus::Event(m_sceneId, &SceneRequests::Delete, deletion);
            if (preventUndoState)
            {
                SceneUIRequestBus::Event(m_sceneId, &SceneUIRequests::RequestPopPreventUndoStateUpdate);
            }
        }
        else
        {
            FinalizeMove();
            SceneUIRequestBus::Event(m_sceneId, &SceneUIRequests::RequestUndoPoint);
        }
    }
}
