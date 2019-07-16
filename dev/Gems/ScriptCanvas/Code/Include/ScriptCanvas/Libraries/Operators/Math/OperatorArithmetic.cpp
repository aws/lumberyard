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

#include "OperatorArithmetic.h"
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Core/Contracts/MathOperatorContract.h>
#include <AzCore/Math/MathUtils.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            void OperatorArithmetic::ConfigureContracts(SourceType sourceType, AZStd::vector<ContractDescriptor>& contractDescs)
            {
                if (sourceType == SourceType::SourceInput
                    || sourceType == SourceType::SourceOutput)
                {
                    ContractDescriptor operatorMethodContract;
                    operatorMethodContract.m_createFunc = [this]() -> MathOperatorContract* { 
                        auto mathContract = aznew MathOperatorContract(OperatorFunction().data());
                        mathContract->SetSupportedNativeTypes(GetSupportedNativeDataTypes());
                        return mathContract;
                    };
                    contractDescs.push_back(AZStd::move(operatorMethodContract));
                }
            }

            void OperatorArithmetic::OnSourceTypeChanged()
            {
            }

            void OperatorArithmetic::OnSourceConnected(const SlotId& sourceSlotId)
            {
                Data::Type type = Data::FromAZType(m_sourceTypes[0]);
                AZStd::string dataTypeName = Data::GetName(type);

                SetSourceNames(dataTypeName, "Result");

                HandleDynamicSlots();
            }

            void OperatorArithmetic::OnSourceDisconnected(const SlotId& sourceSlotId)
            {
                if (!HasSourceConnection())
                {
                    SetSourceNames("Value", "Result");
                }
                else
                {
                    Slot* sourceSlot = GetSlot(sourceSlotId);

                    if (sourceSlot->GetType() == SlotType::DataIn)
                    {
                        if (!m_inputSlots.empty())
                        {
                            for (auto inputSlotId : m_inputSlots)
                            {
                                if (!IsConnected(inputSlotId))
                                {
                                    if (RemoveSlot(inputSlotId))
                                    {
                                        m_inputSlots.erase(inputSlotId);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            void OperatorArithmetic::InvokeOperator()
            {
                if (m_sourceTypes.empty())
                {
                    return;
                }
                    
                AZStd::vector<const Datum*> operands;

                for (const SlotId& inputSlot : m_applicableInputSlots)
                {
                    const Datum* datum = GetInput(inputSlot);

                    if (datum->GetType().IsValid())
                    {
                        operands.push_back(datum);
                    }
                }

                Datum result;
                Evaluate(operands, result);

                PushOutput(result, *GetSlot(m_outputSlot));

                SignalOutput(GetSlotId("Out"));
            }

            void OperatorArithmetic::OnDataInputSlotConnected(const SlotId& slotId, const Endpoint& endpoint)
            {
                if (!m_unary)
                {
                    HandleDynamicSlots();
                }
            }

            void OperatorArithmetic::OnDataInputSlotDisconnected(const SlotId& slotId, const Endpoint& endpoint)
            {
                if (RemoveSlot(slotId))
                {
                    m_inputSlots.erase(slotId);
                }
            }

            void OperatorArithmetic::OnInputChanged(const Datum& input, const SlotId& slotID)
            {
            }

            void OperatorArithmetic::Evaluate(const AZStd::vector<const Datum*>& operands, Datum& result)
            {
                if (operands.empty())
                {
                    return;
                }

                const Datum* operand = operands.front();

                if (operands.size() == 1)
                {
                    result = (*operand);
                    return;
                }
                
                auto type = operand->GetType();

                if (type.GetType() != Data::eType::BehaviorContextObject)
                {
                    Operator(type.GetType(), operands, result);
                }
            }

            void OperatorArithmetic::Operator(Data::eType type, const AZStd::vector<const Datum*>& operands, Datum& result)
            {
                AZ_UNUSED(type);
                AZ_UNUSED(operands);
                AZ_UNUSED(result);
            }

            void OperatorArithmetic::OnInputSignal(const SlotId& slotId)
            {
                const SlotId inSlotId = OperatorBaseProperty::GetInSlotId(this);
                if (slotId == inSlotId)
                {
                    if (m_applicableInputSlots.empty())
                    {
                        for (const SlotId& sourceSlotId : GetSourceSlots())
                        {
                            Slot* slot = GetSlot(sourceSlotId);

                            if (slot->GetType() == SlotType::DataOut)
                            {
                                if (!m_outputSlot.IsValid())
                                {
                                    m_outputSlot = sourceSlotId;
                                }

                                continue;
                            }

                            if (IsConnected(sourceSlotId) || IsValidArithmeticSlot(sourceSlotId))
                            {
                                m_applicableInputSlots.emplace_back(sourceSlotId);
                            }
                        }

                        for (const SlotId& inputSlot : m_inputSlots)
                        {
                            if (IsConnected(inputSlot) || IsValidArithmeticSlot(inputSlot))
                            {
                                m_applicableInputSlots.emplace_back(inputSlot);
                            }
                        }

                        if (!m_outputSlot.IsValid())
                        {
                            for (const SlotId& outputSlotId : m_outputSlots)
                            {
                                m_outputSlot = outputSlotId;

                                if (m_outputSlot.IsValid())
                                {
                                    break;
                                }
                            }
                        }
                    }

                    InvokeOperator();
                }
            }

            void OperatorArithmetic::SetSourceNames(const AZStd::string& inputName, const AZStd::string& outputName)
            {
                for (const auto& sourceSlotId : GetSourceSlots())
                {
                    Slot* sourceSlot = GetSlot(sourceSlotId);

                    if (sourceSlot)
                    {
                        switch (sourceSlot->GetType())
                        {
                        case SlotType::DataIn:
                            sourceSlot->Rename(inputName);
                            break;
                        case SlotType::DataOut:
                            sourceSlot->Rename(outputName);
                            break;
                        default:
                            AZ_Assert("ScriptCanvas", "Unknown Source Slot type for Arithmetic Operator. Cannot perform rename.");
                            break;
                        }
                    }
                }
            }

            void OperatorArithmetic::HandleDynamicSlots()
            {
                if (AreSourceSlotsFull(OperatorBase::SourceType::SourceInput) && !m_unary)
                {
                    bool inputsFull = true;

                    for (auto inputSlotId : m_inputSlots)
                    {
                        if (!IsConnected(inputSlotId))
                        {
                            inputsFull = false;
                            break;
                        }
                    }

                    if (inputsFull)
                    {
                        AddSlotWithSourceType();
                    }
                }
            }

            bool OperatorArithmetic::IsValidArithmeticSlot(const SlotId& slotId) const
            {
                return true;
            }
        }
    }
}

#include <Include/ScriptCanvas/Libraries/Operators/Math/OperatorArithmetic.generated.cpp>