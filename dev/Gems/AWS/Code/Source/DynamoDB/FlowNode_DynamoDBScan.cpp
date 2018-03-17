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
#include <DynamoDB/FlowNode_DynamoDBScan.h>
#include <aws/dynamodb/model/ScanRequest.h>
#include <aws/dynamodb/DynamoDBErrors.h>
#include <aws/core/utils/Outcome.h>
#include <memory>
#include <DynamoDB/DynamoDBUtils.h>

#include <CloudCanvas/CloudCanvasMappingsBus.h>
#include <CloudGemFramework/AwsApiRequestJob.h>

using namespace Aws::DynamoDB::Model;
using namespace Aws::DynamoDB;

namespace LmbrAWS
{
    static const char* CLASS_TAG = "AWS:Primitive:DynamoDB:DynamoDBScan";

    FlowNode_DynamoDBScan::FlowNode_DynamoDBScan(SActivationInfo* activationInfo)
        : BaseMaglevFlowNode< eNCT_Singleton >(activationInfo)
    {
        Aws::StringStream ss;
        DynamoDBUtils::GetComparisonTypeEnumString(ss);
        m_comparisonTypeEnumString.assign(ss.str());

        ss.str("");
        DynamoDBUtils::GetDataEnumString(ss);
        m_dataTypeEnumString.assign(ss.str());
    }

    Aws::Vector<SInputPortConfig> FlowNode_DynamoDBScan::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPorts = {
            InputPortConfig_Void("Scan", _HELP("Activate this to scan table data")),
            InputPortConfig<string>("TableName", _HELP("The name of the table to scan")),
            InputPortConfig<string>("Attribute", _HELP("The attribute to scan about")),
            InputPortConfig<string, string>("AttributeComparison", DynamoDBUtils::GetDefaultComparisonString(), _HELP("The type of comparison to make against the attribute"), "AttributeComparisonType",
                _UICONFIG(m_comparisonTypeEnumString.c_str())),
            InputPortConfig<string>("AttributeComparisonValue", _HELP("The value to compare against the attribute")),
            InputPortConfig<string, string>("AttributeComparisonValueType", DynamoDBUtils::GetDefaultEnumString(), _HELP("The type of data AttributeComparisonValue is"), "AttributeComparisonValueType",
                _UICONFIG(m_dataTypeEnumString.c_str())),
        };

        return inputPorts;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_DynamoDBScan::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts = {
            OutputPortConfig_AnyType("MatchesFound", _HELP("How many matches were found on a successful scan")),
        };
        return outputPorts;
    }

    void FlowNode_DynamoDBScan::ProcessEvent_Internal(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate && IsPortActive(pActInfo, EIP_StartScan))
        {

            AZStd::string tableName = GetPortString(pActInfo, EIP_TableClient).c_str();
            EBUS_EVENT_RESULT(tableName, CloudGemFramework::CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, tableName);

            auto& attributeName = GetPortString(pActInfo, EIP_AttributeName);
            auto& attributeComparison = GetPortString(pActInfo, EIP_AttributeComparisonType);
            auto& attributeComparisonValue = GetPortString(pActInfo, EIP_AttributeComparisonValue);

            auto context = std::make_shared<FlowGraphContext>(pActInfo->pGraph, pActInfo->myID);

            using ScanRequestJob = AWS_API_REQUEST_JOB(DynamoDB, Scan);
            ScanRequestJob::Config config(ScanRequestJob::GetDefaultConfig());

            auto requestJob = ScanRequestJob::Create(
                [this, context](ScanRequestJob* successJob) // OnSuccess handler
            {
                auto flowGraph = context->GetFlowGraph();
                auto nodeId = context->GetFlowNodeId();
                SuccessNotify(flowGraph, nodeId);

                SFlowAddress addr(nodeId, EOP_MatchesFound, true);
                flowGraph->ActivatePort(addr, static_cast<int>(successJob->result.GetCount()));
            },
                [this, context](ScanRequestJob* failureJob) // OnError handler
            {
                auto flowGraph = context->GetFlowGraph();
                auto nodeId = context->GetFlowNodeId();
                ErrorNotify(flowGraph, nodeId, failureJob->error.GetMessage().c_str());

            },
                &config
                );
            requestJob->request.SetTableName(tableName.c_str());

            if (attributeName.length())
            {
                requestJob->request.AddExpressionAttributeNames("#an", attributeName);

                AttributeValue attrVal = DynamoDBUtils::MakeAttributeValue(GetPortString(pActInfo, EIP_AttributeComparisonValueType), GetPortString(pActInfo, EIP_AttributeComparisonValue));
                requestJob->request.AddExpressionAttributeValues(":av", attrVal);
                Aws::String comparisonString = "#an " + DynamoDBUtils::GetDynamoDBStringByLabel(attributeComparison) + " :av";
                requestJob->request.SetFilterExpression(comparisonString);
            }

            requestJob->Start();
        }
    }

    void FlowNode_DynamoDBScan::ApplyResult(const Aws::DynamoDB::Model::ScanRequest& request,
        const Aws::DynamoDB::Model::ScanOutcome& outcome,
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

            SFlowAddress addr(nodeId, EOP_MatchesFound, true);
            flowGraph->ActivatePort(addr, static_cast<int>(outcome.GetResult().GetCount()));
        }
        else
        {
            ErrorNotify(flowGraph, nodeId, outcome.GetError().GetMessage().c_str());
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_DynamoDBScan);
} //namescape Amazon