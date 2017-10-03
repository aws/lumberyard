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

#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>

#include "EntityRefNodeDescriptorComponent.h"

#include "ScriptCanvas/Core/Datum.h"
#include "ScriptCanvas/Core/PureData.h"
#include "ScriptCanvas/Core/Slot.h"

namespace ScriptCanvasEditor
{
    /////////////////////////////////////
    // EntityRefNodeDescriptorComponent
    /////////////////////////////////////

    void EntityRefNodeDescriptorComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);
        if (serializeContext)
        {
            serializeContext->Class<EntityRefNodeDescriptorComponent, NodeDescriptorComponent>()
                ->Version(1)
            ;
        }
    }

    EntityRefNodeDescriptorComponent::EntityRefNodeDescriptorComponent()
        : NodeDescriptorComponent(NodeDescriptorType::EntityRef)
    {
    }
    
    void EntityRefNodeDescriptorComponent::Activate()
    {
        NodeDescriptorComponent::Activate();

        GraphCanvas::NodeNotificationBus::Handler::BusConnect(GetEntityId());
    }
    
    void EntityRefNodeDescriptorComponent::Deactivate()
    {
        NodeDescriptorComponent::Deactivate();

        GraphCanvas::NodeNotificationBus::Handler::BusDisconnect();
        ScriptCanvas::NodeNotificationsBus::Handler::BusDisconnect();
    }

    void EntityRefNodeDescriptorComponent::OnEntityNameChanged(const AZStd::string& name)
    {
        UpdateNodeTitle();
    }
    
    void EntityRefNodeDescriptorComponent::OnAddedToScene(const AZ::EntityId& sceneId)
    {
        PopulateIds();
        ScriptCanvas::NodeNotificationsBus::Handler::BusConnect(m_endpoint.GetNodeId());
        UpdateNodeTitle();
    }

    void EntityRefNodeDescriptorComponent::OnInputChanged(const ScriptCanvas::SlotId& slotId)    
    {
        if (slotId == m_endpoint.GetSlotId())
        {
            AZ::EntityBus::Handler::BusDisconnect();

            AZ::EntityId referencedId = GetReferencedEntityId();

            if (referencedId.IsValid())
            {
                AZ::EntityBus::Handler::BusConnect(referencedId);
            }

            UpdateNodeTitle();
        }
    }

    AZ::EntityId EntityRefNodeDescriptorComponent::GetReferencedEntityId() const
    {
        AZ::EntityId retVal;

        ScriptCanvas::Datum* object = nullptr;
        ScriptCanvas::EditorNodeRequestBus::EventResult(object, m_endpoint.GetNodeId(), &ScriptCanvas::EditorNodeRequests::ModInput, m_endpoint.GetSlotId());

        if (object && object->IS_A<AZ::EntityId>())
        {
            retVal = (*object->GetAs<AZ::EntityId>());
        }

        return retVal;
    }
    
    void EntityRefNodeDescriptorComponent::PopulateIds()
    {
        AZStd::any* userData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(userData, GetEntityId(), &GraphCanvas::NodeRequests::GetUserData);

        if (userData && userData->is<AZ::EntityId>())
        {            
            AZ::EntityId scriptCanvasNodeId = (*AZStd::any_cast<AZ::EntityId>(userData));
            ScriptCanvas::SlotId scriptCanvasSlotId;
            ScriptCanvas::NodeRequestBus::EventResult(scriptCanvasSlotId, scriptCanvasNodeId, &ScriptCanvas::NodeRequests::GetSlotId, ScriptCanvas::PureData::k_setThis);

            m_endpoint = ScriptCanvas::Endpoint(scriptCanvasNodeId, scriptCanvasSlotId);

            if (m_endpoint.IsValid())
            {
                AZ::EntityId referencedId = GetReferencedEntityId();

                if (referencedId.IsValid())
                {
                    AZ::EntityBus::Handler::BusConnect(referencedId);
                }
            }
        }
    }
    
    void EntityRefNodeDescriptorComponent::UpdateNodeTitle()
    {
        bool needsTitle = true;
        
        if (m_endpoint.IsValid())
        {
            AZ::EntityId referencedId = GetReferencedEntityId();

            AZStd::string entityName;
            AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationRequests::GetEntityName, referencedId);
                    
            if (!entityName.empty())
            {
                needsTitle = false;
                GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetTitle, entityName.c_str());
            }
            else if (referencedId.IsValid())
            {
                needsTitle = false;
                GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetTitle, referencedId.ToString().c_str());
            }
        }
        
        if (needsTitle)
        {
            GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetTitle, "<None>");
        }

        GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetSubTitle, "EntityRef");
    }
}