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

#include <Libraries/Logic/IsNull.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <Include/ScriptCanvas/Libraries/Logic/IsNull.generated.cpp>
#include <ScriptCanvas/Core/Contracts/IsReferenceTypeContract.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            IsNull::IsNull()
                : Node()
            {}

            void IsNull::OnInit()
            {
                AZStd::vector<ContractDescriptor> contracts;
                auto func = []() { return aznew IsReferenceTypeContract(); };
                ContractDescriptor descriptor{ AZStd::move(func) };
                contracts.emplace_back(descriptor);

                AddInputDatumOverloadedSlot("Reference", "", contracts);
                AddOutputTypeSlot("Is Null", "", AZStd::move(Data::Type::Boolean()), OutputStorage::Optional);
            }

            void IsNull::OnInputSignal(const SlotId&)
            {
                const bool isNull = GetInput(GetSlotId("Reference"))->Empty();
                PushOutput(Datum(isNull), *GetSlot(GetSlotId("Is Null")));
                SignalOutput(isNull ? IsNullProperty::GetTrueSlotId(this) : IsNullProperty::GetFalseSlotId(this));
            }
        }
    }
}