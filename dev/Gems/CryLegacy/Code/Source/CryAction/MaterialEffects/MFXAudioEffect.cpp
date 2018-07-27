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

#include "CryLegacy_precompiled.h"
#include "MFXAudioEffect.h"

#include <IAudioSystem.h>
#include "Components/IComponentAudio.h"

namespace MaterialEffectsUtils
{
    struct SAudio1pOr3pSwitch
    {
        static const SAudio1pOr3pSwitch& Instance()
        {
            static SAudio1pOr3pSwitch theInstance;
            return theInstance;
        }

        Audio::TAudioControlID GetSwitchId() const
        {
            return m_switchID;
        }

        Audio::TAudioSwitchStateID Get1pStateId() const
        {
            return m_1pStateID;
        }

        Audio::TAudioSwitchStateID Get3pStateId() const
        {
            return m_3pStateID;
        }

        bool IsValid() const
        {
            return m_isValid;
        }

    private:

        SAudio1pOr3pSwitch()
            : m_switchID(INVALID_AUDIO_CONTROL_ID)
            , m_1pStateID(INVALID_AUDIO_SWITCH_STATE_ID)
            , m_3pStateID(INVALID_AUDIO_SWITCH_STATE_ID)
            , m_isValid(false)
        {
            Initialize();
        }

        void Initialize()
        {
            Audio::AudioSystemRequestBus::BroadcastResult(m_switchID, &Audio::AudioSystemRequestBus::Events::GetAudioSwitchID, "1stOr3rdP");
            Audio::AudioSystemRequestBus::BroadcastResult(m_1pStateID, &Audio::AudioSystemRequestBus::Events::GetAudioSwitchStateID, m_switchID, "1stP");
            Audio::AudioSystemRequestBus::BroadcastResult(m_3pStateID, &Audio::AudioSystemRequestBus::Events::GetAudioSwitchStateID, m_switchID, "3rdP");

            m_isValid = (m_switchID != INVALID_AUDIO_CONTROL_ID) &&
                (m_1pStateID != INVALID_AUDIO_SWITCH_STATE_ID) && (m_3pStateID != INVALID_AUDIO_SWITCH_STATE_ID);
        }

        Audio::TAudioControlID      m_switchID;
        Audio::TAudioSwitchStateID  m_1pStateID;
        Audio::TAudioSwitchStateID  m_3pStateID;

        bool                 m_isValid;
    };

