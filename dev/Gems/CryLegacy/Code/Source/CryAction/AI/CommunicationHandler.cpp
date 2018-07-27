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
#include "CommunicationHandler.h"
#include "AIProxy.h"
#include "Components/IComponentAudio.h"

namespace ATLUtils
{
    void SetSwitchState(const char* switchIdName, const char* switchValue, IComponentAudioPtr pIComponentAudio)
    {
        Audio::TAudioControlID switchControlId(INVALID_AUDIO_CONTROL_ID);
        Audio::AudioSystemRequestBus::BroadcastResult(switchControlId, &Audio::AudioSystemRequestBus::Events::GetAudioSwitchID, switchIdName);
        if (switchControlId != INVALID_AUDIO_CONTROL_ID)
        {
            Audio::TAudioSwitchStateID switchStateId(INVALID_AUDIO_SWITCH_STATE_ID);
            Audio::AudioSystemRequestBus::BroadcastResult(switchStateId, &Audio::AudioSystemRequestBus::Events::GetAudioSwitchStateID, switchControlId, switchValue);
            IF_UNLIKELY (switchStateId == INVALID_AUDIO_SWITCH_STATE_ID)
            {
                CryWarning(VALIDATOR_MODULE_AI, VALIDATOR_WARNING, "CommunicationHandler - You are trying to switch the state of the audio switch '%s' to the value '%s'. This switch state doesn't exist.", switchIdName, switchValue);
            }
            pIComponentAudio->SetSwitchState(switchControlId, switchStateId);
        }
    }
}


CommunicationHandler::CommunicationHandler(CAIProxy& proxy, IEntity* entity)
    : m_proxy(proxy)
    , m_entityId(entity->GetId())
    , m_agState(0)
    , m_currentQueryID(0)
    , m_currentPlaying(0)
    , m_signalInputID(0)
    , m_actionInputID(0)
{
    assert(entity);
    Reset();

    Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::AddRequestListener,
        &CommunicationHandler::TriggerFinishedCallback,
        this,
        Audio::eART_AUDIO_CALLBACK_MANAGER_REQUEST,
        Audio::eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE);
}

CommunicationHandler::~CommunicationHandler()
{
    Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::RemoveRequestListener, &CommunicationHandler::TriggerFinishedCallback, this);

    if (m_agState)
    {
        m_agState->RemoveListener(this);
    }
}

void CommunicationHandler::Reset()
{
    // Notify sound/voice listeners that the sounds have stopped
    {
        IComponentAudioPtr pIComponentAudio;
        IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_entityId);
        if (pEntity)
        {
            pIComponentAudio = pEntity->GetOrCreateComponent<IComponentAudio>();
        }

        PlayingSounds::iterator end = m_playingSounds.end();

        for (PlayingSounds::iterator it(m_playingSounds.begin()); it != end; ++it)
        {
            PlayingSound& playingSound(it->second);

            if (playingSound.listener)
            {
                ECommunicationHandlerEvent cancelEvent = (playingSound.type == Sound) ? SoundCancelled : VoiceCancelled;
                playingSound.listener->OnCommunicationHandlerEvent(cancelEvent, playingSound.playID, m_entityId);
            }

            if (pIComponentAudio)
            {
                pIComponentAudio->ExecuteTrigger(playingSound.correspondingStopControlId, eLSM_None);
            }
        }

        m_playingSounds.clear();
    }

    // Notify animation listeners that the animations have stopped
    {
        PlayingAnimations::iterator end = m_playingAnimations.end();

        for (PlayingAnimations::iterator it(m_playingAnimations.begin()); it != end; ++it)
        {
            PlayingAnimation& playingAnim(it->second);

            if (playingAnim.listener)
            {
                ECommunicationHandlerEvent cancelEvent = (playingAnim.method == AnimationMethodAction) ? ActionCancelled : SignalCancelled;
                playingAnim.listener->OnCommunicationHandlerEvent(cancelEvent, playingAnim.playID, m_entityId);
            }
        }

        m_playingAnimations.clear();
    }

    const ICVar* pCvar = gEnv->pConsole->GetCVar("ai_CommunicationForceTestVoicePack");
    const bool useTestVoicePack = pCvar ? pCvar->GetIVal() == 1 : false;


    IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_entityId);
    if (pEntity)
    {
        IComponentAudioPtr pIComponentAudio = pEntity->GetOrCreateComponent<IComponentAudio>();
        if (pIComponentAudio)
        {
            const ICommunicationManager::AudioConfiguration& audioConfigutation = gEnv->pAISystem->GetCommunicationManager()->GetAudioConfiguration();

            if (!audioConfigutation.switchNameForCharacterVoice.empty())
            {
                const char* voiceLibraryName = m_proxy.GetVoiceLibraryName(useTestVoicePack);
                if (voiceLibraryName && voiceLibraryName[0])
                {
                    ATLUtils::SetSwitchState(audioConfigutation.switchNameForCharacterVoice.c_str(), voiceLibraryName, pIComponentAudio);
                }
            }

            if (!audioConfigutation.switchNameForCharacterType.empty())
            {
                if (IAIObject* pAIObject = pEntity->GetAI())
                {
                    if (IAIActorProxy* pAIProxy = pAIObject->GetProxy())
                    {
                        stack_string characterType;
                        characterType.Format("%f", pAIProxy->GetFmodCharacterTypeParam());
                        if (!characterType.empty())
                        {
                            ATLUtils::SetSwitchState(audioConfigutation.switchNameForCharacterType.c_str(), characterType.c_str(), pIComponentAudio);
                        }
                    }
                }
            }
        }
    }
}

