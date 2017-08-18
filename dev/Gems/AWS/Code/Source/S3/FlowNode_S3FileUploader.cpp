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
#pragma warning(push)
#pragma warning(disable: 4819) // Invalid character not in default code page
#include <S3/FlowNode_S3FileUploader.h>
#include <aws/s3/model/HeadBucketRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/memory/stl/AWSStreamFwd.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#pragma warning(pop)
#include <memory>
#include <fstream>

using namespace Aws::S3::Model;

namespace LmbrAWS
{
    static const char* CLASS_TAG = "AWS:Primitive:S3FileUploader";
    static const size_t FIVE_MB = 1024 * 5000;

    FlowNode_S3FileUploader::FlowNode_S3FileUploader(IFlowNode::SActivationInfo* activationInfo)
        : BaseMaglevFlowNode < eNCT_Singleton >(activationInfo)
    {
    }

    Aws::Vector<SInputPortConfig> FlowNode_S3FileUploader::GetInputPorts() const
    {
        static const char* DEFAULT_CONTENT_TYPE = "application/octet-stream";

        static const Aws::Vector<SInputPortConfig> inputPorts = {
            InputPortConfig_Void("UploadFile", _HELP("Activate this to write data to the cloud.")),
            m_bucketClientPort.GetConfiguration("BucketName", _HELP("The name of the bucket to use")),
            InputPortConfig<string>("KeyName", _HELP("What to name the object being uploaded. If you do not update this as you go, the existing object will be overwritten")),
            InputPortConfig<string, string>("ContentType", DEFAULT_CONTENT_TYPE, _HELP("The mime content-type to use for the uploaded objects such as text/html, video/mpeg, video/avi, or application/zip.  This type is then stored in the S3 record and can be used to help identify what type of data you're looking at or retrieving later.")),
            InputPortConfig<string>("FileName", _HELP("Name of file to upload."))
        };

        return inputPorts;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_S3FileUploader::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts(0);
        return outputPorts;
    }

    void FlowNode_S3FileUploader::ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo)
    {
        //Upload File
        if (event == eFE_Activate && IsPortActive(pActInfo, EIP_UploadFile))
        {
            auto client = m_bucketClientPort.GetClient(pActInfo);
            if (!client.IsReady())
            {
                ErrorNotify(pActInfo->pGraph, pActInfo->myID, "Client configuration not ready.");
                return;
            }

            auto& bucketName = client.GetBucketName();
            auto& fileName = GetPortString(pActInfo, EIP_FileName);
            auto& keyName = GetPortString(pActInfo, EIP_KeyName);
            auto& contentType = GetPortString(pActInfo, EIP_ContentType);

            if (!contentType.length())
            {
                ErrorNotify(pActInfo->pGraph, pActInfo->myID, "Please specify a content type");
                CRY_ASSERT_MESSAGE(false, "S3FileUploader file content type expected");
                return;
            }

            if (bucketName.empty())
            {
                ErrorNotify(pActInfo->pGraph, pActInfo->myID, "Please specify a bucket name");
                CRY_ASSERT_MESSAGE(false, "S3FileUploader bucket name expected");
                return;
            }

            if (keyName.empty())
            {
                ErrorNotify(pActInfo->pGraph, pActInfo->myID, "Please specify a key name");
                CRY_ASSERT_MESSAGE(false, "S3FileUploader key name expected");
                return;
            }

            Aws::String strFileName(fileName.c_str());
            std::shared_ptr<Aws::IOStream> fileToUpload = Aws::MakeShared<Aws::FStream>(TAG, strFileName.c_str(), std::ios::binary | std::ios::in);

            if (fileToUpload->good())
            {
                //size_t fileSize = DetermineStreamSize(fileToUpload);

                //wait on transfer manager for this part.
                //if (fileSize > FIVE_MB)
                //{
                //   MultipartUpload(fileName.c_str(), bucketName.c_str(), fileToUpload);
                //}
                //else
                //{
                PutObjectRequest putObjectRequest;
                putObjectRequest.SetKey(keyName);
                putObjectRequest.SetBucket(bucketName);
                putObjectRequest.SetContentType(Aws::String(contentType));
                putObjectRequest.SetBody(fileToUpload);

                auto context = std::make_shared<FlowGraphContext>(pActInfo->pGraph, pActInfo->myID);
                MARSHALL_AWS_BACKGROUND_REQUEST(S3, client, PutObject, FlowNode_S3FileUploader::ApplyResult, putObjectRequest, context)
            }
            else
            {
                Aws::StringStream ss;
                ss << strerror(errno);
                ErrorNotify(pActInfo->pGraph, pActInfo->myID, ss.str().c_str());
            }
        }
    }

    size_t FlowNode_S3FileUploader::DetermineStreamSize(const std::shared_ptr<Aws::IOStream>& fileToUpload)
    {
        fileToUpload->seekg(0, std::fstream::end);
        size_t fileSize = fileToUpload->tellg();
        fileToUpload->seekg(0, std::fstream::beg);
        return fileSize;
    }

    void FlowNode_S3FileUploader::ApplyResult(const Aws::S3::Model::PutObjectRequest&,
        const Aws::S3::Model::PutObjectOutcome& outcome,
        const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
    {
        if (context)
        {
            auto fgContext = std::static_pointer_cast<const FlowGraphContext>(context);

            if (outcome.IsSuccess())
            {
                SuccessNotify(fgContext->GetFlowGraph(), fgContext->GetFlowNodeId());
            }
            else
            {
                ErrorNotify(fgContext->GetFlowGraph(), fgContext->GetFlowNodeId(), outcome.GetError().GetMessage().c_str());
            }
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_S3FileUploader);
}