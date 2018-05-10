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

#include <GraphCanvas/Components/Slots/SlotBus.h>

#include "EBusSenderNodeDescriptorComponent.h"

#include "Editor/Translation/TranslationHelper.h"
#include "ScriptCanvas/Libraries/Core/Method.h"

namespace ScriptCanvasEditor
{
    //////////////////////////////////////
    // EBusSenderNodeDescriptorComponent
    //////////////////////////////////////
    
    void EBusSenderNodeDescriptorComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);
        
        if (serializeContext)
        {
            serializeContext->Class<EBusSenderNodeDescriptorComponent, NodeDescriptorComponent>()
                ->Version(2)
            ;
        }
    }
    
    EBusSenderNodeDescriptorComponent::EBusSenderNodeDescriptorComponent()
        : NodeDescriptorComponent(NodeDescriptorType::EBusSender)
    {
    }

    void EBusSenderNodeDescriptorComponent::Activate()
    {
        GraphCanvas::NodeNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void EBusSenderNodeDescriptorComponent::Deactivate()
    {
        GraphCanvas::NodeNotificationBus::Handler::BusDisconnect();
    }

    void EBusSenderNodeDescriptorComponent::OnAddedToScene(const AZ::EntityId& sceneId)
    {
        AZStd::any* userData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(userData, GetEntityId(), &GraphCanvas::NodeRequests::GetUserData);

        if (userData
            && userData->is<AZ::EntityId>())
        {
            AZ::EntityId scriptCanvasEntityId = (*AZStd::any_cast<AZ::EntityId>(userData));

            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, scriptCanvasEntityId);

            if (entity)
            {
                ScriptCanvas::Nodes::Core::Method* method = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::Method>(entity);

                if (method)
                {
                    if (method->HasBusID())
                    {
                        ScriptCanvas::SlotId slotId = method->GetBusSlotId();

                        AZStd::vector< AZ::EntityId > graphCanvasSlots;
                        GraphCanvas::NodeRequestBus::EventResult(graphCanvasSlots, GetEntityId(), &GraphCanvas::NodeRequests::GetSlotIds);

                        for (const AZ::EntityId& graphCanvasId : graphCanvasSlots)
                        {
                            AZStd::any* slotData = nullptr;
                            GraphCanvas::SlotRequestBus::EventResult(slotData, graphCanvasId, &GraphCanvas::SlotRequests::GetUserData);

                            if (slotData
                                && slotData->is< ScriptCanvas::SlotId >())
                            {
                                ScriptCanvas::SlotId currentSlotId = (*AZStd::any_cast<ScriptCanvas::SlotId>(slotData));

                                if (currentSlotId == slotId)
                                {
                                    GraphCanvas::SlotRequestBus::Event(graphCanvasId, &GraphCanvas::SlotRequests::SetTranslationKeyedName, TranslationHelper::GetEBusSenderBusIdNameKey());
                                    GraphCanvas::SlotRequestBus::Event(graphCanvasId, &GraphCanvas::SlotRequests::SetTranslationKeyedTooltip, TranslationHelper::GetEBusSenderBusIdTooltipKey());

                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
        
    }
}