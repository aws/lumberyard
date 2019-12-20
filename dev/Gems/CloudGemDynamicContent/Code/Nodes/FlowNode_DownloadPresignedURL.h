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

#include <AzCore/PlatformDef.h>
// The AWS Native SDK AWSAllocator triggers a warning due to accessing members of std::allocator directly.
// AWSAllocator.h(70): warning C4996: 'std::allocator<T>::pointer': warning STL4010: Various members of std::allocator are deprecated in C++17.
// Use std::allocator_traits instead of accessing these members directly.
// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.

AZ_PUSH_DISABLE_WARNING(4251 4996, "-Wunknown-warning-option")
#include <aws/core/http/HttpRequest.h>
#include <aws/core/http/HttpResponse.h>
AZ_POP_DISABLE_WARNING

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



