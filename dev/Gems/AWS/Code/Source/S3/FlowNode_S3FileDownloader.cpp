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
#include <AWS_precompiled.h>
#pragma warning(push)
#pragma warning(disable: 4819) // Invalid character not in default code page
#include <S3/FlowNode_S3FileDownloader.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/memory/stl/AWSStreamFwd.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#pragma warning(pop)
#include <memory>
#include <fstream>

#include <CloudCanvas/CloudCanvasMappingsBus.h>
#include <CloudGemFramework/AwsApiRequestJob.h>

using namespace Aws::S3::Model;

namespace LmbrAWS
{
    static const char* CLASS_TAG = "AWS:Primitive:S3FileDownloader";

    FlowNode_S3FileDownloader::FlowNode_S3FileDownloader(IFlowNode::SActivationInfo* activationInfo)
        : BaseMaglevFlowNode < eNCT_Singleton >(activationInfo)
    {
    }

    Aws::Vector<SInputPortConfig> FlowNode_S3FileDownloader::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPorts = {
            InputPortConfig_Void("DownloadFile", _HELP("Activate this to read data from the an s3 bucket.")),
            InputPortConfig<string>("BucketName", _HELP("The name of the bucket to use")),
            InputPortConfig<string>("KeyName", _HELP("The name of the file on S3 to be downloaded.")),
            InputPortConfig<string>("FileName", _HELP("The filename to use for the downloaded object"))
        };

        return inputPorts;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_S3FileDownloader::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts(0);
        return outputPorts;
    }

    void FlowNode_S3FileDownloader::ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo)
    {
        //Download File
        if (event == eFE_Activate && IsPortActive(pActInfo, EIP_DownloadFile))
        {
            AZStd::string bucketName = GetPortString(pActInfo, EIP_BucketClient).c_str();
            EBUS_EVENT_RESULT(bucketName, CloudGemFramework::CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, bucketName);
            auto& fileName = GetPortString(pActInfo, EIP_FileName);
            auto& keyName = GetPortString(pActInfo, EIP_KeyName);

            if (gEnv->pFileIO->IsReadOnly(fileName))
            {
                ErrorNotify(pActInfo->pGraph, pActInfo->myID, "File is read only");
                return;
            }
            
            if (fileName.empty())
            {
                ErrorNotify(pActInfo->pGraph, pActInfo->myID, "Please specify a file name");
                CRY_ASSERT_MESSAGE(false, "S3FileDownloader file name expected");
                return;
            }

            if (bucketName.empty())
            {
                ErrorNotify(pActInfo->pGraph, pActInfo->myID, "Please specify a bucket name");
                CRY_ASSERT_MESSAGE(false, "S3FileDownloader bucket name expected");
                return;
            }

            if (keyName.empty())
            {
                ErrorNotify(pActInfo->pGraph, pActInfo->myID, "Please specify a key name");
                CRY_ASSERT_MESSAGE(false, "S3FileDownloader key name expected");
                return;
            }

            GetObjectRequest getObjectRequest;
            getObjectRequest.SetKey(keyName);
            getObjectRequest.SetBucket(bucketName.c_str());
            Aws::String file(fileName);
            getObjectRequest.SetResponseStreamFactory([file]() { return Aws::New<Aws::FStream>(CLASS_TAG, file.c_str(), std::ios_base::out | std::ios_base::in | std::ios_base::binary | std::ios_base::trunc); });

            auto context = std::make_shared<FlowGraphContext>(pActInfo->pGraph, pActInfo->myID);
            MARSHALL_AWS_BACKGROUND_REQUEST(S3, client, GetObject, FlowNode_S3FileDownloader::ApplyResult, getObjectRequest, context)
        }
    }

    void FlowNode_S3FileDownloader::ApplyResult(const GetObjectRequest& request,
        const GetObjectOutcome& outcome,
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

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_S3FileDownloader);
}