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
// The AWS Native SDK AWSAllocator triggers a warning due to accessing members of std::allocator directly.
// AWSAllocator.h(70): warning C4996: 'std::allocator<T>::pointer': warning STL4010: Various members of std::allocator are deprecated in C++17.
// Use std::allocator_traits instead of accessing these members directly.
// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.

// The AWS gem does not use AzCore, therefore the AZ_PUSH_DISABLE_WARNING macro is not available
#pragma warning(push)
#pragma warning(disable : 4251 4996)
#include <aws/core/utils/json/JsonSerializer.h>
#pragma warning(pop)

namespace LmbrAWS
{
    class FlowNode_SetJsonProperty
        : public CFlowBaseNode< eNCT_Instanced >
    {
    public:
        FlowNode_SetJsonProperty(IFlowNode::SActivationInfo* activationInfo)
        {};

        virtual void GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }
        virtual void GetConfiguration(SFlowNodeConfig& configuration) override;

        virtual void ProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* activationInfo) override;
        virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) override;

        virtual const char* GetClassTag() const override;
    private:

        enum EInputs
        {
            EIP_In = 0,
            EIP_JsonObject,
            EIP_Name,
            EIP_Value
        };

        enum EOutputs
        {
            EOP_Out = 0
        };
    };
} // namespace Amazon



