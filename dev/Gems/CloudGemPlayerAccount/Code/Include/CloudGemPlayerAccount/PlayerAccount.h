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
    class PlayerAccount
    {
    public:
        AZ_TYPE_INFO(PlayerAccount, "{CCD458BE-0D5E-49D3-9DB1-9E86625034A5}")
        AZ_CLASS_ALLOCATOR(PlayerAccount, AZ::SystemAllocator, 0)

        static void Reflect(AZ::ReflectContext* context);

        PlayerAccount();

        const AZStd::string& GetPlayerName() const;
        void SetPlayerName(const AZStd::string& playerName);

    protected:
        AZStd::string m_playerName;
    };

}   // namespace CloudGemPlayerAccount