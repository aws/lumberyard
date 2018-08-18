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

//////////////////////////////////////////////////////////////////////////
class CFlowNode_Indexer
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        InMax = 8,  // max number of value input ports
    };

    enum OutputPorts
    {
        OutIndex,
    };

public:
    CFlowNode_Indexer(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config [] =
        {
            InputPortConfig_AnyType("In0", _HELP("Input 0")),
            InputPortConfig_AnyType("In1", _HELP("Input 1")),
            InputPortConfig_AnyType("In2", _HELP("Input 2")),
            InputPortConfig_AnyType("In3", _HELP("Input 3")),
            InputPortConfig_AnyType("In4", _HELP("Input 4")),
            InputPortConfig_AnyType("In5", _HELP("Input 5")),
            InputPortConfig_AnyType("In6", _HELP("Input 6")),
            InputPortConfig_AnyType("In7", _HELP("Input 7")),
            {0}
        };
        static const SOutputPortConfig out_config [] =
        {
            OutputPortConfig<int>("OutIndex", _HELP("Index of the input port which was triggered (0-7)")),
            {0}
        };
        config.sDescription = _HELP("Whenever an [In#] port is activated, [OutIndex] returns the index of the activated port (0-7).\nWARNING: Does not account for multiple activations on different ports!");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        {
            int out = 0;
            for (int i = 0; i < InputPorts::InMax; i++)
            {
                if (IsPortActive(pActInfo, i))
                {
                    out = i;
                    break;
                }
            }
            ActivateOutput(pActInfo, OutputPorts::OutIndex, out);
            break;
        }
        case eFE_Initialize:
        {
            ActivateOutput(pActInfo, OutputPorts::OutIndex, 0);
            break;
        }
        }
        ;
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
/// Logic Indexer Flow Node Registration
//////////////////////////////////////////////////////////////////////////

REGISTER_FLOW_NODE("Logic:Indexer", CFlowNode_Indexer);
