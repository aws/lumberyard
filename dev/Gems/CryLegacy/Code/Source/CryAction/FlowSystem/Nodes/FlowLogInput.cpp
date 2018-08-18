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

class CFlowLogInput
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CFlowLogInput(SActivationInfo* pActInfo) {}

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    //////////////////////////////////////////////////////////////////////////
    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig_AnyType("in", "Any value to be logged"),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            {0}
        };

        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_ADVANCED);
    }
    //////////////////////////////////////////////////////////////////////////
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate && IsPortActive(pActInfo, 0))
        {
            //          const TFlowInputData &in = pActInfo->pInputPorts[0];
            string strValue;
            pActInfo->pInputPorts[0].GetValueWithConversion(strValue);
            const char* sName = pActInfo->pGraph->GetNodeName(pActInfo->myID);
            CryLogAlways("[flow-log] Node%s: Value=%s", sName, strValue.c_str());
            /*
            switch (in.GetType())
            {
            case eFDT_Bool:
                port.pVar->Set( *pPort->defaultData.GetPtr<bool>() );
                break;
            case eFDT_Int:
                port.pVar->Set( *pPort->defaultData.GetPtr<int>() );
                break;
            case eFDT_Float:
                port.pVar->Set( *pPort->defaultData.GetPtr<float>() );
                break;
            case eFDT_EntityId:
                port.pVar->Set( *pPort->defaultData.GetPtr<int>() );
                break;
            case eFDT_Vec3:
                port.pVar->Set( *pPort->defaultData.GetPtr<Vec3>() );
                break;
            case eFDT_String:
                port.pVar->Set( (pPort->defaultData.GetPtr<string>())->c_str() );
                break;
            default:
                CryLogAlways( "Unknown Type" );
            }
            */
        }
    }
    //////////////////////////////////////////////////////////////////////////
};

FLOW_NODE_BLACKLISTED("Log:LogInput", CFlowLogInput)
