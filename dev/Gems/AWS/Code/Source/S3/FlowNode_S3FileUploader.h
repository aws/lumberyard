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
#pragma warning(disable: 4355) // <future> includes ppltasks.h which throws a C4355 warning: 'this' used in base member initializer list
#include <aws/s3/S3Client.h>
#pragma warning(default: 4355)

namespace LmbrAWS
{
    //////////////////////////////////////////////////////////////////////////////////////
    ///
    /// Creates an s3 file uploader
    ///
    //////////////////////////////////////////////////////////////////////////////////////
    class FlowNode_S3FileUploader
        : public BaseMaglevFlowNode <eNCT_Singleton>
    {
    public:
        FlowNode_S3FileUploader(IFlowNode::SActivationInfo* activationInfo);

        // ~IFlowNode
        virtual ~FlowNode_S3FileUploader() {}

    protected:
        const char* GetFlowNodeDescription() const override { return _HELP("S3 File Uploader"); }
        void ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* activationInfo) override;
        Aws::Vector<SInputPortConfig> GetInputPorts() const override;
        Aws::Vector<SOutputPortConfig> GetOutputPorts() const override;

        virtual const char* GetClassTag() const override;
    private:

        enum
        {
            EIP_UploadFile = EIP_StartIndex,
            EIP_BucketClient,
            EIP_KeyName,
            EIP_ContentType,
            EIP_FileName
        };

        LmbrAWS::S3::BucketClientInputPort m_bucketClientPort {
            EIP_BucketClient
        };

        void ApplyResult(const Aws::S3::Model::PutObjectRequest&, const Aws::S3::Model::PutObjectOutcome&, const std::shared_ptr<const Aws::Client::AsyncCallerContext>&);

        static size_t DetermineStreamSize(const std::shared_ptr<Aws::IOStream>& fileToUpload);
    };
} // namespace Amazon



