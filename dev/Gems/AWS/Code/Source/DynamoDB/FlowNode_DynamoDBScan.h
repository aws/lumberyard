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
    /// Checks over a table for entries where a given attribute passes a comparison check
    ///
    //////////////////////////////////////////////////////////////////////////////////////
    class FlowNode_DynamoDBScan
        : public BaseMaglevFlowNode< eNCT_Singleton >
    {
    public:

        FlowNode_DynamoDBScan(SActivationInfo* activationInfo);

        // ~IFlowNode
        virtual ~FlowNode_DynamoDBScan() {}

    protected:
        const char* GetFlowNodeDescription() const override { return _HELP("Scan for entries which pass a comparison test in DynamoDB"); }
        void ProcessEvent_Internal(EFlowEvent event, SActivationInfo* activationInfo) override;
        Aws::Vector<SInputPortConfig> GetInputPorts() const override;
        Aws::Vector<SOutputPortConfig> GetOutputPorts() const override;

        virtual const char* GetClassTag() const override;
    private:
        enum EInputs
        {
            EIP_StartScan = EIP_StartIndex,
            EIP_TableClient,
            EIP_AttributeName,
            EIP_AttributeComparisonType,
            EIP_AttributeComparisonValue,
            EIP_AttributeComparisonValueType
        };

        enum EOutputs
        {
            EOP_MatchesFound = EOP_StartIndex,
        };

        LmbrAWS::DynamoDB::TableClientInputPort m_tableClientPort {
            EIP_TableClient
        };

        Aws::String m_comparisonTypeEnumString;
        Aws::String m_dataTypeEnumString;

        void ApplyResult(const Aws::DynamoDB::Model::ScanRequest& request,
            const Aws::DynamoDB::Model::ScanOutcome& outcome,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& baseContext);
    };
} // namespace LmbrAWS



