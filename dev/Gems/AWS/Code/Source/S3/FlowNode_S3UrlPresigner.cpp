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
#include <S3/FlowNode_S3UrlPresigner.h>
#include <memory>
#include <fstream>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/http/HttpTypes.h>

using namespace Aws::S3::Model;
using namespace Aws::Http;

namespace LmbrAWS
{
    static const char* CLASS_TAG = "AWS:Primitive:S3UrlPresigner";

    FlowNode_S3UrlPresigner::FlowNode_S3UrlPresigner(IFlowNode::SActivationInfo* activationInfo)
        : BaseMaglevFlowNode < eNCT_Singleton >(activationInfo)
    {
    }

    Aws::Vector<SInputPortConfig> FlowNode_S3UrlPresigner::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPorts = {
            InputPortConfig_Void("PresignUrl", _HELP("Activate this to write data to the cloud. Should be the name of a file you want to upload.")),
            m_bucketClientPort.GetConfiguration("BucketName", _HELP("The name of the bucket to use")),
            InputPortConfig<string>("KeyName", _HELP("What to name the key to presign against")),
            InputPortConfig<string>("RequestMethod", _HELP("The method to presign against"), "Http Request Method",
                _UICONFIG("enum_string:GET=GET,PUT=PUT,POST=POST,DELETE=DELETE"))
        };

        return inputPorts;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_S3UrlPresigner::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts = {
            OutputPortConfig<string>("Url", _HELP("The signed url"))
        };

        return outputPorts;
    }

    void FlowNode_S3UrlPresigner::ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo)
    {
        //Upload File
        if (event == eFE_Activate && IsPortActive(pActInfo, EIP_PresignUrl))
        {
            auto client = m_bucketClientPort.GetClient(pActInfo);
            if (!client.IsReady())
            {
                ErrorNotify(pActInfo->pGraph, pActInfo->myID, "Client configuration not ready.");
                return;
            }

            auto& bucketName = client.GetBucketName();
            auto& keyName = GetPortString(pActInfo, EIP_KeyName);
            auto& method = GetPortString(pActInfo, EIP_RequestMethod);

            HttpMethod httpMethod;

            if (method == "PUT")
            {
                httpMethod = HttpMethod::HTTP_PUT;
            }
            else if (method == "POST")
            {
                httpMethod = HttpMethod::HTTP_POST;
            }
            else if (method == "DELETE")
            {
                httpMethod = HttpMethod::HTTP_DELETE;
            }
            else
            {
                httpMethod = HttpMethod::HTTP_GET;
            }

            Aws::String url = client->GeneratePresignedUrl(bucketName.c_str(), keyName.c_str(), httpMethod);
            if (!url.empty())
            {
                SFlowAddress addr(pActInfo->myID, EOP_SignedUrl, true);
                pActInfo->pGraph->ActivatePortCString(addr, url.c_str());
                SuccessNotify(pActInfo->pGraph, pActInfo->myID);
            }
            else
            {
                SFlowAddress addr(pActInfo->myID, EOP_SignedUrl, true);
                pActInfo->pGraph->ActivatePortCString(addr, "");
                ErrorNotify(pActInfo->pGraph, pActInfo->myID, "Unable to generate Url");
            }
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_S3UrlPresigner);
}