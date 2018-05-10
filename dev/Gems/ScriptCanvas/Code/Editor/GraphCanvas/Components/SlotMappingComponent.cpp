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

#include <GraphCanvas/Components/Slots/SlotBus.h>

#include <Editor/GraphCanvas/Components/SlotMappingComponent.h>

namespace ScriptCanvasEditor
{
    /////////////////////////
    // SlotMappingComponent
    /////////////////////////
    
    void SlotMappingComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<SlotMappingComponent, AZ::Component>()
                ->Version(0)
            ;
        }
    }   

    void SlotMappingComponent::Activate()
    {
        m_slotMapping.clear();
        GraphCanvas::NodeNotificationBus::Handler::BusConnect(GetEntityId());
    }
    
    void SlotMappingComponent::Deactivate()
    {
        GraphCanvas::NodeNotificationBus::Handler::BusDisconnect();
    }
    
    void SlotMappingComponent::OnAddedToScene(const AZ::EntityId&)
    {
        AZStd::vector< AZ::EntityId > slotIds;
        GraphCanvas::NodeRequestBus::EventResult(slotIds, GetEntityId(), &GraphCanvas::NodeRequests::GetSlotIds);
        
        for (const AZ::EntityId& slotId : slotIds)
        {
            OnSlotAdded(slotId);
        }

        SlotMappingRequestBus::Handler::BusConnect(GetEntityId());
    }
    
    void SlotMappingComponent::OnSlotAdded(const AZ::EntityId& slotId)
    {
        AZStd::any* userData = nullptr;
        
        GraphCanvas::SlotRequestBus::EventResult(userData, slotId, &GraphCanvas::SlotRequests::GetUserData);
        
        if (userData && userData->is<ScriptCanvas::SlotId>())
        {
            m_slotMapping[(*AZStd::any_cast<ScriptCanvas::SlotId>(userData))] = slotId;
        }
    }
    
    void SlotMappingComponent::OnSlotRemoved(const AZ::EntityId& slotId)
    {
        AZStd::any* userData = nullptr;

        GraphCanvas::SlotRequestBus::EventResult(userData, slotId, &GraphCanvas::SlotRequests::GetUserData);

        if (userData && userData->is<ScriptCanvas::SlotId>())
        {
            m_slotMapping.erase((*AZStd::any_cast<ScriptCanvas::SlotId>(userData)));
        }
    }

    AZ::EntityId SlotMappingComponent::MapToGraphCanvasId(const ScriptCanvas::SlotId& slotId)
    {
        AZ::EntityId mappedId;

        auto mapIter = m_slotMapping.find(slotId);

        if (mapIter != m_slotMapping.end())
        {
            mappedId = mapIter->second;
        }

        return mappedId;
    }
}