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
#include "precompiled.h"
#include "SlotTypeContract.h"
#include <ScriptCanvas/Core/ContractBus.h>
#include <ScriptCanvas/Core/Slot.h>

namespace ScriptCanvas
{
    AZ::Outcome<void, AZStd::string> SlotTypeContract::OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const
    {
        bool valid = 
               (sourceSlot.GetType() == SlotType::ExecutionIn && targetSlot.GetType() == SlotType::ExecutionOut)
            || (sourceSlot.GetType() == SlotType::ExecutionOut && targetSlot.GetType() == SlotType::ExecutionIn)
            || (sourceSlot.GetType() == SlotType::DataIn && targetSlot.GetType() == SlotType::DataOut)
            || (sourceSlot.GetType() == SlotType::DataOut && targetSlot.GetType() == SlotType::DataIn);

        if (valid)
        {
            return AZ::Success();
        }

        AZStd::string reason;
        
        bool mixedExecutionData = (sourceSlot.GetType() == SlotType::ExecutionOut && targetSlot.GetType() == SlotType::DataIn)
            || (sourceSlot.GetType() == SlotType::DataIn && targetSlot.GetType() == SlotType::ExecutionOut)
            || (sourceSlot.GetType() == SlotType::DataOut && targetSlot.GetType() == SlotType::ExecutionIn)
            || (sourceSlot.GetType() == SlotType::ExecutionIn && targetSlot.GetType() == SlotType::DataOut);

        bool inputSlotConnection = (sourceSlot.GetType() == SlotType::ExecutionIn && targetSlot.GetType() == SlotType::ExecutionIn) || (sourceSlot.GetType() == SlotType::DataIn && targetSlot.GetType() == SlotType::DataIn);
        bool outputSlotConnection = (sourceSlot.GetType() == SlotType::ExecutionOut && targetSlot.GetType() == SlotType::ExecutionOut) || (sourceSlot.GetType() == SlotType::DataOut && targetSlot.GetType() == SlotType::DataOut);

        if (mixedExecutionData)
        {
            reason = "Cannot connect execution slots to data slots or vice-versa";
        }

        if (inputSlotConnection)
        {
            reason = "Cannot connect input slots to other input slots";
        }

        if (outputSlotConnection)
        {
            reason = "Cannot connect output slots to other output slots";
        }

        AZStd::string errorMessage = AZStd::string::format("Connection cannot be created between source slot \"%s\" and target slot \"%s\", %s. (%s)"
            , sourceSlot.GetName().data()
            , targetSlot.GetName().data()
            , reason.c_str()
            , RTTI_GetTypeName()
        );

        return AZ::Failure(errorMessage);

    }

    void SlotTypeContract::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<SlotTypeContract, Contract>()
                ->Version(0)
                ;
        }
    }
}
