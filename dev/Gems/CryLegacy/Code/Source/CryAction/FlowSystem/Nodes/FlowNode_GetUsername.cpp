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
#include "CryLegacy_precompiled.h"
#include <cstdlib>

#include "FlowNode_GetUsername.h"

REGISTER_FLOW_NODE("Game:GetUsername", FlowNode_GetUsername);

FlowNode_GetUsername::FlowNode_GetUsername(SActivationInfo* activationInfo)
{
}

void FlowNode_GetUsername::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig inputs[] =
    {
        InputPortConfig_Void("GetUsername", _HELP("Causes the node to execute")),
        { 0 }
    };
    static const SOutputPortConfig outputs[] =
    {
        OutputPortConfig<string>("Username", _HELP("The system username as a string")),
        OutputPortConfig<string>("Error", _HELP("Fires if the username could not be found")),
        { 0 }
    };

    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Get the currently logged in username from the environment (Currently only supported on Windows, Mac, and IOS)");
    config.SetCategory(EFLN_APPROVED);
}

void FlowNode_GetUsername::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
{
#if defined(WIN64)
    if (event == eFE_Activate && IsPortActive(pActInfo, EIP_GetUsername))
    {
        char* pUsername = nullptr;

        _dupenv_s(&pUsername, 0, "USER");
        if (pUsername == nullptr || *pUsername == 0)
        {
            _dupenv_s(&pUsername, 0, "USERNAME");
            if (pUsername == nullptr || *pUsername == 0)
            {
                _dupenv_s(&pUsername, 0, "COMPUTERNAME");
                if (pUsername == nullptr || *pUsername == 0)
                {
                    ActivateOutput<string>(pActInfo, EOP_Error, "Unable to find USER, USERNAME or COMPUTERNAME in environment");
                    return;
                }
            }
        }

        ActivateOutput<string>(pActInfo, EOP_Username, pUsername);
    }
#elif defined(MAC) || defined(IOS)
    if (event == eFE_Activate && IsPortActive(pActInfo, EIP_GetUsername))
    {
        char* pUsername = nullptr;

        pUsername = getenv("USER");
        if (pUsername == nullptr || *pUsername == 0)
        {
            pUsername = getenv("USERNAME");
            if (pUsername == nullptr || *pUsername == 0)
            {
                pUsername = getenv("COMPUTERNAME");
                if (pUsername == nullptr || *pUsername == 0)
                {
                    ActivateOutput<string>(pActInfo, EOP_Error, "Unable to find USER, USERNAME or COMPUTERNAME in environment");
                    return;
                }
            }
        }

        ActivateOutput<string>(pActInfo, EOP_Username, pUsername);
    }
#else
    CRY_ASSERT_MESSAGE(false, "GetUsername Flow Node not currently supported on this platform.");
    ActivateOutput<string>(pActInfo, EOP_Error, "GetUsername Flow Node not currently supported on this platform.");
#endif

}


