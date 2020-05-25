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

#include <GraphCanvas/Components/Nodes/NodeBus.h>

#include <ScriptCanvas/Core/Slot.h>

#include <Editor/GraphCanvas/Components/DynamicOrderingDynamicSlotComponent.h>

#include <Editor/Include/ScriptCanvas/GraphCanvas/MappingBus.h>

namespace ScriptCanvasEditor
{    
    ////////////////////////////////////////
    // DynamicOrderingDynamicSlotComponent
    ////////////////////////////////////////

    void DynamicOrderingDynamicSlotComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<DynamicOrderingDynamicSlotComponent, DynamicSlotComponent>()
                ->Version(0)
                ;
        }
    }

    DynamicOrderingDynamicSlotComponent::DynamicOrderingDynamicSlotComponent()
        : DynamicSlotComponent()
        , m_deserialized(false)
    {
    }

    DynamicOrderingDynamicSlotComponent::DynamicOrderingDynamicSlotComponent(GraphCanvas::SlotGroup slotGroup)
        : DynamicSlotComponent(slotGroup)
        , m_deserialized(false)
    {
    }

    void DynamicOrderingDynamicSlotComponent::OnSystemTick()
    {
        if (m_nodeId.IsValid())
        {
            SlotMappingRequests* requestInterface = SlotMappingRequestBus::FindFirstHandler(GetEntityId());

            if (requestInterface)
            {
                AZStd::vector< const ScriptCanvas::Slot* > scriptSlots;
                ScriptCanvas::NodeRequestBus::EventResult(scriptSlots, m_nodeId, &ScriptCanvas::NodeRequests::GetAllSlots);

                for (int i = 0; i < scriptSlots.size(); ++i)
                {
                    // Higher means it goes first. So we need to inverse our number.
                    int priority = static_cast<int>(scriptSlots.size()) - i;

                    ScriptCanvas::SlotId scriptCanvasSlotId = scriptSlots[i]->GetId();
                    GraphCanvas::SlotId graphCanvasSlotId = requestInterface->MapToGraphCanvasId(scriptCanvasSlotId);

                    if (graphCanvasSlotId.IsValid())
                    {
                        GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetLayoutPriority, priority);
                    }
                }
            }

            m_nodeId.SetInvalid();
        }

        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    void DynamicOrderingDynamicSlotComponent::OnSceneSet(const AZ::EntityId& sceneId)
    {
        DynamicSlotComponent::OnSceneSet(sceneId);

        if (m_deserialized)
        {
            AZStd::any* userData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(userData, GetEntityId(), &GraphCanvas::NodeRequests::GetUserData);

            if (userData && userData->is<AZ::EntityId>())
            {
                m_nodeId = (userData && userData->is<AZ::EntityId>()) ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();
            }
            
            AZ::SystemTickBus::Handler::BusConnect();
        }
    }

    void DynamicOrderingDynamicSlotComponent::OnSceneMemberDeserialized(const AZ::EntityId& graphId, const GraphCanvas::GraphSerialization& serializationTarget)
    {
        DynamicSlotComponent::OnSceneMemberDeserialized(graphId, serializationTarget);
        m_deserialized = true;
    }

    void DynamicOrderingDynamicSlotComponent::StopQueueSlotUpdates()
    {
        DynamicSlotComponent::StopQueueSlotUpdates();

        if (m_nodeId.IsValid())
        {
            OnSystemTick();
        }
    }

    void DynamicOrderingDynamicSlotComponent::ConfigureGraphCanvasSlot(const ScriptCanvas::Slot* slot, const GraphCanvas::SlotId& graphCanvasSlotId)
    {
        if (!m_nodeId.IsValid())
        {
            m_nodeId = slot->GetNodeId();

            AZ::SystemTickBus::Handler::BusConnect();
        }
    }
}