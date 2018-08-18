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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include <IGameFramework.h>
#include <platform_impl.h>

#define CRYACTION_API DLL_EXPORT

#if !defined(AZ_MONOLITHIC_BUILD) // Module init functions, only required when building as a DLL.
AZ_DECLARE_MODULE_INITIALIZATION
#endif // !defined(AZ_MONOLITHIC_BUILD)

extern "C"
{
CRYACTION_API IGameFramework* CreateGameFramework()
{
    IGameFramework* gameFramework = nullptr;
    CryGameFrameworkBus::BroadcastResult(gameFramework, &CryGameFrameworkRequests::CreateFramework);
    AZ_Assert(gameFramework, "Legacy CreateGameFramework function called, but nothing is subscribed to the CryGameFrameworkRequests.\n"
                             "Please use the Project Configurator to enable the CryLegacy gem for your project.");
    return gameFramework;
}   
}
