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

#define ALLOW_CVAR_FLOWNODE  // uncomment this to recover the functionality

class CFlowNode_CVar
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowNode_CVar(SActivationInfo* pActInfo) {}

    enum EInPorts
    {
        SET = 0,
        GET,
        RESET,
        NAME,
        VALUE,
    };
    enum EOutPorts
    {
        CURVALUE = 0,
    };

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig<SFlowSystemVoid>("Set", _HELP("Trigger to Set CVar's value")),
            InputPortConfig<SFlowSystemVoid>("Get", _HELP("Trigger to Get CVar's value")),
            InputPortConfig<bool>("ResetOnGameEnd", _HELP("Reset the cvar on (level/game) exit")),
            InputPortConfig<string>("CVar", _HELP("Name of the CVar to set/get [case-INsensitive]")),
            InputPortConfig<string>("Value", _HELP("Value of the CVar to set")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig<string>("CurValue", _HELP("Current Value of the CVar [Triggered on Set/Get]")),
            {0}
        };
#ifdef ALLOW_CVAR_FLOWNODE
        config.sDescription = _HELP("Sets/Gets the value of a console variable (CVar).");
#else
        config.sDescription = _HELP("---- this node is currently disabled in code ------");
#endif
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_DEBUG);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
#if defined(ALLOW_CVAR_FLOWNODE)
        if (event == eFE_Activate)
        {
            const bool isSet = (IsPortActive(pActInfo, SET));
            const bool isGet = (IsPortActive(pActInfo, GET));
            if (!isGet && !isSet)
            {
                return;
            }

            const string& cvar = GetPortString(pActInfo, NAME);
            ICVar* pICVar = gEnv->pConsole->GetCVar(cvar.c_str());
            if (pICVar != 0)
            {
                if (isSet)
                {
                    const string& val = GetPortString(pActInfo, VALUE);
                    m_previousValue = string(pICVar->GetString());
                    pICVar->Set(val.c_str());
                }
                const string curVal = pICVar->GetString();
                ActivateOutput(pActInfo, CURVALUE, curVal);
            }
            else
            {
                CryLogAlways("[flow] Cannot find console variable '%s'", cvar.c_str());
            }
        }
        else if (event == eFE_Uninitialize)
        {
            // Reset the cvar back to what it used to be before this node executed.
            ICVar* pICVar = gEnv->pConsole->GetCVar(GetPortString(pActInfo, NAME).c_str());
            if (pICVar != 0)
            {
                if (IsPortActive(pActInfo, SET) && GetPortBool(pActInfo, RESET))
                {
                    pICVar->Set(m_previousValue.c_str());
                }
            }
        }
#endif
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    string m_previousValue;
};

REGISTER_FLOW_NODE("Debug:ConsoleVariable", CFlowNode_CVar);

