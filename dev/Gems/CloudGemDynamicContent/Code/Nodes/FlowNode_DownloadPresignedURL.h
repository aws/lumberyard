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
#include <Util/FlowSystem/BaseMaglevFlowNode.h>
#include <aws/core/http/HttpRequest.h>
#include <aws/core/http/HttpResponse.h>
#include <IFlowSystem.h>
#include <PresignedURL/PresignedURLBus.h>

namespace LmbrAWS
{

    class FlowNode_DownloadPresignedURL
        : public BaseMaglevFlowNode < eNCT_Singleton >
        , public CloudCanvas::PresignedURLResultBus::Handler
    {
    public:
        FlowNode_DownloadPresignedURL(IFlowNode::SActivationInfo* activationInfo);

        // ~IFlowNode
        virtual ~FlowNode_DownloadPresignedURL() {}

        enum
        {
            EIP_Invoke = EIP_StartIndex,
            EIP_DownloadURL,
            EIP_OutputFile
        };

        virtual void GotPresignedURLResult(const AZStd::string& fileRequest, int responseCode, const AZStd::string& resultString, const AZStd::string& outputFile) override;
    protected:
        const char* GetFlowNodeDescription() const override { return _HELP("S3 File Downloader"); }
        Aws::Vector<SInputPortConfig> GetInputPorts() const override;
        Aws::Vector<SOutputPortConfig> GetOutputPorts() const override;

        virtual void ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* activationInfo) override;

        virtual const char* GetClassTag() const override;

        SActivationInfo m_activationInfo;
    };
} // namespace LmbrAWS