    template<typename AudioProxyType>
    void PrepareForAudioTriggerExecution(AudioProxyType* pIAudioProxy, const SMFXAudioEffectParams& audioParams, const SMFXRunTimeEffectParams& runtimeParams)
    {
        const MaterialEffectsUtils::SAudio1pOr3pSwitch& audio1pOr3pSwitch = MaterialEffectsUtils::SAudio1pOr3pSwitch::Instance();

        if (audio1pOr3pSwitch.IsValid())
        {
            pIAudioProxy->SetSwitchState(
                audio1pOr3pSwitch.GetSwitchId(),
                runtimeParams.playSoundFP ? audio1pOr3pSwitch.Get1pStateId() : audio1pOr3pSwitch.Get3pStateId());
        }

        for (SMFXAudioEffectParams::TSwitches::const_iterator it = audioParams.triggerSwitches.begin(), itEnd = audioParams.triggerSwitches.end(); it != itEnd; ++it)
        {
            const SAudioSwitchWrapper& switchWrapper = *it;
            pIAudioProxy->SetSwitchState(switchWrapper.GetSwitchId(), switchWrapper.GetSwitchStateId());
        }

        // Audio: IComponentAudio needs to support multiple IAudioProxy objects to properly handle Rtpcs tied to specific events

        //Note: Rtpcs are global for the audio proxy object.
        //      This can be a problem if the sound is processed through IComponentAudio, where the object is shared for all audio events triggered through it!
        //TODO: Add support to IComponentAudio to handle multiple audio proxy objects
        for (int i = 0; i < runtimeParams.numAudioRtpcs; ++i)
        {
            const char* rtpcName = runtimeParams.audioRtpcs[i].rtpcName;
            if (rtpcName && *rtpcName)
            {
                Audio::TAudioControlID rtpcId = INVALID_AUDIO_CONTROL_ID;
                Audio::AudioSystemRequestBus::BroadcastResult(rtpcId, &Audio::AudioSystemRequestBus::Events::GetAudioRtpcID, rtpcName);
                if (rtpcId != INVALID_AUDIO_CONTROL_ID)
                {
                    pIAudioProxy->SetRtpcValue(rtpcId, runtimeParams.audioRtpcs[i].rtpcValue);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////

void SAudioTriggerWrapper::Init(const char* triggerName)
{
    CRY_ASSERT(triggerName != NULL);

    Audio::AudioSystemRequestBus::BroadcastResult(m_triggerID, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, triggerName);

#if defined(MATERIAL_EFFECTS_DEBUG)
    m_triggerName = triggerName;
#endif
}

void SAudioSwitchWrapper::Init(const char* switchName, const char* switchStateName)
{
    CRY_ASSERT(switchName != NULL);
    CRY_ASSERT(switchStateName != NULL);

    Audio::AudioSystemRequestBus::BroadcastResult(m_switchID, &Audio::AudioSystemRequestBus::Events::GetAudioSwitchID, switchName);
    Audio::AudioSystemRequestBus::BroadcastResult(m_switchStateID, &Audio::AudioSystemRequestBus::Events::GetAudioSwitchStateID, m_switchID, switchStateName);

#if defined(MATERIAL_EFFECTS_DEBUG)
    m_switchName = switchName;
    m_switchStateName = switchStateName;
#endif
}

//////////////////////////////////////////////////////////////////////////

CMFXAudioEffect::CMFXAudioEffect()
    : CMFXEffectBase(eMFXPF_Audio)
{
}

void CMFXAudioEffect::Execute(const SMFXRunTimeEffectParams& params)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_ACTION);

    IF_UNLIKELY (!m_audioParams.trigger.IsValid())
    {
        return;
    }

    IEntity* pOwnerEntity = (params.audioComponentEntityId != 0) ? gEnv->pEntitySystem->GetEntity(params.audioComponentEntityId) : NULL;
    if (pOwnerEntity)
    {
        IComponentAudioPtr pAudioComponent = pOwnerEntity->GetOrCreateComponent<IComponentAudio>();
        CRY_ASSERT(pAudioComponent);

        MaterialEffectsUtils::PrepareForAudioTriggerExecution<IComponentAudio>(pAudioComponent.get(), m_audioParams, params);

        pAudioComponent->ExecuteTrigger(m_audioParams.trigger.GetTriggerId(), eLSM_None);
    }
    else
    {
        Audio::IAudioProxy* pIAudioProxy = nullptr;
        Audio::AudioSystemRequestBus::BroadcastResult(pIAudioProxy, &Audio::AudioSystemRequestBus::Events::GetFreeAudioProxy);
        if (pIAudioProxy != NULL)
        {
            pIAudioProxy->Initialize("MFXAudioEffect");

            MaterialEffectsUtils::PrepareForAudioTriggerExecution<Audio::IAudioProxy>(pIAudioProxy, m_audioParams, params);

            pIAudioProxy->SetPosition(params.pos);
            pIAudioProxy->SetObstructionCalcType(Audio::eAOOCT_SINGLE_RAY);
            pIAudioProxy->SetCurrentEnvironments();
            pIAudioProxy->ExecuteTrigger(m_audioParams.trigger.GetTriggerId(), eLSM_None);
        }
        SAFE_RELEASE(pIAudioProxy);
    }
}

void CMFXAudioEffect::GetResources(SMFXResourceList& resourceList) const
{
    SMFXAudioListNode* pListNode = SMFXAudioListNode::Create();

    pListNode->m_audioParams.triggerName = m_audioParams.trigger.GetTriggerName();

    const size_t switchesCount = MIN(m_audioParams.triggerSwitches.size(), pListNode->m_audioParams.triggerSwitches.max_size());

    for (size_t i = 0; i < switchesCount; ++i)
    {
        IMFXAudioParams::SSwitchData switchData;
        switchData.switchName = m_audioParams.triggerSwitches[i].GetSwitchName();
        switchData.switchStateName = m_audioParams.triggerSwitches[i].GetSwitchStateName();

        pListNode->m_audioParams.triggerSwitches.push_back(switchData);
    }

    SMFXAudioListNode* pNextNode = resourceList.m_audioList;

    if (pNextNode == NULL)
    {
        resourceList.m_audioList = pListNode;
    }
    else
    {
        while (pNextNode->pNext)
        {
            pNextNode = pNextNode->pNext;
        }

        pNextNode->pNext = pListNode;
    }
}

void CMFXAudioEffect::LoadParamsFromXml(const XmlNodeRef& paramsNode)
{
    // Xml data format
    /*
    <Audio trigger="footstep">
        <Switch name="Switch1" state="switch1_state" />
        <Switch name="Switch2" state="swtich2_state" />
        <Switch ... />
    </Audio>
    */

    m_audioParams.trigger.Init(paramsNode->getAttr("trigger"));

    const int childCount = paramsNode->getChildCount();
    m_audioParams.triggerSwitches.reserve(childCount);

    for (int i = 0; i < childCount; ++i)
    {
        const XmlNodeRef& childNode = paramsNode->getChild(i);

        if (strcmp(childNode->getTag(), "Switch") == 0)
        {
            SAudioSwitchWrapper switchWrapper;
            switchWrapper.Init(childNode->getAttr("name"), childNode->getAttr("state"));
            if (switchWrapper.IsValid())
            {
                m_audioParams.triggerSwitches.push_back(switchWrapper);
            }
#if defined(MATERIAL_EFFECTS_DEBUG)
            else
            {
                GameWarning("[MFX] AudioEffect (at line %d) : Switch '%s' or SwitchState '%s' not valid", paramsNode->getLine(), switchWrapper.GetSwitchName(), switchWrapper.GetSwitchStateName());
            }
#endif
        }
    }

#if defined(MATERIAL_EFFECTS_DEBUG)
    if (!m_audioParams.trigger.IsValid())
    {
        GameWarning("[MFX] AudioEffect (at line %d) : Trigger '%s'not valid", paramsNode->getLine(), m_audioParams.trigger.GetTriggerName());
    }
#endif
}

void CMFXAudioEffect::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this));
    pSizer->AddObject(&m_audioParams);
}

