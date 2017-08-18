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

#pragma once
#include <Util/FlowSystem/BaseMaglevFlowNode.h>
#pragma warning(disable: 4355) // <future> includes ppltasks.h which throws a C4355 warning: 'this' used in base member initializer list
#include <aws/dynamodb/DynamoDBClient.h>
#pragma warning(default: 4355)

namespace LmbrAWS
{
    //////////////////////////////////////////////////////////////////////////////////////
    ///
    /// For a given table/key combo will perform a comparison check on an attribute
    ///
    //////////////////////////////////////////////////////////////////////////////////////
    class FlowNode_DynamoDBQuery
        : public BaseMaglevFlowNode< eNCT_Singleton >
    {
    public:

        FlowNode_DynamoDBQuery(SActivationInfo* activationInfo);

        // ~IFlowNode
        virtual ~FlowNode_DynamoDBQuery() {}

        virtual const char* GetClassTag() const override;
    protected:
        const char* GetFlowNodeDescription() const override { return _HELP("Query values in DynamoDB"); }
        void ProcessEvent_Internal(EFlowEvent event, SActivationInfo* activationInfo) override;
        Aws::Vector<SInputPortConfig> GetInputPorts() const override;
        Aws::Vector<SOutputPortConfig> GetOutputPorts() const override;

    private:
        enum EInputs
        {
            EIP_StartQuery = EIP_StartIndex,
            EIP_TableClient,
            EIP_TableKeyName,
            EIP_KeyName,
            EIP_AttributeName,
            EIP_AttributeComparisonType,
            EIP_AttributeComparisonValue,
            EIP_AttributeComparisonValueType,
        };

        enum EOutputs
        {
            EOP_MatchFound = EOP_StartIndex,
            EOP_NoMatch
        };

        LmbrAWS::DynamoDB::TableClientInputPort m_tableClientPort {
            EIP_TableClient
        };
        Aws::String m_comparisonTypeEnumString;
        Aws::String m_dataTypeEnumString;

        void ApplyResult(const Aws::DynamoDB::Model::QueryRequest& request,
            const Aws::DynamoDB::Model::QueryOutcome& outcome,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& baseContext);
    };
} // namespace LmbrAWS



