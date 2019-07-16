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

#include "OperatorInsert.h"
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Core/Contracts/SupportsMethodContract.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            void OperatorInsert::ConfigureContracts(SourceType sourceType, AZStd::vector<ContractDescriptor>& contractDescs)
            {
                if (sourceType == SourceType::SourceInput)
                {
                    ContractDescriptor supportsMethodContract;
                    supportsMethodContract.m_createFunc = [this]() -> SupportsMethodContract* { return aznew SupportsMethodContract("Insert"); };
                    contractDescs.push_back(AZStd::move(supportsMethodContract));
                }
            }

            void OperatorInsert::OnSourceTypeChanged()
            {
                if (Data::IsVectorContainerType(GetSourceAZType()))
                {
                    // Add the INDEX as the INPUT slot
                    m_inputSlots.insert(AddInputDatumSlot("Index", "", Data::Type::Number(), Datum::eOriginality::Original, false));

                    // Add the INPUT slot for the data to insert
                    Data::Type type = Data::FromAZType(m_sourceTypes[0]);
                    m_inputSlots.insert(AddInputDatumSlot(Data::GetName(type), "", type, Datum::eOriginality::Original, false));

                }
                else
                {
                    for (AZ::TypeId sourceType : m_sourceTypes)
                    {
                        Data::Type type = Data::FromAZType(sourceType);
                        m_inputSlots.insert(AddInputDatumSlot(Data::GetName(type), "", type, Datum::eOriginality::Original, false));
                    }
                }
                
                m_outputSlots.insert(AddOutputTypeSlot(Data::GetName(GetSourceType()), "Container", GetSourceType(), Node::OutputStorage::Required, false));
            }

            void OperatorInsert::InvokeOperator()
            {
                const SlotSet& slotSets = GetSourceSlots();

                if (!slotSets.empty())
                {
                    SlotId sourceSlotId = (*slotSets.begin());

                    const Datum* containerDatum = GetInput(sourceSlotId);
                    if (containerDatum != nullptr && IsConnected(sourceSlotId))
                    {
                        AZ::BehaviorMethod* method = GetOperatorMethod("Insert");
                        AZ_Assert(method, "The contract must have failed because you should not be able to invoke an operator for a type that does not have the method");

                        AZStd::array<AZ::BehaviorValueParameter, BehaviorContextMethodHelper::MaxCount> params;

                        AZ::BehaviorValueParameter* paramFirst(params.begin());
                        AZ::BehaviorValueParameter* paramIter = paramFirst;

                        if (Data::IsVectorContainerType(GetSourceAZType()))
                        {
                            // Container
                            auto behaviorParameter = method->GetArgument(0);
                            AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> param0 = containerDatum->ToBehaviorValueParameter(*behaviorParameter);
                            paramIter->Set(param0.GetValue());
                            ++paramIter;

                            // Get the size of the container
                            auto sizeOutcome = BehaviorContextMethodHelper::CallMethodOnDatum(*containerDatum, "Size");
                            if (!sizeOutcome)
                            {
                                SCRIPTCANVAS_REPORT_ERROR((*this), "Failed to get size of container: %s", sizeOutcome.GetError().c_str());
                                return;
                            }

                            auto inputSlotIterator = m_inputSlots.begin();

                            // Index
                            behaviorParameter = method->GetArgument(1);
                            const Datum* indexDatum = GetInput(*inputSlotIterator);
                            AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> param2 = indexDatum->ToBehaviorValueParameter(*behaviorParameter);

                            paramIter->Set(param2.GetValue());
                            ++paramIter;

                            // Value to insert
                            behaviorParameter = method->GetArgument(2);
                            ++inputSlotIterator;
                            const Datum* inputDatum = GetInput(*inputSlotIterator);
                            AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> param1 = inputDatum->ToBehaviorValueParameter(*behaviorParameter);
                            paramIter->Set(param1.GetValue());
                            ++paramIter;
                        }
                        else if (Data::IsMapContainerType(GetSourceAZType()))
                        {
                            auto inputSlotIterator = m_inputSlots.begin();
                            const Datum* keyDatum = GetInput(*inputSlotIterator++);
                            const Datum* valueDatum = GetInput(*inputSlotIterator);

                            if (keyDatum && valueDatum)
                            {
                                // Container
                                const AZ::BehaviorParameter* behaviorParameter = method->GetArgument(0);
                                AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> param0 = containerDatum->ToBehaviorValueParameter(*behaviorParameter);
                                paramIter->Set(param0.GetValue());
                                ++paramIter;

                                // Key
                                behaviorParameter = method->GetArgument(1);
                                AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> param1 = keyDatum->ToBehaviorValueParameter(*behaviorParameter);
                                paramIter->Set(param1.GetValue());
                                ++paramIter;

                                // Value to insert
                                behaviorParameter = method->GetArgument(2);
                                AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> param2 = valueDatum->ToBehaviorValueParameter(*behaviorParameter);
                                paramIter->Set(param2.GetValue());
                                ++paramIter;
                            }
                        }

                        AZStd::vector<SlotId> resultSlotIDs;
                        resultSlotIDs.push_back();
                        BehaviorContextMethodHelper::Call(*this, false, method, paramFirst, paramIter, resultSlotIDs);

                        // Push the source container as an output to support chaining
                        PushOutput(*containerDatum, *GetSlot(*m_outputSlots.begin()));
                    }
                }

                SignalOutput(GetSlotId("Out"));
            }

            void OperatorInsert::OnInputSignal(const SlotId& slotId)
            {
                const SlotId inSlotId = OperatorBaseProperty::GetInSlotId(this);
                if (slotId == inSlotId)
                {
                    InvokeOperator();
                }
            }
        }
    }
}

#include <Include/ScriptCanvas/Libraries/Operators/Containers/OperatorInsert.generated.cpp>