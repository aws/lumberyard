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
//   ChatPlay "Voting" Flow Nodes
//
////////////////////////////////////////////////////////////////////////////

#include "ChatPlay_precompiled.h"

#include <sstream>

#include <AzCore/std/sort.h>
#include <FlowSystem/Nodes/FlowBaseNode.h>

#include <ChatPlay/ChatPlayTypes.h>
#include <ChatPlay/ChatPlayBus.h>

////////////////////////////////////////////////////////////////////////////////

namespace ChatPlay
{
    // Reference to a vote; updates based on changes to the entity or vote name.
    // Abstract version of logic that would otherwise appear in all flow nodes below.
    template<int PORT>
    class VoteRef
    {
    public:
        // Helper function for legacy support where an EntityID was required for a vote
        AZStd::string MakeVoteId(EntityId entityId, const AZStd::string& voteName)
        {
            AZStd::string voteId = AZStd::string::format("%u%s", entityId, voteName.c_str());
            return voteId;
        }

        // Updates the reference based on the provided activation info.
        // Return true if a reference change occurred; false otherwise.
        bool Update(IFlowNode::SActivationInfo* info)
        {
            if (m_entity != info->pEntity || IsPortActive(info, PORT))
            {
                AZStd::string voteName = GetPortString(info, PORT);

                if (!info->pEntity)
                {
                    // Do nothing if entity is invalid
                    return false;
                }

                AZStd::string id = MakeVoteId(info->pEntity->GetId(), voteName);

                if (id == m_voteId)
                {
                    // Do nothing if the Vote ID hasn't changed
                    return false;
                }

                bool voteExists = false;
                ChatPlayRequestBus::BroadcastResult(voteExists, &ChatPlayRequestBus::Events::CreateVote, id);

                if (voteExists)
                {
                    m_voteId = id;
                    m_entity = info->pEntity;
                    m_isValid = true;
                    return true;
                }
            }

            return false;
        }

        bool SetChannel(const AZStd::string& channledId)
        {
            bool result = false;
            ChatPlayVoteRequestBus::EventResult(result, m_voteId, &ChatPlayVoteRequestBus::Events::SetChannel, channledId);

            return result;
        }

        void ClearChannel()
        {
            ChatPlayVoteRequestBus::Event(m_voteId, &ChatPlayVoteRequestBus::Events::ClearChannel);
        }

        // Returns true if we were able to create an option or if the option existed already
        bool CreateOption(const AZStd::string& optionName)
        {
            bool result = false;

            // Try to create an option
            ChatPlayVoteRequestBus::EventResult(result, m_voteId, &ChatPlayVoteRequestBus::Events::AddOption, optionName);

            // If the add failed, check if the option already existed
            if (!result)
            {
                ChatPlayVoteRequestBus::EventResult(result, m_voteId, &ChatPlayVoteRequestBus::Events::OptionExists, optionName);
            }

            return result;
        }

        int GetCount(const AZStd::string& optionName)
        {
            int result = 0;
            ChatPlayVoteRequestBus::EventResult(result, m_voteId, &ChatPlayVoteRequestBus::Events::GetOptionCount, optionName);
            return result;
        }

        void SetCount(const AZStd::string& optionName, int count)
        {
            ChatPlayVoteRequestBus::Event(m_voteId, &ChatPlayVoteRequestBus::Events::SetOptionCount, optionName, count);
        }

        bool GetEnabled(const AZStd::string& optionName)
        {
            bool result = false;
            ChatPlayVoteRequestBus::EventResult(result, m_voteId, &ChatPlayVoteRequestBus::Events::GetOptionEnabled, optionName);
            return result;
        }

        void SetEnabled(const AZStd::string& optionName, bool enabled)
        {
            ChatPlayVoteRequestBus::Event(m_voteId, &ChatPlayVoteRequestBus::Events::SetOptionEnabled, optionName, enabled);
        }

