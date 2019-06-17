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
#include <AzCore/IO/SystemFile.h>
#include <Nodes/FlowNode_DownloadPresignedURL.h>
#include <aws/core/utils/StringUtils.h>
#include <PresignedURL/PresignedURLBus.h>
#include <Util/FlowSystem/FlowGraphContext.h>

namespace LmbrAWS
{
    static const char* CLASS_TAG = "AWS:DynamicContent:DownloadPresignedURL";

    FlowNode_DownloadPresignedURL::FlowNode_DownloadPresignedURL(IFlowNode::SActivationInfo* activationInfo)
        : BaseMaglevFlowNode < eNCT_Singleton >(activationInfo)
    {
    }

     Aws::Vector<SInputPortConfig> FlowNode_DownloadPresignedURL::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPorts = {
            InputPortConfig_Void("DownloadFile", _HELP("Activate this to attempt to download a presigned URL.")),
            InputPortConfig<string>("PresignedURL", _HELP("The presigned URL to attept to download")),
            InputPortConfig<string>("FileName", _HELP("The filename to use for the downloaded object"))
        };

        return inputPorts;
    }


    Aws::Vector<SOutputPortConfig> FlowNode_DownloadPresignedURL::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts(0);
        return outputPorts;
    }

    void FlowNode_DownloadPresignedURL::ProcessEvent_Internal(EFlowEvent event, SActivationInfo* pActInfo)
    {
        m_activationInfo = *pActInfo;
        if (event == eFE_Activate && pActInfo->pInputPorts[0].IsUserFlagSet())
        {
            AZStd::string inputString{ GetPortString(pActInfo, EIP_DownloadURL) };
            AZStd::string outputFile{ GetPortString(pActInfo, EIP_OutputFile) };

            EBUS_EVENT(CloudCanvas::PresignedURLRequestBus, RequestDownloadSignedURL, inputString, outputFile, AZ::EntityId());
        }
    }

    void FlowNode_DownloadPresignedURL::GotPresignedURLResult(const AZStd::string& fileRequest, int responseCode, const AZStd::string& resultString, const AZStd::string& outputFile)
    {
        if (responseCode == static_cast<int>(Aws::Http::HttpResponseCode::OK))
        {
            AZ_TracePrintf("DownloadPresigned","Downloaded signed URL to %s", outputFile.c_str());
            SuccessNotify(m_activationInfo.pGraph, m_activationInfo.myID);
        }
        else
        {
            AZ_TracePrintf("DownloadPresigned", "Failed to download signed URL to %s (%s)", outputFile.c_str(), resultString.c_str());
            ErrorNotify(m_activationInfo.pGraph, m_activationInfo.myID, "Failed to download signed url");
        }
    }
    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_DownloadPresignedURL);
} // namespace AWS





