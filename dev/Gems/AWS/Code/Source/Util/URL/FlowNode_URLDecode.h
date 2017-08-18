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

namespace LmbrAWS
{
    class FlowNode_URLDecode
        : public CFlowBaseNode< eNCT_Singleton >
    {
    public:
        FlowNode_URLDecode(IFlowNode::SActivationInfo* activationInfo)
        {};


        virtual void GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }
        virtual void GetConfiguration(SFlowNodeConfig& configuration) override;

        virtual void ProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* activationInfo) override;

        virtual const char* GetClassTag() const override;
    };
} // namespace LmbrAWS



