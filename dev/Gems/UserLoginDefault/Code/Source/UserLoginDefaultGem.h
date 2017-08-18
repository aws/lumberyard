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

#include <IGem.h>
#include <IPlatformOS.h>

namespace UserLoginDefault
{
    class UserLoginDefaultGem
        : public CryHooksModule
        , public IPlatformOS::IPlatformListener
    {
    public:
        AZ_RTTI(UserLoginDefaultGem, "{1E91EADA-7278-485B-BCB7-2ABC530B6369}", CryHooksModule);

        // CrySystemEventBus
        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams) override;
        void OnCrySystemShutdown(ISystem& system) override;
        // ~CrySystemEventBus

        // IPlatformOS::IPlatformListener
        void OnPlatformEvent(const IPlatformOS::SPlatformEvent& event) override;
        // ~IPlatformOS::IPlatformListener

        void UserLogin(unsigned int user);
    };
} // namespace UserLoginDefault
