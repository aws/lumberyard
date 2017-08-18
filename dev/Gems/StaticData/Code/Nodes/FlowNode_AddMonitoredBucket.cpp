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
#include <StdAfx.h>
#include <Nodes/FlowNode_AddMonitoredBucket.h>
#include <LmbrAWS/ILmbrAWS.h>
#include <LmbrAWS/IAWSClientManager.h>
#include <StaticData/StaticDataBus.h>

using namespace Aws;
using namespace Aws::Lambda;
using namespace Aws::Client;

namespace LmbrAWS
{
    namespace FlowNode_AddMonitoredBucketInternal
    {
        static const char* CLASS_TAG = "AWS:StaticData:AddMonitoredBucket";
    }

    FlowNode_AddMonitoredBucket::FlowNode_AddMonitoredBucket(SActivationInfo* activationInfo) : BaseMaglevFlowNode< eNCT_Singleton >(activationInfo)
    {        
    }

    Aws::Vector<SInputPortConfig> FlowNode_AddMonitoredBucket::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPortConfiguration =
        {
            InputPortConfig_Void("AddBucket", "Add a bucket to watch for updates"),
            InputPortConfig<string>("BucketName", "Name of the bucket to watch"),
        };

        return inputPortConfiguration;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_AddMonitoredBucket::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts = {
            OutputPortConfig<string>("Finished", _HELP("Added the bucket")),
        };
        return outputPorts;
    }

    void FlowNode_AddMonitoredBucket::ProcessEvent_Internal(EFlowEvent event, SActivationInfo* activationInfo)
    {
        if (event == eFE_Activate && IsPortActive(activationInfo, EIP_Add))
        {
            auto& dataType = GetPortString(activationInfo, EIP_BucketName);

            bool success = false;
            EBUS_EVENT_RESULT(success, CloudCanvas::StaticData::StaticDataRequestBus, AddMonitoredBucket, dataType.c_str());
            if (success)
            {
                SFlowAddress addr(activationInfo->myID, EOP_Finished, true);
                activationInfo->pGraph->ActivatePort(addr, 0);
                SuccessNotify(activationInfo->pGraph, activationInfo->myID);
            }
            else
            {
                ErrorNotify(activationInfo->pGraph, activationInfo->myID, "Failed to add the bucket");
            }
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(FlowNode_AddMonitoredBucketInternal::CLASS_TAG, FlowNode_AddMonitoredBucket);
}