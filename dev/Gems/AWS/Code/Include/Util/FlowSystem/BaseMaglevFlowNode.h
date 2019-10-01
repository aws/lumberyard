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
#pragma once

#include <mutex>

#include <FlowSystem/Nodes/FlowBaseNode.h>

#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/core/utils/memory/stl/AWSString.h>

#include <Util/FlowSystem/FlowGraphContext.h>

#include <Util/FlowSystem/AsyncFlowgraphResult.h>

#include "ILocalizationManager.h"

// The AWS Native SDK has potential collisions on two macros:
//
// GetMessage is defined in windows.h which collides with 
// AWSError's GetMessage call
#ifdef GetMessage
#undef GetMessage
#endif

// And IN is defined in CryAISystem/StdAfx.h
// and appears in the dynamoDB model
// ComparisonOperator.h file
#ifdef IN
#undef IN
#endif

namespace LmbrAWS
{
    static const char* TAG = "Amazon::BaseMaglevFlowNode";
    typedef FlowGraphContextWithExtra<Aws::String> FlowGraphStringContext;

    // return a localized string, this is a convenience function to reduce typing and make localization tagging
    // of inline strings similar to other system
    inline string _LOC(const string& stringToLocalize)
    {
        string outString;
        LocalizationManagerRequestBus::Broadcast(&LocalizationManagerRequestBus::Events::LocalizeString_s, stringToLocalize, outString, false);
        return outString;
    }


    /*
    * Base class for implementing a Flow node for maglev. This provides default success and error ports
    * and adds some convenience methods.
    */
    template<ENodeCloneType NodeType>
    class BaseMaglevFlowNode
        : public CFlowBaseNode<NodeType>
    {
    public:

        BaseMaglevFlowNode(IFlowNode::SActivationInfo* activationInfo)
            : m_inputPortConfiguration(nullptr)
            , m_outputPortConfiguration(nullptr) {}

        // IFlowNode
        void GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }
        void GetConfiguration(SFlowNodeConfig& configuration) override
        {
            if (!m_inputPortConfiguration)
            {
                std::lock_guard<std::mutex> locker(m_portLock);
                if (!m_inputPortConfiguration)
                {
                    auto inputPorts = GetInputPorts();
                    auto outputPorts = GetOutputPorts();

                    SInputPortConfig* inputPortConfiguration = Aws::NewArray<SInputPortConfig>(inputPorts.size() + 1, TAG);
                    SOutputPortConfig* outputPortConfiguration = Aws::NewArray<SOutputPortConfig>(outputPorts.size() + 3, TAG);

                    for (unsigned i = 0; i < inputPorts.size(); ++i)
                    {
                        inputPortConfiguration[i] = inputPorts[i];
                    }

                    inputPortConfiguration[inputPorts.size()] = {0};

                    outputPortConfiguration[0] = OutputPortConfig_Void("Success", _HELP("Activated upon a successful operation"));
                    outputPortConfiguration[1] = OutputPortConfig<string>("Error", _HELP("Activated upon an error being detected. The value of the port is the error message."));

                    for (unsigned i = 0; i < outputPorts.size(); ++i)
                    {
                        outputPortConfiguration[i + 2] = outputPorts[i];
                    }

                    outputPortConfiguration[outputPorts.size() + 2] = {0};

                    m_inputPortConfiguration = inputPortConfiguration;
                    m_outputPortConfiguration = outputPortConfiguration;
                }
            }

            configuration.pInputPorts = m_inputPortConfiguration;
            configuration.pOutputPorts = m_outputPortConfiguration;
            configuration.sDescription = GetFlowNodeDescription();
            configuration.SetCategory(GetFlowNodeFlags());
            configuration.sUIClassName = GetUIName();
        }

        void ProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* activationInfo) override
        {
            ProcessEvent_Internal(event, activationInfo);
        }

        virtual const char* GetClassTag() const = 0;

        // ~IFlowNode
        virtual ~BaseMaglevFlowNode()
        {
            if (m_inputPortConfiguration)
            {
                Aws::DeleteArray(m_inputPortConfiguration);
            }
            if (m_outputPortConfiguration)
            {
                Aws::DeleteArray(m_outputPortConfiguration);
            }
        }

        // Helper to route to our FlowInit data instead having the data defined on the class like we were previously which required an instance of the class to be created
        virtual const char* GetUIName() const override
        {
            IFlowSystem* pFlow = GetISystem()->GetIFlowSystem();
            if (pFlow)
            {
                return pFlow->GetUINameFromClassTag(GetClassTag()).c_str();
            }
            return "";
        }

    protected:
        virtual const char* GetFlowNodeDescription() const = 0;
        virtual int GetFlowNodeFlags() const { return EFLN_ADVANCED; }

        /**
          Override for you to handle flow node events. Only fires on an activate event.
        */
        virtual void ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* activationInfo) = 0;

        /**
         * Override this to return your available input ports.
        */
        virtual Aws::Vector<SInputPortConfig> GetInputPorts() const = 0;

        /**
         * Override this to return your available output ports.
         */
        virtual Aws::Vector<SOutputPortConfig> GetOutputPorts() const = 0;

        //Call this when you want to post an error to the error port
        void ErrorNotify(IFlowGraph* flowGraph, TFlowNodeId nodeId, const char* errorMessage)
        {
            // Describe the node that generated the error.

            string inputDescription;

            SFlowNodeConfig config;
            flowGraph->GetNodeConfiguration(nodeId, config);

            if (config.pInputPorts != nullptr)
            {
                for (TFlowPortId portId = 0; config.pInputPorts[portId].name != nullptr; ++portId)
                {
                    const TFlowInputData* inputData = flowGraph->GetInputValue(nodeId, portId);
                    if (inputData != nullptr)
                    {
                        string portValue;
                        if (inputData->GetValueWithConversion(portValue))
                        {
                            if (!inputDescription.empty())
                            {
                                inputDescription += ", ";
                            }

                            inputDescription += config.pInputPorts[portId].name;
                            inputDescription += "=";
                            inputDescription += portValue;
                        }
                    }
                }
            }

            if (inputDescription.empty())
            {
                inputDescription = "(none)";
            }

            gEnv->pLog->LogError("(Cloud Canvas) Flow graph %s node with inputs %s failed with error: %s", flowGraph->GetNodeTypeName(nodeId), inputDescription.c_str(), errorMessage);
            SFlowAddress addr(nodeId, EOP_Error, true);
            flowGraph->ActivatePortCString(addr, errorMessage);
        }

        //Call this when you want to post a success to the success port
        void SuccessNotify(IFlowGraph* flowGraph, TFlowNodeId nodeId)
        {
            SFlowAddress addr(nodeId, EOP_Success, true);
            flowGraph->ActivatePort(addr, true);
        }

        enum
        {
            EOP_Success,
            EOP_Error,
            EOP_StartIndex         // last one, for use in derived class enums
        };

        enum
        {
            EIP_StartIndex         // last one, for use in derived class enums
        };

    private:
        SInputPortConfig* m_inputPortConfiguration;
        SOutputPortConfig* m_outputPortConfiguration;
        std::mutex m_portLock;
    };
}