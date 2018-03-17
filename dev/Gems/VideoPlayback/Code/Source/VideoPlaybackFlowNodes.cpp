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

#include "VideoPlayback_precompiled.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <Include/VideoPlayback/VideoPlaybackBus.h>

namespace AZ
{
    namespace VideoPlayback
    {

        class Play : public CFlowBaseNode<eNCT_Singleton>
        {
            enum INPUTS
            {
                Activate = 0,
                LOOP, 
                PlaybackSpeed,
            };

        public:

            Play(SActivationInfo* actInfo)
            {
            }

            void GetMemoryUsage(ICrySizer* s) const override
            {
                s->Add(*this);
            }

            void GetConfiguration(SFlowNodeConfig& config) override
            {
                static const SInputPortConfig inConfig[] =
                {
                    InputPortConfig<bool>("Activate", true, _HELP("Start playing the video on this entity")),
                    InputPortConfig<bool>("Loop", _HELP("Continuously loop the video")),
                    InputPortConfig<float>("PlaybackSpeed", 1.0f, _HELP("Set how fast the video should play")),
                    { 0 }
                };
                static const SOutputPortConfig outConfig[] =
                {
                    { 0 }
                };

                config.sDescription = _HELP("This node allows playing a movie on the specified entity");
                config.pInputPorts = inConfig;
                config.pOutputPorts = outConfig;
                config.SetCategory(EFLN_APPROVED | EFLN_TARGET_ENTITY);
            }

            void ProcessEvent(EFlowEvent event, SActivationInfo* actInfo) override
            {
                switch (event)
                {
                case eFE_Activate:
                {
                    // Post the event to this specific entity.

                    if (GetPortBool(actInfo, LOOP))
                    {
                        // Enable looping on this video.
                        EBUS_EVENT_ID(actInfo->entityId, VideoPlaybackRequestBus, EnableLooping, true);
                    }

                    //Set the playback speed
                    float playbackSpeed = GetPortFloat(actInfo, PlaybackSpeed);
                    EBUS_EVENT_ID(actInfo->entityId, VideoPlaybackRequestBus, SetPlaybackSpeed, playbackSpeed);
                    
                    EBUS_EVENT_ID(actInfo->entityId, VideoPlaybackRequestBus, Play);
                }
                break;
                }
            }
        };

        class Pause : public CFlowBaseNode<eNCT_Singleton>
        {
            enum INPUTS
            {
                Activate = 0,
            };

        public:

            Pause(SActivationInfo* actInfo)
            {
            }

            void GetMemoryUsage(ICrySizer* s) const override
            {
                s->Add(*this);
            }

            void GetConfiguration(SFlowNodeConfig& config) override
            {
                static const SInputPortConfig inConfig[] =
                {
                    InputPortConfig<bool>("Activate", true, _HELP("Pause the video on this entity")),
                    { 0 }
                };
                static const SOutputPortConfig outConfig[] =
                {
                    { 0 }
                };

                config.sDescription = _HELP("This node allows pausing a movie on the specified entity");
                config.pInputPorts = inConfig;
                config.pOutputPorts = outConfig;
                config.SetCategory(EFLN_APPROVED | EFLN_TARGET_ENTITY);
            }

            void ProcessEvent(EFlowEvent event, SActivationInfo* actInfo) override
            {
                switch (event)
                {
                case eFE_Activate:
                {
                    // Post the event to this specific entity.
                    EBUS_EVENT_ID(actInfo->entityId, VideoPlaybackRequestBus, Pause);
                }
                break;
                }
            }
        };

        class Stop : public CFlowBaseNode<eNCT_Singleton>
        {
            enum INPUTS
            {
                Activate = 0,
            };

        public:

            Stop(SActivationInfo* actInfo)
            {
            }

            void GetMemoryUsage(ICrySizer* s) const override
            {
                s->Add(*this);
            }

            void GetConfiguration(SFlowNodeConfig& config) override
            {
                static const SInputPortConfig inConfig[] =
                {
                    InputPortConfig<bool>("Activate", true, _HELP("Stop the video on this entity")),
                    { 0 }
                };
                static const SOutputPortConfig outConfig[] =
                {
                    { 0 }
                };

                config.sDescription = _HELP("This node allows stopping a movie on the specified entity");
                config.pInputPorts = inConfig;
                config.pOutputPorts = outConfig;
                config.SetCategory(EFLN_APPROVED | EFLN_TARGET_ENTITY);
            }

            void ProcessEvent(EFlowEvent event, SActivationInfo* actInfo) override
            {
                switch (event)
                {
                case eFE_Activate:
                {
                    // Post the event to this specific entity.
                    EBUS_EVENT_ID(actInfo->entityId, VideoPlaybackRequestBus, Stop);
                }
                break;
                }
            }
        };

