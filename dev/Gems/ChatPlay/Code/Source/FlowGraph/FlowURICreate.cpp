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
////////////////////////////////////////////////////////////////////////////
//
//   Flow node to create Twitch JoinIn link
//
////////////////////////////////////////////////////////////////////////////

#include "ChatPlay_precompiled.h"

#include <sstream>
#include <Base64.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <GridMate/GridMate.h>
#include <GridMate/NetworkGridMate.h>
#include <GridMate/Session/LANSession.h>
#include <JoinIn/JoinInCVars.h>

namespace ChatPlay
{

    /******************************************************************************/
    // Create Twitch JoinIn link

    class CFlowNode_TwitchJoinInLinkCreate
        : public CFlowBaseNode < eNCT_Singleton >
    {
        enum InputPorts
        {
            Activate = 0,
            Command
        };

        enum OutputPorts
        {
            Out = 0,
            Error
        };

    public:

        explicit CFlowNode_TwitchJoinInLinkCreate(IFlowNode::SActivationInfo*)
        {
        }

        ~CFlowNode_TwitchJoinInLinkCreate() override
        {
        }
        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Activate", _HELP("Generate the link")),
                InputPortConfig<string>("Command", _HELP("Command that will be embedded in the Twitch JoinIn link")),
                { 0 }
            };
            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig<string>("Out", _HELP("Generated link")),
                OutputPortConfig<bool>("Error", _HELP("Signalled if there was an error generating the link")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Create a Twitch JoinIn link");
            config.SetCategory(EFLN_APPROVED);
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
        {
            switch (event)
            {
            case eFE_Initialize:
                break;

            case eFE_Activate:

                if (IsPortActive(pActInfo, InputPorts::Activate))
                {
                    AZStd::string retVal;
                    if (gEnv && gEnv->pNetwork)
                    {
                        GridMate::Network* gm = static_cast<GridMate::Network*>(gEnv->pNetwork);
                        GridMate::GridSession* gSession = gm->GetCurrentSession();
                        if (gSession && gSession->GetMyMember())
                        {
                            AZStd::string baseLink(JoinInCVars::GetInstance()->GetURIScheme());
                            auto gameNameCVar = gEnv->pConsole->GetCVar("sys_game_name");

                            if (gameNameCVar)
                            {
                                AZStd::string gameName(gameNameCVar->GetString());
                                AZStd::string command = GetPortString(pActInfo, InputPorts::Command).c_str();

                                AZStd::string portAndHost = gSession->GetMyMember()->GetId().ToAddress().c_str();
                                AZStd::size_t nextSplicePos = portAndHost.find('|');

                                if (nextSplicePos != AZStd::string::npos)
                                {
                                    AZStd::string host = portAndHost.substr(0, nextSplicePos);
                                    AZStd::string port = portAndHost.substr(nextSplicePos + 1);

                                    AZStd::string str = AZStd::string::format("%s %s address=%s port=%s", command.c_str(), gameName.c_str(), host.c_str(), port.c_str());

                                    const unsigned int expectedLen = Base64::encodedsize_base64(str.length()) + 1;

                                    AZStd::vector<char> buffer(expectedLen);
                                    unsigned int len = Base64::encode_base64(buffer.data(), str.c_str(), strlen(str.c_str()), true);

                                    if (len == expectedLen)
                                    {
                                        AZStd::string encodedUri(buffer.data(), len);

                                        retVal = AZStd::string::format("%s:%s", baseLink.c_str(), encodedUri.c_str());
                                    }
                                    else
                                    {
                                        AZ_Warning("JoinIn", false, "Could not create JoinIn link from \"%s\"; encoded URI was not of expected length %d, got %d. ", str, expectedLen, len);
                                    }
                                }
                                else
                                {
                                    AZ_Warning("JoinIn", false, "Could not create JoinIn link; multiplayer session did not return data in expected format. Got \"%s\".", portAndHost);
                                }
                            }
                            else
                            {
                                AZ_Warning("JoinIn", false, "Could not create JoinIn link; Could not determine the game/project name.");
                            }
                        }
                        else
                        {
                            AZ_Warning("JoinIn", false, "Could not create JoinIn link; multiplayer session may not be initialized.");
                        }
                    }

                    if (!retVal.empty())
                    {
                        ActivateOutput(pActInfo, OutputPorts::Out, string(retVal.c_str()));
                    }
                    else
                    {
                        ActivateOutput(pActInfo, OutputPorts::Error, true);
                    }
                }

                break;
            }
        }


        void GetMemoryUsage(ICrySizer*) const override
        {
        }
    };

    REGISTER_FLOW_NODE("Twitch:JoinIn:CreateLink", CFlowNode_TwitchJoinInLinkCreate);

}
