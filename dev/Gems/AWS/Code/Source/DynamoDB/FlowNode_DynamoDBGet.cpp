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
#include <DynamoDB/FlowNode_DynamoDBGet.h>
#include <aws/dynamodb/model/GetItemRequest.h>
#include <aws/dynamodb/model/GetItemResult.h>
#include <aws/core/utils/Outcome.h>
#include <memory>

using namespace Aws::DynamoDB::Model;

namespace LmbrAWS
{
    static const char* CLASS_TAG = "AWS:Primitive:DynamoDB:DynamoDBGet";

    static const char*  DEFAULT_HASHKEY_NAME = "hk";

    FlowNode_DynamoDBGet::FlowNode_DynamoDBGet(IFlowNode::SActivationInfo* activationInfo)
        : BaseMaglevFlowNode< eNCT_Singleton >(activationInfo)
    {
    }

    Aws::Vector<SInputPortConfig> FlowNode_DynamoDBGet::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPorts = {
            InputPortConfig_Void("GetItem", _HELP("Activate this to read data from the cloud")),
            m_tableClientInputPort.GetConfiguration("TableName", _HELP("The name of the table to use")),
            InputPortConfig<string, string>("TableKeyName", DEFAULT_HASHKEY_NAME, _HELP("The name of the key used in this table")),
            InputPortConfig<string>("Key", _HELP("The key you wish to read"), "KeyValue"),
            InputPortConfig<string>("Attribute", _HELP("The attribute you wish to read"), "AttributeToReturn"),
        };

        return inputPorts;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_DynamoDBGet::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts = {
            OutputPortConfig<string>("DataOut", _HELP("String data read from the cloud")),
            OutputPortConfig<float>("NumberOut", _HELP("Number Data read from the cloud")),
            OutputPortConfig<bool>("BoolOut", _HELP("Boolean Data read from the cloud")),
            OutputPortConfig_Void("NoResults", _HELP("No matching results found"))
        };

        return outputPorts;
    }

    void FlowNode_DynamoDBGet::ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo)
    {
        //Get Item
        if (event == eFE_Activate && IsPortActive(pActInfo, EIP_ActivateGet))
        {
            auto client = m_tableClientInputPort.GetClient(pActInfo);
            if (!client.IsReady())
            {
                ErrorNotify(pActInfo->pGraph, pActInfo->myID, "Client configuration not ready.");
                return;
            }

            auto& tableName = client.GetTableName();
            auto& keyName = GetPortString(pActInfo, EIP_TableKeyName);
            auto& key = GetPortString(pActInfo, EIP_Key);
            auto& attribute = GetPortString(pActInfo, EIP_Attribute);

            Aws::DynamoDB::Model::GetItemRequest getRequest;
            getRequest.SetTableName(tableName);
            AttributeValue hk;
            hk.SetS(key);

            getRequest.AddKey(keyName, hk);
            Aws::Vector<Aws::String> attributesToGet;
            attributesToGet.push_back(attribute.c_str());

            auto context = std::make_shared<FlowGraphStringContext>(Aws::String(attribute), pActInfo->pGraph, pActInfo->myID);
            MARSHALL_AWS_BACKGROUND_REQUEST(DynamoDB, client, GetItem, FlowNode_DynamoDBGet::ApplyResult, getRequest, context)
        }
    }

    void FlowNode_DynamoDBGet::ApplyResult(const Aws::DynamoDB::Model::GetItemRequest& request,
        const Aws::DynamoDB::Model::GetItemOutcome& outcome,
        const std::shared_ptr<const Aws::Client::AsyncCallerContext>& baseContext)
    {
        auto context = std::static_pointer_cast<const FlowGraphStringContext>(baseContext);

        if (!context)
        {
            return;
        }

        auto& attrToGet = context->GetExtra();
        auto flowGraph = context->GetFlowGraph();
        auto nodeId = context->GetFlowNodeId();

        if (outcome.IsSuccess())
        {
            //update this once third party is updated
            const GetItemResult& result = const_cast<Aws::DynamoDB::Model::GetItemOutcome&>(outcome).GetResult();
            auto& items = result.GetItem();

            auto attrElem = items.find(attrToGet);
            if (items.size() && attrElem != items.end())
            {
                auto& attrData = attrElem->second;

                if (attrData.GetS().length())
                {
                    auto& data = attrData.GetS();
                    SFlowAddress addr(nodeId, EOP_DataOut, true);
                    flowGraph->ActivatePortCString(addr, data.c_str());
                }

                if (attrData.GetN().length())
                {
                    std::string stStr(attrData.GetN().c_str());

                    SFlowAddress numAddr(nodeId, EOP_NumberOut, true);
                    flowGraph->ActivatePort(numAddr, std::stof(stStr));
                }

                auto boolData = attrData.GetBool();
                SFlowAddress boolAddr(nodeId, EOP_BoolOut, true);
                flowGraph->ActivatePort(boolAddr, boolData);
            }
            else
            {
                SFlowAddress addr(nodeId, EOP_NoMatches, true);
                flowGraph->ActivatePortCString(addr, "");
            }
            SuccessNotify(flowGraph, nodeId);
        }
        else
        {
            ErrorNotify(flowGraph, nodeId, outcome.GetError().GetMessage().c_str());
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_DynamoDBGet);
} //namescape Amazon