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
    class FlowNode_ConfigureAuthenticatedCognito
        : public BaseMaglevFlowNode< eNCT_Singleton >
    {
    public:
        FlowNode_ConfigureAuthenticatedCognito(SActivationInfo* activationInfo);

        virtual ~FlowNode_ConfigureAuthenticatedCognito() {}

        enum EInputs
        {
            EIP_Configure = EIP_StartIndex,
            EIP_AccountNumber,
            EIP_IdentityPool,
            EIP_ProviderName,
            EIP_ProviderToken
        };

        enum EOutputs
        {
            EOP_IdentityID = EOP_StartIndex
        };

    protected:
        const char* GetFlowNodeDescription() const override { return _HELP("Create an authenticated identity on the device in your AWS account. \
                                                                           The first time the player runs the game and this node is triggered, an ID is generated for the player.\
                                                                           The same ID will be returned any time the user logs in with the same account , even on a second device. \
                                                                           You MUST use one of the player authentication nodes to use any other AWS node."); }
        void ProcessEvent_Internal(EFlowEvent event, SActivationInfo* activationInfo) override;
        Aws::Vector<SInputPortConfig> GetInputPorts() const override;
        Aws::Vector<SOutputPortConfig> GetOutputPorts() const override;

        virtual const char* GetClassTag() const override;
    };
} // namespace AWS



