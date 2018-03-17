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
#include <Nodes/FlowNode_RequestFileStatus.h>
#include <DynamicContent/DynamicContentBus.h>

using namespace Aws;
using namespace Aws::Client;

namespace LmbrAWS
{
    namespace FlowNode_RequestFileStatusInternal
    {
        static const char* CLASS_TAG = "AWS:DynamicContent:RequestFileStatus";
    }

    FlowNode_RequestFileStatus::FlowNode_RequestFileStatus(SActivationInfo* activationInfo) : BaseMaglevFlowNode< eNCT_Singleton >(activationInfo)
    {        
    }

    Aws::Vector<SInputPortConfig> FlowNode_RequestFileStatus::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPortConfiguration =
        {
            InputPortConfig_Void("RequestStatus", "Request the status of the given file"),
            InputPortConfig<string>("FileName", "Full name of the file to check the status of (folder and key)"),
        };

        return inputPortConfiguration;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_RequestFileStatus::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts = 
        {
            OutputPortConfig<string>("Finished", _HELP("Finished sending request")),
        };
        return outputPorts;
    }

    void FlowNode_RequestFileStatus::ProcessEvent_Internal(EFlowEvent event, SActivationInfo* activationInfo)
    {
        if (event == eFE_Activate && IsPortActive(activationInfo, EIP_RequestStatus))
        {
            bool requestSuccess = false;
            auto& fileName = GetPortString(activationInfo, EIP_FileName);
            if (fileName.length())
            {
                EBUS_EVENT_RESULT(requestSuccess, CloudCanvas::DynamicContent::DynamicContentRequestBus, RequestFileStatus, fileName.c_str(), "");
            }

            if (requestSuccess)
            {
                SFlowAddress addr(activationInfo->myID, EOP_Finished, true);
                activationInfo->pGraph->ActivatePort(addr, 0);
                SuccessNotify(activationInfo->pGraph, activationInfo->myID);
            }
            else
            {
                ErrorNotify(activationInfo->pGraph, activationInfo->myID, "Failed to request file status");
            }
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(FlowNode_RequestFileStatusInternal::CLASS_TAG, FlowNode_RequestFileStatus);

}