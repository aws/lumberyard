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

#include "Operator.h"

#include <AzCore/RTTI/BehaviorContext.h>

#include <ScriptCanvas/Core/Contracts/TypeContract.h>
#include <ScriptCanvas/Core/Contracts/ConnectionLimitContract.h>
#include <ScriptCanvas/Core/Contracts/SupportsMethodContract.h>

#include <ScriptCanvas/Libraries/Core/MethodUtility.h>

#include <ScriptCanvas/Data/Data.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            OperatorBase::OperatorBase()
                : m_sourceType(ScriptCanvas::Data::Type::Invalid())
                , m_sourceDisplayType(ScriptCanvas::Data::Type::Invalid())
            {
            }

            OperatorBase::OperatorBase(const OperatorConfiguration& operatorConfiguration)
                : Node()
                , m_sourceType(ScriptCanvas::Data::Type::Invalid())
                , m_sourceDisplayType(ScriptCanvas::Data::Type::Invalid())
                , m_operatorConfiguration(operatorConfiguration)
            {
            }

            bool OperatorBase::IsSourceSlotId(const SlotId& slotId) const
            {
                return m_sourceSlots.find(slotId) != m_sourceSlots.end();
            }

            const OperatorBase::SlotSet& OperatorBase::GetSourceSlots() const
            {
                return m_sourceSlots;
            }

            ScriptCanvas::Data::Type OperatorBase::GetSourceType() const
            {
                return m_sourceType;
            }

            AZ::TypeId OperatorBase::GetSourceAZType() const
            {
                return m_sourceTypeId;
            }

            ScriptCanvas::Data::Type OperatorBase::GetDisplayType() const
            {
                return m_sourceDisplayType;
            }

            void OperatorBase::OnInit()
            {
                // If we don't have any source slots. Add at least one.
                if (m_sourceSlots.empty())
                {
                    for (const auto& sourceConfiguration : m_operatorConfiguration.m_sourceSlotConfigurations)
                    {
                        AddSourceSlot(sourceConfiguration);
                    }
                }
                else
                {
                    for (auto sourceSlotId : m_sourceSlots)
                    {
                        EndpointNotificationBus::MultiHandler::BusConnect({ GetEntityId(), sourceSlotId });
                    }
                }

                for (const SlotId& inputSlot : m_inputSlots)
                {
                    EndpointNotificationBus::MultiHandler::BusConnect({ GetEntityId(), inputSlot });
                }

                for (const SlotId& outputSlot : m_outputSlots)
                {
                    EndpointNotificationBus::MultiHandler::BusConnect({ GetEntityId(), outputSlot });
                }

                if (m_sourceType.IsValid())
                {
                    PopulateAZTypes(m_sourceType);
                }
            }

            ScriptCanvas::SlotId OperatorBase::AddSourceSlot(SourceSlotConfiguration sourceConfiguration)
            {
                SlotId sourceSlotId;
                DataSlotConfiguration slotConfiguration;

                if (sourceConfiguration.m_sourceType == SourceType::SourceInput)
                {
                    slotConfiguration.m_contractDescs = { { []() { return aznew ConnectionLimitContract(1); } } };
                }

                ConfigureContracts(sourceConfiguration.m_sourceType, slotConfiguration.m_contractDescs);

                slotConfiguration.m_name = sourceConfiguration.m_name;
                slotConfiguration.m_toolTip = sourceConfiguration.m_tooltip;
                slotConfiguration.m_addUniqueSlotByNameAndType = true;
                slotConfiguration.m_dynamicDataType = sourceConfiguration.m_dynamicDataType;

                if (sourceConfiguration.m_sourceType == SourceType::SourceInput)
                {
                    slotConfiguration.m_slotType = SlotType::DataIn;
                    sourceSlotId = AddInputDatumSlot(slotConfiguration, Datum());
                }
                else if (sourceConfiguration.m_sourceType == SourceType::SourceOutput)
                {
                    slotConfiguration.m_slotType = SlotType::DataOut;
                    sourceSlotId = AddDataSlot(slotConfiguration);
                }

                m_sourceSlots.insert(sourceSlotId);

                if (m_sourceType.IsValid())
                {
                    Slot* slot = GetSlot(sourceSlotId);

                    if (slot)
                    {
                        slot->SetDisplayType(m_sourceType);
                    }
                }

                EndpointNotificationBus::MultiHandler::BusConnect({ GetEntityId(), sourceSlotId });

                return sourceSlotId;
            }

            void OperatorBase::ConfigureContracts(SourceType sourceType, AZStd::vector<ContractDescriptor>& contractDescs)
            {
                AZ_UNUSED(contractDescs);
                AZ_UNUSED(sourceType);
            }

            void OperatorBase::RemoveInputs()
            {
                for (SlotId slotId : m_inputSlots)
                {
                    RemoveSlot(slotId);
                }

                m_inputSlots.clear();
            }

            void OperatorBase::OnEndpointConnected(const Endpoint& endpoint)
            {
                const SlotId& currentSlotId = EndpointNotificationBus::GetCurrentBusId()->GetSlotId();

                bool isInBatchAdd = false;
                GraphRequestBus::EventResult(isInBatchAdd, GetGraphEntityId(), &GraphRequests::IsBatchAddingGraphData);
                
                if (IsSourceSlotId(currentSlotId))
                {
                    auto node = AZ::EntityUtils::FindFirstDerivedComponent<Node>(endpoint.GetNodeId());
                    if (node)
                    {
                        // If we are connecting to a dyanmic slot. We only want to update our typing
                        // If it has a display type(as we will adopt its display type)
                        Slot* slot = node->GetSlot(endpoint.GetSlotId());

                        if (!slot->IsDynamicSlot() || slot->HasDisplayType())
                        {
                            auto dataType = node->GetSlotDataType(endpoint.GetSlotId());
                            if (dataType != Data::Type::Invalid())
                            {
                                SetSourceType(dataType);
                            }

                            DisplaySourceType(dataType);
                        }

                        if (m_sourceType.IsValid())
                        {
                            OnSourceConnected(currentSlotId);
                        }
                    }
                }
                else if (!isInBatchAdd)
                {
                    Slot* slot = GetSlot(currentSlotId);
                    if (slot && slot->GetType() == SlotType::DataIn)
                    {
                        OnDataInputSlotConnected(currentSlotId, endpoint);
                        return;
                    }
                }
            }

            void OperatorBase::OnEndpointDisconnected(const Endpoint& endpoint)
            {
                const SlotId& currentSlotId = EndpointNotificationBus::GetCurrentBusId()->GetSlotId();

                if (IsSourceSlotId(currentSlotId))
                {
                    OnSourceDisconnected(currentSlotId);

                    if (!HasSourceConnection())
                    {
                        DisplaySourceType(Data::Type::Invalid());
                    }
                }
                else
                {
                    Slot* slot = GetSlot(currentSlotId);
                    if (slot && slot->GetType() == SlotType::DataIn)
                    {
                        OnDataInputSlotDisconnected(currentSlotId, endpoint);
                        EndpointNotificationBus::MultiHandler::BusDisconnect({ GetEntityId(), slot->GetId() });
                        return;
                    }
                }
            }

            AZ::BehaviorMethod* OperatorBase::GetOperatorMethod(const char* methodName)
            {
                AZ::BehaviorMethod* method = nullptr;

                const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(m_sourceTypeId);
                if (behaviorClass)
                {
                    auto methodIt = behaviorClass->m_methods.find(methodName);

                    if (methodIt != behaviorClass->m_methods.end())
                    {
                        method = methodIt->second;
                    }
                }

                return method;
            }

            SlotId OperatorBase::AddSlotWithSourceType()
            {
                Data::Type type = Data::FromAZType(m_sourceTypes[0]);
                SlotId inputSlotId = AddInputDatumSlot(Data::GetName(type), "", type, Datum::eOriginality::Original, false);
                m_inputSlots.insert(inputSlotId);
                EndpointNotificationBus::MultiHandler::BusConnect({ GetEntityId(), inputSlotId });

                OnInputSlotAdded(inputSlotId);

                return inputSlotId;
            }

            void OperatorBase::SetSourceType(ScriptCanvas::Data::Type dataType)
            {
                if (dataType != m_sourceType)
                {
                    RemoveInputs();
                    RemoveOutputs();

                    m_sourceType = dataType;

                    PopulateAZTypes(m_sourceType);

                    OnSourceTypeChanged();
                }

                DisplaySourceType(dataType);
            }

            void OperatorBase::DisplaySourceType(ScriptCanvas::Data::Type dataType)
            {
                if (m_sourceDisplayType != dataType)
                {
                    m_sourceDisplayType = dataType;

                    OnDisplayTypeChanged(dataType);

                    for (auto sourceSlotId : m_sourceSlots)
                    {
                        auto sourceSlot = GetSlot(sourceSlotId);

                        if (sourceSlot)
                        {
                            sourceSlot->SetDisplayType(dataType);
                        }
                    }
                }
            }

            bool OperatorBase::HasSourceConnection() const
            {
                for (auto sourceSlotId : m_sourceSlots)
                {
                    if (IsConnected(sourceSlotId))
                    {
                        return true;
                    }
                }

                return false;
            }

            bool OperatorBase::AreSourceSlotsFull(SourceType sourceType) const
            {
                bool isFull = true;
                for (auto sourceSlotId : m_sourceSlots)
                {
                    auto sourceSlot = GetSlot(sourceSlotId);

                    switch (sourceType)
                    {
                    case SourceType::SourceInput:
                        if (sourceSlot->GetType() == SlotType::DataIn)
                        {
                            isFull = IsConnected(sourceSlotId);
                        }
                        break;
                    case SourceType::SourceOutput:
                        if (sourceSlot->GetType() == SlotType::DataOut)
                        {
                            isFull = IsConnected(sourceSlotId);
                        }
                        break;
                    default:
                        // No idea what this is. But it's not supported.
                        AZ_Error("ScriptCanvas", false, "Operator is operating upon an unknown SourceType");
                        isFull = false;
                        break;
                    }

                    if (!isFull)
                    {
                        break;
                    }
                }

                return isFull;
            }

            void OperatorBase::PopulateAZTypes(ScriptCanvas::Data::Type dataType)
            {
                m_sourceTypeId = ScriptCanvas::Data::ToAZType(dataType);

                m_sourceTypes.clear();

                if (ScriptCanvas::Data::IsContainerType(m_sourceTypeId))
                {
                    AZStd::vector<AZ::Uuid> types = ScriptCanvas::Data::GetContainedTypes(m_sourceTypeId);
                    m_sourceTypes.insert(m_sourceTypes.end(), types.begin(), types.end());
                }
                else
                {
                    // The data type is then a source type
                    m_sourceTypes.push_back(m_sourceTypeId);
                }
            }

            void OperatorBase::RemoveOutputs()
            {
                for (SlotId slotId : m_outputSlots)
                {
                    RemoveSlot(slotId);
                }

                m_outputSlots.clear();
            }

        }
    }
}

#include <Include/ScriptCanvas/Libraries/Operators/Operator.generated.cpp>