bool CommunicationHandler::IsInAGState(const char* name)
{
    if (GetAGState())
    {
        char inputValue[256] = "";
        m_agState->GetInput(m_signalInputID, inputValue);

        if (strcmp(inputValue, name) == 0)
        {
            return true;
        }

        m_agState->GetInput(m_actionInputID, inputValue);

        if (strcmp(inputValue, name) == 0)
        {
            return true;
        }
    }

    return false;
}

void CommunicationHandler::ResetAnimationState()
{
    if (GetAGState())
    {
        m_agState->SetInput(m_signalInputID, "none");
        m_agState->SetInput(m_actionInputID, "idle");

        m_agState->Update();
        m_agState->ForceTeleportToQueriedState();
    }
}

void CommunicationHandler::OnReused(IEntity* entity)
{
    assert(entity);
    m_entityId = entity->GetId();
    Reset();
}

SCommunicationSound CommunicationHandler::PlaySound(CommPlayID playID, const char* name, IEventListener* listener)
{
    return PlaySound(playID, name, Sound, eLSM_None, listener);
}

void CommunicationHandler::StopSound(const SCommunicationSound& soundToStop)
{
    IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_entityId);
    if (pEntity)
    {
        IComponentAudioPtr pIComponentAudio = pEntity->GetOrCreateComponent<IComponentAudio>();
        if (pIComponentAudio)
        {
            if (soundToStop.stopSoundControlId != INVALID_AUDIO_CONTROL_ID)
            {
                pIComponentAudio->ExecuteTrigger(soundToStop.stopSoundControlId, eLSM_None);
            }
            else
            {
                pIComponentAudio->StopTrigger(soundToStop.playSoundControlId);
            }

            PlayingSounds::iterator it = m_playingSounds.find(soundToStop.playSoundControlId);
            if (it != m_playingSounds.end())
            {
                PlayingSound& playingSound = it->second;
                if (playingSound.listener)
                {
                    ECommunicationHandlerEvent cancelEvent = (playingSound.type == Sound) ? SoundCancelled : VoiceCancelled;
                    playingSound.listener->OnCommunicationHandlerEvent(cancelEvent, playingSound.playID, m_entityId);
                }

                m_playingSounds.erase(it);
            }
        }
    }
}

SCommunicationSound CommunicationHandler::PlayVoice(CommPlayID playID, const char* variationName, ELipSyncMethod lipSyncMethod, IEventListener* listener)
{
    return PlaySound(playID, variationName, Voice, lipSyncMethod, listener);
}

void CommunicationHandler::StopVoice(const SCommunicationSound& voiceToStop)
{
    StopSound(voiceToStop);
}

