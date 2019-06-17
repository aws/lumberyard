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

#include "OperatorSize.h"
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Core/Contracts/SupportsMethodContract.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            void OperatorSize::ConfigureContracts(SourceType sourceType, AZStd::vector<ContractDescriptor>& contractDescs)
            {
                if (sourceType == SourceType::SourceInput)
                {
                    ContractDescriptor supportsMethodContract;
                    supportsMethodContract.m_createFunc = [this]() -> SupportsMethodContract* { return aznew SupportsMethodContract("Size"); };
                    contractDescs.push_back(AZStd::move(supportsMethodContract));
                }
            }

            void OperatorSize::OnSourceTypeChanged()
            {
                m_outputSlots.insert(AddOutputTypeSlot("Size", "The number of elements in the container", Data::Type::Number(), Node::OutputStorage::Required, false));
            }

            void OperatorSize::InvokeOperator()
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
                            // Get the size of the container
                            auto sizeOutcome = BehaviorContextMethodHelper::CallMethodOnDatum(*containerDatum, "Size");
                            if (!sizeOutcome)
                            {
                                SCRIPTCANVAS_REPORT_ERROR((*this), "Failed to get size of container: %s", sizeOutcome.GetError().c_str());
                                return;
                            }

                            // Index
                            Datum sizeResult = sizeOutcome.TakeValue();
                            const size_t* sizePtr = sizeResult.GetAs<size_t>();

                            PushOutput(sizeResult, *GetSlot(*m_outputSlots.begin()));
                        }
                        else
                        {
                            Datum zero(0);
                            PushOutput(zero, *GetSlot(*m_outputSlots.begin()));
                        }
                    }
                }

                SignalOutput(GetSlotId("Out"));
            }

            void OperatorSize::OnInputSignal(const SlotId& slotId)
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

#include <Include/ScriptCanvas/Libraries/Operators/Containers/OperatorSize.generated.cpp>