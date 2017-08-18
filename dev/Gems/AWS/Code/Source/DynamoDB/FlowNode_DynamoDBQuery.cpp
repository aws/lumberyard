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
#include <DynamoDB/FlowNode_DynamoDBQuery.h>
#include <aws/dynamodb/model/QueryRequest.h>
#include <aws/dynamodb/DynamoDBErrors.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <DynamoDB/DynamoDBUtils.h>
#include <memory>
#include <string.h>

using namespace Aws::DynamoDB::Model;
using namespace Aws::DynamoDB;

namespace LmbrAWS
{
    static const char* CLASS_TAG = "AWS:Primitive:DynamoDB:DynamoDBQuery";

    static const char*  DEFAULT_HASHKEY_NAME = "hk";

    FlowNode_DynamoDBQuery::FlowNode_DynamoDBQuery(SActivationInfo* activationInfo)
        : BaseMaglevFlowNode< eNCT_Singleton >(activationInfo)
    {
        Aws::StringStream ss;
        DynamoDBUtils::GetComparisonTypeEnumString(ss);
        m_comparisonTypeEnumString.assign(ss.str());

        ss.str("");
        DynamoDBUtils::GetDataEnumString(ss);
        m_dataTypeEnumString.assign(ss.str());
    }

    Aws::Vector<SInputPortConfig> FlowNode_DynamoDBQuery::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPorts = {
            InputPortConfig_Void("Query", _HELP("Activate this to query table data")),
            m_tableClientPort.GetConfiguration("TableName", _HELP("The name of the table to query")),
            InputPortConfig<string, string>("TableKeyName", DEFAULT_HASHKEY_NAME, _HELP("The name of the key used in this table")),
            InputPortConfig<string>("Key", _HELP("The key to search for"), "KeyValue"),
            InputPortConfig<string>("Attribute", _HELP("The attribute to query about"), "AttributeToCheck"),
            InputPortConfig<string, string>("AttributeComparison", DynamoDBUtils::GetDefaultComparisonString(), _HELP("The type of comparison to make against the attribute"), "AttributeComparisonType",
                _UICONFIG(m_comparisonTypeEnumString.c_str())),
            InputPortConfig<string>("AttributeComparisonValue", _HELP("The value to compare against the attribute")),
            InputPortConfig<string, string>("AttributeComparisonValueType", DynamoDBUtils::GetDefaultEnumString(), _HELP("The type of data AttributeComparisonValue is"), "AttributeComparisonValueType",
                _UICONFIG(m_dataTypeEnumString.c_str())),
        };

        return inputPorts;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_DynamoDBQuery::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts = {
            OutputPortConfig_AnyType("MatchFound", _HELP("A match was found")),
            OutputPortConfig_AnyType("NoMatch", _HELP("No match found")),
        };
        return outputPorts;
    }

    void FlowNode_DynamoDBQuery::ProcessEvent_Internal(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate && IsPortActive(pActInfo, EIP_StartQuery))
        {
            auto client = m_tableClientPort.GetClient(pActInfo);
            if (!client.IsReady())
            {
                ErrorNotify(pActInfo->pGraph, pActInfo->myID, "Client configuration not ready.");
                return;
            }

            auto& tableName = client.GetTableName();
            auto& tableKeyName = GetPortString(pActInfo, EIP_TableKeyName);
            auto& keyName = GetPortString(pActInfo, EIP_KeyName);
            auto& attributeName = GetPortString(pActInfo, EIP_AttributeName);
            auto& attributeComparison = GetPortString(pActInfo, EIP_AttributeComparisonType);

            Aws::DynamoDB::Model::QueryRequest queryRequest;
            queryRequest.SetTableName(tableName);

            Aws::String comparisonString;

            queryRequest.AddExpressionAttributeNames("#tk", tableKeyName);

            comparisonString += "#tk";
            comparisonString += "=";
            comparisonString += ":cv";

            AttributeValue attrVal;
            attrVal.SetS(keyName.c_str());

            queryRequest.AddExpressionAttributeValues(":cv", attrVal);

            if (attributeName.length())
            {
                queryRequest.AddExpressionAttributeNames("#an", attributeName);

                AttributeValue comparisonAttr = DynamoDBUtils::MakeAttributeValue(GetPortString(pActInfo, EIP_AttributeComparisonValueType), GetPortString(pActInfo, EIP_AttributeComparisonValue));

                queryRequest.AddExpressionAttributeValues(":av", comparisonAttr);
                Aws::String atrComparisonString = "#an " + DynamoDBUtils::GetDynamoDBStringByLabel(attributeComparison.c_str()) + " :av";
                queryRequest.SetFilterExpression(atrComparisonString);
            }
            queryRequest.SetKeyConditionExpression(comparisonString);

            auto context = std::make_shared<FlowGraphContext>(pActInfo->pGraph, pActInfo->myID);
            MARSHALL_AWS_BACKGROUND_REQUEST(DynamoDB, client, Query, FlowNode_DynamoDBQuery::ApplyResult, queryRequest, context)
        }
    }

    void FlowNode_DynamoDBQuery::ApplyResult(const Aws::DynamoDB::Model::QueryRequest& request,
        const Aws::DynamoDB::Model::QueryOutcome& outcome,
        const std::shared_ptr<const Aws::Client::AsyncCallerContext>& baseContext)
    {
        if (!baseContext)
        {
            return;
        }

        auto context = std::static_pointer_cast<const FlowGraphContext>(baseContext);

        auto flowGraph = context->GetFlowGraph();
        auto nodeId = context->GetFlowNodeId();

        if (outcome.IsSuccess())
        {
            SuccessNotify(flowGraph, nodeId);
            if (outcome.GetResult().GetCount())
            {
                SFlowAddress addr(nodeId, EOP_MatchFound, true);
                flowGraph->ActivatePort(addr, 0);
            }
            else
            {
                SFlowAddress addr(nodeId, EOP_NoMatch, true);
                flowGraph->ActivatePort(addr, 0);
            }
        }
        else
        {
            ErrorNotify(flowGraph, nodeId, outcome.GetError().GetMessage().c_str());
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_DynamoDBQuery);
} //namescape Amazon