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
#include <Util/ScreenCapture/FlowNode_ScreenCapture.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

namespace LmbrAWS
{
    static const char* CLASS_TAG = "Image:ScreenCapture";

    FlowNode_ScreenCapture::FlowNode_ScreenCapture(IFlowNode::SActivationInfo* activationInfo)
        : BaseMaglevFlowNode < eNCT_Singleton >(activationInfo)
    {
    }

    Aws::Vector<SInputPortConfig> FlowNode_ScreenCapture::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPorts = {
            InputPortConfig_Void("Capture", _HELP("Activate this to capture a screenshot.")),
            InputPortConfig<string>("FileName", _HELP("The file to write the screenshot to.")),
            InputPortConfig<string>("ImageType", _HELP("The file-type to use for the screen shot"), "ImageType",
                _UICONFIG("enum_string:Jpeg=jpg,Tiff=tiff"))
        };

        return inputPorts;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_ScreenCapture::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts(0);
        return outputPorts;
    }

    void FlowNode_ScreenCapture::ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo)
    {
        auto startIndex = EIP_StartIndex;

        //capture screenshot
        if (event == eFE_Activate && pActInfo->pInputPorts[startIndex].IsUserFlagSet())
        {
            auto& fileName = GetPortString(pActInfo, startIndex + 1);
            auto& contentType = GetPortString(pActInfo, startIndex + 2);

            Aws::StringStream ss;
            ss << "capture_file_format " << contentType;
            gEnv->pConsole->ExecuteString(ss.str().c_str());
            ss.str("");
            ss << "capture_file_name " << fileName.c_str();
            gEnv->pConsole->ExecuteString(ss.str().c_str());
            gEnv->pConsole->ExecuteString("capture_frames 1");
            gEnv->pConsole->ExecuteString("capture_frame_once 1");

            SuccessNotify(pActInfo->pGraph, pActInfo->myID);
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_ScreenCapture);
}