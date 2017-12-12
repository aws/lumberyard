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
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/string/string.h>

namespace CloudGemPlayerAccount
{
    // Groups together information common to registration and login responses
    class BasicResultInfo
    {

    public:
        AZ_TYPE_INFO(BasicResultInfo, "{3556096E-6225-46C3-A845-0909924ED7E1}")
        AZ_CLASS_ALLOCATOR(BasicResultInfo, AZ::SystemAllocator, 0)

        static void Reflect(AZ::ReflectContext* context);

        // Most calls to CloudGemPlayerAccountRequestBus will return the requestId for the background task that was created.
        // This requestId field can be used to correlate the response to the request that created it.
        AZ::u32 requestId;

        bool wasSuccessful;
        AZStd::string username;

        // Error information. Only valid if wasSuccessful is false.
        AZStd::string errorTypeName;
        // If >= 0, errorTypeValue corresponds to a CognitoIdentityProviderErrors value. 
        // Otherwise, there was an unexpected error (most likely the user needs to sign in for the operation). In this case, see errorTypeName for more details.
        int errorTypeValue;
        AZStd::string errorMessage;
    };
}   // namespace CloudGemPlayerAccount