        class IsPlaying : public CFlowBaseNode<eNCT_Singleton>
        {
            enum INPUTS
            {
                Activate = 0
            };
            enum OUTPUTS
            {
                Playing = 0
            };

        public:

            IsPlaying(SActivationInfo* actInfo)
            {
            }

            void GetMemoryUsage(ICrySizer* s) const override
            {
                s->Add(*this);
            }

            void GetConfiguration(SFlowNodeConfig& config) override
            {
                static const SInputPortConfig inConfig[] =
                {
                    InputPortConfig<bool>("Activate", true, _HELP("Retrieve whether or not the video is playing")),
                    { 0 }
                };
                static const SOutputPortConfig outConfig[] =
                {
                    OutputPortConfig<bool>("Playing", _HELP("Returns whether or not the video is playing")),
                    { 0 }
                };

                config.sDescription = _HELP("This node allows checking if a video is playing on the specified entity");
                config.pInputPorts = inConfig;
                config.pOutputPorts = outConfig;
                config.SetCategory(EFLN_APPROVED | EFLN_TARGET_ENTITY);
            }

            void ProcessEvent(EFlowEvent event, SActivationInfo* actInfo) override
            {
                switch (event)
                {
                case eFE_Activate:
                {
                    // Post the event to this specific entity.
                    bool playing = false;
                    EBUS_EVENT_ID_RESULT(playing, actInfo->entityId, VideoPlaybackRequestBus, IsPlaying);

                    ActivateOutput(actInfo, Playing, playing);
                }
                break;
                }
            }
        };

        class PlaybackEvents 
            : public CFlowBaseNode<eNCT_Singleton>
            , public VideoPlaybackNotificationBus::Handler
        {
            enum INPUTS
            {
                Activate = 0
            };

            enum OUTPUTS
            {
                PlaybackStarted = 0,
                PlaybackPaused,
                PlaybackStopped,
                PlaybackFinished
            };

        public:

            PlaybackEvents(SActivationInfo* actInfo)
            {
                m_eventToOutput = -1;
            }

            void GetMemoryUsage(ICrySizer* s) const override
            {
                s->Add(*this);
            }

            void GetConfiguration(SFlowNodeConfig& config) override
            {
                static const SInputPortConfig inConfig[] =
                {
                    InputPortConfig<bool>("Activate", true, _HELP("Fires events based on video playback on this entity")),
                    { 0 }
                };
                static const SOutputPortConfig outConfig[] =
                {
                    OutputPortConfig<bool>("PlaybackStarted", _HELP("Trigged when video playback starts")),
                    OutputPortConfig<bool>("PlaybackPaused", _HELP("Trigged when video playback pauses")),
                    OutputPortConfig<bool>("PlaybackStopped", _HELP("Trigged when video playback is stopped manually")),
                    OutputPortConfig<bool>("PlaybackFinished", _HELP("Trigged when video playback finishes (video may loop)")),
                    { 0 }
                };

                config.sDescription = _HELP("This node triggers an output when a video on the entity starts playing");
                config.pInputPorts = inConfig;
                config.pOutputPorts = outConfig;
                config.SetCategory(EFLN_APPROVED | EFLN_TARGET_ENTITY);
            }

            //Don't handle events directly
            void ProcessEvent(EFlowEvent event, SActivationInfo* actInfo) override
            {
                switch (event)
                {
                case eFE_Initialize:
                {
                    actInfo->pGraph->SetRegularlyUpdated(actInfo->myID, true);
                    break;
                }
                case IFlowNode::eFE_Activate:
                {
                    VideoPlaybackNotificationBus::Handler::BusConnect(actInfo->entityId);
                    break;
                }
                case IFlowNode::eFE_Update:
                {
                    if(m_eventToOutput > -1)
                    {
                        ActivateOutput(actInfo, m_eventToOutput, true);
                        m_eventToOutput = -1;
                    }
                    break;
                }
                default:
                    break;
                }
            }

            void OnPlaybackStarted() override 
            {
                m_eventToOutput = PlaybackStarted;
            }

            void OnPlaybackPaused() override
            {
                m_eventToOutput = PlaybackPaused;
            }

            void OnPlaybackStopped () override
            {
                m_eventToOutput = PlaybackStopped;
            }

            void OnPlaybackFinished() override
            {
                m_eventToOutput = PlaybackFinished;
            }

        private:
            AZ::s8 m_eventToOutput;
        };

        REGISTER_FLOW_NODE("VideoPlayback:Play", Play);
        REGISTER_FLOW_NODE("VideoPlayback:Pause", Pause);
        REGISTER_FLOW_NODE("VideoPlayback:Stop", Stop);
        REGISTER_FLOW_NODE("VideoPlayback:IsPlaying", IsPlaying);
        REGISTER_FLOW_NODE("VideoPlayback:PlaybackEvents", PlaybackEvents);
    } // namespace VideoPlayback
}//namespace AZ