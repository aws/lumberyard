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
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <aws/core/utils/json/JsonSerializer.h>

namespace LmbrAWS
{
    class FlowNode_IterateJsonArray
        : public CFlowBaseNode< eNCT_Instanced >
    {
    public:
        FlowNode_IterateJsonArray(IFlowNode::SActivationInfo* activationInfo)
        {};

        virtual void GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }
        virtual void GetConfiguration(SFlowNodeConfig& configuration) override;

        virtual void ProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* activationInfo) override;
        virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) override;
        virtual const char* GetClassTag() const override;
    private:

        enum EInputs
        {
            EIP_Begin = 0,
            EIP_Continue,
            EIP_JsonArray
        };

        enum EOutputs
        {
            EOP_NextValue = 0,
            EOP_IsNotEmpty,
            EOP_IsEmpty,
            EOP_Error
        };

    private:
        std::queue<Aws::Utils::Json::JsonValue> m_jsonArray;
    };
} // namespace Amazon



