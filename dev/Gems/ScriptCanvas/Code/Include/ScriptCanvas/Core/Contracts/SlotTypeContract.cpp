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

#include "SlotTypeContract.h"
#include <ScriptCanvas/Core/ContractBus.h>
#include <ScriptCanvas/Core/Slot.h>

namespace ScriptCanvas
{
    static const char* GetConnectionFailureReason(SlotType sourceType, SlotType targetType)
    {
        if (SlotUtils::IsData(sourceType) != SlotUtils::IsData(targetType) || SlotUtils::IsExecution(sourceType) != SlotUtils::IsExecution(targetType))
        {
            return "Cannot connect execution slots to data slots or vice-versa";
        }
        else if (SlotUtils::IsInput(sourceType) == SlotUtils::IsInput(targetType))
        {
            return "Cannot connect input slots to other input slots";
        }
        else if (SlotUtils::IsOutput(sourceType) == SlotUtils::IsOutput(targetType))
        {
            return "Cannot connect output slots to other output slots";
        }
        else
        {
            AZ_Assert(!SlotUtils::CanConnect(sourceType, targetType), "no failure reason for connectable types");
            return "";
        }
    }

    AZ::Outcome<void, AZStd::string> SlotTypeContract::OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const
    {
        const auto sourceType = sourceSlot.GetType();
        const auto targetType = targetSlot.GetType();
        
        if (SlotUtils::CanConnect(sourceType, targetType))
        {
            return AZ::Success();
        }
        
        return AZ::Failure(AZStd::string::format
            ( "Connection cannot be created between source slot \"%s\" and target slot \"%s\", %s. (%s)"
            , sourceSlot.GetName().data()
            , targetSlot.GetName().data()
            , GetConnectionFailureReason(sourceType, targetType)
            , RTTI_GetTypeName()));
    }

    void SlotTypeContract::Reflect(AZ::ReflectContext* reflection)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Class<SlotTypeContract, Contract>()
                ->Version(0)
                ;
        }
    }
}
