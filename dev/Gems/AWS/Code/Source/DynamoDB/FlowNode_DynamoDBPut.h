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
    /// Creates a distributed hashmap in the sky
    ///
    //////////////////////////////////////////////////////////////////////////////////////
    class FlowNode_DynamoDBPut
        : public BaseMaglevFlowNode< eNCT_Singleton >
    {
    public:

        FlowNode_DynamoDBPut(IFlowNode::SActivationInfo* activationInfo);

        // ~IFlowNode
        virtual ~FlowNode_DynamoDBPut() {}

    protected:
        const char* GetFlowNodeDescription() const override { return _HELP("Put values in dynamo DB"); }
        void ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* activationInfo) override;
        Aws::Vector<SInputPortConfig> GetInputPorts() const override;
        Aws::Vector<SOutputPortConfig> GetOutputPorts() const override;

        virtual const char* GetClassTag() const override;
    private:

        enum EInputs
        {
            EIP_StartPut = EIP_StartIndex,
            EIP_TableClient,
            EIP_TableKeyName,
            EIP_Key,
            EIP_Attribute,
            EIP_Value,
            EIP_DataType,
            EIP_KeyMustNotExist,
        };

        enum EOutputs
        {
            EOP_KeyAlreadyExists = EOP_StartIndex
        };

        LmbrAWS::DynamoDB::TableClientInputPort m_tableClientPort {
            EIP_TableClient
        };
        Aws::String m_dataTypeEnumString;

        void ApplyResult(const Aws::DynamoDB::Model::PutItemRequest& request,
            const Aws::DynamoDB::Model::PutItemOutcome& outcome,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& baseContext);
    };
} // namespace Amazon