void CommunicationHandler::PlayAnimation(CommPlayID playID, const char* name, EAnimationMethod method, IEventListener* listener)
{
    bool ok = false;

    if (GetAGState())
    {
        AnimationGraphInputID inputID = method == AnimationMethodAction ? m_actionInputID : m_signalInputID;

        if (ok = m_agState->SetInput(inputID, name, &m_currentQueryID))
        {
            //Force the animation graph to update to the new signal, to provide quicker communication responsiveness
            m_agState->Update();
            m_agState->ForceTeleportToQueriedState();
            std::pair<PlayingAnimations::iterator, bool> iresult = m_playingAnimations.insert(
                    PlayingAnimations::value_type(m_currentQueryID, PlayingAnimation()));

            PlayingAnimation& playingAnimation = iresult.first->second;
            playingAnimation.listener = listener;
            playingAnimation.name = name;
            playingAnimation.method = method;
            playingAnimation.playing = m_currentPlaying;
            playingAnimation.playID = playID;
        }
        m_currentPlaying = false;
        m_currentQueryID = 0;
    }

    if (!ok && listener)
    {
        listener->OnCommunicationHandlerEvent(
            (method == AnimationMethodAction) ? ActionFailed : SignalFailed, playID, m_entityId);
    }
}

void CommunicationHandler::StopAnimation(CommPlayID playID, const char* name, EAnimationMethod method)
{
    if (GetAGState())
    {
        if (method == AnimationMethodSignal)
        {
            m_agState->SetInput(m_signalInputID, "none");
        }
        else
        {
            m_agState->SetInput(m_actionInputID, "idle");
        }
        m_agState->Update();
        m_agState->ForceTeleportToQueriedState();
    }

    PlayingAnimations::iterator it = m_playingAnimations.begin();
    PlayingAnimations::iterator end = m_playingAnimations.end();

    for (; it != end; )
    {
        PlayingAnimation& playingAnim = it->second;

        bool animMatch = playID ? playingAnim.playID == playID : strcmp(playingAnim.name, name) == 0;

        if (animMatch)
        {
            if (playingAnim.listener)
            {
                ECommunicationHandlerEvent cancelEvent = (playingAnim.method == AnimationMethodSignal) ? SignalCancelled : ActionCancelled;
                playingAnim.listener->OnCommunicationHandlerEvent(cancelEvent, playingAnim.playID, m_entityId);
            }

            m_playingAnimations.erase(it++);
        }
        else
        {
            ++it;
        }
    }
}

SCommunicationSound CommunicationHandler::PlaySound(CommPlayID playID, const char* name, ESoundType type, ELipSyncMethod lipSyncMethod, IEventListener* listener)
{
    IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_entityId);
    if (pEntity)
    {
        IComponentAudioPtr pIComponentAudio = pEntity->GetOrCreateComponent<IComponentAudio>();
        if (pIComponentAudio)
        {
            const ICommunicationManager::AudioConfiguration& audioConfigutation = gEnv->pAISystem->GetCommunicationManager()->GetAudioConfiguration();

            Audio::TAudioControlID playCommunicationControlId(INVALID_AUDIO_CONTROL_ID);
            Audio::TAudioControlID stopCommunicationControlId(INVALID_AUDIO_CONTROL_ID);
            stack_string playTriggerName;
            playTriggerName.Format("%s%s", audioConfigutation.prefixForPlayTrigger.c_str(), name);
            stack_string stopTriggerName;
            stopTriggerName.Format("%s%s", audioConfigutation.prefixForStopTrigger.c_str(), name);

            Audio::AudioSystemRequestBus::BroadcastResult(playCommunicationControlId, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, playTriggerName);
            Audio::AudioSystemRequestBus::BroadcastResult(stopCommunicationControlId, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, stopTriggerName);
            if (playCommunicationControlId != INVALID_AUDIO_CONTROL_ID)
            {
                if (listener)
                {
                    std::pair<PlayingSounds::iterator, bool> iresult = m_playingSounds.insert(
                            PlayingSounds::value_type(playCommunicationControlId, PlayingSound()));

                    PlayingSound& playingSound = iresult.first->second;
                    playingSound.listener = listener;
                    playingSound.type = type;
                    playingSound.playID = playID;
                }

                Audio::SAudioCallBackInfos callbackInfos(this, reinterpret_cast<void*>(static_cast<UINT_PTR>(m_entityId)), this, Audio::eARF_PRIORITY_NORMAL | Audio::eARF_SYNC_FINISHED_CALLBACK);
                pIComponentAudio->ExecuteTrigger(playCommunicationControlId, lipSyncMethod, DEFAULT_AUDIO_PROXY_ID, callbackInfos);

                SCommunicationSound soundInfo;
                soundInfo.playSoundControlId = playCommunicationControlId;
                soundInfo.stopSoundControlId = stopCommunicationControlId;
                return soundInfo;
            }
            else
            {
                CryLogAlways("The audio trigger to play communication '%s' is not defined.", name);
            }
        }
    }

    return SCommunicationSound();
}

