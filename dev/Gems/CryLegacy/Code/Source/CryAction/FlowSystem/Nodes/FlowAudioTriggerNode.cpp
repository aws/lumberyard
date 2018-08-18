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

// Description : FlowGraph Node that executes AudioTriggers.


#include "CryLegacy_precompiled.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include "Components/IComponentAudio.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
class CFlowNode_AudioTrigger
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    ///////////////////////////////////////////////////////////////////////////////////////////////
    CFlowNode_AudioTrigger(SActivationInfo* pActInfo)
        : m_bIsPlaying(false)
        , m_nPlayTriggerID(INVALID_AUDIO_CONTROL_ID)
        , m_nStopTriggerID(INVALID_AUDIO_CONTROL_ID)
    {
        ZeroStruct(m_oPlayActivationInfo);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    ~CFlowNode_AudioTrigger() override
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    IFlowNodePtr Clone(SActivationInfo* pActInfo) override
    {
        return new CFlowNode_AudioTrigger(pActInfo);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    enum INPUTS
    {
        eIn_PlayTrigger,
        eIn_StopTrigger,
        eIn_Play,
        eIn_Stop,
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    enum OUTPUTS
    {
        eOut_Done,
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig<string>("audioTrigger_PlayTrigger", _HELP("name of the Play Trigger"), "PlayTrigger"),
            InputPortConfig<string>("audioTrigger_StopTrigger", _HELP("name of the Stop Trigger"), "StopTrigger"),

            InputPortConfig_Void("Play", _HELP("Executes the Play Trigger")),
            InputPortConfig_Void("Stop", _HELP("Executes the Stop Trigger if it is provided, o/w stops all events started by the Start Trigger")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("Done", _HELP("Activated when all of the triggered events have finished playing")),
            {0}
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("This node executes Audio Triggers.");
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.SetCategory(EFLN_APPROVED);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
        bool bPlay = m_bIsPlaying;
        ser.BeginGroup("FlowAudioTriggerNode");
        ser.Value("play", bPlay);
        ser.EndGroup();

        if (ser.IsReading())
        {
            m_bIsPlaying = false;
            Init(pActInfo, bPlay);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::AddRequestListener,
                &CFlowNode_AudioTrigger::OnAudioTriggerFinished,
                this,
                Audio::eART_AUDIO_CALLBACK_MANAGER_REQUEST,
                Audio::eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE);

            Stop(pActInfo->pEntity);
            Init(pActInfo, false);
            break;
        }
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, CFlowNode_AudioTrigger::eIn_Stop))
            {
                Stop(pActInfo->pEntity);
            }

            if (IsPortActive(pActInfo, CFlowNode_AudioTrigger::eIn_Play))
            {
                m_oPlayActivationInfo = *pActInfo;
                Play(pActInfo->pEntity, pActInfo);
            }

            if (IsPortActive(pActInfo, CFlowNode_AudioTrigger::eIn_PlayTrigger))
            {
                Stop(pActInfo->pEntity);
                GetTriggerID(pActInfo, CFlowNode_AudioTrigger::eIn_PlayTrigger, m_nPlayTriggerID);
            }

            if (IsPortActive(pActInfo, CFlowNode_AudioTrigger::eIn_StopTrigger))
            {
                GetTriggerID(pActInfo, CFlowNode_AudioTrigger::eIn_StopTrigger, m_nStopTriggerID);
            }

            break;
        }
        case eFE_Uninitialize:
        {
            ForceStopAll(pActInfo->pEntity);
            ZeroStruct(m_oPlayActivationInfo);
            Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::RemoveRequestListener, &CFlowNode_AudioTrigger::OnAudioTriggerFinished, this);
            break;
        }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    //////////////////////////////////////////////////////////////////////////
    void TriggerFinished(const Audio::TAudioControlID nTriggerID)
    {
        if (nTriggerID == m_nPlayTriggerID)
        {
            ActivateOutput(&m_oPlayActivationInfo, CFlowNode_AudioTrigger::eOut_Done, true);
        }
    }

