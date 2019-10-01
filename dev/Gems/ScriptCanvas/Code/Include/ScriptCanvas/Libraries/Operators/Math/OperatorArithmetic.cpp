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

#include <ScriptCanvas/Utils/SerializationUtils.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            bool OperatorArithmetic::OperatorArithmeticVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& classElement)
            {
                if (classElement.GetVersion() < Version::RemoveOperatorBase)
                {
                    // Remove OperatorBase
                    if (!SerializationUtils::RemoveBaseClass(serializeContext, classElement))
                    {
                        return false;
                    }

                    classElement.RemoveElementByName(AZ_CRC("m_unary", 0x1ddbd40d));
                }

                return true;
            }

            void OperatorArithmetic::OnDynamicGroupDisplayTypeChanged(const AZ::Crc32& dynamicGroup, const Data::Type& dataType)
            {
                if (dynamicGroup == GetArithmeticDynamicTypeGroup())
                {
                    UpdateArithmeticSlotNames();

                    if (dataType.IsValid())
                    {
                        AZStd::vector< Slot* > groupedSlots = GetSlotsWithDynamicGroup(GetArithmeticDynamicTypeGroup());

                        for (Slot* slot : groupedSlots)
                        {
                            if (slot->IsInput())
                            {
                                Datum* newDatum = ModInput(slot->GetId());
                                InitializeDatum(newDatum, dataType);
                            }
                        }
                    }                    
                }
            }

            void OperatorArithmetic::OnInputSignal(const SlotId& slotId)
            {
                const SlotId inSlotId = OperatorBaseProperty::GetInSlotId(this);
                if (slotId == inSlotId)
                {
                    if (m_applicableInputSlots.empty())
                    {
                        AZStd::vector< Slot* > groupedSlots = GetSlotsWithDynamicGroup(GetArithmeticDynamicTypeGroup());

                        for (Slot* groupedSlot : groupedSlots)
                        {
                            if (groupedSlot->IsInput())
                            {
                                SlotId groupedSlotId = groupedSlot->GetId();

                                if (IsConnected(groupedSlotId) || IsValidArithmeticSlot(groupedSlotId))
                                {
                                    m_applicableInputSlots.emplace_back(groupedSlotId);
                                }
                            }
                            else if (groupedSlot->IsOutput())
                            {
                                m_outputSlot = groupedSlot->GetId();                                
                            }
                        }
                    }

                    InvokeOperator();
                }
            }

            void OperatorArithmetic::OnConfigured()
            {
                auto slotList = GetSlotsWithDynamicGroup(GetArithmeticDynamicTypeGroup());

                // If we have no dynamically grouped slots. Add in our defaults.
                if (slotList.empty())
                {
                    CreateSlot("Value", "An operand to use in performing the specified Operation", ConnectionType::Input);
                    CreateSlot("Value", "An operand to use in performing the specified Operation", ConnectionType::Input);

                    CreateSlot("Result", "The result of the specified operation", ConnectionType::Output);
                }
            }

            void OperatorArithmetic::OnInit()
            {
                // Version conversion for previous elements                
                for (Slot& currentSlot : ModSlots())
                {                    
                    if (currentSlot.IsData())
                    {
                        auto& contracts = currentSlot.GetContracts();

                        for (auto& currentContract : contracts)
                        {
                            MathOperatorContract* contract = azrtti_cast<MathOperatorContract*>(currentContract.get());

                            if (contract)
                            {
                                if (contract->HasOperatorFunction())
                                {
                                    contract->SetSupportedOperator(OperatorFunction());
                                    contract->SetSupportedNativeTypes(GetSupportedNativeDataTypes());
                                }
                            }                            
                        }

                        if (!currentSlot.IsDynamicSlot())
                        {
                            currentSlot.SetDynamicDataType(DynamicDataType::Value);
                        }

                        currentSlot.SetDisplayGroup(GetArithmeticDisplayGroup());

                        if (currentSlot.GetDynamicGroup() != GetArithmeticDynamicTypeGroup())
                        {
                            SetDynamicGroup(currentSlot.GetId(), GetArithmeticDynamicTypeGroup());
                        }
                    }
                }
            }

            bool OperatorArithmetic::IsNodeExtendable() const
            {
                return true;
            }

            int OperatorArithmetic::GetNumberOfExtensions() const
            {
                return 1;
            }

            ExtendableSlotConfiguration OperatorArithmetic::GetExtensionConfiguration(int extensionIndex) const
            {
                ExtendableSlotConfiguration configuration;

                if (extensionIndex == 0)
                {
                    configuration.m_name = "Add Operand";
                    configuration.m_tooltip = "Adds a new operand for the operator";

                    configuration.m_displayGroup = GetArithmeticDisplayGroup();
                    configuration.m_identifier = GetArithmeticExtensionId();

                    configuration.m_connectionType = ConnectionType::Input;
                }

                return configuration;
            }

            SlotId OperatorArithmetic::HandleExtension(AZ::Crc32 extensionId)
            {
                if (extensionId == GetArithmeticExtensionId())
                {
                    SlotId retVal = CreateSlot("Value", "An operand to use in performing the specified Operation", ConnectionType::Input);
                    UpdateArithmeticSlotNames();

                    return retVal;
                }

                return SlotId();
            }

            bool OperatorArithmetic::CanDeleteSlot(const SlotId& slotId) const
            {
                Slot* slot = GetSlot(slotId);            

                if (slot->GetDynamicGroup() == GetArithmeticDynamicTypeGroup())
                {
                    if (!slot->IsOutput())
                    {                        
                        auto slotList = GetSlotsWithDynamicGroup(GetArithmeticDynamicTypeGroup());

                        int inputCount = 0;

                        for (Slot* slotElement : slotList)
                        {
                            if (slotElement->IsInput())
                            {
                                ++inputCount;
                            }
                        }

                        // Only allow removal if our input count if greater then 2 to maintain our
                        // visual expectation.
                        return inputCount > 2;
                    }
                }

                return false;
            }

            void OperatorArithmetic::Evaluate(const ArithmeticOperands& operands, Datum& result)
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

            void OperatorArithmetic::Operator(Data::eType type, const ArithmeticOperands& operands, Datum& result)
            {
                AZ_UNUSED(type);
                AZ_UNUSED(operands);
                AZ_UNUSED(result);
            }

            void OperatorArithmetic::InitializeDatum(Datum* datum, const Data::Type& dataType)
            {
                AZ_UNUSED(datum);
                AZ_UNUSED(dataType);
            }

            void OperatorArithmetic::InvokeOperator()
            {
                Data::Type displayType = GetDisplayType(GetArithmeticDynamicTypeGroup());

                if (!displayType.IsValid())
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

            SlotId OperatorArithmetic::CreateSlot(AZStd::string_view name, AZStd::string_view toolTip, ConnectionType connectionType)
            {
                DynamicDataSlotConfiguration slotConfiguration;

                slotConfiguration.m_name = name;
                slotConfiguration.m_toolTip = toolTip;
                slotConfiguration.SetConnectionType(connectionType);

                ContractDescriptor operatorMethodContract;
                operatorMethodContract.m_createFunc = [this]() -> MathOperatorContract* {
                    auto mathContract = aznew MathOperatorContract(OperatorFunction().data());
                    mathContract->SetSupportedNativeTypes(GetSupportedNativeDataTypes());
                    return mathContract;
                };

                slotConfiguration.m_contractDescs.push_back(AZStd::move(operatorMethodContract));

                slotConfiguration.m_displayGroup = GetArithmeticDisplayGroup();
                slotConfiguration.m_dynamicGroup = GetArithmeticDynamicTypeGroup();
                slotConfiguration.m_dynamicDataType = DynamicDataType::Any;

                slotConfiguration.m_addUniqueSlotByNameAndType = false;

                SlotId slotId = AddSlot(slotConfiguration);

                Datum* newDatum = ModInput(slotId);
                InitializeDatum(newDatum, GetDisplayType(GetArithmeticDynamicTypeGroup()));

                return slotId;
            }

            void OperatorArithmetic::UpdateArithmeticSlotNames()
            {
                Data::Type displayType = GetDisplayType(GetArithmeticDynamicTypeGroup());

                if (displayType.IsValid())
                {
                    AZStd::string dataTypeName = Data::GetName(displayType);
                    SetSourceNames(dataTypeName, "Result");
                }
                else
                {
                    SetSourceNames("Value", "Result");
                }
            }

            void OperatorArithmetic::SetSourceNames(const AZStd::string& inputName, const AZStd::string& outputName)
            {
                AZStd::vector< Slot* > groupedSlots = GetSlotsWithDynamicGroup(GetArithmeticDynamicTypeGroup());
                for (Slot* slot : groupedSlots)
                {
                    if (slot)
                    {
                        if (!slot->IsData())
                        {
                            AZ_Assert(false, "%s", "Unknown Source Slot type for Arithmetic Operator. Cannot perform rename.");
                        }
                        else if (slot->IsInput())
                        {
                            slot->Rename(inputName);
                        }
                        else if (slot->IsOutput())
                        {
                            slot->Rename(outputName);
                        }
                    }
                }
            }

            bool OperatorArithmetic::IsValidArithmeticSlot(const SlotId& slotId) const
            {
                const Datum* datum = GetInput(slotId);
                return (datum != nullptr);
            }
        }
    }
}

#include <Include/ScriptCanvas/Libraries/Operators/Math/OperatorArithmetic.generated.cpp>