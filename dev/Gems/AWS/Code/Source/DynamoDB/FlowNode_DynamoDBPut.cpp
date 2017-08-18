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
#include <DynamoDB/FlowNode_DynamoDBPut.h>
#include <DynamoDB/DynamoDBUtils.h>
#include <aws/dynamodb/model/PutItemRequest.h>
#include <aws/dynamodb/DynamoDBErrors.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <memory>

using namespace Aws::DynamoDB::Model;
using namespace Aws::DynamoDB;

namespace LmbrAWS
{
    static const char* CLASS_TAG = "AWS:Primitive:DynamoDB:DynamoDBPut";

    static const char*  DEFAULT_HASHKEY_NAME = "hk";

    FlowNode_DynamoDBPut::FlowNode_DynamoDBPut(IFlowNode::SActivationInfo* activationInfo)
        : BaseMaglevFlowNode< eNCT_Singleton >(activationInfo)
    {
        Aws::StringStream ssdataType;
        DynamoDBUtils::GetDataEnumString(ssdataType);
        m_dataTypeEnumString.assign(ssdataType.str());
    }

    Aws::Vector<SInputPortConfig> FlowNode_DynamoDBPut::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPorts = {
            InputPortConfig_Void("PutItem", _HELP("Activate this to write data to the cloud")),
            m_tableClientPort.GetConfiguration("TableName", _HELP("The name of the table to use")),
            InputPortConfig<string, string>("TableKeyName", DEFAULT_HASHKEY_NAME, _HELP("Key name used in this table")),
            InputPortConfig<string>("Key", _HELP("The key you wish to write"), "KeyValue"),
            InputPortConfig<string>("Attribute", _HELP("The attribute you wish to write"), "AttributeToWrite"),
            InputPortConfig<string>("DataIn", _HELP("The data to write")),
            InputPortConfig<string, string>("DataType", DynamoDBUtils::GetDefaultEnumString(), _HELP("The data type to write as"), "DataType",
                _UICONFIG(m_dataTypeEnumString.c_str())),
            InputPortConfig<bool>("KeyMustNotExist", true, _HELP("The key must not already exist"))
        };

        return inputPorts;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_DynamoDBPut::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts = {
            OutputPortConfig_Void("KeyAlreadyExists", _HELP("Key already exists, no change made")),
        };
        return outputPorts;
    }

    void FlowNode_DynamoDBPut::ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate && IsPortActive(pActInfo, EIP_StartPut))
        {
            auto client = m_tableClientPort.GetClient(pActInfo);
            if (!client.IsReady())
            {
                ErrorNotify(pActInfo->pGraph, pActInfo->myID, "Client configuration not ready.");
                return;
            }

            auto& tableName = client.GetTableName();
            auto& keyName = GetPortString(pActInfo, EIP_TableKeyName);
            auto& key = GetPortString(pActInfo, EIP_Key);
            auto& attribute = GetPortString(pActInfo, EIP_Attribute);
            bool keyMustNotExist = GetPortBool(pActInfo, EIP_KeyMustNotExist);

            Aws::DynamoDB::Model::PutItemRequest putRequest;
            putRequest.SetTableName(tableName);
            AttributeValue hk;
            hk.SetS(key);
            putRequest.AddItem(keyName, hk);

            putRequest.AddItem(attribute, DynamoDBUtils::MakeAttributeValue(GetPortString(pActInfo, EIP_DataType), GetPortString(pActInfo, EIP_Value)));

            if (keyMustNotExist)
            {
                Aws::String conditionStr;
                DynamoDBUtils::AddAttributeRequirement("attribute_not_exists", "#kn", conditionStr);
                putRequest.AddExpressionAttributeNames("#kn", keyName);
                putRequest.SetConditionExpression(conditionStr);
            }

            auto context = std::make_shared<FlowGraphContext>(pActInfo->pGraph, pActInfo->myID);
            MARSHALL_AWS_BACKGROUND_REQUEST(DynamoDB, client, PutItem, FlowNode_DynamoDBPut::ApplyResult, putRequest, context)
        }
    }

    void FlowNode_DynamoDBPut::ApplyResult(const Aws::DynamoDB::Model::PutItemRequest& request,
        const Aws::DynamoDB::Model::PutItemOutcome& outcome,
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
            SuccessNotify(flowGraph, nodeId);
        }
        else
        {
            if (outcome.GetError().GetErrorType() == DynamoDBErrors::CONDITIONAL_CHECK_FAILED)
            {
                SFlowAddress addr(nodeId, EOP_KeyAlreadyExists, true);
                flowGraph->ActivatePortCString(addr, "");
            }
            ErrorNotify(flowGraph, nodeId, outcome.GetError().GetMessage().c_str());
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_DynamoDBPut);
} //namescape Amazon