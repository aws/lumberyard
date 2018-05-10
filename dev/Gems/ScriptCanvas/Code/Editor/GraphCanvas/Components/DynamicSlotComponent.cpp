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

#include <AzCore/Serialization/SerializeContext.h>

#include <GraphCanvas/Components/Nodes/NodeBus.h>

#include <Editor/GraphCanvas/Components/DynamicSlotComponent.h>
#include <Editor/Nodes/NodeUtils.h>

namespace ScriptCanvasEditor
{
    /////////////////////////
    // DynamicSlotComponent
    /////////////////////////

    void DynamicSlotComponent::Reflect(AZ::ReflectContext* reflectionContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectionContext);

        if (serializeContext)
        {
            serializeContext->Class<DynamicSlotComponent, AZ::Component>()
                ->Version(1)
                ->Field("SlotGroup", &DynamicSlotComponent::m_slotGroup)
            ;
        }
    }

    DynamicSlotComponent::DynamicSlotComponent()
        : m_slotGroup(GraphCanvas::SlotGroups::Invalid)
    {
    }

    DynamicSlotComponent::DynamicSlotComponent(GraphCanvas::SlotGroup slotGroup)
        : m_slotGroup(slotGroup)
    {
    }

    void DynamicSlotComponent::Init()
    {
    }

    void DynamicSlotComponent::Activate()
    {
        GraphCanvas::SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void DynamicSlotComponent::Deactivate()
    {
        GraphCanvas::SceneMemberNotificationBus::Handler::BusDisconnect();
    }

    void DynamicSlotComponent::OnSceneSet(const AZ::EntityId&)
    {
        AZStd::any* userData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(userData, GetEntityId(), &GraphCanvas::NodeRequests::GetUserData);

        if (userData)
        {
            AZ::EntityId* entityId = AZStd::any_cast<AZ::EntityId>(userData);

            if (entityId)
            {
                ScriptCanvas::NodeNotificationsBus::Handler::BusConnect((*entityId));
            }
        }

        GraphCanvas::SceneMemberNotificationBus::Handler::BusDisconnect();
    }

    void DynamicSlotComponent::OnSlotAdded(const ScriptCanvas::SlotId& slotId)
    {
        const AZ::EntityId* scriptCanvasNodeId = ScriptCanvas::NodeNotificationsBus::GetCurrentBusId();

        const ScriptCanvas::Slot* slot = nullptr;
        ScriptCanvas::NodeRequestBus::EventResult(slot, (*scriptCanvasNodeId), &ScriptCanvas::NodeRequests::GetSlot, slotId);

        Nodes::DisplayScriptCanvasSlot(GetEntityId(), (*slot), m_slotGroup);
    }

    void DynamicSlotComponent::OnSlotRemoved(const ScriptCanvas::SlotId& slotId)
    {
        AZStd::vector< AZ::EntityId > slotIds;
        GraphCanvas::NodeRequestBus::EventResult(slotIds, GetEntityId(), &GraphCanvas::NodeRequests::GetSlotIds);

        for (AZ::EntityId entityId : slotIds)
        {
            AZStd::any* userData = nullptr;
            GraphCanvas::SlotRequestBus::EventResult(userData, entityId, &GraphCanvas::SlotRequests::GetUserData);

            if (userData)
            {
                ScriptCanvas::SlotId* testId = AZStd::any_cast<ScriptCanvas::SlotId>(userData);

                if (testId && (*testId) == slotId)
                {
                    GraphCanvas::NodeRequestBus::Event(GetEntityId(), &GraphCanvas::NodeRequests::RemoveSlot, entityId);
                }
            }
        }
    }
}