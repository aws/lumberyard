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
#include <DynamoDB/FlowNode_DynamoDBUpdate.h>
#include <DynamoDB/DynamoDBUtils.h>
#include <aws/dynamodb/model/UpdateItemRequest.h>
#include <aws/dynamodb/DynamoDBErrors.h>
#include <aws/core/utils/Outcome.h>
#include <memory>

using namespace Aws::DynamoDB::Model;
using namespace Aws::DynamoDB;

namespace LmbrAWS
{
    static const char* CLASS_TAG = "AWS:Primitive:DynamoDB:DynamoDBUpdate";

    static const char*  DEFAULT_HASHKEY_NAME = "hk";

    FlowNode_DynamoDBUpdate::FlowNode_DynamoDBUpdate(SActivationInfo* activationInfo)
        : BaseMaglevFlowNode< eNCT_Singleton >(activationInfo)
    {
        Aws::StringStream ssdataType;
        DynamoDBUtils::GetDataEnumString(ssdataType);
        m_dataTypeEnumString.assign(ssdataType.str());
    }

    Aws::Vector<SInputPortConfig> FlowNode_DynamoDBUpdate::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPorts = {
            InputPortConfig_Void("UpdateItem", _HELP("Activate this to write data to the cloud on an existing item")),
            m_tableClientPort.GetConfiguration("TableName", _HELP("The name of the table to use")),
            InputPortConfig<string, string>("TableKeyName", DEFAULT_HASHKEY_NAME, _HELP("Key name to use in this table")),
            InputPortConfig<string>("Key", _HELP("The key you wish to write"), "KeyValue"),
            InputPortConfig<string>("Attribute", _HELP("The attribute you wish to write"), "AttributeToWrite"),
            InputPortConfig<string>("DataIn", _HELP("The data to write")),
            InputPortConfig<string, string>("DataType", DynamoDBUtils::GetDefaultEnumString(), _HELP("The data type to write as"), "DataType",
                _UICONFIG(m_dataTypeEnumString.c_str())),
            InputPortConfig<bool>("KeyMustExist", true, _HELP("The key must already exist in the table")),
            InputPortConfig<bool>("AttributeMustExist", true, _HELP("The attribute must already exist for that key")),
        };

        return inputPorts;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_DynamoDBUpdate::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts = {
            OutputPortConfig_Void("ConditionsFailed", _HELP("Key or attribute not found")),
        };
        return outputPorts;
    }

    void FlowNode_DynamoDBUpdate::ProcessEvent_Internal(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate && IsPortActive(pActInfo, EIP_StartUpdate))
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
            auto& attributeStr = GetPortString(pActInfo, EIP_Attribute);
            auto keyMustExist = GetPortBool(pActInfo, EIP_KeyMustExist);
            auto attributeMustExist = GetPortBool(pActInfo, EIP_AttributeMustExist);

            Aws::DynamoDB::Model::UpdateItemRequest updateRequest;
            updateRequest.SetTableName(tableName);

            AttributeValue hk;
            hk.SetS(key);

            updateRequest.AddKey(keyName, hk);

            Aws::String conditionStr;

            if (keyMustExist)
            {
                DynamoDBUtils::AddAttributeRequirement("attribute_exists", "#kn", conditionStr);
                updateRequest.AddExpressionAttributeNames("#kn", keyName);
            }

            if (attributeMustExist)
            {
                DynamoDBUtils::AddAttributeRequirement("attribute_exists", "#an", conditionStr);
            }

            if (conditionStr.length())
            {
                updateRequest.SetConditionExpression(conditionStr);
            }

            Aws::String updateStr("SET ");

            updateStr += "#an";
            updateStr += " = ";
            updateStr += ":uv";

            AttributeValue attrVal = DynamoDBUtils::MakeAttributeValue(GetPortString(pActInfo, EIP_DataType), GetPortString(pActInfo, EIP_Value));

            updateRequest.AddExpressionAttributeNames("#an", attributeStr);
            updateRequest.AddExpressionAttributeValues(":uv", attrVal);

            updateRequest.SetUpdateExpression(updateStr);

            auto context = std::make_shared<FlowGraphContext>(pActInfo->pGraph, pActInfo->myID);
            MARSHALL_AWS_BACKGROUND_REQUEST(DynamoDB, client, UpdateItem, FlowNode_DynamoDBUpdate::ApplyResult, updateRequest, context)
        }
    }

    void FlowNode_DynamoDBUpdate::ApplyResult(const Aws::DynamoDB::Model::UpdateItemRequest& request,
        const Aws::DynamoDB::Model::UpdateItemOutcome& outcome,
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
                SFlowAddress addr(nodeId, EOP_ConditionsFailed, true);
                flowGraph->ActivatePortCString(addr, "");
            }

            ErrorNotify(flowGraph, nodeId, outcome.GetError().GetMessage().c_str());
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_DynamoDBUpdate);
} //namescape Amazon