        void RemoveOption(const AZStd::string& optionName)
        {
            ChatPlayVoteRequestBus::Event(m_voteId, &ChatPlayVoteRequestBus::Events::RemoveOption, optionName);
        }

        void Visit(const AZStd::function<void(VoteOption& option)>& visitor)
        {
            ChatPlayVoteRequestBus::Event(m_voteId, &ChatPlayVoteRequestBus::Events::Visit, visitor);
        }

        void Reset()
        {
            ChatPlayRequestBus::Broadcast(&ChatPlayRequestBus::Events::DestroyVote, m_voteId);

            m_entity = nullptr;
            m_voteId.clear();
            m_isValid = false;
        }

        explicit operator bool() const
        {
            return m_isValid;
        }

    private:
        IEntity* m_entity = nullptr;
        AZStd::string m_voteId;
        bool m_isValid = false;
    };

    ////////////////////////////////////////////////////////////////////////////////
    // The vote node is used to control the operation of a vote.
    // The vote node holds a reference to the vote as long as entity and name are valid.
    //
    class CFlowNode_ChatPlayVote
        : public CFlowBaseNode < eNCT_Instanced >
    {
        enum InputPorts
        {
            InputPorts_Vote,
            InputPorts_Channel,
            InputPorts_Enable,
            InputPorts_Disable,
        };

        enum OutputPorts
        {
            OutputPorts_Done,
            OutputPorts_Error,
        };

    public:

        explicit CFlowNode_ChatPlayVote(IFlowNode::SActivationInfo*)
        {
        }

        //     ~CFlowNode_ChatPlayVote() override
        //     {
        //     }

        IFlowNodePtr Clone(SActivationInfo* pActInfo)
        {
            return new CFlowNode_ChatPlayVote(pActInfo);
        }

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig<string>("VoteName", _HELP("Name of the vote")),
                InputPortConfig<string>("Channel", _HELP("ChatPlay channel to connect the vote to")),
                InputPortConfig_Void("Enable", _HELP("Vote will exist and can be voted for")),
                InputPortConfig_Void("Disable", _HELP("Vote will exist but can not be voted for")),
                { 0 }
            };
            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig_Void("Done", _HELP("Signalled when the operation completes")),
                OutputPortConfig_Void("Error", _HELP("Signalled if there is an error")),
                { 0 }
            };

            config.nFlags |= EFLN_TARGET_ENTITY;
            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Control ChatPlay vote operations for a specific vote");
            config.SetCategory(EFLN_APPROVED);
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
        {
            switch (event)
            {
            case eFE_Uninitialize:
                m_vote.Reset();
                break;

            case eFE_Activate:
                m_vote.Update(pActInfo);

                if (IsPortActive(pActInfo, InputPorts_Enable))
                {
                    if (m_vote)
                    {
                        AZStd::string channelName = GetPortString(pActInfo, InputPorts_Channel).c_str();
                        if (m_vote.SetChannel(channelName))
                        {
                            ActivateOutput(pActInfo, OutputPorts_Done, 1);
                        }
                        else
                        {
                            ActivateOutput(pActInfo, OutputPorts_Error, 1);
                        }
                    }
                    else
                    {
                        ActivateOutput(pActInfo, OutputPorts_Error, 1);
                    }
                }

                if (IsPortActive(pActInfo, InputPorts_Disable))
                {
                    if (m_vote)
                    {
                        m_vote.ClearChannel();
                        ActivateOutput(pActInfo, OutputPorts_Done, 1);
                    }
                    else
                    {
                        ActivateOutput(pActInfo, OutputPorts_Error, 1);
                    }
                }
                break;

            default:
                break;
            }
        }

        void GetMemoryUsage(ICrySizer*) const override
        {
        }

    private:
        VoteRef<InputPorts_Vote> m_vote;
    };

    ////////////////////////////////////////////////////////////////////////////////
    // The option node is used to control an option on a specific vote.
    //
    // The node will hold a reference to the vote, so lifetime is ensured while the
    // option is being manipulated.
    //
    class CFlowNode_ChatPlayVoteOption
        : public CFlowBaseNode < eNCT_Instanced >
    {
        enum InputPorts
        {
            InputPorts_Vote,
            InputPorts_Option,
            InputPorts_Enable,
            InputPorts_Disable,
            InputPorts_Remove,
        };

        enum OutputPorts
        {
            OutputPorts_Done,
            OutputPorts_Error,
        };

    public:
        explicit CFlowNode_ChatPlayVoteOption(SActivationInfo*)
        {
        }

        IFlowNodePtr Clone(SActivationInfo* pActInfo)
        {
            return new CFlowNode_ChatPlayVoteOption(pActInfo);
        }

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig<string>("VoteName", _HELP("Name of the vote")),
                InputPortConfig<string>("OptionName", _HELP("Name of the voting option")),
                InputPortConfig_Void("Enable", _HELP("Option will exist and can be voted for")),
                InputPortConfig_Void("Disable", _HELP("Option will exist but can not be voted for")),
                InputPortConfig_Void("Remove", _HELP("Option will cease to exist")),
                { 0 }
            };

            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig_Void("Done", _HELP("Signalled when the operation is complete.")),
                OutputPortConfig_Void("Error", _HELP("Signalled if an error has occurred.")),
                { 0 }
            };

            config.nFlags |= EFLN_TARGET_ENTITY;
            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Control ChatPlay vote operations for a specific option on a specific vote");
            config.SetCategory(EFLN_APPROVED);
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
        {
            switch (event)
            {
            case eFE_Uninitialize:
                m_vote.Reset();
                break;

            case eFE_Activate:
            {
                m_vote.Update(pActInfo);

                AZStd::string optionName = GetPortString(pActInfo, InputPorts_Option).c_str();

                if (IsPortActive(pActInfo, InputPorts_Enable))
                {
                    if (m_vote.CreateOption(optionName))
                    {
                        m_vote.SetEnabled(optionName, true);
                        ActivateOutput(pActInfo, OutputPorts_Done, 1);
                    }
                    else
                    {
                        ActivateOutput(pActInfo, OutputPorts_Error, 1);
                    }
                }

                if (IsPortActive(pActInfo, InputPorts_Disable))
                {
                    if (m_vote.CreateOption(optionName))
                    {
                        m_vote.SetEnabled(optionName, false);
                        ActivateOutput(pActInfo, OutputPorts_Done, 1);
                    }
                    else
                    {
                        ActivateOutput(pActInfo, OutputPorts_Error, 1);
                    }
                }

                if (IsPortActive(pActInfo, InputPorts_Remove))
                {
                    if (m_vote)
                    {
                        m_vote.RemoveOption(optionName);
                        ActivateOutput(pActInfo, OutputPorts_Done, 1);
                    }
                    else
                    {
                        ActivateOutput(pActInfo, OutputPorts_Error, 1);
                    }
                }
                break;
            }
            default:
                break;
            }
        }

        void GetMemoryUsage(ICrySizer*) const override
        {
        }

    private:
        VoteRef<InputPorts_Vote> m_vote;
    };

    ////////////////////////////////////////////////////////////////////////////////
    // The score node lets the user query the current vote count for a specific
    // option. The node holds a reference to the vote to ensure option lifetime
    // while its in use.

    class CFlowNode_ChatPlayVoteScore
        : public CFlowBaseNode < eNCT_Instanced >
    {
        enum InputPorts
        {
            InputPorts_Activate,
            InputPorts_Vote,
            InputPorts_Option,
            InputPorts_Reset,
        };

        enum OutputPorts
        {
            OutputPorts_Done,
            OutputPorts_Error,
            OutputPorts_Count,
            OutputPorts_Enabled,
        };

    public:
        explicit CFlowNode_ChatPlayVoteScore(IFlowNode::SActivationInfo*)
        {
        }

        IFlowNodePtr Clone(SActivationInfo* pActInfo)
        {
            return new CFlowNode_ChatPlayVoteScore(pActInfo);
        }

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Activate", _HELP("Query score for an option")),
                InputPortConfig<string>("VoteName", _HELP("Name of the vote")),
                InputPortConfig<string>("OptionName", _HELP("Name of the voting option")),
                InputPortConfig_Void("Reset", _HELP("Reset the count to 0")),
                { 0 }
            };

            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig_Void("Done", _HELP("Signalled when the operation completes")),
                OutputPortConfig_Void("Error", _HELP("Signalled if there is an error")),
                OutputPortConfig<int>("Count", _HELP("Current vote count")),
                OutputPortConfig<bool>("Enabled", _HELP("Current option state")),
                { 0 }
            };

            config.nFlags |= EFLN_TARGET_ENTITY;
            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Report vote scores for a single voting option");
            config.SetCategory(EFLN_APPROVED);
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
        {
            switch (event)
            {
            case eFE_Uninitialize:
                m_vote.Reset();
                break;

            case eFE_Activate:
            {
                m_vote.Update(pActInfo);

                AZStd::string optionName = GetPortString(pActInfo, InputPorts_Option).c_str();

                if (IsPortActive(pActInfo, InputPorts_Activate))
                {
                    if (m_vote.CreateOption(optionName))
                    {
                        ActivateOutput<int>(pActInfo, OutputPorts_Count, m_vote.GetCount(optionName));
                        ActivateOutput<bool>(pActInfo, OutputPorts_Enabled, m_vote.GetEnabled(optionName));
                        ActivateOutput(pActInfo, OutputPorts_Done, 1);
                    }
                    else
                    {
                        ActivateOutput(pActInfo, OutputPorts_Error, 1);
                    }
                }

                if (IsPortActive(pActInfo, InputPorts_Reset))
                {
                    if (m_vote.CreateOption(optionName))
                    {
                        m_vote.SetCount(optionName, 0);
                        ActivateOutput<int>(pActInfo, OutputPorts_Count, m_vote.GetCount(optionName));
                        ActivateOutput<bool>(pActInfo, OutputPorts_Enabled, m_vote.GetEnabled(optionName));
                        ActivateOutput(pActInfo, OutputPorts_Done, 1);
                    }
                    else
                    {
                        ActivateOutput(pActInfo, OutputPorts_Error, 1);
                    }
                }
                break;
            }
            default:
                break;
            }
        }

        void GetMemoryUsage(ICrySizer*) const override
        {
        }

    private:
        VoteRef<InputPorts_Vote> m_vote;
    };

    ////////////////////////////////////////////////////////////////////////////////
    // The high score node provides the current top 4 voting options.
    //
    // The options are guaranteed to have a stable order when the counts are equal,
    // but the order is not explicitly defined (don't assume reverse-lexical will
    // always be true).
    class CFlowNode_ChatPlayVoteHighScores
        : public CFlowBaseNode < eNCT_Instanced >
    {
        enum InputPorts
        {
            InputPorts_Activate,
            InputPorts_Vote,
            InputPorts_Reset,
        };

        enum OutputPorts
        {
            OutputPorts_Done,
            OutputPorts_Error,
            OutputPorts_OptionCount1,
            OutputPorts_OptionCount2,
            OutputPorts_OptionCount3,
            OutputPorts_OptionCount4,
            OutputPorts_OptionName1,
            OutputPorts_OptionName2,
            OutputPorts_OptionName3,
            OutputPorts_OptionName4,
        };

    public:

        explicit CFlowNode_ChatPlayVoteHighScores(IFlowNode::SActivationInfo*)
        {
        }

        IFlowNodePtr Clone(SActivationInfo* pActInfo)
        {
            return new CFlowNode_ChatPlayVoteHighScores(pActInfo);
        }

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Activate", _HELP("Query high scores")),
                InputPortConfig<string>("VoteName", _HELP("Name of the vote")),
                InputPortConfig_Void("Reset", _HELP("Reset all counts to 0")),
                { 0 }
            };

            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig_Void("Done", _HELP("Signalled when the operation is complete.")),
                OutputPortConfig_Void("Error", _HELP("Signalled if an error has occurred.")),
                OutputPortConfig<int>("Count1", _HELP("Vote count for rank 1 option")),
                OutputPortConfig<int>("Count2", _HELP("Vote count for rank 2 option")),
                OutputPortConfig<int>("Count3", _HELP("Vote count for rank 3 option")),
                OutputPortConfig<int>("Count4", _HELP("Vote count for rank 4 option")),
                OutputPortConfig<string>("Name1", _HELP("Name of rank 1 option")),
                OutputPortConfig<string>("Name2", _HELP("Name of rank 2 option")),
                OutputPortConfig<string>("Name3", _HELP("Name of rank 3 option")),
                OutputPortConfig<string>("Name4", _HELP("Name of rank 4 option")),
                { 0 }
            };

            config.nFlags |= EFLN_TARGET_ENTITY;
            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Top 4 voting options");
            config.SetCategory(EFLN_APPROVED);
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
        {
            switch (event)
            {
            case eFE_Uninitialize:
                m_vote.Reset();
                break;

            case eFE_Activate:
                m_vote.Update(pActInfo);

                if (IsPortActive(pActInfo, InputPorts_Activate))
                {
                    m_vote.Update(pActInfo);
                    if (!m_vote)
                    {
                        ActivateOutput(pActInfo, OutputPorts_Error, 1);
                        break;
                    }

                    // Make a vector containing the vote's options
                    AZStd::vector<VoteOption> options;
                    m_vote.Visit([&](VoteOption& option){ options.emplace_back(option); });

                    // Sort the vector!
                    auto countComparator = [](const VoteOption& a, const VoteOption& b) {
                        // Sort largest -> smallest
                        return a.GetCount() > b.GetCount();
                    };
                    AZStd::sort(options.begin(), options.end(), countComparator);

                    auto it = options.rbegin();

                    for (int i = 0; i < 4; ++i, ++it)
                    {
                        if (i < options.size())
                        {
                            ActivateOutput<int>(pActInfo, OutputPorts_OptionCount1 + i, it->GetCount());
                            ActivateOutput<string>(pActInfo, OutputPorts_OptionName1 + i, it->GetName().c_str());
                        }
                        else
                        {
                            // The vote had less than 4 options, fill the rest of the ports with 0/""
                            ActivateOutput<int>(pActInfo, OutputPorts_OptionCount1 + i, 0);
                            ActivateOutput<string>(pActInfo, OutputPorts_OptionName1 + i, "");
                        }
                    }
                    ActivateOutput(pActInfo, OutputPorts_Done, 1);
                }

                if (IsPortActive(pActInfo, InputPorts_Reset))
                {
                    VoteRef<InputPorts_Vote> vote;
                    vote.Update(pActInfo);
                    if (!vote)
                    {
                        ActivateOutput(pActInfo, OutputPorts_Error, 1);
                        break;
                    }

                    vote.Visit([](VoteOption& option){ option.SetCount(0); });
                    ActivateOutput(pActInfo, OutputPorts_Done, 1);
                }
                break;

            default:
                break;
            }
        }

        void GetMemoryUsage(ICrySizer*) const override
        {
        }

    private:
        VoteRef<InputPorts_Vote> m_vote;
    };

    REGISTER_FLOW_NODE("Twitch:ChatPlay:Voting:HighScores", CFlowNode_ChatPlayVoteHighScores);
    REGISTER_FLOW_NODE("Twitch:ChatPlay:Voting:Score", CFlowNode_ChatPlayVoteScore);
    REGISTER_FLOW_NODE("Twitch:ChatPlay:Voting:Option", CFlowNode_ChatPlayVoteOption);
    REGISTER_FLOW_NODE("Twitch:ChatPlay:Voting:Vote", CFlowNode_ChatPlayVote);
}