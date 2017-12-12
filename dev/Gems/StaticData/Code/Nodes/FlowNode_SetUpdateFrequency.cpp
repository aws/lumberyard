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
#include <Nodes/FlowNode_SetUpdateFrequency.h>
#include <LmbrAWS/ILmbrAWS.h>
#include <LmbrAWS/IAWSClientManager.h>
#include <StaticData/StaticDataBus.h>

using namespace Aws;
using namespace Aws::Lambda;
using namespace Aws::Client;

namespace LmbrAWS
{
    namespace FlowNode_SetUpdateFrequencyInternal
    {
        static const char* CLASS_TAG = "AWS:StaticData:SetUpdateFrequency";
    }

    FlowNode_SetUpdateFrequency::FlowNode_SetUpdateFrequency(SActivationInfo* activationInfo) : BaseMaglevFlowNode< eNCT_Singleton >(activationInfo)
    {        
    }

    Aws::Vector<SInputPortConfig> FlowNode_SetUpdateFrequency::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPortConfiguration =
        {
            InputPortConfig_Void("SetTimer", "Set a recurring update timer to a given value (0 to clear)"),
            InputPortConfig<int>("TimerValue", "Timer value to poll at (0 to clear)"),
        };

        return inputPortConfiguration;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_SetUpdateFrequency::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts = {
            OutputPortConfig<string>("Set", _HELP("Timer has been set")),
        };
        return outputPorts;
    }

    void FlowNode_SetUpdateFrequency::ProcessEvent_Internal(EFlowEvent event, SActivationInfo* activationInfo)
    {
        if (event == eFE_Activate && IsPortActive(activationInfo, EIP_SetTimer))
        {
            auto timerValue = GetPortInt(activationInfo, EIP_TimerValue);

            bool success = false;
            EBUS_EVENT_RESULT(success, CloudCanvas::StaticData::StaticDataRequestBus, SetUpdateFrequency, timerValue);
            if (success)
            {
                SFlowAddress addr(activationInfo->myID, EOP_Finished, true);
                activationInfo->pGraph->ActivatePort(addr, 0);
                SuccessNotify(activationInfo->pGraph, activationInfo->myID);
            }
            else
            {
                ErrorNotify(activationInfo->pGraph, activationInfo->myID, "Failed to set the timer");
            }
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(FlowNode_SetUpdateFrequencyInternal::CLASS_TAG, FlowNode_SetUpdateFrequency);
}