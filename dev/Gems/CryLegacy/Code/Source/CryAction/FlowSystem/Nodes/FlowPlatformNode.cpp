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

#include "CryLegacy_precompiled.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>

class CFlowNode_Platform
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_Platform(SActivationInfo* pActInfo) {}

    enum EInPort
    {
        eInPort_Get = 0
    };
    enum EOutPort
    {
        eOutPort_Pc = 0,
        eOutPort_Mac,
        eOutPort_PS4, // ACCEPTED_USE
        eOutPort_XboxOne, // ACCEPTED_USE
        eOutPort_Android,
        eOutPort_iOS,
        eOutPort_AppleTV
    };

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig_AnyType("Check", _HELP("Triggers a check of the current platform")),
            {0}
        };
        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig_AnyType("PC", _HELP("Outputs the signal from Check input if the game runs on PC")),
            OutputPortConfig_AnyType("Mac", _HELP("Outputs the signal from Check input if the game runs on Mac")),
            OutputPortConfig_AnyType("PS4", _HELP("Outputs the signal from Check input if the game runs on PS4")), // ACCEPTED_USE
            OutputPortConfig_AnyType("XboxOne", _HELP("Outputs the signal from Check input if the game runs on Xbox One")), // ACCEPTED_USE
            OutputPortConfig_AnyType("Android", _HELP("Outputs the signal from Check input if the game runs on Android")),
            OutputPortConfig_AnyType("iOS", _HELP("Outputs the signal from Check input if the game runs on iOS")),
            OutputPortConfig_AnyType("AppleTV", _HELP("Outputs the signal from Check input if the game runs on AppleTV")),
            {0}
        };
        config.sDescription = _HELP("Provides branching logic for different platforms");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate)
        {
            if (!IsPortActive(pActInfo, eInPort_Get))
            {
                return;
            }
#if defined(WIN32) || defined(WIN64)
            ActivateOutput(pActInfo, eOutPort_Pc, GetPortAny(pActInfo, eInPort_Get));
#elif defined(AZ_PLATFORM_APPLE_OSX)
            ActivateOutput(pActInfo, eOutPort_Mac, GetPortAny(pActInfo, eInPort_Get));
#elif defined(ANDROID)
            ActivateOutput(pActInfo, eOutPort_Android, GetPortAny(pActInfo, eInPort_Get));
#elif defined(IOS)
            ActivateOutput(pActInfo, eOutPort_iOS, GetPortAny(pActInfo, eInPort_Get));
#elif defined(APPLETV)
            ActivateOutput(pActInfo, eOutPort_AppleTV, GetPortAny(pActInfo, eInPort_Get));
#elif defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(FlowPlatformNode_cpp, AZ_RESTRICTED_PLATFORM)
#endif
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

REGISTER_FLOW_NODE("Game:CheckPlatform", CFlowNode_Platform);

