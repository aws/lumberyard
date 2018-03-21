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
#include <CloudGemDynamicContent_precompiled.h>
#include <Nodes/FlowNode_RequestManifest.h>
#include <DynamicContent/DynamicContentBus.h>

using namespace Aws;
using namespace Aws::Client;

namespace LmbrAWS
{
    namespace FlowNode_RequestManifestInternal
    {
        static const char* CLASS_TAG = "AWS:DynamicContent:RequestManifest";
    }

    FlowNode_RequestManifest::FlowNode_RequestManifest(SActivationInfo* activationInfo) : BaseMaglevFlowNode< eNCT_Singleton >(activationInfo),
        m_activationInfo(*activationInfo)
    {        
    }

    Aws::Vector<SInputPortConfig> FlowNode_RequestManifest::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPortConfiguration =
        {
            InputPortConfig_Void("RequestManifest", "Request an updated pak containing a top level manifest from the Dynamic Content system"),
            InputPortConfig<string>("ManifestName", "Name of manifest to request update for"),
        };

        return inputPortConfiguration;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_RequestManifest::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts = 
        {
            OutputPortConfig<string>("Finished", _HELP("Finished sending request")),
            OutputPortConfig_Void("PakContentReady", _HELP("New pak content ready")),
            OutputPortConfig_Void("RequestsCompleted", _HELP("All currently outstanding requests have completed")),
        };
        return outputPorts;
    }

    void FlowNode_RequestManifest::ProcessEvent_Internal(EFlowEvent event, SActivationInfo* activationInfo)
    {
        m_activationInfo = *activationInfo;
        if (event == eFE_Activate && IsPortActive(activationInfo, EIP_RequestManifest))
        {
            bool requestSuccess = false;
            auto& manifestName = GetPortString(activationInfo, EIP_ManifestName);
            if (manifestName.length())
            {
                EBUS_EVENT_RESULT(requestSuccess, CloudCanvas::DynamicContent::DynamicContentRequestBus, RequestManifest, manifestName.c_str());
            }

            if (requestSuccess)
            {
                CloudCanvas::DynamicContent::DynamicContentUpdateBus::Handler::BusConnect();

                SFlowAddress addr(activationInfo->myID, EOP_Finished, true);
                activationInfo->pGraph->ActivatePort(addr, 0);
                SuccessNotify(activationInfo->pGraph, activationInfo->myID);
            }
            else
            {
                ErrorNotify(activationInfo->pGraph, activationInfo->myID, "Failed to request manifest update");
            }
        }
        else if (event == eFE_Uninitialize)
        {
            CloudCanvas::DynamicContent::DynamicContentUpdateBus::Handler::BusDisconnect();
        }
    }

    void FlowNode_RequestManifest::NewContentReady(const AZStd::string& outputFile)
    {
    }

    void FlowNode_RequestManifest::NewPakContentReady(const AZStd::string& pakFileName)
    {
        ActivateOutput(&m_activationInfo, EOP_PakContentReady, true);
    }

    void FlowNode_RequestManifest::RequestsCompleted()
    {
        ActivateOutput(&m_activationInfo, EOP_RequestsCompleted, true);
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(FlowNode_RequestManifestInternal::CLASS_TAG, FlowNode_RequestManifest);

}