void CommunicationHandler::TriggerFinishedCallback(Audio::SAudioRequestInfo const* const pAudioRequestInfo)
{
    EntityId entityId = static_cast<EntityId>(reinterpret_cast<UINT_PTR>(pAudioRequestInfo->pUserData));

    if (IEntity* pEntity = gEnv->pEntitySystem->GetEntity(entityId))
    {
        if (IAIObject* pAIObject = pEntity->GetAI())
        {
            if (IAIActorProxy* pAiProxy = pAIObject->GetProxy())
            {
                if (IAICommunicationHandler* pCommunicationHandler = pAiProxy->GetCommunicationHandler())
                {
                    pCommunicationHandler->OnSoundTriggerFinishedToPlay(pAudioRequestInfo->nAudioControlID);
                }
            }
        }
    }
}

void CommunicationHandler::OnSoundTriggerFinishedToPlay(const Audio::TAudioControlID nTriggerID)
{
    PlayingSounds::iterator it = m_playingSounds.find(nTriggerID);
    if (it != m_playingSounds.end())
    {
        PlayingSound& playing = it->second;
        if (playing.listener)
        {
            ECommunicationHandlerEvent outEvent = (playing.type == Sound) ? SoundFinished : VoiceFinished;

            // Let the CommunicationPlayer know this sound/voice has finished
            playing.listener->OnCommunicationHandlerEvent(outEvent, playing.playID, m_entityId);
        }

        m_playingSounds.erase(it);
    }
}

IAnimationGraphState* CommunicationHandler::GetAGState()
{
    if (m_agState)
    {
        return m_agState;
    }

    if (IActor* actor = CCryAction::GetCryAction()->GetIActorSystem()->GetActor(m_entityId))
    {
        if (m_agState = actor->GetAnimationGraphState())
        {
            m_agState->AddListener("AICommunicationHandler", this);
            m_signalInputID = m_agState->GetInputId("Signal");
            m_actionInputID = m_agState->GetInputId("Action");
        }
    }

    return m_agState;
}

void CommunicationHandler::QueryComplete(TAnimationGraphQueryID queryID, bool succeeded)
{
    if (queryID == m_currentQueryID) // this call happened during SetInput
    {
        m_currentPlaying = true;
        m_agState->QueryLeaveState(&m_currentQueryID);

        return;
    }

    PlayingAnimations::iterator animationIt = m_playingAnimations.find(queryID);
    if (animationIt != m_playingAnimations.end())
    {
        PlayingAnimation& playingAnimation = animationIt->second;
        if (!playingAnimation.playing)
        {
            ECommunicationHandlerEvent event;

            if (playingAnimation.method == AnimationMethodAction)
            {
                event = succeeded ? ActionStarted : ActionFailed;
            }
            else
            {
                event = succeeded ? SignalStarted : SignalFailed;
            }

            if (playingAnimation.listener)
            {
                playingAnimation.listener->OnCommunicationHandlerEvent(event, playingAnimation.playID, m_entityId);
            }

            if (succeeded)
            {
                playingAnimation.playing = true;

                TAnimationGraphQueryID leaveQueryID;
                m_agState->QueryLeaveState(&leaveQueryID);

                m_playingAnimations.insert(PlayingAnimations::value_type(leaveQueryID, playingAnimation));
            }
        }
        else
        {
            ECommunicationHandlerEvent event;

            if (playingAnimation.method == AnimationMethodAction)
            {
                event = ActionCancelled;
            }
            else
            {
                event = succeeded ? SignalFinished : SignalCancelled;
            }

            if (playingAnimation.listener)
            {
                playingAnimation.listener->OnCommunicationHandlerEvent(event, playingAnimation.playID, m_entityId);
            }
        }

        m_playingAnimations.erase(animationIt);
    }
}

void CommunicationHandler::DestroyedState(IAnimationGraphState* agState)
{
    if (agState == m_agState)
    {
        m_agState = 0;
    }
}

bool CommunicationHandler::IsPlayingAnimation() const
{
    return !m_playingAnimations.empty();
}

bool CommunicationHandler::IsPlayingSound() const
{
    return true;
}
