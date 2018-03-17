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
#include <Multiplayer_precompiled.h>
#include <ISystem.h>
#include <CryString.h>
#include <Multiplayer/FlowMultiplayerNodes.h>


namespace
{
    const char* g_multiplayerIsClientNodePath = "Multiplayer:IsClient";
    const char* g_multiplayerIsServerNodePath = "Multiplayer:IsServer";
}


/*!
* FlowGraph node to check if the current network session is a client
*/
class FlowNode_IsClient
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    FlowNode_IsClient(SActivationInfo* activationInfo)
        : CFlowBaseNode()
        , m_activationInfo(*activationInfo)
    {
    }

    virtual void GetMemoryUsage(ICrySizer* sizer) const
    {
        sizer->Add(*this);
    }

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new FlowNode_IsClient(activationInfo);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Activate", _HELP("Trigger to update the output")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("True", _HELP("Network session is a client")),
            OutputPortConfig_Void("False", _HELP("Network session is not a client (dedicated)")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Check to see if the current network session is a client");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        m_activationInfo = *activationInfo;

        if (eFE_Activate == event && IsPortActive(activationInfo, InputPortActivate))
        {
            if (gEnv->IsClient() && !gEnv->IsDedicated())
            {
                ActivateOutput(&m_activationInfo, OutputPortTrue, true);
            }
            else
            {
                ActivateOutput(&m_activationInfo, OutputPortFalse, true);
            }
        }
    }

protected:

    enum InputPorts
    {
        InputPortActivate = 0,
    };

    enum OutputPorts
    {
        OutputPortTrue = 0,
        OutputPortFalse,
    };

    SActivationInfo m_activationInfo;
};


/*!
* FlowGraph node to check if the current network session is a server (hosting)
*/
class FlowNode_IsServer
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    FlowNode_IsServer(SActivationInfo* activationInfo)
        : CFlowBaseNode()
        , m_activationInfo(*activationInfo)
    {
    }

    virtual void GetMemoryUsage(ICrySizer* sizer) const
    {
        sizer->Add(*this);
    }

    IFlowNodePtr Clone(SActivationInfo*  activationInfo) override
    {
        return new FlowNode_IsServer(activationInfo);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Activate", _HELP("Trigger to update the output")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("True", _HELP("Network session is hosting (server or listenserver)")),
            OutputPortConfig_Void("False", _HELP("Network session is not hosting (client)")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Check to see if the current network session is hosting (server or listenserver) or not (client)");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        m_activationInfo = *activationInfo;

        if (eFE_Activate == event && IsPortActive(activationInfo, InputPortActivate))
        {
            if (gEnv->bServer)
            {
                ActivateOutput(&m_activationInfo, OutputPortTrue, true);
            }
            else
            {
                ActivateOutput(&m_activationInfo, OutputPortFalse, true);
            }
        }
    }

protected:

    enum InputPorts
    {
        InputPortActivate = 0,
    };

    enum OutputPorts
    {
        OutputPortTrue = 0,
        OutputPortFalse,
    };

    SActivationInfo m_activationInfo;
};

REGISTER_FLOW_NODE(g_multiplayerIsClientNodePath, FlowNode_IsClient);
REGISTER_FLOW_NODE(g_multiplayerIsServerNodePath, FlowNode_IsServer);