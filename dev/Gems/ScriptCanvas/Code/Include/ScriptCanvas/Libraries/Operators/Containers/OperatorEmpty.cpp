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

#include "OperatorEmpty.h"
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Core/Contracts/SupportsMethodContract.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            void OperatorEmpty::ConfigureContracts(SourceType sourceType, AZStd::vector<ContractDescriptor>& contractDescs)
            {
                if (sourceType == SourceType::SourceInput)
                {
                    ContractDescriptor supportsMethodContract;
                    supportsMethodContract.m_createFunc = [this]() -> SupportsMethodContract* { return aznew SupportsMethodContract("Size"); };
                    contractDescs.push_back(AZStd::move(supportsMethodContract));
                }
            }

            void OperatorEmpty::OnSourceTypeChanged()
            {
                m_outputSlots.insert(AddSlot("True", "The container is empty", SlotType::ExecutionOut));
                m_outputSlots.insert(AddSlot("False", "The container is not empty", SlotType::ExecutionOut));
                m_outputSlots.insert(AddOutputTypeSlot("Is Empty", "True if the container is empty, false if it's not.", Data::Type::Boolean(), Node::OutputStorage::Required, false));
            }

            void OperatorEmpty::InvokeOperator()
            {
                const SlotSet& slotSets = GetSourceSlots();

                if (!slotSets.empty())
                {
                    SlotId sourceSlotId = (*slotSets.begin());

                    if (Data::IsVectorContainerType(GetSourceAZType()) || Data::IsMapContainerType(GetSourceAZType()))
                    {
                        const Datum* containerDatum = GetInput(sourceSlotId);

                        if (containerDatum && !containerDatum->Empty())
                        {
                            // Is the container empty?
                            auto emptyOutcome = BehaviorContextMethodHelper::CallMethodOnDatum(*containerDatum, "Empty");
                            if (!emptyOutcome)
                            {
                                SCRIPTCANVAS_REPORT_ERROR((*this), "Failed to call Empty on container: %s", emptyOutcome.GetError().c_str());
                                return;
                            }

                            Datum emptyResult = emptyOutcome.TakeValue();
                            bool isEmpty = *emptyResult.GetAs<bool>();

                            PushOutput(Datum(isEmpty), *GetSlot(GetSlotId("Is Empty")));

                            if (isEmpty)
                            {
                                SignalOutput(GetSlotId("True"));
                            }
                            else
                            {
                                SignalOutput(GetSlotId("False"));
                            }
                        }
                    }
                    else
                    {
                        // This should not be possible because we use a contract to verify if the Size method is available
                        SCRIPTCANVAS_REPORT_ERROR((*this), "The Empty node only works on containers");
                    }
                }

                SignalOutput(GetSlotId("Out"));
            }

            void OperatorEmpty::OnInputSignal(const SlotId& slotId)
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

#include <Include/ScriptCanvas/Libraries/Operators/Containers/OperatorEmpty.generated.cpp>