private:
    enum EPlayMode
    {
        ePM_None = 0,
        ePM_Play = 1,
        ePM_PlayStop = 2,
        ePM_ForceStop = 3,
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    static void OnAudioTriggerFinished(const Audio::SAudioRequestInfo* const pAudioRequestInfo)
    {
        auto pAudioTrigger = static_cast<CFlowNode_AudioTrigger*>(pAudioRequestInfo->pOwner);
        if (pAudioTrigger->m_oPlayActivationInfo.pGraph)
        {
            pAudioTrigger->TriggerFinished(pAudioRequestInfo->nAudioControlID);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void GetTriggerID(SActivationInfo* const pActInfo, const uint32 nPortIdx, Audio::TAudioControlID& rOutTriggerID)
    {
        rOutTriggerID = INVALID_AUDIO_CONTROL_ID;

        const string& rTriggerName = GetPortString(pActInfo, nPortIdx);
        if (!rTriggerName.empty())
        {
            Audio::AudioSystemRequestBus::BroadcastResult(rOutTriggerID, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, rTriggerName.c_str());
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void Init(SActivationInfo* const pActInfo, const bool bPlay)
    {
        m_bIsPlaying = false;
        GetTriggerID(pActInfo, CFlowNode_AudioTrigger::eIn_PlayTrigger, m_nPlayTriggerID);
        GetTriggerID(pActInfo, CFlowNode_AudioTrigger::eIn_StopTrigger, m_nStopTriggerID);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void ExecuteOnProxy(IEntity* const pEntity, const Audio::TAudioControlID nTriggerID, const EPlayMode ePlayMode)
    {
        const IComponentAudioPtr pAudioComponent = pEntity->GetOrCreateComponent<IComponentAudio>();
        if (pAudioComponent)
        {
            switch (ePlayMode)
            {
                case CFlowNode_AudioTrigger::ePM_Play:
                {
                    Audio::SAudioCallBackInfos callbackInfos(this, nullptr, this, Audio::eARF_PRIORITY_NORMAL | Audio::eARF_SYNC_FINISHED_CALLBACK);
                    pAudioComponent->ExecuteTrigger(nTriggerID, eLSM_None, DEFAULT_AUDIO_PROXY_ID, callbackInfos);
                    break;
                }
                case CFlowNode_AudioTrigger::ePM_PlayStop:
                {
                    pAudioComponent->ExecuteTrigger(nTriggerID, eLSM_None);
                    break;
                }
                case CFlowNode_AudioTrigger::ePM_ForceStop:
                {
                    pAudioComponent->StopTrigger(nTriggerID);
                    break;
                }
                default:
                {
                    AZ_Assert(false, "ExecuteOnProxy - Unknown Play mode in Audio Trigger FlowNode!");
                    break;
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void ExecuteOnGlobalObject(const Audio::TAudioControlID nTriggerID, const EPlayMode ePlayMode)
    {
        switch (ePlayMode)
        {
            case CFlowNode_AudioTrigger::ePM_Play:
            {
                m_oExecuteRequestData.nTriggerID = nTriggerID;
                m_oRequest.pOwner = this;
                m_oRequest.pData = &m_oExecuteRequestData;
                Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, m_oRequest);
                break;
            }
            case CFlowNode_AudioTrigger::ePM_PlayStop:
            {
                m_oExecuteRequestData.nTriggerID = nTriggerID;
                m_oRequest.pData = &m_oExecuteRequestData;
                Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, m_oRequest);
                break;
            }
            case CFlowNode_AudioTrigger::ePM_ForceStop:
            {
                m_oStopRequestData.nTriggerID = nTriggerID;
                m_oRequest.pData = &m_oStopRequestData;
                Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, m_oRequest);
                break;
            }
            default:
            {
                AZ_Assert(false, "ExecuteOnGlobalObject - Unknown Play mode in Audio:Trigger FlowNode!");
                break;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void Play(IEntity* const pEntity, const SActivationInfo* const pActInfo)
    {
        if (m_nPlayTriggerID != INVALID_AUDIO_CONTROL_ID)
        {
            if (pEntity)
            {
                ExecuteOnProxy(pEntity, m_nPlayTriggerID, CFlowNode_AudioTrigger::ePM_Play);
            }
            else
            {
                ExecuteOnGlobalObject(m_nPlayTriggerID, CFlowNode_AudioTrigger::ePM_Play);
            }

            m_bIsPlaying = true;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void Stop(IEntity* const pEntity)
    {
        if (m_bIsPlaying)
        {
            if (m_nStopTriggerID != INVALID_AUDIO_CONTROL_ID)
            {
                if (pEntity)
                {
                    ExecuteOnProxy(pEntity, m_nStopTriggerID, CFlowNode_AudioTrigger::ePM_PlayStop);
                }
                else
                {
                    ExecuteOnGlobalObject(m_nStopTriggerID, CFlowNode_AudioTrigger::ePM_PlayStop);
                }
            }
            else
            {
                if (pEntity)
                {
                    ExecuteOnProxy(pEntity, m_nPlayTriggerID, CFlowNode_AudioTrigger::ePM_ForceStop);
                }
                else
                {
                    ExecuteOnGlobalObject(m_nPlayTriggerID, CFlowNode_AudioTrigger::ePM_ForceStop);
                }
            }

            m_bIsPlaying = false;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void ForceStopAll(IEntity* const pEntity)
    {
        if (pEntity)
        {
            if (m_nPlayTriggerID != INVALID_AUDIO_CONTROL_ID)
            {
                ExecuteOnProxy(pEntity, m_nPlayTriggerID, ePM_ForceStop);
            }
            if (m_nStopTriggerID != INVALID_AUDIO_CONTROL_ID)
            {
                ExecuteOnProxy(pEntity, m_nStopTriggerID, ePM_ForceStop);
            }
        }
        else
        {
            if (m_nPlayTriggerID != INVALID_AUDIO_CONTROL_ID)
            {
                ExecuteOnGlobalObject(m_nPlayTriggerID, ePM_ForceStop);
            }
            if (m_nStopTriggerID != INVALID_AUDIO_CONTROL_ID)
            {
                ExecuteOnGlobalObject(m_nStopTriggerID, ePM_ForceStop);
            }
        }

        m_bIsPlaying = false;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool m_bIsPlaying;
    Audio::TAudioControlID m_nPlayTriggerID;
    Audio::TAudioControlID m_nStopTriggerID;

    SActivationInfo m_oPlayActivationInfo;

    Audio::SAudioRequest m_oRequest;
    Audio::SAudioObjectRequestData<Audio::eAORT_EXECUTE_TRIGGER> m_oExecuteRequestData;
    Audio::SAudioObjectRequestData<Audio::eAORT_STOP_TRIGGER> m_oStopRequestData;
};

REGISTER_FLOW_NODE("Audio:Trigger", CFlowNode_AudioTrigger);
