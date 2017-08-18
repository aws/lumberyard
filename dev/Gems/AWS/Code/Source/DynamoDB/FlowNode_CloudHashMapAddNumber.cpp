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
#include <DynamoDB/FlowNode_CloudHashMapAddNumber.h>
#include <aws/dynamodb/model/UpdateItemRequest.h>
#include <aws/core/utils/Outcome.h>
#include <memory>

using namespace Aws::DynamoDB::Model;

namespace LmbrAWS
{
    static const char* CLASS_TAG = "AWS:Primitive:DynamoDB:DynamoDBAddNumber";

    static const char*  DEFAULT_HASHKEY_NAME = "hk";

    FlowNode_CloudHashMapAddNumber::FlowNode_CloudHashMapAddNumber(IFlowNode::SActivationInfo* activationInfo)
        : BaseMaglevFlowNode< eNCT_Singleton >(activationInfo)
    {
    }

    Aws::Vector<SInputPortConfig> FlowNode_CloudHashMapAddNumber::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPorts = {
            InputPortConfig_Void("Add", _HELP("Activate this to write val to the cloud")),
            m_tableClientPort.GetConfiguration("TableName", _HELP("The name of the table to use")),
            InputPortConfig<string, string>("TableKeyName", DEFAULT_HASHKEY_NAME, _HELP("Key name used in this table")),
            InputPortConfig<string>("Key", _HELP("The key you wish to write")),
            InputPortConfig<string>("Attribute", _HELP("The attribute you wish to write")),
            InputPortConfig<int>("Value", _HELP("The val to write"))
        };

        return inputPorts;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_CloudHashMapAddNumber::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts = {
            OutputPortConfig<string>("NewValue", _HELP("The new value of the attribute after the addition"))
        };

        return outputPorts;
    }

    void FlowNode_CloudHashMapAddNumber::ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate && IsPortActive(pActInfo, EIP_Add))
        {
            auto client = m_tableClientPort.GetClient(pActInfo);
            if (!client.IsReady())
            {
                ErrorNotify(pActInfo->pGraph, pActInfo->myID, "Client configuration not ready.");
                return;
            }

            auto& tableName =  client.GetTableName();
            auto& keyName = GetPortString(pActInfo, EIP_TableKeyName);
            auto& key = GetPortString(pActInfo, EIP_Key);
            auto& attr = GetPortString(pActInfo, EIP_Attribute);
            int valueToAdd = GetPortInt(pActInfo, EIP_Value);
            Aws::String valueToAddAsString(std::to_string(valueToAdd).c_str());

            Aws::DynamoDB::Model::UpdateItemRequest updateRequest;
            updateRequest.SetTableName(tableName);
            AttributeValue hk,  val;
            hk.SetS(key);
            val.SetN(valueToAddAsString);

            Aws::Map<Aws::String, Aws::String> names;
            Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>  values;

            names["#attr"] = attr;
            values[":val"] = val;

            updateRequest.SetUpdateExpression("Add #attr :val");
            updateRequest.SetExpressionAttributeNames(names);
            updateRequest.SetExpressionAttributeValues(values);
            updateRequest.AddKey(keyName, hk);
            updateRequest.SetReturnValues(Aws::DynamoDB::Model::ReturnValue::UPDATED_NEW);

            auto context = std::make_shared<FlowGraphStringContext>(Aws::String(attr), pActInfo->pGraph, pActInfo->myID);

            MARSHALL_AWS_BACKGROUND_REQUEST(DynamoDB, client, UpdateItem, FlowNode_CloudHashMapAddNumber::ApplyResult, updateRequest, context)
        }
    }

    void FlowNode_CloudHashMapAddNumber::ApplyResult(const Aws::DynamoDB::Model::UpdateItemRequest& request,
        const Aws::DynamoDB::Model::UpdateItemOutcome& outcome,
        const std::shared_ptr<const Aws::Client::AsyncCallerContext>& baseContext)
    {
        auto context = std::static_pointer_cast<const FlowGraphStringContext>(baseContext);

        auto attrToGet = context->GetExtra();
        auto flowGraph = context->GetFlowGraph();
        auto nodeId = context->GetFlowNodeId();

        if (context && outcome.IsSuccess())
        {
            const UpdateItemResult& result = const_cast<Aws::DynamoDB::Model::UpdateItemOutcome&>(outcome).GetResult();
            auto& attrs = result.GetAttributes();
            const auto& updatedAttribute = attrs.find(attrToGet);

            auto asNumber = std::stoi(updatedAttribute->second.GetN().c_str());

            SFlowAddress addr(nodeId, EOP_NewValue, true);
            flowGraph->ActivatePort(addr, asNumber);

            SuccessNotify(flowGraph, nodeId);
        }
        else if (context)
        {
            ErrorNotify(flowGraph, nodeId, outcome.GetError().GetMessage().c_str());
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_CloudHashMapAddNumber);
} //namescape Amazon