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
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>

#include "ScriptEventSenderNodeDescriptorComponent.h"

#include "Editor/Translation/TranslationHelper.h"
#include "ScriptCanvas/Libraries/Core/Method.h"

#include <Editor/Include/ScriptCanvas/Bus/EditorScriptCanvasBus.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/GraphCanvas/DynamicSlotBus.h>
#include <ScriptCanvas/Libraries/Core/ScriptEventBase.h>

namespace ScriptCanvasEditor
{
    /////////////////////////////////////////////
    // ScriptEventSenderNodeDescriptorComponent
    /////////////////////////////////////////////

    void ScriptEventSenderNodeDescriptorComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<ScriptEventSenderNodeDescriptorComponent, NodeDescriptorComponent>()
                ->Version(1)
                ;
        }
    }

    ScriptEventSenderNodeDescriptorComponent::ScriptEventSenderNodeDescriptorComponent()
        : NodeDescriptorComponent(NodeDescriptorType::EBusSender)
    {
    }

    ScriptEventSenderNodeDescriptorComponent::ScriptEventSenderNodeDescriptorComponent(const AZ::Data::AssetId& assetId, const ScriptCanvas::EBusEventId& eventId)
        : NodeDescriptorComponent(NodeDescriptorType::EBusSender)
        , m_assetId(assetId)
        , m_eventId(eventId)
    {

    }

    void ScriptEventSenderNodeDescriptorComponent::Activate()
    {
        NodeDescriptorComponent::Activate();

        AZ::Data::AssetBus::Handler::BusConnect(m_assetId);
    }

    void ScriptEventSenderNodeDescriptorComponent::Deactivate()
    {
        NodeDescriptorComponent::Deactivate();

        AZ::Data::AssetBus::Handler::BusDisconnect();
    }

    void ScriptEventSenderNodeDescriptorComponent::OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType assetType)
    {
        SignalNeedsVersionConversion();
    }

    void ScriptEventSenderNodeDescriptorComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        SignalNeedsVersionConversion();
    }

    void ScriptEventSenderNodeDescriptorComponent::OnVersionConversionBegin()
    {
        DynamicSlotRequestBus::Event(GetEntityId(), &DynamicSlotRequests::StartQueueSlotUpdates);
    }

    void ScriptEventSenderNodeDescriptorComponent::OnVersionConversionEnd()
    {
        UpdateTitles();
        DynamicSlotRequestBus::Event(GetEntityId(), &DynamicSlotRequests::StopQueueSlotUpdates);
    }

    void ScriptEventSenderNodeDescriptorComponent::OnAddedToGraphCanvasGraph(const GraphCanvas::GraphId& graphId, const AZ::EntityId& scriptCanvasNodeId)
    {
        m_scriptCanvasId = scriptCanvasNodeId;
        EditorNodeNotificationBus::Handler::BusConnect(scriptCanvasNodeId);

        ScriptCanvas::Nodes::Core::Method* method = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::Method>(m_scriptCanvasId);

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

        UpdateTitles();
    }

    void ScriptEventSenderNodeDescriptorComponent::UpdateTitles()
    {
        if (m_assetId.IsValid())
        {
            AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(m_assetId, true, nullptr, true);

            if (asset.IsReady())
            {
                const ScriptEvents::ScriptEvent& definition = asset.Get()->m_definition;

                GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetSubTitle, definition.GetName());

                for (auto eventDefinition : definition.GetMethods())
                {
                    if (eventDefinition.GetEventId() == m_eventId)
                    {
                        GraphCanvas::NodeRequestBus::Event(GetEntityId(), &GraphCanvas::NodeRequests::SetTooltip, eventDefinition.GetTooltip());
                        GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetTitle, eventDefinition.GetName());
                    }
                }
            }
        }
    }

    void ScriptEventSenderNodeDescriptorComponent::SignalNeedsVersionConversion()
    {
        AZ::EntityId graphCanvasGraphId;
        GraphCanvas::SceneMemberRequestBus::EventResult(graphCanvasGraphId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

        EditorGraphRequestBus::Event(scriptCanvasId, &EditorGraphRequests::QueueVersionUpdate, GetEntityId());
    }
}
