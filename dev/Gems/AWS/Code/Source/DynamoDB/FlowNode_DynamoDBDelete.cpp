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
#include <DynamoDB/FlowNode_DynamoDBDelete.h>
#include <aws/dynamodb/model/DeleteItemRequest.h>
#include <aws/core/utils/Outcome.h>
#include <memory>

using namespace Aws::DynamoDB::Model;

namespace LmbrAWS
{
    static const char* CLASS_TAG = "AWS:Primitive:DynamoDB:DynamoDBDelete";

    static const char*  DEFAULT_HASHKEY_NAME = "hk";

    FlowNode_DynamoDBDelete::FlowNode_DynamoDBDelete(IFlowNode::SActivationInfo* activationInfo)
        : BaseMaglevFlowNode< eNCT_Singleton >(activationInfo)
    {
    }

    Aws::Vector<SInputPortConfig> FlowNode_DynamoDBDelete::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPorts = {
            InputPortConfig_Void("DeleteItem", _HELP("Activate this to delete a record from dynamo db")),
            m_tableClientPort.GetConfiguration("TableName", _HELP("The name of the table to delete from use")),
            InputPortConfig<string, string>("TableKeyName", DEFAULT_HASHKEY_NAME, _HELP("Key name used in this table")),
            InputPortConfig<string>("Key", _HELP("The key you wish to delete"), "KeyValue")
        };

        return inputPorts;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_DynamoDBDelete::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts = {
            OutputPortConfig_Void("DeletedItems", _HELP("Activated when matches were found to delete")),
            OutputPortConfig_Void("NoResults", _HELP("No matching results found"))
        };
        return outputPorts;
    }

    void FlowNode_DynamoDBDelete::ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate && IsPortActive(pActInfo, EIP_StartDelete))
        {
            auto client = m_tableClientPort.GetClient(pActInfo);
            if (!client.IsReady())
            {
                ErrorNotify(pActInfo->pGraph, pActInfo->myID, "Client configuration not ready.");
                return;
            }

            auto& tableName = client.GetTableName();
            auto& keyName = GetPortString(pActInfo, EIP_KeyName);
            auto& key = GetPortString(pActInfo, EIP_Key);

            Aws::DynamoDB::Model::DeleteItemRequest deleteRequest;
            deleteRequest.SetTableName(tableName);
            AttributeValue hashKey;
            hashKey.SetS(key.c_str());
            deleteRequest.AddKey(keyName, hashKey);
            deleteRequest.SetReturnValues(ReturnValue::ALL_OLD);

            auto context = std::make_shared<FlowGraphContext>(pActInfo->pGraph, pActInfo->myID);
            MARSHALL_AWS_BACKGROUND_REQUEST(DynamoDB, client, DeleteItem, FlowNode_DynamoDBDelete::ApplyResult, deleteRequest, context);
        }
    }

    void FlowNode_DynamoDBDelete::ApplyResult(const Aws::DynamoDB::Model::DeleteItemRequest& request,
        const Aws::DynamoDB::Model::DeleteItemOutcome& outcome,
        const std::shared_ptr<const Aws::Client::AsyncCallerContext>& baseContext)
    {
        auto context = std::static_pointer_cast<const FlowGraphContext>(baseContext);

        if (!context)
        {
            return;
        }

        auto flowGraph = context->GetFlowGraph();
        auto nodeId = context->GetFlowNodeId();

        if (outcome.IsSuccess())
        {
            const DeleteItemResult& result = const_cast<Aws::DynamoDB::Model::DeleteItemOutcome&>(outcome).GetResult();
            auto& attributes = result.GetAttributes();
            if (attributes.size())
            {
                SFlowAddress addr(nodeId, EOP_DeleteFoundMatch, true);
                flowGraph->ActivatePortCString(addr, "");
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

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_DynamoDBDelete);
} //namescape Amazon