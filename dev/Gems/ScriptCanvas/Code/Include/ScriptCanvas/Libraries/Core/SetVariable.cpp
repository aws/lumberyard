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

#include <precompiled.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Libraries/Core/SetVariable.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <Libraries/Core/MethodUtility.h>

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
                }
            }

            void SetVariableNode::OnInputSignal(const SlotId& slotID)
            {
                if (slotID == GetSlotId(GetInputSlotName()))
                {
                    const Datum* sourceDatum = GetInput(m_variableDataInSlotId);
                    Datum* variableDatum = ModDatum();
                    if (sourceDatum && variableDatum)
                    {
                        *variableDatum = *sourceDatum;
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
                        RemoveInputSlot();
                        AddInputSlot();
                    }

                    if (m_variableId.IsValid())
                    {
                        VariableNotificationBus::Handler::BusConnect(m_variableId);
                    }

                    VariableNodeNotificationBus::Event(GetEntityId(), &VariableNodeNotifications::OnVariableIdChanged, oldVariableId, m_variableId);
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

            const Datum* SetVariableNode::GetDatum() const
            {
                return ModDatum();
            }

            Datum* SetVariableNode::ModDatum() const
            {
                VariableDatum* variableDatum{};
                VariableRequestBus::EventResult(variableDatum, m_variableId, &VariableRequests::GetVariableDatum);
                return variableDatum ? &variableDatum->GetData() : nullptr;
            }

            void SetVariableNode::AddInputSlot()
            {
                if (m_variableId.IsValid())
                {
                    AZStd::string_view varName;
                    Data::Type varType;
                    VariableRequestBus::EventResult(varName, m_variableId, &VariableRequests::GetName);
                    VariableRequestBus::EventResult(varType, m_variableId, &VariableRequests::GetType);

                    const AZStd::string sourceSlotName(AZStd::string::format("%s", Data::GetName(varType)));
                    m_variableDataInSlotId = AddInputDatumSlot(sourceSlotName, "", varType, Datum::eOriginality::Copy);
                }
            }

            void SetVariableNode::RemoveInputSlot()
            {
                SlotId oldVariableDataInSlotId;
                AZStd::swap(oldVariableDataInSlotId, m_variableDataInSlotId);
                RemoveSlot(oldVariableDataInSlotId);
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
                                varNameToIdList.emplace_back(variablePair.first, variablePair.second.m_varName);
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
                    RemoveInputSlot();
                }
                VariableNodeNotificationBus::Event(GetEntityId(), &VariableNodeNotifications::OnVariableRemovedFromNode, removedVariableId);
            }
        }
    }
}

#include <Include/ScriptCanvas/Libraries/Core/SetVariable.generated.cpp>