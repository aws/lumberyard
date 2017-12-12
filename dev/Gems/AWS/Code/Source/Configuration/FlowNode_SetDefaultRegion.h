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

namespace LmbrAWS
{
    class FlowNode_SetDefaultRegion
        : public BaseMaglevFlowNode<eNCT_Singleton>
    {
    public:

        FlowNode_SetDefaultRegion(IFlowNode::SActivationInfo* activationInfo)
            : BaseMaglevFlowNode< eNCT_Singleton >(activationInfo)
        {};

        virtual ~FlowNode_SetDefaultRegion() {}

        enum EInputs
        {
            EIP_Activate = EIP_StartIndex,
            EIP_Region
        };

    protected:
        virtual const char* GetClassTag() const override;

    private:
        const char* GetFlowNodeDescription() const override
        {
            return _HELP("Set (override) region for all AWS clients in current project. \
                Choose Apply if you want to immediately apply the configuration change to all AWS clients. \
                If Apply is set to false you will need to add ApplyConfiguration flow node in order to activate changes.");
        }
        void ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* activationInfo) override;
        Aws::Vector<SInputPortConfig> GetInputPorts() const override;
        Aws::Vector<SOutputPortConfig> GetOutputPorts() const override;
    };
} // namespace Amazon

