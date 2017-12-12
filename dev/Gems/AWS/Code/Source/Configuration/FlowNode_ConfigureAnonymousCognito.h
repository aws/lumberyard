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
    //////////////////////////////////////////////////////////////////////////////////////
    ///
    /// Configures game-time components to use Cognitos credentials configured via this flow node
    ///
    //////////////////////////////////////////////////////////////////////////////////////
    class FlowNode_ConfigureAnonymousCognito
        : public BaseMaglevFlowNode< eNCT_Singleton >
    {
    public:
        FlowNode_ConfigureAnonymousCognito(IFlowNode::SActivationInfo* activationInfo)
            : BaseMaglevFlowNode< eNCT_Singleton >(activationInfo)
        {};

        virtual ~FlowNode_ConfigureAnonymousCognito() {}

        enum EInputs
        {
            EIP_Configure = EIP_StartIndex,
            EIP_AccountNumber,
            EIP_IdentityPool,
            EIP_IdentityFileOverride
        };

        enum EOutputs
        {
            EOP_IdentityID = EOP_StartIndex
        };

    protected:
        const char* GetFlowNodeDescription() const override { return _HELP("Create an anonymous identity on the device in your AWS account. \
                                                                           The first time the player runs the game and this node is triggered, an anonymous ID is generated for the player.\
                                                                           This ID is persisted locally and future runs of the game will use the same identity.\
                                                                           You MUST use one of the player authentication nodes to use any other AWS node."); }
        void ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* activationInfo) override;
        Aws::Vector<SInputPortConfig> GetInputPorts() const override;
        Aws::Vector<SOutputPortConfig> GetOutputPorts() const override;

        virtual const char* GetClassTag() const override;
    };
} // namespace AWS



