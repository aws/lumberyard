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
#include <StdAfx.h>
#include <Configuration/FlowNode_ConfigureAnonymousCognito.h>
#include <aws/identity-management/auth/CognitoCachingCredentialsProvider.h>
#include <aws/identity-management/auth/PersistentCognitoIdentityProvider.h>
#include <LmbrAWS/ILmbrAWS.h>
#include <LmbrAWS/IAWSClientManager.h>

namespace LmbrAWS
{
    static const char* CLASS_TAG = "AWS:Configuration:ConfigureAnonymousPlayer";

    Aws::Vector<SInputPortConfig> FlowNode_ConfigureAnonymousCognito::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPortConfiguration =
        {
            InputPortConfig_Void("Configure", _HELP("Configure your game to use Cognito anonymously.")),
            InputPortConfig<string>("AWSAccountNumber", _HELP("Your AWS account number. This is needed for configuring Cognito")),
            InputPortConfig<string>("IdentityPool", _HELP("The Unique ID of your Cognito Identity Pool. AWSConsole->Cognito->Your Pool Name->Edit Identity Pool->Identity Pool Id"), "IdentityPoolID"),
            InputPortConfig<string>("IdentityFileOverride", _HELP("If specified, will cause the Cognito ID to be cached to this path instead of the usual place (HOME_DIR)/.aws/.identities ."), "CachingFileLocationOverride")
        };

        return inputPortConfiguration;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_ConfigureAnonymousCognito::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts = {
            OutputPortConfig<string>("Cognito IdentityID", _HELP("The unique ID of the user."))
        };
        return outputPorts;
    }

    void FlowNode_ConfigureAnonymousCognito::ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* activationInfo)
    {
        if (event == eFE_Activate && activationInfo->pInputPorts[EIP_Configure].IsUserFlagSet())
        {
            auto awsAccountNumber = Aws::String(GetPortString(activationInfo, EIP_AccountNumber));
            auto identityPool = Aws::String(GetPortString(activationInfo, EIP_IdentityPool));
            std::shared_ptr<Aws::Auth::PersistentCognitoIdentityProvider> identityProvider;

            const string& identityFileOverride = GetPortString(activationInfo, EIP_IdentityFileOverride);

            if (identityFileOverride != "")
            {
                identityProvider = Aws::MakeShared<Aws::Auth::DefaultPersistentCognitoIdentityProvider>(CLASS_TAG, identityPool, awsAccountNumber, identityFileOverride.c_str());
            }
            else
            {
                identityProvider = Aws::MakeShared<Aws::Auth::DefaultPersistentCognitoIdentityProvider>(CLASS_TAG, identityPool, awsAccountNumber);
            }

            if (!awsAccountNumber.empty() && !identityPool.empty())
            {
                auto credProvider = Aws::MakeShared<Aws::Auth::CognitoCachingAnonymousCredentialsProvider>(CLASS_TAG, identityProvider);

                auto credentials = credProvider->GetAWSCredentials();
                if (credentials.GetAWSAccessKeyId() == "")
                {
                    ErrorNotify(activationInfo->pGraph, activationInfo->myID, "Unable to configure Cognito. No credentials were returned");
                }
                else
                {
                    auto clientManager = gEnv->pLmbrAWS->GetClientManager();
                    clientManager->GetDefaultClientSettings().credentialProvider = credProvider;

                    clientManager->ApplyConfiguration();

                    SFlowAddress addr(activationInfo->myID, EOP_IdentityID, true);
                    activationInfo->pGraph->ActivatePortCString(addr, identityProvider->GetIdentityId().c_str());
                    SuccessNotify(activationInfo->pGraph, activationInfo->myID);
                }
            }
            else
            {
                ErrorNotify(activationInfo->pGraph, activationInfo->myID, "Unable to configure cognito identity provider.");
            }
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_ConfigureAnonymousCognito);
}