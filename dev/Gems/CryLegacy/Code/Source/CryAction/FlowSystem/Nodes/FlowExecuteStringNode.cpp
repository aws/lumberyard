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

class CFlowNode_ExecuteString
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_ExecuteString(SActivationInfo* pActInfo) {}

    enum EInPorts
    {
        SET = 0,
        STRING,
        NEXT_FRAME,
    };

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig<SFlowSystemVoid>("Set", _HELP("Trigger to Set CVar's value")),
            InputPortConfig<string>("String", _HELP("String you want to execute")),
            InputPortConfig<bool>("NextFrame", false, _HELP("If true it will execute the command next frame")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            {0}
        };
        config.sDescription = _HELP("Executes a string like when using the console");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_DEBUG);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate)
        {
            const bool isSet = (IsPortActive(pActInfo, SET));
            if (isSet)
            {
                const string& stringToExecute = GetPortString(pActInfo, STRING);
                const bool nextFrame = GetPortBool(pActInfo, NEXT_FRAME);
                gEnv->pConsole->ExecuteString(stringToExecute, true, nextFrame);
                ;
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

REGISTER_FLOW_NODE("Debug:ExecuteString", CFlowNode_ExecuteString);

