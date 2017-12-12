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
#include <Nodes/FlowNode_RemoveMonitoredBucket.h>
#include <LmbrAWS/ILmbrAWS.h>
#include <LmbrAWS/IAWSClientManager.h>
#include <StaticData/StaticDataBus.h>

using namespace Aws;
using namespace Aws::Lambda;
using namespace Aws::Client;

namespace LmbrAWS
{
    namespace FlowNode_RemoveMonitoredBucketInternal
    {
        static const char* CLASS_TAG = "AWS:StaticData:RemoveMonitoredBucket";
    }

    FlowNode_RemoveMonitoredBucket::FlowNode_RemoveMonitoredBucket(SActivationInfo* activationInfo) : BaseMaglevFlowNode< eNCT_Singleton >(activationInfo)
    {        
    }

    Aws::Vector<SInputPortConfig> FlowNode_RemoveMonitoredBucket::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPortConfiguration =
        {
            InputPortConfig_Void("Remove", "Remove a bucket name from the list of monitored buckets"),
            InputPortConfig<string>("BucketName", "Name of bucket to remove"),
        };

        return inputPortConfiguration;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_RemoveMonitoredBucket::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts = {
            OutputPortConfig<string>("Finished", _HELP("Finished removing bucket")),
        };
        return outputPorts;
    }

    void FlowNode_RemoveMonitoredBucket::ProcessEvent_Internal(EFlowEvent event, SActivationInfo* activationInfo)
    {
        if (event == eFE_Activate && IsPortActive(activationInfo, EIP_Remove))
        {
            auto& bucketName = GetPortString(activationInfo, EIP_BucketName);

            bool removeSuccess = false;
            EBUS_EVENT_RESULT(removeSuccess, CloudCanvas::StaticData::StaticDataRequestBus, RemoveMonitoredBucket, bucketName.c_str());
            if (removeSuccess)
            {
                SFlowAddress addr(activationInfo->myID, EOP_Finished, true);
                activationInfo->pGraph->ActivatePort(addr, 0);
                SuccessNotify(activationInfo->pGraph, activationInfo->myID);
            }
            else
            {
                ErrorNotify(activationInfo->pGraph, activationInfo->myID, "Failed to remove monitored bucket");
            }
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(FlowNode_RemoveMonitoredBucketInternal::CLASS_TAG, FlowNode_RemoveMonitoredBucket);
}