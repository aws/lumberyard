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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Allows for loading/unloading preload requests.


#include "CryLegacy_precompiled.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <IAudioSystem.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
class CFlowNode_AudioPreloadData
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    ///////////////////////////////////////////////////////////////////////////////////////////////
    CFlowNode_AudioPreloadData(SActivationInfo* pActInfo)
        : m_bEnable(false)
    {}

    ///////////////////////////////////////////////////////////////////////////////////////////////
    ~CFlowNode_AudioPreloadData() override
    {}

    ///////////////////////////////////////////////////////////////////////////////////////////////
    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_AudioPreloadData(pActInfo);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    enum INPUTS
    {
        eIn_PreloadRequestFirst = 0,
        eIn_PreloadRequest1,
        eIn_PreloadRequest2,
        eIn_PreloadRequest3,
        eIn_PreloadRequest4,
        eIn_PreloadRequest5,
        eIn_PreloadRequest6,
        eIn_PreloadRequestLast,
        eIn_Enable,
        eIn_Disable,
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    enum OUTPUTS
    {
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig<string>("audioPreloadRequest_PreloadRequest0", _HELP("name of preload request"), "Preload Request"),
            InputPortConfig<string>("audioPreloadRequest_PreloadRequest1", _HELP("name of preload request"), "Preload Request"),
            InputPortConfig<string>("audioPreloadRequest_PreloadRequest2", _HELP("name of preload request"), "Preload Request"),
            InputPortConfig<string>("audioPreloadRequest_PreloadRequest3", _HELP("name of preload request"), "Preload Request"),
            InputPortConfig<string>("audioPreloadRequest_PreloadRequest4", _HELP("name of preload request"), "Preload Request"),
            InputPortConfig<string>("audioPreloadRequest_PreloadRequest5", _HELP("name of preload request"), "Preload Request"),
            InputPortConfig<string>("audioPreloadRequest_PreloadRequest6", _HELP("name of preload request"), "Preload Request"),
            InputPortConfig<string>("audioPreloadRequest_PreloadRequest7", _HELP("name of preload request"), "Preload Request"),
            InputPortConfig_Void("Load", _HELP("loads all supplied preload requests")),
            InputPortConfig_Void("Unload", _HELP("unloads all supplied preload requests")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {
            {0}
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Node that allows for handling audio preload requests.");
        config.SetCategory(EFLN_APPROVED);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void Enable(SActivationInfo* pActInfo, bool bEnable)
    {
        if (m_bEnable != bEnable)
        {
            for (uint32 i = CFlowNode_AudioPreloadData::eIn_PreloadRequestFirst; i <= CFlowNode_AudioPreloadData::eIn_PreloadRequestLast; ++i)
            {
                const string& rPreloadName = GetPortString(pActInfo, static_cast<int>(i));

                if (!rPreloadName.empty())
                {
                    Audio::SAudioRequest oAudioRequestData;
                    Audio::TAudioPreloadRequestID nPreloadRequestID = INVALID_AUDIO_PRELOAD_REQUEST_ID;

                    Audio::AudioSystemRequestBus::BroadcastResult(nPreloadRequestID, &Audio::AudioSystemRequestBus::Events::GetAudioPreloadRequestID, rPreloadName.c_str());
                    if (nPreloadRequestID != INVALID_AUDIO_PRELOAD_REQUEST_ID)
                    {
                        if (bEnable)
                        {
                            Audio::SAudioManagerRequestData<Audio::eAMRT_PRELOAD_SINGLE_REQUEST> oAMData(nPreloadRequestID);
                            oAudioRequestData.pData = &oAMData;
                            Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, oAudioRequestData);
                        }
                        else
                        {
                            Audio::SAudioManagerRequestData<Audio::eAMRT_UNLOAD_SINGLE_REQUEST> oAMData(nPreloadRequestID);
                            oAudioRequestData.pData = &oAMData;
                            Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, oAudioRequestData);
                        }
                    }
                }
            }

            m_bEnable = bEnable;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
        bool bEnable = m_bEnable;
        ser.BeginGroup("FlowAudioPreloadData");
        ser.Value("enable", bEnable);
        ser.EndGroup();

        if (ser.IsReading())
        {
            Enable(pActInfo, bEnable);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            if (gEnv->IsEditor() && !gEnv->IsEditing())
            {
                Enable(pActInfo, false);
            }

            m_bEnable = false;
            break;
        }
        case eFE_Activate:
        {
            // Enable
            if (IsPortActive(pActInfo, CFlowNode_AudioPreloadData::eIn_Enable))
            {
                Enable(pActInfo, true);
            }

            // Disable
            if (IsPortActive(pActInfo, CFlowNode_AudioPreloadData::eIn_Disable))
            {
                Enable(pActInfo, false);
            }
            break;
        }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

private:
    bool m_bEnable;
};

REGISTER_FLOW_NODE("Audio:PreloadData", CFlowNode_AudioPreloadData);
