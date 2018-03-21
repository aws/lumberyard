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
////////////////////////////////////////////////////////////////////////////
//
//   Twitch API Nodes
//
////////////////////////////////////////////////////////////////////////////

#include "ChatPlay_precompiled.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>

#include <Broadcast/BroadcastAPI.h>
#include <ChatPlay/ChatPlayBus.h>

namespace ChatPlay
{

    /******************************************************************************/
    // Call Twitch API to GET output data through FlowGraph
    // Examples could be getting current viewers, followed channels, or stream lifetime
    // Will output the result or an error when the GET input is triggered
    class CFlowNode_TwitchAPIGet
        : public CFlowBaseNode < eNCT_Instanced >
    {
        enum InputPorts
        {
            Channel = 0,
            ApiKey,
            Get,
        };

        enum OutputPorts
        {
            Output = 0,
            Error,
        };

    public:

        explicit CFlowNode_TwitchAPIGet(IFlowNode::SActivationInfo*)
        {
        }

        ~CFlowNode_TwitchAPIGet() override
        {
        }

        IFlowNodePtr Clone(SActivationInfo* pActInfo)
        {
            return new CFlowNode_TwitchAPIGet(pActInfo);
        }

        template<class T>
        static void VerifyNodeAndCallback(TFlowGraphId graphId, TFlowNodeId nodeId, CFlowNode_TwitchAPIGet* typedNode,
            IBroadcast::ApiCallResult callResult, Aws::Http::HttpResponseCode httpResponse, const T& result)
        {
            if (gEnv && gEnv->pFlowSystem)
            {
                IFlowGraph* graph = gEnv->pFlowSystem->GetGraphById(graphId);
                if (graph)
                {
                    IFlowNodeData* nodeData = graph->GetNodeData(nodeId);
                    if (nodeData)
                    {
                        IFlowNodePtr nodePtr = nodeData->GetNode();
                        if (nodePtr && nodePtr == typedNode)
                        {
                            typedNode->OutputAPIResult(callResult, httpResponse, result);
                        }
                    }
                }
            }
        }

        void GetConfiguration(SFlowNodeConfig& config) override
        {
#ifndef CONSOLE
            EBUS_EVENT_RESULT(m_broadcastAPI, ChatPlayRequestBus, GetBroadcastAPI);
#else
            m_broadcastAPI = nullptr;
#endif // !CONSOLE
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig<string>("Channel", _HELP("Twitch channel name")),
                InputPortConfig<int>("API_Key", 0, _HELP("API call type and key to get value of"), 0, _UICONFIG(m_broadcastAPI->GetFlowNodeString())),
                InputPortConfig_AnyType("Get", _HELP("Start the API call")),
                { 0 }
            };
            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig_AnyType("Output", _HELP("Signaled with the returned value for the given key.")),
                OutputPortConfig<int>("Error", _HELP("Signaled with the HTTP reponse code from the server if the API call failed. Signalled with -1 if the API call was successful but a parsing error occurred.")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Performs a Twitch API GET call and returns the value of a given key");
            config.SetCategory(EFLN_APPROVED);
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
        {
#ifndef CONSOLE
            switch (event)
            {
            case eFE_Initialize:
                m_actInfo = *pActInfo;
                {
                    string channelId = GetPortString(pActInfo, InputPorts::Channel);
                    if (m_channelId != channelId)
                    {
                        m_channelId = channelId;
                        // on channelId change
                    }
                }

                break;

            case eFE_Activate:
                m_actInfo = *pActInfo;

                if (IsPortActive(pActInfo, InputPorts::Channel))
                {
                    string channelId = GetPortString(pActInfo, InputPorts::Channel);
                    if (m_channelId != channelId)
                    {
                        m_channelId = channelId;
                        // on channelId change
                    }
                }

                if (IsPortActive(pActInfo, InputPorts::ApiKey))
                {
                    // on API key change
                }

                if (IsPortActive(pActInfo, InputPorts::Get))
                {
                    int typeIdentifier = GetPortInt(pActInfo, InputPorts::ApiKey);
                    TFlowGraphId graphId = pActInfo->pGraph->GetGraphId();
                    TFlowNodeId nodeId = pActInfo->myID;

                    if (typeIdentifier < 100)
                    {
                        m_broadcastAPI->GetBoolValue(m_channelId.c_str(), static_cast<IBroadcast::ApiKey>(typeIdentifier), std::bind(&CFlowNode_TwitchAPIGet::VerifyNodeAndCallback<bool>, graphId, nodeId, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                    }
                    else if (typeIdentifier < 200)
                    {
                        m_broadcastAPI->GetIntValue(m_channelId.c_str(), static_cast<IBroadcast::ApiKey>(typeIdentifier), std::bind(&CFlowNode_TwitchAPIGet::VerifyNodeAndCallback<int>, graphId, nodeId, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                    }
                    else if (typeIdentifier < 300)
                    {
                        m_broadcastAPI->GetFloatValue(m_channelId.c_str(), static_cast<IBroadcast::ApiKey>(typeIdentifier), std::bind(&CFlowNode_TwitchAPIGet::VerifyNodeAndCallback<float>, graphId, nodeId, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                    }
                    else if (typeIdentifier < 400)
                    {
                        m_broadcastAPI->GetStringValue(m_channelId.c_str(), static_cast<IBroadcast::ApiKey>(typeIdentifier), std::bind(&CFlowNode_TwitchAPIGet::VerifyNodeAndCallback<std::string>, graphId, nodeId, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                    }
                }

                break;
            }
#endif // !CONSOLE
        }

        void GetMemoryUsage(ICrySizer*) const override
        {
        }

#ifndef CONSOLE
        template<class T>
        void OutputAPIResult(IBroadcast::ApiCallResult callResult, Aws::Http::HttpResponseCode httpResponse, const T& result)
        {
            if (callResult == IBroadcast::ApiCallResult::SUCCESS)
            {
                ActivateOutput(&m_actInfo, OutputPorts::Output, result);
            }
            else
            {
                ActivateOutput(&m_actInfo, OutputPorts::Error, static_cast<int>(callResult));
            }
        }

    private:
        SActivationInfo m_actInfo;
        IBroadcast* m_broadcastAPI;
        string m_channelId;
#endif // !CONSOLE
    };

#ifndef CONSOLE
    template<>
    void CFlowNode_TwitchAPIGet::OutputAPIResult<std::string>(IBroadcast::ApiCallResult callResult, Aws::Http::HttpResponseCode httpResponse, const std::string& result)
    {
        if (callResult == IBroadcast::ApiCallResult::SUCCESS)
        {
            ActivateOutput(&m_actInfo, OutputPorts::Output, string(result.c_str()));
        }
        else
        {
            ActivateOutput(&m_actInfo, OutputPorts::Error, static_cast<int>(callResult));
        }
    }
#endif // !CONSOLE

    REGISTER_FLOW_NODE("Twitch:API:GET", CFlowNode_TwitchAPIGet);

}