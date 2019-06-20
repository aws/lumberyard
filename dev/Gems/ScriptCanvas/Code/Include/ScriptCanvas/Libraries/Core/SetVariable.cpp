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

#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Libraries/Core/SetVariable.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <Libraries/Core/MethodUtility.h>
#include <Core/ExecutionNotificationsBus.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            void SetVariableNode::OnInit()
            {
                VariableNodeRequestBus::Handler::BusConnect(GetEntityId());
                if (m_variableId.IsValid())
                {
                    VariableNotificationBus::Handler::BusConnect(m_variableId);
                    RefreshPropertyFunctions();
                    PopulateNodeType();
                }
            }

            void SetVariableNode::OnInputSignal(const SlotId& slotID)
            {
                if (slotID == GetSlotId(GetInputSlotName()))
                {
                    const Datum* sourceDatum = GetInput(m_variableDataInSlotId);
                    VariableDatumBase* variableDatum = ModVariable();
                    if (sourceDatum && variableDatum)
                    {
                        variableDatum->GetData() = *sourceDatum;
                        SC_EXECUTION_TRACE_VARIABLE_CHANGE((*this), (variableDatum->GetId()), (CreateVariableChange(*variableDatum)));
                    }
                    
                    Slot* resultSlot = GetSlot(m_variableDataOutSlotId);
                    if (resultSlot && variableDatum)
                    {
                        PushOutput(variableDatum->GetData(), *resultSlot);

                        // Push the data for each property slot out as well
                        for (auto&& propertyAccount : m_propertyAccounts)
                        {
                            Slot* propertySlot = GetSlot(propertyAccount.m_propertySlotId);
                            if (propertySlot && propertyAccount.m_getterFunction)
                            {
                                auto outputOutcome = propertyAccount.m_getterFunction(variableDatum->GetData());
                                if (!outputOutcome)
                                {
                                    SCRIPTCANVAS_REPORT_ERROR((*this), outputOutcome.TakeError().data());
                                    return;
                                }
                                PushOutput(outputOutcome.TakeValue(), *propertySlot);
                            }
                        }
                    }

                    SignalOutput(GetSlotId(GetOutputSlotName()));
                }
            }

            void SetVariableNode::SetId(const VariableId& variableDatumId)
            {
                if (m_variableId != variableDatumId)
                {
                    VariableId oldVariableId = m_variableId;
                    m_variableId = variableDatumId;

                    VariableNotificationBus::Handler::BusDisconnect();

                    ScriptCanvas::Data::Type oldType = ScriptCanvas::Data::Type::Invalid();
                    VariableRequestBus::EventResult(oldType, oldVariableId, &VariableRequests::GetType);

                    ScriptCanvas::Data::Type newType = ScriptCanvas::Data::Type::Invalid();
                    VariableRequestBus::EventResult(newType, m_variableId, &VariableRequests::GetType);

                    if (oldType != newType)
                    {
                        ScopedBatchOperation scopedBatchOperation(AZ_CRC("SetVariableIdChanged", 0xc072e633));
                        RemoveSlots();
                        AddSlots();
                    }

                    if (m_variableId.IsValid())
                    {
                        VariableNotificationBus::Handler::BusConnect(m_variableId);
                    }

                    VariableNodeNotificationBus::Event(GetEntityId(), &VariableNodeNotifications::OnVariableIdChanged, oldVariableId, m_variableId);

                    PopulateNodeType();
                }
            }

            const VariableId& SetVariableNode::GetId() const
            {
                return m_variableId;
            }

            const SlotId& SetVariableNode::GetDataInSlotId() const
            {
                return m_variableDataInSlotId;
            }

            const SlotId& SetVariableNode::GetDataOutSlotId() const
            {
                return m_variableDataOutSlotId;
            }

            const Datum* SetVariableNode::GetDatum() const
            {
                auto variableDatum = ModVariable();
                return variableDatum ? &variableDatum->GetData() : nullptr;
            }

            VariableDatumBase* SetVariableNode::ModVariable() const
            {
                VariableDatum* variableDatum{};
                VariableRequestBus::EventResult(variableDatum, m_variableId, &VariableRequests::GetVariableDatum);
                AZ_Warning("ScriptCanvas", !variableDatum || variableDatum->GetId() == m_variableId, "Mismatch in SetVariableNode: m_variableId: %s but found variable id: %s", m_variableId.ToString().data(), variableDatum->GetId().ToString().data());
                return variableDatum;
            }

            void SetVariableNode::AddSlots()
            {
                if (m_variableId.IsValid())
                {
                    AZStd::string_view varName;
                    Data::Type varType;
                    VariableRequestBus::EventResult(varName, m_variableId, &VariableRequests::GetName);
                    VariableRequestBus::EventResult(varType, m_variableId, &VariableRequests::GetType);

                    m_variableDataInSlotId = AddInputDatumSlot(Data::GetName(varType), "", varType, Datum::eOriginality::Copy);
                    m_variableDataOutSlotId = AddOutputTypeSlot(Data::GetName(varType), "", varType, OutputStorage::Optional);
                    AddPropertySlots(varType);
                }
            }

            void SetVariableNode::RemoveSlots()
            {
                ClearPropertySlots();

                SlotId oldVariableDataInSlotId;
                AZStd::swap(oldVariableDataInSlotId, m_variableDataInSlotId);
                RemoveSlot(oldVariableDataInSlotId);

                SlotId oldVariableDataOutSlotId;
                AZStd::swap(oldVariableDataOutSlotId, m_variableDataOutSlotId);
                RemoveSlot(oldVariableDataOutSlotId);
            }

            void SetVariableNode::OnIdChanged(const VariableId& oldVariableId)
            {
                if (m_variableId != oldVariableId)
                {
                    VariableId newVariableId = m_variableId;
                    m_variableId = oldVariableId;
                    SetId(newVariableId);
                }
            }

            AZStd::vector<AZStd::pair<VariableId, AZStd::string>> SetVariableNode::GetGraphVariables() const
            {
                AZStd::vector<AZStd::pair<VariableId, AZStd::string>> varNameToIdList;

                if (m_variableId.IsValid())
                {
                    ScriptCanvas::Data::Type baseType = ScriptCanvas::Data::Type::Invalid();
                    VariableRequestBus::EventResult(baseType, m_variableId, &VariableRequests::GetType);

                    const AZStd::unordered_map<VariableId, VariableNameValuePair>* variableMap{};
                    GraphVariableManagerRequestBus::EventResult(variableMap, GetGraphId(), &GraphVariableManagerRequests::GetVariables);
                    if (variableMap && baseType.IsValid())
                    {
                        for (const auto& variablePair : *variableMap)
                        {
                            ScriptCanvas::Data::Type variableType = variablePair.second.m_varDatum.GetData().GetType();

                            if (variableType == baseType)
                            {
                                varNameToIdList.emplace_back(variablePair.first, variablePair.second.GetVariableName());
                            }
                        }
                    }
                }

                return varNameToIdList;
            }

            void SetVariableNode::OnVariableRemoved()
            {
                VariableNotificationBus::Handler::BusDisconnect();
                VariableId removedVariableId;
                AZStd::swap(removedVariableId, m_variableId);
                {
                    ScopedBatchOperation scopedBatchOperation(AZ_CRC("SetVariableRemoved", 0xd7da59f5));
                    RemoveSlots();
                }
                VariableNodeNotificationBus::Event(GetEntityId(), &VariableNodeNotifications::OnVariableRemovedFromNode, removedVariableId);
            }

            void SetVariableNode::AddPropertySlots(const Data::Type& type)
            {
                Data::GetterContainer getterFunctions = Data::ExplodeToGetters(type);
                for (const auto& getterWrapperPair : getterFunctions)
                {
                    const AZStd::string& propertyName = getterWrapperPair.first;
                    const Data::GetterWrapper& getterWrapper = getterWrapperPair.second;
                    Data::PropertyMetadata propertyAccount;
                    propertyAccount.m_propertyType = getterWrapper.m_propertyType;
                    propertyAccount.m_propertyName = propertyName;

                    const AZStd::string resultSlotName(AZStd::string::format("%s: %s", propertyName.data(), Data::GetName(getterWrapper.m_propertyType).data()));
                    propertyAccount.m_propertySlotId = AddOutputTypeSlot(resultSlotName, "", getterWrapper.m_propertyType, OutputStorage::Optional);

                    propertyAccount.m_getterFunction = getterWrapper.m_getterFunction;
                    m_propertyAccounts.push_back(propertyAccount);
                }
            }

            void SetVariableNode::ClearPropertySlots()
            {
                auto oldPropertyAccounts = AZStd::move(m_propertyAccounts);
                m_propertyAccounts.clear();
                for (auto&& propertyAccount : oldPropertyAccounts)
                {
                    RemoveSlot(propertyAccount.m_propertySlotId);
                }
            }

            void SetVariableNode::RefreshPropertyFunctions()
            {
                Data::Type sourceType;
                VariableRequestBus::EventResult(sourceType, m_variableId, &VariableRequests::GetType);
                if (!sourceType.IsValid())
                {
                    return;
                }

                auto getterWrapperMap = Data::ExplodeToGetters(sourceType);

                for (auto&& propertyAccount : m_propertyAccounts)
                {
                    if (!propertyAccount.m_getterFunction)
                    {
                        auto foundPropIt = getterWrapperMap.find(propertyAccount.m_propertyName);
                        if (foundPropIt != getterWrapperMap.end() && propertyAccount.m_propertyType.IS_A(foundPropIt->second.m_propertyType))
                        {
                            propertyAccount.m_getterFunction = foundPropIt->second.m_getterFunction;
                        }
                        else
                        {
                            AZ_Error("Script Canvas", false, "Property (%s : %s) getter method could not be found in Data::PropertyTraits or the property type has changed."
                                " Output will not be pushed on the property's slot.",
                                propertyAccount.m_propertyName.c_str(), Data::GetName(propertyAccount.m_propertyType).data());
                        }
                    }
                }
            }

        }
    }
}

#include <Include/ScriptCanvas/Libraries/Core/SetVariable.generated.cpp>