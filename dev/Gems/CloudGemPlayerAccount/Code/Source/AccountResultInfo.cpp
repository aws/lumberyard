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
#include "CloudGemPlayerAccount_precompiled.h"

#include "AccountResultInfo.h"

#include <AzCore/base.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace CloudGemPlayerAccount
{
    void AccountResultInfo::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<AccountResultInfo>()
                ->Version(1);
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<AccountResultInfo>()
                ->Property("requestId", BehaviorValueGetter(&AccountResultInfo::requestId), nullptr)
                ->Property("wasSuccessful", BehaviorValueGetter(&AccountResultInfo::wasSuccessful), nullptr)
                ->Property("errorTypeName", BehaviorValueGetter(&AccountResultInfo::errorTypeName), nullptr)
                ->Property("errorMessage", BehaviorValueGetter(&AccountResultInfo::errorMessage), nullptr)
                ;
        }
    }
}   // namespace CloudGemPlayerAccount
