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

#include <QCoreApplication>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

#include <Components/Nodes/NodeComponent.h>

#include <Components/GeometryComponent.h>
#include <GraphCanvas/Components/Nodes/NodeUIBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>

namespace GraphCanvas
{
    //////////////////
    // NodeComponent
    //////////////////

    void NodeComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<NodeComponent, GraphCanvasPropertyComponent>()
            ->Version(3)
            ->Field("Configuration", &NodeComponent::m_configuration)
            ->Field("Slots", &NodeComponent::m_slots)
            ->Field("UserData", &NodeComponent::m_userData)
            ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<NodeComponent>("Node", "The node's UI representation")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "Node's class attributes")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ->DataElement(AZ::Edit::UIHandlers::Default, &NodeComponent::m_configuration, "Configuration", "This node's properties")
            ;
    }

    AZ::Entity* NodeComponent::CreateCoreNodeEntity(const NodeConfiguration& config)
    {
        // Create this Node's entity.
        AZ::Entity* entity = aznew AZ::Entity();

        entity->CreateComponent<NodeComponent>(config);
        entity->CreateComponent<GeometryComponent>();

        return entity;
    }

    NodeComponent::NodeComponent()
    {
    }

    NodeComponent::NodeComponent(const NodeConfiguration& config)
        : m_configuration(config)
    {
    }

    NodeComponent::~NodeComponent()
    {
        for (auto& slotRef : m_slots)
        {
            delete slotRef;
        }
    }
	
	void NodeComponent::Init()
    {
        GraphCanvasPropertyComponent::Init();

        for (auto entityRef : m_slots)
        {
            AZ::Entity* slotEntity = entityRef;
            if (slotEntity)
            {
                if (slotEntity->GetState() == AZ::Entity::ES_CONSTRUCTED)
                {
                    slotEntity->Init();
                }
            }
        }
    }

    void NodeComponent::Activate()
    {
        AZ::EntityBus::Handler::BusConnect(GetEntityId());
    }

    void NodeComponent::Deactivate()
    {
        GraphCanvasPropertyComponent::Deactivate();

        SceneMemberRequestBus::Handler::BusDisconnect();
        NodeRequestBus::Handler::BusDisconnect();

        for (AZ::Entity* slotEntity : m_slots)
        {
            if (slotEntity)
            {
                if (slotEntity && slotEntity->GetState() == AZ::Entity::ES_ACTIVE)
                {
                    slotEntity->Deactivate();
                }
            }
        }
    }

    void NodeComponent::OnEntityActivated(const AZ::EntityId&)
    {
        AZ::EntityBus::Handler::BusDisconnect();

        // Removing the Node properties from the side panel until we decide what we want to show.
        //GraphCanvasPropertyComponent::Activate();

        NodeRequestBus::Handler::BusConnect(GetEntityId());
        SceneMemberRequestBus::Handler::BusConnect(GetEntityId());

        for (AZ::Entity* slotEntity : m_slots)
        {
            if (slotEntity)
            {
                if (slotEntity && slotEntity->GetState() == AZ::Entity::ES_INIT)
                {
                    slotEntity->Activate();
                    SlotRequestBus::Event(slotEntity->GetId(), &SlotRequests::SetNode, GetEntityId());
                    StyleNotificationBus::Event(slotEntity->GetId(), &StyleNotifications::OnStyleChanged);
                }
            }
        }

        NodeNotificationBus::Event(GetEntityId(), &NodeNotifications::OnNodeActivated);
    }

    void NodeComponent::SetScene(const AZ::EntityId& sceneId)
    {
        if (SceneNotificationBus::Handler::BusIsConnected())
        {
            NodeNotificationBus::Event(GetEntityId(), &NodeNotifications::OnRemovedFromScene, m_sceneId);
            SceneNotificationBus::Handler::BusDisconnect(m_sceneId);
        }

        m_sceneId = sceneId;

        if (m_sceneId.IsValid())
        {
            SceneNotificationBus::Handler::BusConnect(m_sceneId);
            SceneMemberNotificationBus::Handler::BusConnect(m_sceneId);

            SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnSceneSet, m_sceneId);

            OnStylesChanged();

            AZ::EntityId grid;
            SceneRequestBus::EventResult(grid, m_sceneId, &SceneRequests::GetGrid);

            NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::SetGrid, grid);
            NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::SetSnapToGrid, true);
            NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::SetResizeToGrid, true);

            NodeNotificationBus::Event(GetEntityId(), &NodeNotifications::OnAddedToScene, m_sceneId);
        }
    }

    void NodeComponent::ClearScene(const AZ::EntityId& oldSceneId)
    {
        SceneNotificationBus::Handler::BusDisconnect(oldSceneId);

        AZ_Assert(m_sceneId.IsValid(), "This node (ID: %p) is not in a scene (ID: %p)!", GetEntityId(), m_sceneId);
        AZ_Assert(GetEntityId().IsValid(), "This node (ID: %p) doesn't have an Entity!", GetEntityId());

        if (!m_sceneId.IsValid() || !GetEntityId().IsValid())
        {
            return;
        }
        
        SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnSceneCleared, m_sceneId);
        m_sceneId.SetInvalid();
    }

    void NodeComponent::SignalMemberSetupComplete()
    {
        SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnMemberSetupComplete);
    }

    AZ::EntityId NodeComponent::GetScene() const
    {
        return m_sceneId;
    }

    bool NodeComponent::LockForExternalMovement(const AZ::EntityId& sceneMemberId)
    {
        if (!m_lockingSceneMember.IsValid())
        {
            m_lockingSceneMember = sceneMemberId;
        }

        return m_lockingSceneMember == sceneMemberId;
    }

    void NodeComponent::UnlockForExternalMovement(const AZ::EntityId& sceneMemberId)
    {
        if (m_lockingSceneMember == sceneMemberId)
        {
            m_lockingSceneMember.SetInvalid();
        }
    }
        
    void NodeComponent::OnSceneReady()
    {
        SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnSceneReady);
    }

    void NodeComponent::OnStylesChanged()
    {
        for (auto& slotRef : m_slots)
        {
            StyleNotificationBus::Event(slotRef->GetId(), &StyleNotifications::OnStyleChanged);
        }
    }

    void NodeComponent::SetTooltip(const AZStd::string& tooltip)
    {
        m_configuration.SetTooltip(tooltip);
        NodeNotificationBus::Event(GetEntityId(), &NodeNotifications::OnTooltipChanged, m_configuration.GetTooltip());
    }

    void NodeComponent::SetTranslationKeyedTooltip(const TranslationKeyedString& tooltip)
    {
        m_configuration.SetTooltip(tooltip.GetDisplayString());
        NodeNotificationBus::Event(GetEntityId(), &NodeNotifications::OnTooltipChanged, m_configuration.GetTooltip());
    }

    void NodeComponent::AddSlot(const AZ::EntityId& slotId)
    {
        AZ_Assert(slotId.IsValid(), "Slot entity (ID: %s) is not valid!", slotId.ToString().data());

        if (SlotRequestBus::FindFirstHandler(slotId))
        {
            auto slotEntity = AzToolsFramework::GetEntityById(slotId);
            auto entry = AZStd::find_if(m_slots.begin(), m_slots.end(), [slotId](AZ::Entity* slot) { return slot->GetId() == slotId; });
            if (entry == m_slots.end() && slotEntity)
            {
                m_slots.emplace_back(slotEntity);
                SlotRequestBus::Event(slotId, &SlotRequests::SetNode, GetEntityId());
                NodeNotificationBus::Event(GetEntityId(), &NodeNotifications::OnSlotAdded, slotId);
            }
        }
        else
        {
            AZ_Assert(false, "Entity (ID: %s) does not implement SlotRequestBus", slotId.ToString().data());
        }
    }

    void NodeComponent::RemoveSlot(const AZ::EntityId& slotId)
    {
        AZ_Assert(slotId.IsValid(), "Slot (ID: %s) is not valid!", slotId.ToString().data());

        auto entry = AZStd::find_if(m_slots.begin(), m_slots.end(), [slotId](AZ::Entity* slot) { return slot->GetId() == slotId; });
        AZ_Assert(entry != m_slots.end(), "Slot (ID: %s) is unknown", slotId.ToString().data());

        if (entry != m_slots.end())
        {
            m_slots.erase(entry);

            NodeNotificationBus::Event(GetEntityId(), &NodeNotifications::OnSlotRemoved, slotId);
            SlotRequestBus::Event(slotId, &SlotRequests::ClearConnections);
            SlotRequestBus::Event(slotId, &SlotRequests::SetNode, AZ::EntityId());

            QGraphicsItem* item = nullptr;
            VisualRequestBus::EventResult(item, slotId, &VisualRequests::AsGraphicsItem);

            delete item;

            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, slotId);
        }
    }

    AZStd::vector<AZ::EntityId> NodeComponent::GetSlotIds() const
    {
        AZStd::vector<AZ::EntityId> result;
        for (auto slot : m_slots)
        {
            result.push_back(slot->GetId());
        }
        return result;
    }

    AZStd::any* NodeComponent::GetUserData() 
    {
        return &m_userData;
    }

    bool NodeComponent::IsWrapped() const
    {
        return m_wrappingNode.IsValid();
    }

    void NodeComponent::SetWrappingNode(const AZ::EntityId& wrappingNode)
    {
        if (!wrappingNode.IsValid())
        {
            AZ::EntityId wrappedNodeCache = m_wrappingNode;

            m_wrappingNode = wrappingNode;

            if (wrappedNodeCache.IsValid())
            {
                NodeNotificationBus::Event(GetEntityId(), &NodeNotifications::OnNodeUnwrapped, wrappedNodeCache);
            }
        }
        else if (!m_wrappingNode.IsValid())
        {
            m_wrappingNode = wrappingNode;

            NodeNotificationBus::Event(GetEntityId(), &NodeNotifications::OnNodeWrapped, wrappingNode);
        }
        else
        {
            AZ_Warning("Graph Canvas", false, "The same node is trying to be wrapped by two objects at once.");
        }
    }

    AZ::EntityId NodeComponent::GetWrappingNode() const
    {
        return m_wrappingNode;
    }
}