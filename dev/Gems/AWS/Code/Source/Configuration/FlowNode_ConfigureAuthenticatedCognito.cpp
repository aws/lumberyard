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
#include <AWS_precompiled.h>
#include <Configuration/FlowNode_ConfigureAuthenticatedCognito.h>
#include <aws/identity-management/auth/CognitoCachingCredentialsProvider.h>
#include <aws/identity-management/auth/PersistentCognitoIdentityProvider.h>

#include <CloudCanvas/CloudCanvasIdentityBus.h>

using namespace Aws;
using namespace Aws::Client;

namespace LmbrAWS
{
    static const char* CLASS_TAG = "AWS:Configuration:ConfigureAuthenticatedPlayer";

    FlowNode_ConfigureAuthenticatedCognito::FlowNode_ConfigureAuthenticatedCognito(SActivationInfo* activationInfo)
        : BaseMaglevFlowNode< eNCT_Singleton >(activationInfo)
    {
    }

    Aws::Vector<SInputPortConfig> FlowNode_ConfigureAuthenticatedCognito::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPortConfiguration =
        {
            InputPortConfig_Void("Configure", _HELP("Configure your game to use Cognito using the given values")),
            InputPortConfig<string>("AWSAccountNumber", _HELP("Your AWS account number. This is needed for configuring Cognito")),
            InputPortConfig<string>("IdentityPool", _HELP("The unique id of your Cognito Identity Pool. AWSConsole->Cognito->Your Pool Name->Edit Identity Pool->Identity Pool Id"), "IdentityPoolID"),
            InputPortConfig<string>("ProviderName", _HELP("Provider name to authenticate your user with."), "ProviderName",
                _UICONFIG("enum_string:Amazon=www.amazon.com,Facebook=graph.facebook.com,Google=accounts.google.com")),
            InputPortConfig<string>("ProviderToken", _HELP("Valid Provider token with which to authenticate the user"))
        };

        return inputPortConfiguration;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_ConfigureAuthenticatedCognito::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts = {
            OutputPortConfig<string>("Cognito IdentityID", _HELP("The unique ID of the user."))
        };
        return outputPorts;
    }

    void FlowNode_ConfigureAuthenticatedCognito::ProcessEvent_Internal(EFlowEvent event, SActivationInfo* activationInfo)
    {
        if (event == eFE_Activate && activationInfo->pInputPorts[0].IsUserFlagSet())
        {
            auto awsAccountNumber = Aws::String(GetPortString(activationInfo, EIP_AccountNumber));
            auto identityPool = Aws::String(GetPortString(activationInfo, EIP_IdentityPool));
            auto providerName = Aws::String(GetPortString(activationInfo, EIP_ProviderName));
            auto providerToken = Aws::String(GetPortString(activationInfo, EIP_ProviderToken));

            if (!awsAccountNumber.empty() && !identityPool.empty())
            {
                auto identityProvider = Aws::MakeShared<Aws::Auth::DefaultPersistentCognitoIdentityProvider>(CLASS_TAG, identityPool, awsAccountNumber);
                Aws::Auth::LoginAccessTokens accessTokens;
                accessTokens.accessToken = providerToken;
                accessTokens.longTermToken = providerToken;
                identityProvider->PersistLogins(Aws::Map<String, Aws::Auth::LoginAccessTokens>{
                        {providerName, accessTokens}
                    });

                auto credProvider = Aws::MakeShared<Aws::Auth::CognitoCachingAuthenticatedCredentialsProvider>(CLASS_TAG, identityProvider);
                auto credentials = credProvider->GetAWSCredentials();
                if (credentials.GetAWSAccessKeyId() == "")
                {
                    ErrorNotify(activationInfo->pGraph, activationInfo->myID, "Unable to configure Cognito. No credentials were returned");
                }
                else
                {
                    EBUS_EVENT(CloudGemFramework::CloudCanvasPlayerIdentityBus, SetPlayerCredentialsProvider, credProvider);
                    EBUS_EVENT(CloudGemFramework::CloudCanvasPlayerIdentityBus, ApplyConfiguration);

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

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_ConfigureAuthenticatedCognito);
}