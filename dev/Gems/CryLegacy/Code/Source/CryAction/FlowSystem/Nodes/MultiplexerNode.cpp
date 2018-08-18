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
class CMultiplexer_Node
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum TriggerMode
    {
        eTM_Always = 0,
        eTM_PortsOnly,
        eTM_IndexOnly,
    };

    enum InputPorts
    {
        Index,
        Mode,
        InMax = 8,  // max number of value input nodes
    };

    enum OutputPorts
    {
        Out,
    };

public:
    CMultiplexer_Node(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config [] =
        {
            InputPortConfig<int>("Index", _HELP("Select which [In#] to send to [Out].  0 <= [Index] <= 7.")),
            InputPortConfig<int>("Mode", eTM_Always, _HELP("Always: Both [Index] and [In#] activates output.\nPortsOnly: Only [In#] activates output.\nIndexOnly: Only [Index] activates output."), 0, _UICONFIG("enum_int:Always=0,PortsOnly=1,IndexOnly=2")),
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
            OutputPortConfig_AnyType("Out", _HELP("Multiplexer output"), _HELP("Output")),
            {0}
        };
        config.sDescription = _HELP("Selects one of the [In#] inputs using [Index] and sends it to [Out].\n3 Different [Mode]s:Always: Both [Index] and [In#] activate output.\nPortsOnly: Only [In#] activate output.\nIndexOnly: Only [Index] activate output.");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED); // POLICY CHANGE: Should we output on Initialize?
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            const int mode = GetPortInt(pActInfo, InputPorts::Mode);
            if (event == eFE_Initialize && mode == eTM_Always)
            {
                return;
            }

            // initialize & always -> no trigger, why?      (ly-note: is this a bug then?)
            // initialize & ports only -> same as activate
            // initialize & index only -> same as activate

            // activate & always: trigger if In[Index] or Index is activated
            // activate & port only: trigger only if In[Index] is activated
            // activate & index only: trigger only if Index is activated

            // ly-note: we could put the [In#] ports first, then we wouldn't need to do this mask/offset.  could also allow for more inputs.
            const int port = (GetPortInt(pActInfo, InputPorts::Index) & 0x07) + 2;
            const bool bPortActive = IsPortActive(pActInfo, port);
            const bool bIndexActive = IsPortActive(pActInfo, InputPorts::Index);

            if (mode == eTM_IndexOnly && !bIndexActive)
            {
                return;
            }
            else if (mode == eTM_PortsOnly && !bPortActive)
            {
                return;
            }
            ActivateOutput(pActInfo, OutputPorts::Out, pActInfo->pInputPorts[port]);
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};

//////////////////////////////////////////////////////////////////////////
class CDeMultiplexer_Node
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum TriggerMode
    {
        eTM_Always = 0,
        eTM_InputOnly,
        eTM_IndexOnly,
    };

    enum InputPorts
    {
        Index,
        Mode,
        In,
    };

    enum OutputPorts
    {
        OutMax = 8, // max number of value output ports
    };

public:
    CDeMultiplexer_Node(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config [] =
        {
            InputPortConfig<int>("Index", _HELP("Selects [Out#] port to send [In] to.  0 <= [Index] <= 7.")),
            InputPortConfig<int>("Mode", eTM_Always, _HELP("Always: Both [In] and [Index] activates output.\nInputOnly: Only [In] activates output.\nIndexOnly: Only [Index] activates output."), 0, _UICONFIG("enum_int:Always=0,InputOnly=1,IndexOnly=2")),
            InputPortConfig_AnyType("In", _HELP("Input to send to [Out#].")),
            {0}
        };
        static const SOutputPortConfig out_config [] =
        {
            OutputPortConfig_AnyType("Out0", _HELP("Output 0")),
            OutputPortConfig_AnyType("Out1", _HELP("Output 1")),
            OutputPortConfig_AnyType("Out2", _HELP("Output 2")),
            OutputPortConfig_AnyType("Out3", _HELP("Output 3")),
            OutputPortConfig_AnyType("Out4", _HELP("Output 4")),
            OutputPortConfig_AnyType("Out5", _HELP("Output 5")),
            OutputPortConfig_AnyType("Out6", _HELP("Output 6")),
            OutputPortConfig_AnyType("Out7", _HELP("Output 7")),
            {0}
        };
        config.sDescription = _HELP("Sends the [In] input to the selected [Out#] port.\n3 different [Mode]s:\nAlways: Both [In] and [Index] activates output.\nInputOnly: Only [In] activates output.\nIndexOnly: Only [Index] activates output.");
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED); // POLICY CHANGE: Should we output on Initialize?
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        case eFE_Initialize:
        {
            const int mode = GetPortInt(pActInfo, InputPorts::Mode);
            if (event == eFE_Initialize && mode == eTM_Always)
            {
                return;
            }

            const bool bIndexActive = IsPortActive(pActInfo, InputPorts::Index);
            const bool bInputActive = IsPortActive(pActInfo, InputPorts::In);
            if (mode == eTM_IndexOnly && !bIndexActive)
            {
                return;
            }
            if (mode == eTM_InputOnly && !bInputActive)
            {
                return;
            }

            const int port = (GetPortInt(pActInfo, InputPorts::Index) & 0x07);
            ActivateOutput(pActInfo, port, GetPortAny(pActInfo, InputPorts::In));
            break;
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
/// Logic Multiplexing Flow Node Registration
//////////////////////////////////////////////////////////////////////////

REGISTER_FLOW_NODE("Logic:Multiplexer", CMultiplexer_Node);
REGISTER_FLOW_NODE("Logic:DeMultiplexer", CDeMultiplexer_Node);
