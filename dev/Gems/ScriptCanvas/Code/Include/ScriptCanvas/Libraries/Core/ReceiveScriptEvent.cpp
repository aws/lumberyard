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

#include "ReceiveScriptEvent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/TickBus.h>

#include <ScriptCanvas/Execution/RuntimeBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <ScriptEvents/ScriptEventsAsset.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            const char* ReceiveScriptEvent::c_busIdName = "Source";
            const char* ReceiveScriptEvent::c_busIdTooltip = "ID used to connect on a specific Event address";

            ReceiveScriptEvent::~ReceiveScriptEvent()
            {
                if (m_ebus && m_handler && m_ebus->m_destroyHandler)
                {
                    m_ebus->m_destroyHandler->Invoke(m_handler);
                }

                m_ebus = nullptr;
                m_handler = nullptr;
            }

            void ReceiveScriptEvent::Initialize(const AZ::Data::AssetId assetId)
            {
                ScriptEventBase::Initialize(assetId);

                AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(assetId);
                if (asset.IsReady())
                {
                    CompleteInitialize(asset);
                }
            }

            void ReceiveScriptEvent::CompleteInitialize(AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset)
            {
                bool wasConfigured = IsConfigured();

                SlotIdMapping populationMapping;
                PopulateAsset(asset, populationMapping);

                // If we are configured, but we added more elements. We want to update.
                if (wasConfigured && !populationMapping.empty())
                {
                    m_eventSlotMapping.insert(populationMapping.begin(), populationMapping.end());
                }
                else if (!wasConfigured)
                {
                    m_eventSlotMapping = populationMapping;
                }                
            }

            void ReceiveScriptEvent::PopulateAsset(AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset, SlotIdMapping& populationMapping)
            {
                if (CreateHandler(asset))
                {
                    if (!CreateEbus())
                    {
                        // Asset version is likely out of date with this event - for now prompt to open and re-save, TODO: auto fix graph.
                        AZ::Data::AssetInfo assetInfo;
                        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, asset.GetId());

                        AZStd::string graphAssetName;
                        RuntimeRequestBus::EventResult(graphAssetName, GetGraphId(), &RuntimeRequests::GetAssetName);

                        AZ_Error("Script Event", false, "The Script Event asset (%s) has been modified. Open the graph (%s) and re-save it.", assetInfo.m_relativePath.c_str(), graphAssetName.c_str());
                        return;
                    }

                    const bool wasConfigured = IsConfigured();

                    if (!wasConfigured)
                    {
                        AZ::Uuid addressTypeId = m_definition.GetAddressType();
                        AZ::Uuid addressId = m_definition.GetAddressTypeProperty().GetId();

                        if (m_definition.IsAddressRequired())
                        {
                            const auto busToolTip = AZStd::string::format("%s (Type: %s)", c_busIdTooltip, m_ebus->m_idParam.m_name);
                            const AZ::TypeId& busId = m_ebus->m_idParam.m_typeId;                            

                            SlotId addressSlotId;

                            DataSlotConfiguration config;
                            auto remappingIter = m_eventSlotMapping.find(addressId);

                            if (remappingIter != m_eventSlotMapping.end())
                            {
                                config.m_slotId = remappingIter->second;
                            }

                            if (busId == azrtti_typeid<AZ::EntityId>())
                            {
                                config.m_name = c_busIdName;
                                config.m_toolTip = busToolTip;
                                config.m_slotType = SlotType::DataIn;
                                config.m_contractDescs = { { []() { return aznew RestrictedTypeContract({ Data::Type::EntityID() }); } } };                                

                                addressSlotId = AddInputDatumSlot(config, Datum(SelfReferenceId));
                            }
                            else
                            {
                                Data::Type busIdType(AZ::BehaviorContextHelper::IsStringParameter(m_ebus->m_idParam) ? Data::Type::String() : Data::FromAZType(busId));                                

                                config.m_name = c_busIdName;
                                config.m_toolTip = busToolTip;
                                config.m_slotType = SlotType::DataIn;
                                config.m_contractDescs = { { [busIdType]() { return aznew RestrictedTypeContract({ busIdType }); } } };

                                addressSlotId = AddInputDatumSlot(config, Datum(busIdType, Datum::eOriginality::Original));
                            }

                            populationMapping[addressId] = addressSlotId;
                        }
                    }

                    if (CreateEbus())
                    {
                        const AZ::BehaviorEBusHandler::EventArray& events = m_handler->GetEvents();
                        for (int eventIndex(0); eventIndex < events.size(); ++eventIndex)
                        {
                            InitializeEvent(asset, eventIndex, populationMapping);
                        }
                    }
                }
            }


            void ReceiveScriptEvent::InitializeEvent(AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset, int eventIndex, SlotIdMapping& populationMapping)
            {
                if (!m_handler)
                {
                    AZ_Error("Script Canvas", false, "BehaviorEBusHandler is nullptr. Cannot initialize event");
                    return;
                }

                const AZ::BehaviorEBusHandler::EventArray& events = m_handler->GetEvents();
                if (eventIndex >= events.size())
                {
                    AZ_Error("Script Canvas", false, "Event index %d is out of range. Total number of events: %zu", eventIndex, events.size());
                    return;
                }

                const AZ::BehaviorEBusHandler::BusForwarderEvent& event = events[eventIndex];

                if (m_version == 0)
                {
                    m_version = m_definition.GetVersion();
                }

                auto methodDefinition = AZStd::find_if(m_definition.GetMethods().begin(), m_definition.GetMethods().end(), [&event](const ScriptEvents::Method& methodDefinition)->bool { return methodDefinition.GetEventId() == event.m_eventId; });
                if (methodDefinition == m_definition.GetMethods().end())
                {
                    AZ_Assert(false, "The script event definition does not have the event for which this method was created.");
                }

                AZ::Uuid namePropertyId = methodDefinition->GetNameProperty().GetId();
                auto eventId = AZ::Crc32(namePropertyId.ToString<AZStd::string>().c_str());

                m_userData.m_handler = this;
                m_userData.m_methodDefinition = methodDefinition;

                AZ_Assert(!event.m_parameters.empty(), "No parameters in event!");
                if (!events[eventIndex].m_function)
                {
                    m_handler->InstallGenericHook(events[eventIndex].m_name, &OnEventGenericHook, &m_userData);
                }

                if (m_eventMap.find(eventId) == m_eventMap.end())
                {
                    m_eventMap[eventId] = ConfigureEbusEntry(*methodDefinition, event, populationMapping);
                }

                PopulateNodeType();
            }

            Internal::ScriptEventEntry ReceiveScriptEvent::ConfigureEbusEntry(const ScriptEvents::Method& methodDefinition, const AZ::BehaviorEBusHandler::BusForwarderEvent& event, SlotIdMapping& populationMapping)
            {
                const size_t sentinel(event.m_parameters.size());

                Internal::ScriptEventEntry eBusEventEntry;
                eBusEventEntry.m_scriptEventAssetId = m_scriptEventAssetId;
                eBusEventEntry.m_numExpectedArguments = static_cast<int>(sentinel - AZ::eBehaviorBusForwarderEventIndices::ParameterFirst);

                if (event.HasResult())
                {
                    const AZ::BehaviorParameter& argument(event.m_parameters[AZ::eBehaviorBusForwarderEventIndices::Result]);
                    Data::Type inputType(AZ::BehaviorContextHelper::IsStringParameter(argument) ? Data::Type::String() : Data::FromAZType(argument.m_typeId));
                    const AZStd::string argumentTypeName = Data::GetName(inputType);

                    AZ::Uuid resultIdentifier = methodDefinition.GetReturnTypeProperty().GetId();

                    DataSlotConfiguration slotConfiguration;

                    slotConfiguration.m_name = AZStd::string::format("Result: %s", argumentTypeName.c_str());

                    slotConfiguration.m_slotType = SlotType::DataIn;
                    slotConfiguration.m_dataType = inputType;
                    slotConfiguration.m_addUniqueSlotByNameAndType = false;

                    auto remappingIdIter = m_eventSlotMapping.find(resultIdentifier);

                    if (remappingIdIter != m_eventSlotMapping.end())
                    {
                        slotConfiguration.m_slotId = remappingIdIter->second;
                    }

                    SlotId slotId = AddInputDatumSlot(slotConfiguration, Datum(inputType, Datum::eOriginality::Copy, nullptr, AZ::Uuid::CreateNull()));

                    populationMapping[resultIdentifier] = slotId;
                    eBusEventEntry.m_resultSlotId = slotId;
                }

                for (size_t parameterIndex(AZ::eBehaviorBusForwarderEventIndices::ParameterFirst)
                    ; parameterIndex < sentinel
                    ; ++parameterIndex)
                {
                    const AZ::BehaviorParameter& parameter(event.m_parameters[parameterIndex]);
                    Data::Type outputType(AZ::BehaviorContextHelper::IsStringParameter(parameter) ? Data::Type::String() : Data::FromAZType(parameter.m_typeId));

                    // Get the name and tooltip from the script event definition
                    size_t eventParamIndex = parameterIndex - AZ::eBehaviorBusForwarderEventIndices::ParameterFirst;
                    AZStd::string argName = methodDefinition.GetParameters()[eventParamIndex].GetName();
                    AZStd::string argToolTip = methodDefinition.GetParameters()[eventParamIndex].GetTooltip();
                    AZ::Uuid argIdentifier = methodDefinition.GetParameters()[eventParamIndex].GetNameProperty().GetId();

                    if (argName.empty())
                    {
                        argName = AZStd::string::format("%s", Data::GetName(outputType).c_str());
                    }

                    DataSlotConfiguration slotConfiguration;
                    slotConfiguration.m_name = argName;
                    slotConfiguration.m_toolTip = argToolTip;
                    slotConfiguration.m_slotType = SlotType::DataOut;
                    slotConfiguration.m_addUniqueSlotByNameAndType = false;

                    auto remappingIdIter = m_eventSlotMapping.find(argIdentifier);

                    if (remappingIdIter != m_eventSlotMapping.end())
                    {
                        slotConfiguration.m_slotId = remappingIdIter->second;
                    }

                    slotConfiguration.m_dataType = AZStd::move(outputType);

                    SlotId slotId = AddDataSlot(slotConfiguration);

                    AZ_Error("ScriptCanvas", populationMapping.find(argIdentifier) == populationMapping.end(), "Trying to create the same slot twice. Unable to create sane mapping.");
                    
                    populationMapping[argIdentifier] = slotId;
                    eBusEventEntry.m_parameterSlotIds.push_back(slotId);
                }

                const AZStd::string eventID(AZStd::string::format("ExecutionSlot:%s", event.m_name));

                {
                    AZ::Uuid outputSlotId = methodDefinition.GetNameProperty().GetId();
                    SlotConfiguration slotConfiguration;

                    slotConfiguration.m_name = eventID;
                    slotConfiguration.m_slotType = SlotType::ExecutionOut;
                    slotConfiguration.m_addUniqueSlotByNameAndType = true;

                    auto remappingIter = m_eventSlotMapping.find(outputSlotId);

                    if (remappingIter != m_eventSlotMapping.end())
                    {
                        slotConfiguration.m_slotId = remappingIter->second;
                    }

                    SlotId slotId = AddSlot(slotConfiguration);

                    populationMapping[outputSlotId] = slotId;
                    eBusEventEntry.m_eventSlotId = slotId;

                    AZ_Assert(eBusEventEntry.m_eventSlotId.IsValid(), "the event execution out slot must be valid");
                }

                eBusEventEntry.m_eventName = event.m_name;

                return eBusEventEntry;
            }

            void ReceiveScriptEvent::OnEventGenericHook(void* userData, const char* eventName, int eventIndex, AZ::BehaviorValueParameter* result, int numParameters, AZ::BehaviorValueParameter* parameters)
            {
                ReceiveScriptEvent::EventHookUserData* eventHookUserData(reinterpret_cast<ReceiveScriptEvent::EventHookUserData*>(userData));
                eventHookUserData->m_handler->OnEvent(eventName, eventIndex, result, numParameters, parameters);
            }

            bool ReceiveScriptEvent::IsEventConnected(const Internal::ScriptEventEntry& entry) const
            {
                return Node::IsConnected(*GetSlot(entry.m_eventSlotId))
                    || (entry.m_resultSlotId.IsValid() && Node::IsConnected(*GetSlot(entry.m_resultSlotId)))
                    || AZStd::any_of(entry.m_parameterSlotIds.begin(), entry.m_parameterSlotIds.end(), [this](const SlotId& id) { return this->Node::IsConnected(*GetSlot(id)); });
            }

            void ReceiveScriptEvent::OnEvent(const char* eventName, const int eventIndex, AZ::BehaviorValueParameter* result, const int numParameters, AZ::BehaviorValueParameter* parameters)
            {
                AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::ScriptCanvas, "ReceiveScriptEvent::OnEvent %s", eventName);

                SCRIPTCANVAS_RETURN_IF_ERROR_STATE((*this));

                ScriptEvents::Method method;
                if (!GetScriptEvent().FindMethod(eventName, method))
                {
                    AZ_Warning("Script Events", false, "Failed to find method: %s when trying to handle it.", eventName);
                    return;
                }

                auto iter = m_eventMap.find(AZ::Crc32(method.GetNameProperty().GetId().ToString<AZStd::string>().c_str()));
                if (iter == m_eventMap.end())
                {
                    SCRIPTCANVAS_REPORT_ERROR((*this), "Invalid event index in Ebus handler");
                    return;
                }

                Internal::ScriptEventEntry& scriptEventEntry = iter->second;

                if (!IsEventConnected(scriptEventEntry))
                {
                    // this is fine, it's optional to implement the calls
                    return;
                }

                scriptEventEntry.m_resultEvaluated = !scriptEventEntry.IsExpectingResult();

                // route the parameters to the connect nodes input
                if (scriptEventEntry.m_parameterSlotIds.size() != numParameters)
                {
                    SCRIPTCANVAS_REPORT_ERROR((*this), "The Script Event, %s, has %d parameters. While the ScriptCanvas Node (%s) has (%zu) slots. Make sure the node is updated in the Script Canvas graph.", eventName, numParameters, GetNodeName().c_str(), scriptEventEntry.m_parameterSlotIds.size());
                }
                else
                {
                    for (int parameterIndex(0); parameterIndex < numParameters; ++parameterIndex)
                    {
                        const Slot* slot = GetSlot(scriptEventEntry.m_parameterSlotIds[parameterIndex]);
                        const auto& value = *(parameters + parameterIndex);
                        const Datum input(value);

                        if (slot->IsTypeMatchFor(input.GetType()))
                        {
                            PushOutput(input, *slot);
                        }
                        else
                        {
                            SCRIPTCANVAS_REPORT_ERROR((*this), "Type mismatch in Script Event %s on ScriptCanvas Node (%s). Make sure the nodes is updated in the Script Canvas graph", eventName, GetNodeName().c_str());
                        }
                    }
                }

                // now, this should pass execution off to the nodes that will push their output into this result input
                SignalOutput(scriptEventEntry.m_eventSlotId);

                // route executed nodes output to my input, and my input to the result
                if (scriptEventEntry.IsExpectingResult())
                {
                    if (result)
                    {
                        if (const Datum* resultInput = GetInput(scriptEventEntry.m_resultSlotId))
                        {
                            scriptEventEntry.m_resultEvaluated = resultInput->ToBehaviorContext(*result);
                            AZ_Warning("Script Canvas", scriptEventEntry.m_resultEvaluated, "%s expects a result value of type %s to be provided, got %s", scriptEventEntry.m_eventName.c_str(), Data::GetName(resultInput->GetType()).c_str(), result->m_name);
                        }
                        else
                        {
                            AZ_Warning("Script Canvas", false, "Script Canvas handler expecting a result, but had no ability to return it");
                        }
                    }
                    else
                    {
                        AZ_Warning("Script Canvas", false, "Script Canvas handler is expecting a result, but was called without expecting one!");
                    }   
                }
                else
                {
                    AZ_Warning("Script Canvas", !result, "Script Canvas handler is not expecting a result, but was called expecting one!");
                }
            }

            void ReceiveScriptEvent::OnActivate()
            {
                ScriptEventBase::OnActivate();

                if (m_ebus && m_handler)
                {
                    Connect();
                }
            }

            void ReceiveScriptEvent::OnDeactivate()
            {
                bool queueDisconnect = false;
                Disconnect(queueDisconnect);

                ScriptEventBase::OnDeactivate();
            }

            void ReceiveScriptEvent::Connect()
            {
                if (!m_handler)
                {
                    AZ_Error("Script Canvas", false, "Cannot connect to an EBus Handler for Script Event: %s", m_definition.GetName().c_str());
                    return;
                }

                AZ::EntityId connectToEntityId;
                AZ::BehaviorValueParameter busIdParameter;
                busIdParameter.Set(m_ebus->m_idParam);
                const AZ::Uuid busIdType = m_ebus->m_idParam.m_typeId;

                const Datum* busIdDatum = IsIDRequired() ? GetInput(GetSlotId(c_busIdName)) : nullptr;
                if (busIdDatum && !busIdDatum->Empty())
                {
                    if (busIdDatum->IS_A(Data::FromAZType(busIdType)) || busIdDatum->IsConvertibleTo(Data::FromAZType(busIdType)))
                    {
                        auto busIdOutcome = busIdDatum->ToBehaviorValueParameter(m_ebus->m_idParam);
                        if (busIdOutcome.IsSuccess())
                        {
                            busIdParameter = busIdOutcome.TakeValue();
                        }
                    }

                    if (busIdType == azrtti_typeid<AZ::EntityId>())
                    {
                        if (auto busEntityId = busIdDatum->GetAs<AZ::EntityId>())
                        {
                            if (!busEntityId->IsValid() || *busEntityId == ScriptCanvas::SelfReferenceId)
                            {
                                RuntimeRequestBus::EventResult(connectToEntityId, m_executionUniqueId, &RuntimeRequests::GetRuntimeEntityId);
                                busIdParameter.m_value = &connectToEntityId;
                            }
                        }
                    }

                }
                if (!IsIDRequired() || busIdParameter.GetValueAddress())
                {
                    // Ensure we disconnect if this bus is already connected, this could happen if a different bus Id is provided
                    // and this node is connected through the Connect slot.
                    m_handler->Disconnect();

                    AZ_VerifyError("Script Canvas", m_handler->Connect(&busIdParameter),
                        "Unable to connect to EBus with BusIdType %s. The BusIdType of the Script Event (%s) does not match the BusIdType provided.",
                        busIdType.ToString<AZStd::string>().data(), m_definition.GetName().c_str());
                }
            }

            void ReceiveScriptEvent::Disconnect(bool queueDisconnect)
            {
                AZStd::string busName = m_definition.GetName();

                auto disconnectFunc = [this, busName]() {
                    if (!m_handler)
                    {
                        AZ_Error("Script Canvas", false, "Cannot disconnect from an EBus Handler for Script Event: %s", busName.c_str());
                        return;
                    }

                    m_handler->Disconnect();
                };

                if (queueDisconnect)
                {
                    AZ::TickBus::QueueFunction(disconnectFunc);
                }
                else
                {
                    disconnectFunc();
                }
            }

            AZStd::vector<SlotId> ReceiveScriptEvent::GetEventSlotIds() const
            {
                AZStd::vector<SlotId> eventSlotIds;

                for (const auto& iter : m_eventMap)
                {
                    eventSlotIds.push_back(iter.second.m_eventSlotId);
                }

                return eventSlotIds;
            }

            AZStd::vector<SlotId> ReceiveScriptEvent::GetNonEventSlotIds() const
            {
                AZStd::vector<SlotId> nonEventSlotIds;

                for (const auto& slot : m_slots)
                {
                    const SlotId slotId = slot.GetId();

                    if (!IsEventSlotId(slotId))
                    {
                        nonEventSlotIds.push_back(slotId);
                    }
                }

                return nonEventSlotIds;
            }

            bool ReceiveScriptEvent::IsEventSlotId(const SlotId& slotId) const
            {
                for (const auto& eventItem : m_eventMap)
                {
                    const auto& event = eventItem.second;
                    if (slotId == event.m_eventSlotId
                        || slotId == event.m_resultSlotId
                        || event.m_parameterSlotIds.end() != AZStd::find_if(event.m_parameterSlotIds.begin(), event.m_parameterSlotIds.end(), [&slotId](const SlotId& candidate) { return slotId == candidate; }))
                    {
                        return true;
                    }
                }

                return false;
            }

            bool ReceiveScriptEvent::IsIDRequired() const
            {
                return m_definition.IsAddressRequired();
            }

            bool ReceiveScriptEvent::CreateHandler(AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset)
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);

                if (m_handler)
                {
                    return true;
                }

                if (!asset)
                {
                    return false;
                }

                m_definition = asset.Get()->m_definition;
                if (m_version == 0)
                {
                    m_version = m_definition.GetVersion();
                }

                if (!m_scriptEvent)
                {
                    ScriptEvents::ScriptEventBus::BroadcastResult(m_scriptEvent, &ScriptEvents::ScriptEventRequests::RegisterScriptEvent, m_scriptEventAssetId, m_version);
                }

                return true;

            }

            void ReceiveScriptEvent::OnScriptEventReady(const AZ::Data::Asset<ScriptEvents::ScriptEventsAsset>& asset)
            {
                if (CreateHandler(asset))
                {
                    CompleteInitialize(asset);
                }
            }

            bool ReceiveScriptEvent::CreateEbus()
            {
                if (!m_ebus)
                {
                    AZ::BehaviorContext* behaviorContext = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationBus::Events::GetBehaviorContext);

                    const AZStd::string& name = m_definition.GetName();
                    const auto& ebusIterator = behaviorContext->m_ebuses.find(name);
                    if (ebusIterator == behaviorContext->m_ebuses.end())
                    {
                        AZ_Error("Script Canvas", false, "ReceiveScriptEvent::CreateHandler - No ebus by name of %s in the behavior context!", name.c_str());
                        return false;
                    }

                    m_ebus = ebusIterator->second;
                    AZ_Assert(m_ebus, "Behavior Context EBus does not exist: %s", name.c_str());
                    AZ_Assert(m_ebus->m_createHandler, "The ebus %s has no create handler!", name.c_str());
                    AZ_Assert(m_ebus->m_destroyHandler, "The ebus %s has no destroy handler!", name.c_str());

                    AZ_Verify(m_ebus->m_createHandler->InvokeResult(m_handler, &m_definition), "Behavior Context EBus handler creation failed %s", name.c_str());

                    AZ_Assert(m_handler, "Ebus create handler failed %s", name.c_str());
                }

                return true;
            }

            void ReceiveScriptEvent::OnInputSignal(const SlotId& slotId)
            {
                SlotId connectSlot = ReceiveScriptEventProperty::GetConnectSlotId(this);

                if (connectSlot == slotId)
                {
                    const Datum* busIdDatum = GetInput(GetSlotId(c_busIdName));
                    if (IsIDRequired() && (!busIdDatum || busIdDatum->Empty()))
                    {
                        SlotId failureSlot = ReceiveScriptEventProperty::GetOnFailureSlotId(this);
                        SignalOutput(failureSlot);
                        SCRIPTCANVAS_REPORT_ERROR((*this), "In order to connect this node, a valid BusId must be provided.");
                        return;
                    }
                    else
                    {
                        Connect();
                        SlotId onConnectSlotId = ReceiveScriptEventProperty::GetOnConnectedSlotId(this);
                        SignalOutput(onConnectSlotId);
                        return;
                    }
                }

                SlotId disconnectSlot = ReceiveScriptEventProperty::GetDisconnectSlotId(this);
                if (disconnectSlot == slotId)
                {
                    Disconnect();

                    SlotId onDisconnectSlotId = ReceiveScriptEventProperty::GetOnDisconnectedSlotId(this);
                    SignalOutput(onDisconnectSlotId);
                    return;
                }
            }

            void ReceiveScriptEvent::OnWriteEnd()
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
                if (!m_ebus)
                {
                    AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(m_scriptEventAssetId);
                    CreateHandler(asset);
                }
            }

            void ReceiveScriptEvent::UpdateScriptEventAsset()
            {
                AZStd::unordered_map< AZ::Uuid, Datum > slotDatumValues;

                for (auto mapPair : m_eventSlotMapping)
                {
                    Datum* datum = ModInput(mapPair.second);

                    if (datum)
                    {
                        slotDatumValues.insert(AZStd::make_pair(mapPair.first, Datum(*datum)));
                    }

                    const bool removeConnections = false;
                    RemoveSlot(mapPair.second, removeConnections);
                }
                
                m_eventMap.clear();
                m_scriptEvent.reset();

                delete m_handler;
                m_handler = nullptr;
                m_ebus = nullptr;

                m_version = 0;

                SlotIdMapping populationMapping;

                AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(m_scriptEventAssetId);
                PopulateAsset(asset, populationMapping);

                // Erase anything we remapped since we don't want to signal out that it changed.
                for (auto mappingPair : populationMapping)
                {
                    m_eventSlotMapping.erase(mappingPair.first);
                }

                // Actually do the removal signalling for any slots that weren't updated
                for (auto mappingPair : m_eventSlotMapping)
                {
                    RemoveConnectionsForSlot(mappingPair.second);
                }

                m_eventSlotMapping = AZStd::move(populationMapping);

                for (auto datumPair : slotDatumValues)
                {
                    auto slotIter = m_eventSlotMapping.find(datumPair.first);

                    if (slotIter != m_eventSlotMapping.end())
                    {
                        SlotId slotId = slotIter->second;
                        Datum* datum = ModInput(slotId);

                        if (datum->GetType() == datumPair.second.GetType())
                        {
                            *datum = datumPair.second;
                        }
                    }
                }
            }

        } // namespace Core
    } // namespace Nodes
} // namespace ScriptCanvas

#include <Include/ScriptCanvas/Libraries/Core/ReceiveScriptEvent.generated.cpp>

