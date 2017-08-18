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
#include <Nodes/FlowNode_LoadStaticData.h>
#include <LmbrAWS/ILmbrAWS.h>
#include <LmbrAWS/IAWSClientManager.h>
#include <StaticData/StaticDataBus.h>

using namespace Aws;
using namespace Aws::Lambda;
using namespace Aws::Client;

namespace LmbrAWS
{
    namespace FlowNode_LoadStaticDataInternal
    {
        static const char* CLASS_TAG = "AWS:StaticData:LoadStaticData";
    }

    FlowNode_LoadStaticData::FlowNode_LoadStaticData(SActivationInfo* activationInfo) : BaseMaglevFlowNode< eNCT_Singleton >(activationInfo)
    {        
    }

    Aws::Vector<SInputPortConfig> FlowNode_LoadStaticData::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPortConfiguration =
        {
            InputPortConfig_Void("Load", "Load a type of Static Data"),
            InputPortConfig<string>("StaticDataType", "Your static data type to load"),
        };

        return inputPortConfiguration;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_LoadStaticData::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts = {
            OutputPortConfig<string>("Finished", _HELP("Finished attempting to load")),
        };
        return outputPorts;
    }

    void FlowNode_LoadStaticData::ProcessEvent_Internal(EFlowEvent event, SActivationInfo* activationInfo)
    {
        if (event == eFE_Activate && IsPortActive(activationInfo, EIP_Load))
        {
            auto& dataType = GetPortString(activationInfo, EIP_StaticDataType);

            bool reloadSuccess = false;
            EBUS_EVENT_RESULT(reloadSuccess, CloudCanvas::StaticData::StaticDataRequestBus, ReloadTagType, dataType.c_str());
            if (reloadSuccess)
            {
                SFlowAddress addr(activationInfo->myID, EOP_Finished, true);
                activationInfo->pGraph->ActivatePort(addr, 0);
                SuccessNotify(activationInfo->pGraph, activationInfo->myID);
            }
            else
            {
                ErrorNotify(activationInfo->pGraph, activationInfo->myID, "Failed to load static data");
            }
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(FlowNode_LoadStaticDataInternal::CLASS_TAG, FlowNode_LoadStaticData);
}