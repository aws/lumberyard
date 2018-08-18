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

// Description : Dialog System


#include "CryLegacy_precompiled.h"
#include "DialogActorContext.h"
#include "DialogSession.h"
#include "DialogCommon.h"
#include "ICryAnimation.h"
#include "IFacialAnimation.h"
#include "StringUtils.h"
#include "IMovementController.h"
#include "ILocalizationManager.h"
#include "IAIObject.h"
#include "IAIActor.h"
#include "IAIActorProxy.h"
#include "Components/IComponentAudio.h"

static const float LOOKAT_TIMEOUT   = 1.0f;
static const float ANIM_TIMEOUT     = 1.0f;
static const float SOUND_TIMEOUT    = 1.0f;
static const float PLAYER_CHECKTIME = 0.2f;    // check the player every 0.2 seconds

static const uint32 INVALID_FACIAL_CHANNEL_ID = ~0;

#define DIALOG_VOICE_POSITION_NEEDS_UPDATE -2

string CDialogActorContext::m_tempBuffer;

void CDialogActorContext::ResetStaticData()
{
    stl::free_container(m_tempBuffer);
}

// returns ptr to static string buffer.
const char* CDialogActorContext::GetSoundKey(const string& soundName)
{
    stack_string const sLocalizationFolder(PathUtil::GetLocalizationFolder() + "dialog/");
    const size_t prefixLen = sLocalizationFolder.size();

    // check if it already starts localizationfoldername/dialog. then replace it
    if (_strnicmp(soundName.c_str(), sLocalizationFolder.c_str(), prefixLen) == 0)
    {
        m_tempBuffer.assign (soundName.c_str() + prefixLen);
    }
    else
    {
        m_tempBuffer.assign (soundName);
    }
    PathUtil::RemoveExtension(m_tempBuffer);
    return m_tempBuffer.c_str();
}

// returns ptr to static string buffer.
const char* CDialogActorContext::FullSoundName(const string& soundName, bool bWav)
{
    stack_string const sLocalizationFolder(PathUtil::GetLocalizationFolder() + "dialog/");

    // check if it already starts with full path
    if (CryStringUtils::stristr(soundName.c_str(), sLocalizationFolder.c_str()) == soundName.c_str())
    {
        m_tempBuffer = soundName;
    }
    else
    {
        m_tempBuffer  = sLocalizationFolder.c_str();
        m_tempBuffer += soundName;
    }
    PathUtil::RemoveExtension(m_tempBuffer);
    if (bWav)
    {
        m_tempBuffer += ".wav";
    }
    return m_tempBuffer.c_str();
}

////////////////////////////////////////////////////////////////////////////
CDialogActorContext::CDialogActorContext(CDialogSession* pSession, CDialogScript::TActorID actorID)
    : m_pCurLine(0)
{
    static int uniqueID = 0;
    m_ContextID = ++uniqueID;

    m_pSession   = pSession;
    m_actorID    = actorID;
    m_entityID   = pSession->GetActorEntityId(m_actorID);
    IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_entityID);
    const char* debugName = pEntity ? pEntity->GetName() : "<no entity>";
    m_pIActor = CCryAction::GetCryAction()->GetIActorSystem()->GetActor(m_entityID);
    m_bIsLocalPlayer = m_pIActor != 0 && CCryAction::GetCryAction()->GetClientActor() == m_pIActor;
    m_SpeechAuxProxy = INVALID_AUDIO_PROXY_ID;

    ResetState();
    DiaLOG::Log(DiaLOG::eAlways, "[DIALOG] CDialogActorContext::ctor: %s 0x%p actorID=%d entity=%s entityId=%u",
        m_pSession->GetDebugName(), this, m_actorID, debugName, m_entityID);
}

////////////////////////////////////////////////////////////////////////////
CDialogActorContext::~CDialogActorContext()
{
    if (m_SpeechAuxProxy != INVALID_AUDIO_PROXY_ID)
    {
        IEntity* pActorEntity = m_pSession->GetActorEntity(m_actorID);
        if (pActorEntity)
        {
            IComponentAudioPtr pActorAudioComponent = m_pSession->GetEntityAudioComponent(pActorEntity);
            if (pActorAudioComponent)
            {
                pActorAudioComponent->RemoveAsListenerFromAuxAudioProxy(m_SpeechAuxProxy, &CDialogActorContext::OnAudioTriggerFinished);
                pActorAudioComponent->RemoveAuxAudioProxy(m_SpeechAuxProxy);
            }
        }
    }

    StopSound();
    CancelCurrent();
    IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_entityID);
    const char* debugName = pEntity ? pEntity->GetName() : "<no entity>";
    DiaLOG::Log(DiaLOG::eAlways, "[DIALOG] CDialogActorContext::dtor: %s 0x%p actorID=%d entity=%s entityId=%d",
        m_pSession->GetDebugName(), this, m_actorID, debugName, m_entityID);
}

////////////////////////////////////////////////////////////////////////////
void CDialogActorContext::ResetState()
{
    // stop sound just in case to prevent forgetting to unrgister ourself as sound listener(can lead to a crash due dangling ptr)
    StopSound();
    m_pCurLine = 0;
    m_pAGState = 0;
    m_queryID = 0;
    m_bAnimStarted = false;
    m_bAnimUseAGSignal = false;
    m_bAnimUseEP = false;
    m_lookAtActorID = CDialogScript::NO_ACTOR_ID;
    m_stickyLookAtActorID = CDialogScript::NO_ACTOR_ID;
    m_bLookAtNeedsReset = false;
    m_goalPipeID = 0;
    m_exPosAnimPipeID = 0;
    m_phase = eDAC_Idle;
    m_bInCancel = false;
    m_bAbortFromAI = false;
    m_abortReason = CDialogSession::eAR_None;
    m_bIsAware = true;
    m_bIsAwareLooking = true;
    m_bIsAwareInRange = true;
    m_bSoundStopsAnim = false;
    m_checkPlayerTimeOut = 0.0f; // start to check player on start
    m_playerAwareTimeOut = m_pSession->GetPlayerAwarenessGraceTime(); // set timeout
    m_currentEffectorChannelID = INVALID_FACIAL_CHANNEL_ID;
    m_bNeedsCancel = false; // no cancel on start
    m_bSoundStarted = false;
    m_VoiceAttachmentIndex = DIALOG_VOICE_POSITION_NEEDS_UPDATE;
    m_BoneHeadJointID = DIALOG_VOICE_POSITION_NEEDS_UPDATE;
}

////////////////////////////////////////////////////////////////////////////
void CDialogActorContext::BeginSession()
{
    DiaLOG::Log(DiaLOG::eAlways, "[DIALOG] CDialogActorContext::BeginSession: %s Now=%f 0x%p actorID=%d ",
        m_pSession->GetDebugName(), m_pSession->GetCurTime(), this, m_actorID);

    ResetState();

    IEntitySystem* pES = gEnv->pEntitySystem;
    pES->AddEntityEventListener(m_entityID, ENTITY_EVENT_AI_DONE, this);
    pES->AddEntityEventListener(m_entityID, ENTITY_EVENT_DONE, this);
    pES->AddEntityEventListener(m_entityID, ENTITY_EVENT_RESET, this);

    switch (GetAIBehaviourMode())
    {
    case CDialogSession::eDIB_InterruptAlways:
        ExecuteAI(m_goalPipeID, "ACT_ANIM");
        break;
    case CDialogSession::eDIB_InterruptMedium:
        ExecuteAI(m_goalPipeID, "ACT_DIALOG");
        break;
    case CDialogSession::eDIB_InterruptNever:
        break;
    }
    m_bNeedsCancel = true;
}

////////////////////////////////////////////////////////////////////////////
void CDialogActorContext::EndSession()
{
    CancelCurrent();
    DiaLOG::Log(DiaLOG::eAlways, "[DIALOG] CDialogActorContext::EndSession: %s now=%f 0x%p actorID=%d ",
        m_pSession->GetDebugName(), m_pSession->GetCurTime(), this, m_actorID);
}

////////////////////////////////////////////////////////////////////////////
bool CDialogActorContext::Update(float dt)
{
    if (IsAborted())
    {
        return true;
    }

    // FIXME: this should never happen, as we should get a notification before.
    //        temp leave in for tracking down a bug [AlexL: 23/11/2006]
    IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_entityID);
    if (pEntity == 0)
    {
        m_pIActor = 0;
        GameWarning("[DIALOG] CDialogActorContext::Update: %s Actor=%d (EntityId=%d) no longer existent.",
            m_pSession->GetDebugName(), m_actorID, m_entityID);
        AbortContext(true, CDialogSession::eAR_EntityDestroyed);
        return true;
    }

    CDialogSession::AlertnessInterruptMode alertnessInterruptMode = m_pSession->GetAlertnessInterruptMode();
    if (alertnessInterruptMode != CDialogSession::None)
    {
        if (IAIObject* aiObject = pEntity->GetAI())
        {
            if (IAIActorProxy* aiActorProxy = aiObject->GetProxy())
            {
                if (aiActorProxy->GetAlertnessState() >= alertnessInterruptMode)
                {
                    AbortContext(true, CDialogSession::eAR_AIAborted);
                    return true;
                }
            }
        }
    }

    float now = m_pSession->GetCurTime();

    if (!CheckActorFlags(CDialogSession::eDACF_NoActorDeadAbort) && m_pIActor && m_pIActor->IsDead())
    {
        if (!IsAborted())
        {
            DiaLOG::Log(DiaLOG::eAlways, "[DIALOG] CDialogActorContext::Update: %s Actor=%d (EntityId=%d) has died. Aborting.",
                m_pSession->GetDebugName(), m_actorID, m_entityID);
            AbortContext(true, CDialogSession::eAR_ActorDead);
            return true;
        }
    }

    if (m_bIsLocalPlayer)
    {
        // when the local player is involved in the conversation do some special checks
        if (DoLocalPlayerChecks(dt) == false)
        {
            DiaLOG::Log(DiaLOG::eAlways, "[DIALOG] CDialogActorContext::Update: %s Abort from LocalPlayer.", m_pSession->GetDebugName());
            AbortContext(true, m_bIsAwareInRange == false ? CDialogSession::eAR_PlayerOutOfRange : CDialogSession::eAR_PlayerOutOfView);
            return true;
        }
    }

    if (m_bAbortFromAI)
    {
        m_bAbortFromAI = false;
        if (!IsAborted())
        {
            DiaLOG::Log(DiaLOG::eAlways, "[DIALOG] CDialogActorContext::Update: %s Abort from AI.", m_pSession->GetDebugName());
            AbortContext(true, CDialogSession::eAR_AIAborted);
            return true;
        }
    }

    if (SessionAllowsLookAt())
    {
        DoStickyLookAt();
    }

    int loop = 0;
    do
    {
        // DiaLOG::Log("[DIALOG] DiaLOG::eAlways"CDialogActorContext::Update: %s now=%f actorId=%d loop=%d", m_pSession->GetDebugName(), now, m_actorID, loop);

        bool bAdvance = false;
        switch (m_phase)
        {
        case eDAC_Idle:
            break;

        case eDAC_NewLine:
        {
            m_bHasScheduled = false;
            m_lookAtTimeOut = LOOKAT_TIMEOUT;
            m_animTimeOut   = ANIM_TIMEOUT;
            m_soundTimeOut  = SOUND_TIMEOUT;     // wait one second until sound is timed out
            m_bAnimScheduled = false;
            m_bAnimStarted   = false;
            m_bSoundScheduled = false;
            m_bSoundStarted   = false;
            m_soundLength   = 0.0f;

            if (m_pCurLine->m_flagResetLookAt)
            {
                m_stickyLookAtActorID = CDialogScript::NO_ACTOR_ID;
                m_lookAtActorID = CDialogScript::NO_ACTOR_ID;
            }
            // check if look-at sticky is set
            else if (m_pCurLine->m_lookatActor != CDialogScript::NO_ACTOR_ID)
            {
                if (m_pCurLine->m_flagLookAtSticky == false)
                {
                    m_lookAtActorID = m_pCurLine->m_lookatActor;
                    m_stickyLookAtActorID = CDialogScript::NO_ACTOR_ID;
                }
                else
                {
                    m_stickyLookAtActorID = m_pCurLine->m_lookatActor;
                    m_lookAtActorID = CDialogScript::NO_ACTOR_ID;
                }
            }

            bAdvance = true;

            // handle the first sticky look-at here
            if (SessionAllowsLookAt())
            {
                DoStickyLookAt();
            }

#if 0   // test for immediate scheduling of next line while the current line is still playing
            // maybe we're going to use some special delay TAGS like FORCE_NEXT, WAIT
            if (m_pCurLine->m_delay <= -999.0f)
            {
                m_bHasScheduled = true;
                m_pSession->ScheduleNextLine(0.0f);
            }
#endif
        }
        break;

        case eDAC_LookAt:
        {
            bool bWaitForLookAtFinish = true;
            bAdvance = true;

            if (SessionAllowsLookAt() == false)
            {
                break;
            }

            // Question: maybe do this although EP is requested
            if (bWaitForLookAtFinish && m_lookAtActorID != CDialogScript::NO_ACTOR_ID && m_pCurLine->m_flagAGEP == false)
            {
                IEntity* pActorEntity = m_pSession->GetActorEntity(m_actorID);
                IEntity* pLookAtEntity = m_pSession->GetActorEntity(m_lookAtActorID);
                if (pActorEntity != 0)
                {
                    m_lookAtTimeOut -= dt;
                    bool bTargetReached;
                    //bool bSuccess = DoLookAt(pActorEntity, pLookAtEntity, bTargetReached);
                    DoLookAt(pActorEntity, pLookAtEntity, bTargetReached);
                    //DiaLOG::Log(DiaLOG::eDebugA, "[DIALOG] CDialogActorContext::Update: %s now=%f actorID=%d phase=eDAC_LookAt %d",
                    //  m_pSession->GetDebugName(), now, m_actorID, bTargetReached);

                    if (/* bSuccess == false || */ bTargetReached || m_lookAtTimeOut <= 0.0f)
                    {
                        DiaLOG::Log(DiaLOG::eAlways, "[DIALOG] CDialogActorContext::Update: %s now=%f actorID=%d phase=eDAC_LookAt %s",
                            m_pSession->GetDebugName(), now, m_actorID, bTargetReached ? "Target Reached" : "Timed Out");
                        m_lookAtActorID = CDialogScript::NO_ACTOR_ID;
                    }
                    else
                    {
                        bAdvance = false;
                    }
                }
            }
        }
        break;

        case eDAC_Anim:
        {
            bAdvance = true;
            if (SessionAllowsLookAt() && m_lookAtActorID != CDialogScript::NO_ACTOR_ID)     // maybe: don't do this when EP is requested [&& m_pCurLine->m_flagAGEP == false]
            {
                IEntity* pActorEntity = m_pSession->GetActorEntity(m_actorID);
                IEntity* pLookAtEntity = m_pSession->GetActorEntity(m_lookAtActorID);
                if (pActorEntity != 0)
                {
                    m_lookAtTimeOut -= dt;
                    bool bTargetReached;
                    //bool bSuccess = DoLookAt(pActorEntity, pLookAtEntity, bTargetReached);
                    DoLookAt(pActorEntity, pLookAtEntity, bTargetReached);
                    if (/* bSuccess == false || */ bTargetReached || m_lookAtTimeOut <= 0.0f)
                    {
                        DiaLOG::Log(DiaLOG::eAlways, "[DIALOG] CDialogActorContext::Update: %s now=%f actorID=%d phase=eDAC_Anim %s",
                            m_pSession->GetDebugName(), now, m_actorID, bTargetReached ? "Target Reached" : "Timed Out");
                        m_lookAtActorID = CDialogScript::NO_ACTOR_ID;
                    }
                    else
                    {
                        bAdvance = false;
                    }
                }
            }
            const bool bHasAnim = !m_pCurLine->m_anim.empty();
            if (SessionAllowsAnim() && bHasAnim)
            {
                bAdvance = false;
                if (!m_bAnimScheduled)
                {
                    m_bSoundStopsAnim = false;     // whenever there is a new animation, no need for the still playing sound
                                                   // to stop the animation
                    m_bAnimStarted = false;
                    m_bAnimScheduled = true;
                    // schedule animation
                    IEntity* pActorEntity = m_pSession->GetActorEntity(m_actorID);
                    if (pActorEntity)
                    {
                        DoAnimAction(pActorEntity, m_pCurLine->m_anim, m_pCurLine->m_flagAGSignal, m_pCurLine->m_flagAGEP);
                    }
                    else
                    {
                        bAdvance = true;
                    }
                }
                else
                {
                    // we scheduled it already
                    // wait until it starts or timeout
                    m_animTimeOut -= dt;
                    bAdvance = m_animTimeOut <= 0.0f || m_bAnimStarted;
                    if (bAdvance)
                    {
                        DiaLOG::Log(DiaLOG::eAlways, "[DIALOG] CDialogActorContext::Update: %s Now=%f actorID=%d phase=eDAC_Anim %s",
                            m_pSession->GetDebugName(), now, m_actorID, m_bAnimStarted ? "Anim Started" : "Anim Timed Out");
                    }
                }
            }
        }
        break;

        case eDAC_ScheduleSoundPlay:
        {
            bAdvance = true;
            const bool bHasSound  = m_pCurLine->m_audioID != INVALID_AUDIO_CONTROL_ID;
            if (bHasSound)
            {
                if (m_bSoundScheduled == false)
                {
                    IEntity* pActorEntity = m_pSession->GetActorEntity(m_actorID);
                    if (pActorEntity == 0)
                    {
                        break;
                    }

                    IComponentAudioPtr pActorAudioComponent = m_pSession->GetEntityAudioComponent(pActorEntity);
                    if (pActorAudioComponent == 0)
                    {
                        break;
                    }

                    if (m_SpeechAuxProxy == INVALID_AUDIO_PROXY_ID)
                    {
                        m_SpeechAuxProxy = pActorAudioComponent->CreateAuxAudioProxy();
                        pActorAudioComponent->AddAsListenerToAuxAudioProxy(m_SpeechAuxProxy, &CDialogActorContext::OnAudioTriggerFinished, Audio::eART_AUDIO_CALLBACK_MANAGER_REQUEST, Audio::eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE);
                    }
                    UpdateAuxProxyPosition();

                    Audio::SAudioCallBackInfos callbackInfos(0, (void*)CDialogActorContext::GetClassIdentifier(), reinterpret_cast<void*>(static_cast<UINT_PTR>(m_ContextID)), Audio::eARF_PRIORITY_NORMAL | Audio::eARF_SYNC_FINISHED_CALLBACK);
                    if (!pActorAudioComponent->ExecuteTrigger(m_pCurLine->m_audioID, eLSM_None, m_SpeechAuxProxy, callbackInfos))
                    {
                        m_bSoundStarted = false;
                        m_bHasScheduled = false;
                        m_pSession->ScheduleNextLine(2.0f);
                    }
                    else
                    {
                        m_bSoundStarted = true;
                        m_bHasScheduled = true;
                        //todo: get the length of the sound to schedule the next line (set m_soundLength). Currently we schedule the next line as soon as the current one is finished, ignoring the specified "delay" parameter. its done in: DialogTriggerFinishedCallback

                        const char* triggerName = nullptr;
                        Audio::AudioSystemRequestBus::BroadcastResult(triggerName, &Audio::AudioSystemRequestBus::Events::GetAudioControlName, Audio::eACT_TRIGGER, m_pCurLine->m_audioID);
                        if (!triggerName)
                        {
                            triggerName = "dialog trigger";
                        }

                        DiaLOG::Log(DiaLOG::eDebugA, "[DIALOG] CDialogActorContex::Update: %s Now=%f actorID=%d phase=eDAC_ScheduleSoundPlay: Starting '%s'",
                            m_pSession->GetDebugName(), now, m_actorID, triggerName);
                    }
                }
                else
                {
                    // sound has been scheduled
                    // wait for sound start or timeout
                    m_soundTimeOut -= dt;
                    const bool bTimedOut = m_soundTimeOut <= 0.0f;
                    bAdvance = bTimedOut || m_bSoundStarted;
                    if (bAdvance)
                    {
                        if (bTimedOut)
                        {
                            StopSound(true);     // unregister from sound as listener and free resource
                        }
                        DiaLOG::Log(DiaLOG::eAlways, "[DIALOG] CDialogActorContext::Update: %s Now=%f actorID=%d phase=eDAC_ScheduleSoundPlay %s",
                            m_pSession->GetDebugName(), now, m_actorID, m_bSoundStarted ? "Sound Started" : "Sound Timed Out");
                    }
                }
            }
        }
        break;

        case eDAC_SoundFacial:
        {
            bAdvance = true;
            const bool bHasSound  = m_pCurLine->m_audioID != INVALID_AUDIO_CONTROL_ID;
            const bool bHasFacial = !m_pCurLine->m_facial.empty() || m_pCurLine->m_flagResetFacial;
            if (bHasFacial)
            {
                IEntity* pActorEntity = m_pSession->GetActorEntity(m_actorID);
                if (pActorEntity)
                {
                    DoFacialExpression(pActorEntity, m_pCurLine->m_facial, m_pCurLine->m_facialWeight, m_pCurLine->m_facialFadeTime);
                }
            }

            //hd-todo: re-add me, when we have a way to query the length of the audio-lines
            //float delay = m_pCurLine->m_delay;
            //if (bHasSound && m_bSoundStarted)
            //{
            //  if (m_soundLength <= 0.0f)
            //  {
            //      m_soundLength = cry_random(2.0f, 6.0f);
            //      DiaLOG::Log(DiaLOG::eAlways, "[DIALOG] CDialogActorContext::Update: %s Now=%f actorID=%d phase=eDAC_SoundFacial Faking SoundTime to %f",
            //          m_pSession->GetDebugName(), now, m_actorID, m_soundLength);
            //  }

            //  DiaLOG::Log(DiaLOG::eAlways, "[DIALOG] CDialogActorContext::Update: %s Now=%f actorID=%d phase=eDAC_SoundFacial Delay=%f SoundTime=%f",
            //      m_pSession->GetDebugName(), now , m_actorID, delay, m_soundLength);
            //  delay += m_soundLength;
            //}

            //// schedule END
            //if (delay < 0.0f)
            //  delay = 0.0f;
            //m_pSession->ScheduleNextLine(delay+0.05f); // ~1 frame at 20 fps. we now stop the current line sound before going for next line. Sometimes, this stop call is right before the
            //                                                                                   // sound has actually stopped in engine side. When that happens, the engine is adding ~2 seconds extra to the sound duration (because fade problems related to NAX header)
            //                                                                                   // and that looks bad if subtitles are enabled. So we add this small delay here to try to minimize that situation.
            //                                                                                   // this is part of the workaround for some dialogs apparently never finishing in the engine side (rare).
            //m_bHasScheduled = true;
        }
        break;

        case eDAC_EndLine:
            bAdvance = true;
            if (m_bHasScheduled == false)
            {
                m_bHasScheduled = true;
                m_pSession->ScheduleNextLine(m_pCurLine->m_delay);
            }
            break;
        }

        if (bAdvance)
        {
            AdvancePhase();
            dt = 0.0f;
            ++loop;
            assert (loop <= eDAC_EndLine + 1);
            if (loop > eDAC_EndLine + 1)
            {
                DiaLOG::Log(DiaLOG::eAlways, "[DIALOG] CDialogActorContext::Update: %s Actor=%d InternalLoopCount=%d. Final Exit!",
                    m_pSession->GetDebugName(), m_actorID, loop);
            }
        }
        else
        {
            if (IsStillPlaying())
            {
                UpdateAuxProxyPosition();
            }

            break;
        }
    }
    while (true);

    return true;
}
////////////////////////////////////////////////////////////////////////////

void CDialogActorContext::UpdateAuxProxyPosition()
{
    IEntity* pActorEntity = m_pSession->GetActorEntity(m_actorID);
    if (pActorEntity == 0)
    {
        return;
    }

    IComponentAudioPtr pActorAudioComponent = m_pSession->GetEntityAudioComponent(pActorEntity);
    if (pActorAudioComponent == 0)
    {
        return;
    }

    ICharacterInstance* const pCharacter = pActorEntity->GetCharacter(0);

    //cache some IDs
    if (m_VoiceAttachmentIndex == DIALOG_VOICE_POSITION_NEEDS_UPDATE && pCharacter)
    {
        const IAttachmentManager* pAttachmentManager = pCharacter->GetIAttachmentManager();
        if (pAttachmentManager)
        {
            m_VoiceAttachmentIndex = pAttachmentManager->GetIndexByName("voice");  //First prio: get the "voice" attachment position

            if (m_VoiceAttachmentIndex == -1)
            {
                // There's no attachment so let's try to find the head bone.
                IDefaultSkeleton& rIDefaultSkeleton = pCharacter->GetIDefaultSkeleton();
                m_BoneHeadJointID = rIDefaultSkeleton.GetJointIDByName("Bip01 Head");  //Second prio: use the Bip01 head bone position
                if (m_BoneHeadJointID == -1)
                {
                    // Has it been named differently?
                    m_BoneHeadJointID = rIDefaultSkeleton.GetJointIDByName("def_head");  //third prio: use the Bip01 head bone position
                }
            }
        }
    }

    Matrix34 tmHead;

    //try to get a position close to the "mouth" of our actor
    if (m_VoiceAttachmentIndex > 0 && pCharacter)
    {
        const IAttachmentManager* pAttachmentManager = pCharacter->GetIAttachmentManager();
        const IAttachment* pAttachment = pAttachmentManager->GetInterfaceByIndex(m_VoiceAttachmentIndex);
        if (pAttachment)
        {
            tmHead = Matrix34(pAttachment->GetAttModelRelative());
        }
    }
    else if (m_BoneHeadJointID > 0 && pCharacter)
    {
        // re-query SkeletonPose to prevent crash on removed Character
        if (ISkeletonPose* const pSkeletonPose = pCharacter->GetISkeletonPose())
        {
            tmHead = Matrix34(pSkeletonPose->GetAbsJointByID(m_BoneHeadJointID));
        }
    }
    else
    {
        //last-resort: if we could not find a head attachment point or a head-bone: Lets just assume the head is 1.80m above the feets
        tmHead = Matrix34(IDENTITY, Vec3(0.0f, 1.80f, 0.0f));
    }

    pActorAudioComponent->SetAuxAudioProxyOffset(Audio::SATLWorldPosition(tmHead), m_SpeechAuxProxy);
}

////////////////////////////////////////////////////////////////////////////
static const char* PhaseNames[] =
{
    "eDAC_Idle",
    "eDAC_NewLine",
    "eDAC_LookAt",
    "eDAC_Anim",
    "eDAC_ScheduleSoundPlay",
    "eDAC_SoundFacial",
    "eDAC_EndLine"
};

////////////////////////////////////////////////////////////////////////////
void CDialogActorContext::AdvancePhase()
{
    const int oldPhase = m_phase;
    ++m_phase;
    if (m_phase > eDAC_EndLine)
    {
        m_phase = eDAC_Idle;
    }

    DiaLOG::Log(DiaLOG::eAlways, "[DIALOG] CDialogActorContext:AdvancePhase: %s ActorID=%d %s->%s",
        m_pSession->GetDebugName(), m_actorID, PhaseNames[oldPhase], PhaseNames[m_phase]);
}

////////////////////////////////////////////////////////////////////////////
bool CDialogActorContext::PlayLine(const CDialogScript::SScriptLine* pLine)
{
    m_pCurLine = pLine;
    m_phase = eDAC_NewLine;
    return true;
}

////////////////////////////////////////////////////////////////////////////
bool CDialogActorContext::DoAnimAction(IEntity* pEntity, const string& action, bool bIsSignal, bool bUseEP)
{
    m_bAnimUseEP = bUseEP;
    m_bAnimUseAGSignal = bIsSignal;

    if (m_bAnimUseEP == false)
    {
        const bool bSuccess = DoAnimActionAG(pEntity, action.c_str());
        return bSuccess;
    }
    else
    {
        const bool bSuccess = DoAnimActionEP(pEntity, action.c_str());
        return bSuccess;
    }
    return false; // make some compilers happy
}

////////////////////////////////////////////////////////////////////////////
bool CDialogActorContext::DoAnimActionAG(IEntity* pEntity, const char* sAction)
{
    IActor* pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_entityID);
    if (pActor)
    {
        m_pAGState = pActor->GetAnimationGraphState();
        if (m_pAGState)
        {
            m_pAGState->AddListener("dsctx", this);
            if (m_bAnimUseAGSignal)
            {
                DiaLOG::Log(DiaLOG::eAlways, "[DIALOG] CDialogActorContext:DoAnimAction: %s ActorID=%d Setting signal to '%s'",
                    m_pSession->GetDebugName(), m_actorID, sAction);
                m_pAGState->SetInput("Signal", sAction, &m_queryID);
            }
            else
            {
                DiaLOG::Log(DiaLOG::eAlways, "[DIALOG] CDialogActorContext:DoAnimAction: %s ActorID=%d Setting input to '%s'",
                    m_pSession->GetDebugName(), m_actorID, sAction);
                m_pAGState->SetInput("Action", sAction, &m_queryID);
            }
        }
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////
bool CDialogActorContext::DoAnimActionEP(IEntity* pEntity, const char* sAction)
{
    IAIObject* pAI = pEntity->GetAI();
    if (pAI == 0)
    {
        return false;
    }

    IPipeUser* pPipeUser = pAI->CastToIPipeUser();
    if (!pPipeUser)
    {
        return false;
    }

    Vec3 pos = pEntity->GetWorldPos();
    Vec3 dir = pAI->GetMoveDir();

    // EP Direction is either lookat direction or forward direction
    CDialogScript::TActorID lookAt = m_pCurLine->m_lookatActor;
    if (lookAt != CDialogScript::NO_ACTOR_ID)
    {
        IEntity* pLookAtEntity = m_pSession->GetActorEntity(lookAt);
        if (pLookAtEntity != 0)
        {
            dir = pLookAtEntity->GetWorldPos();
            dir -= pos;
            dir.z = 0;
            dir.NormalizeSafe(pAI->GetMoveDir());
        }
    }
    pPipeUser->SetRefPointPos(pos, dir);

    static const Vec3 startRadius (0.1f, 0.1f, 0.1f);
    static const float dirTolerance = 5.f;
    static const float targetRadius = 0.05f;

    IAISignalExtraData* pData = gEnv->pAISystem->CreateSignalExtraData();
    pData->iValue2 = m_bAnimUseAGSignal ? 1 : 0;
    pData->SetObjectName(sAction);
    pData->point = startRadius;
    pData->fValue = dirTolerance;
    pData->point2.x = targetRadius;

    const bool ok = ExecuteAI(m_exPosAnimPipeID, "ACT_ANIMEX", pData);
    return ok;
}

////////////////////////////////////////////////////////////////////////////
bool CDialogActorContext::DoFacialExpression(IEntity* pEntity, const string& expression, float weight, float fadeTime)
{
    ICharacterInstance* pCharacter = pEntity->GetCharacter(0);
    if (!pCharacter)
    {
        return false;
    }
    IFacialInstance* pFacialInstance = pCharacter->GetFacialInstance();
    if (!pFacialInstance)
    {
        return false;
    }
    IFacialModel* pFacialModel = pFacialInstance->GetFacialModel();
    if (!pFacialModel)
    {
        return false;
    }
    IFacialEffectorsLibrary* pLibrary = pFacialModel->GetLibrary();
    if (!pLibrary)
    {
        return false;
    }

    IFacialEffector* pEffector = 0;
    if (!expression.empty())
    {
        pEffector = pLibrary->Find(expression.c_str());
        if (!pEffector)
        {
            GameWarning("[DIALOG] CDialogActorContext::DoFacialExpression: %s Cannot find '%s' for Entity '%s'", m_pSession->GetDebugName(), expression.c_str(), pEntity->GetName());
            return false;
        }
    }

    if (m_currentEffectorChannelID != INVALID_FACIAL_CHANNEL_ID)
    {
        // we fade out with the same fadeTime as fade in
        pFacialInstance->StopEffectorChannel(m_currentEffectorChannelID, fadeTime);
        m_currentEffectorChannelID = INVALID_FACIAL_CHANNEL_ID;
    }

    if (pEffector)
    {
        m_currentEffectorChannelID = pFacialInstance->StartEffectorChannel(pEffector, weight, fadeTime); // facial instance wants ms
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////
bool CDialogActorContext::DoLookAt(IEntity* pEntity, IEntity* pLookAtEntity, bool& bReachedTarget)
{
    bReachedTarget = false;
    IAIObject* pAI = pEntity->GetAI();
    if (pAI == 0)
    {
        return false;
    }

    IAIActor* pAIActor = pAI->CastToIAIActor();
    if (!pAIActor)
    {
        return false;
    }

    if (pLookAtEntity == 0)
    {
        pAIActor->ResetLookAt();
        bReachedTarget = true;
        m_bLookAtNeedsReset = false;
    }
    else
    {
        IAIObject* pTargetAI = pLookAtEntity->GetAI();
        Vec3 pos = pTargetAI ? pTargetAI->GetPos() : pLookAtEntity->GetWorldPos();
        bReachedTarget = pAIActor->SetLookAtPointPos(pos, true);
        m_bLookAtNeedsReset = true;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////
void CDialogActorContext::DoStickyLookAt()
{
    if (m_stickyLookAtActorID != CDialogScript::NO_ACTOR_ID)
    {
        IEntity* pActorEntity = m_pSession->GetActorEntity(m_actorID);
        IEntity* pLookAtEntity = m_pSession->GetActorEntity(m_stickyLookAtActorID);
        if (pActorEntity != 0)
        {
            bool bTargetReached;
            DoLookAt(pActorEntity, pLookAtEntity, bTargetReached);
        }
    }
}

////////////////////////////////////////////////////////////////////////////
void CDialogActorContext::CancelCurrent(bool bResetStates)
{
    if (!m_bNeedsCancel)
    {
        return;
    }

    assert (m_bInCancel == false);
    if (m_bInCancel == true)
    {
        return;
    }
    m_bInCancel = true;

    // remove from AG
    if (m_pAGState != 0)
    {
        m_pAGState->RemoveListener(this);
        if (bResetStates)
        {
            ResetAGState();
        }
        m_pAGState = 0;
    }
    m_queryID = 0;
    m_bAnimStarted = false;

    // reset lookat
    IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_entityID);
    if (pEntity)
    {
        IAIObject* pAI = pEntity->GetAI();
        if (pAI)
        {
            IAIActor* pAIActor = pAI->CastToIAIActor();
            if (pAIActor)
            {
                if (m_bLookAtNeedsReset)
                {
                    pAIActor->ResetLookAt();
                    m_bLookAtNeedsReset = false;
                }

                IPipeUser* pPipeUser = pAI->CastToIPipeUser();
                if (pPipeUser)
                {
                    if (m_goalPipeID > 0)
                    {
                        if (GetAIBehaviourMode() == CDialogSession::eDIB_InterruptMedium)
                        {
                            int dummyPipe = 0;
                            ExecuteAI(dummyPipe, "ACT_DIALOG_OVER", 0, false);
                        }
                        pPipeUser->UnRegisterGoalPipeListener(this, m_goalPipeID);
                        pPipeUser->RemoveSubPipe(m_goalPipeID, true);
                        m_goalPipeID = 0;
                    }
                    if (m_exPosAnimPipeID > 0)
                    {
                        pPipeUser->UnRegisterGoalPipeListener(this, m_exPosAnimPipeID);
                        pPipeUser->CancelSubPipe(m_exPosAnimPipeID);
                        pPipeUser->RemoveSubPipe(m_exPosAnimPipeID, false);
                        m_exPosAnimPipeID = 0;
                    }
                }
            }
        }
    }

    // facial expression is always reset
    // if (bResetStates)
    {
        // Reset Facial Expression
        IEntity* pActorEntity = m_pSession->GetActorEntity(m_actorID);
        if (pActorEntity)
        {
            DoFacialExpression(pActorEntity, "", 1.0f, 0.0f);
        }
    }

    // should we stop the current sound?
    // we don't stop the sound if the actor aborts and has eDACF_NoAbortSound set.
    // if actor died (entity destroyed) though, sound is stopped anyway
    const bool bDontStopSound = (IsAborted() == false && CheckActorFlags(CDialogSession::eDACF_NoAbortSound))
        || (IsAborted()
            && CheckActorFlags(CDialogSession::eDACF_NoAbortSound)
            && GetAbortReason() != CDialogSession::eAR_ActorDead
            && GetAbortReason() != CDialogSession::eAR_EntityDestroyed);

    if (bDontStopSound == false)
    {
        // stop sound (this also forces m_soundId to be INVALID_SOUNDID) and markes Context as non-playing!
        // e.g. IsStillPlaying will then return false
        // we explicitly stop the sound in the d'tor if we don't take this path here
        StopSound();
    }

    IEntitySystem* pES = gEnv->pEntitySystem;
    pES->RemoveEntityEventListener(m_entityID, ENTITY_EVENT_AI_DONE, this);
    pES->RemoveEntityEventListener(m_entityID, ENTITY_EVENT_DONE, this);
    pES->RemoveEntityEventListener(m_entityID, ENTITY_EVENT_RESET, this);

    m_phase = eDAC_Idle;

    m_bInCancel = false;
    m_bNeedsCancel = false;
}

////////////////////////////////////////////////////////////////////////////
void CDialogActorContext::ResetAGState()
{
    if (!m_pAGState)
    {
        return;
    }

    if (m_bAnimUseAGSignal)
    {
        if (!m_bAnimStarted)
        {
            m_pAGState->SetInput("Signal", "none");
        }
    }
    else
    {
        m_pAGState->SetInput("Action", "idle");
    }
}

////////////////////////////////////////////////////////////////////////////
void CDialogActorContext::AbortContext(bool bCancel, CDialogSession::EAbortReason reason)
{
    DiaLOG::Log(DiaLOG::eAlways, "[DIALOG] CDialogActorContext:AbortContext: %s actorID=%d ", m_pSession->GetDebugName(), m_actorID);
    m_phase = eDAC_Aborted;
    m_abortReason = reason;
    if (bCancel)
    {
        CancelCurrent();
    }
    // need to set it again, as CancelCurrent might have changed them
    m_phase = eDAC_Aborted;
    m_abortReason = reason;
}

////////////////////////////////////////////////////////////////////////////
bool CDialogActorContext::IsAborted() const
{
    return m_phase == eDAC_Aborted;
}

// IAnimationGraphStateListener
////////////////////////////////////////////////////////////////////////////
void CDialogActorContext::SetOutput(const char* output, const char* value)
{
}

////////////////////////////////////////////////////////////////////////////
void CDialogActorContext::QueryComplete(TAnimationGraphQueryID queryID, bool succeeded)
{
    IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_entityID);
    if (!pEntity)
    {
        return;
    }

    DiaLOG::Log(DiaLOG::eAlways, "[DIALOG] CDialogActorContext:QueryComplete: %s actorID=%d m_queryID=%d queryID=%d",
        m_pSession->GetDebugName(), m_actorID, m_queryID, queryID);

    if (queryID != m_queryID)
    {
        return;
    }

    if (succeeded || (m_bAnimStarted && !m_bAnimUseAGSignal))
    {
        if (m_bAnimStarted == false)
        {
            m_bAnimStarted = true;
            if (m_bAnimUseAGSignal)
            {
                m_pAGState->QueryLeaveState(&m_queryID);
            }
            else
            {
                m_pAGState->QueryChangeInput("Action", &m_queryID);
            }
        }
        else
        {
            if (m_pAGState)
            {
                m_pAGState->RemoveListener(this);
                m_pAGState = 0;
                m_queryID = 0;
                m_bAnimStarted = false;
            }
        }
    }
    else
    {
        ;
    }
}

////////////////////////////////////////////////////////////////////////////
void CDialogActorContext::DestroyedState(IAnimationGraphState*)
{
    m_pAGState = 0;
    m_queryID = 0;
    m_bAnimStarted = false;
}

// ~IAnimationGraphStateListener

////////////////////////////////////////////////////////////////////////////
bool CDialogActorContext::ExecuteAI(int& goalPipeID, const char* signalText, IAISignalExtraData* pExtraData, bool bRegisterAsListener)
{
    IEntitySystem* pSystem = gEnv->pEntitySystem;
    IEntity* pEntity = pSystem->GetEntity(m_entityID);
    if (pEntity == 0)
    {
        return false;
    }

    IAIObject* pAI = pEntity->GetAI();
    if (pAI == 0)
    {
        return false;
    }

    unsigned short nType = pAI->GetAIType();
    if (nType != AIOBJECT_ACTOR)
    {
        if (nType == AIOBJECT_PLAYER)
        {
            goalPipeID = -1;

            // not needed for player
            // pAI->SetSignal( 10, signalText, pEntity, NULL ); // 10 means this signal must be sent (but sent[!], not set)
            // even if the same signal is already present in the queue
            return true;
        }

        // invalid AIObject type
        return false;
    }

    IPipeUser* pPipeUser = pAI->CastToIPipeUser();
    if (pPipeUser)
    {
        if (goalPipeID > 0)
        {
            pPipeUser->RemoveSubPipe(goalPipeID, true);
            pPipeUser->UnRegisterGoalPipeListener(this, goalPipeID);
            goalPipeID = 0;
        }
    }

    goalPipeID = gEnv->pAISystem->AllocGoalPipeId();
    if (pExtraData == 0)
    {
        pExtraData = gEnv->pAISystem->CreateSignalExtraData();
    }
    pExtraData->iValue = goalPipeID;

    if (pPipeUser && bRegisterAsListener)
    {
        pPipeUser->RegisterGoalPipeListener(this, goalPipeID, "CDialogActorContext::ExecuteAI");
    }

    IAIActor* pAIActor = CastToIAIActorSafe(pAI);
    if (pAIActor)
    {
        pAIActor->SetSignal(10, signalText, pEntity, pExtraData);   // 10 means this signal must be sent (but sent[!], not set)
    }
    // even if the same signal is already present in the queue
    return true;
}

////////////////////////////////////////////////////////////////////////////
void CDialogActorContext::StopSound(bool bUnregisterOnly)
{
    if (bUnregisterOnly)
    {
        return;  //nothing to do here, because we dont registered for anything.
    }

    // Stop Sound
    if (IsStillPlaying())
    {
        IEntity* pActorEntity = m_pSession->GetActorEntity(m_actorID);
        if (pActorEntity == 0)
        {
            return;
        }

        IComponentAudioPtr pActorAudioComponent = m_pSession->GetEntityAudioComponent(pActorEntity);
        if (pActorAudioComponent == 0)
        {
            return;
        }

        pActorAudioComponent->StopTrigger(m_pCurLine->m_audioID);
    }
}

bool CDialogActorContext::DoLocalPlayerChecks(const float dt)
{
    // don't check this every frame, but only every .2 secs
    m_checkPlayerTimeOut -= dt;
    if (m_checkPlayerTimeOut <= 0.0f)
    {
        do // a dummy loop to use break
        {
            float awareDistance;
            float awareDistanceSq;
            float awareAngle;
            m_pSession->GetPlayerAwarenessValues(awareDistance, awareAngle);
            awareDistanceSq = awareDistance * awareDistance;

            m_checkPlayerTimeOut = PLAYER_CHECKTIME;
            const float spotAngleCos = cos_tpl(DEG2RAD(awareAngle));

            const CDialogSession::TActorContextMap& contextMap = m_pSession->GetAllContexts();
            if (contextMap.size() == 1 && contextMap.begin()->first == m_actorID)
            {
                m_bIsAware = true;
                break;
            }

            // early out, when we don't have to do any checks
            if (awareDistance <= 0.0f && awareAngle <= 0.0f)
            {
                m_bIsAware = true;
                break;
            }

            IEntity* pThisEntity = m_pSession->GetActorEntity(m_actorID);
            if (!pThisEntity)
            {
                assert (false);
                m_bIsAware = true;
                break;
            }
            IMovementController* pMC = (m_pIActor != NULL) ? m_pIActor->GetMovementController() : NULL;
            if (!pMC)
            {
                assert (false);
                m_bIsAware = true;
                break;
            }
            SMovementState moveState;
            pMC->GetMovementState(moveState);
            Vec3 viewPos = moveState.eyePosition;
            Vec3 viewDir = moveState.eyeDirection;
            viewDir.z = 0.0f;
            viewDir.NormalizeSafe();

            // check the player's position
            // check the player's view direction
            AABB groupBounds;
            groupBounds.Reset();

            CDialogSession::TActorContextMap::const_iterator iter = contextMap.begin();
            CDialogScript::SActorSet lookingAt = 0;
            while (iter != contextMap.end())
            {
                if (iter->first != m_actorID)
                {
                    IEntity* pActorEntity = m_pSession->GetActorEntity(iter->first);
                    if (pActorEntity)
                    {
                        Vec3 vEntityPos = pActorEntity->GetWorldPos();
                        AABB worldBounds;
                        pActorEntity->GetWorldBounds(worldBounds);
                        groupBounds.Add(worldBounds);
                        // calc if we look at it somehow
                        Vec3 vEntityDir = vEntityPos - viewPos;
                        vEntityDir.z = 0.0f;
                        vEntityDir.NormalizeSafe();
                        if (viewDir.IsUnit() && vEntityDir.IsUnit())
                        {
                            const float dot = clamp_tpl(viewDir.Dot(vEntityDir), -1.0f, +1.0f); // clamping should not be needed
                            if (spotAngleCos <= dot)
                            {
                                lookingAt.SetActor(iter->first);
                            }
                            DiaLOG::Log(DiaLOG::eDebugC, "Angle to actor %d is %f deg", iter->first, RAD2DEG(acos_tpl(dot)));
                        }
                    }
                }
                ++iter;
            }

            const float distanceSq = pThisEntity->GetWorldPos().GetSquaredDistance(groupBounds.GetCenter());
            CCamera& camera = gEnv->pSystem->GetViewCamera();
            const bool bIsInAABB  = camera.IsAABBVisible_F(groupBounds);
            const bool bIsInRange = distanceSq <= awareDistanceSq;
            const bool bIsLooking = contextMap.empty() || lookingAt.NumActors() > 0;
            m_bIsAwareLooking = awareAngle <= 0.0f || (bIsInAABB || bIsLooking);
            m_bIsAwareInRange = awareDistance <= 0.0f || bIsInRange;
            m_bIsAware = m_bIsAwareLooking && m_bIsAwareInRange;

            DiaLOG::Log(DiaLOG::eDebugB, "[DIALOG] LPC: %s awDist=%f awAng=%f AABBVis=%d IsLooking=%d InRange=%d [Distance=%f LookingActors=%d] Final=%saware",
                m_pSession->GetDebugName(), awareDistance, awareAngle, bIsInAABB, bIsLooking, bIsInRange, sqrt_tpl(distanceSq), lookingAt.NumActors(), m_bIsAware ? "" : "not ");
        } while (false);
    }

    if (m_bIsAware)
    {
        m_playerAwareTimeOut = m_pSession->GetPlayerAwarenessGraceTime();
    }
    else
    {
        m_playerAwareTimeOut -= dt;
        if (m_playerAwareTimeOut <= 0)
        {
            return false;
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////
bool CDialogActorContext::IsStillPlaying() const
{
    return m_bSoundStarted && m_pCurLine && m_pCurLine->m_audioID != INVALID_AUDIO_CONTROL_ID;
}

// IEntityEventListener
void CDialogActorContext::OnEntityEvent(IEntity* pEntity, SEntityEvent& event)
{
    switch (event.event)
    {
    case ENTITY_EVENT_RESET:
    case ENTITY_EVENT_DONE:
        AbortContext(true, CDialogSession::eAR_EntityDestroyed);
        break;
    default:
        break;
    }
}

// ~IEntityEventListener

const char* EventName(EGoalPipeEvent event)
{
    switch (event)
    {
    case ePN_Deselected:
        return "ePN_Deselected";
    case ePN_Removed:
        return "ePN_Removed";
    case ePN_Finished:
        return "ePN_Finished";
    case ePN_Suspended:
        return "ePN_Suspended";
    case ePN_Resumed:
        return "ePN_Resumed";
    case ePN_AnimStarted:
        return "ePN_AnimStarted";
    case ePN_RefPointMoved:
        return "ePN_RefPointMoved";
    default:
        return "UNKNOWN GOAL PIPEEVENT";
    }
}

// IGoalPipeListener
////////////////////////////////////////////////////////////////////////////
void CDialogActorContext::OnGoalPipeEvent(IPipeUser* pPipeUser, EGoalPipeEvent event, int goalPipeId, bool& unregisterListenerAfterEvent)
{
    if (m_goalPipeID != goalPipeId && m_exPosAnimPipeID != goalPipeId)
    {
        return;
    }

#if 1
    DiaLOG::Log(DiaLOG::eAlways, "[DIALOG] CDialogActorContext::OnGoalPipeEvent: %s 0x%p actorID=%d goalPipe=%d m_goalPipeID=%d m_exPosAnimPipeID=%d event=%s",
        m_pSession->GetDebugName(), this, m_actorID, goalPipeId, m_goalPipeID, m_exPosAnimPipeID, EventName(event));
#endif

    switch (event)
    {
    case ePN_Deselected:
    case ePN_Removed:
        if (CheckActorFlags(CDialogSession::eDACF_NoAIAbort) == false)
        {
            if (m_goalPipeID == goalPipeId)
            {
                m_bAbortFromAI = true;
            }
        }
        break;
    case ePN_AnimStarted:
        m_bAnimStarted = true;
        break;
    default:
        break;
    }
}
// ~IGoalPipeListener


void CDialogActorContext::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(m_pSession);
    pSizer->AddObject(m_pCurLine);
    pSizer->AddObject(m_pIActor);
    pSizer->AddObject(m_pAGState);
}

////////////////////////////////////////////////////////////////////////////
void CDialogActorContext::OnAudioTriggerFinished(Audio::SAudioRequestInfo const* const pAudioRequestInfo)
{
    CDialogActorContext* dialogContext = GetDialogActorContextByAudioCallbackData(pAudioRequestInfo);
    if (dialogContext)
    {
        //hd-todo: for now we schedule the next line, as soon as the current line has finished. replace this with the old manual delay system, as soon as we have a way to figure out the length of the audio-lines
        dialogContext->m_pSession->ScheduleNextLine(0.2f);
        //mark as finished
        dialogContext->m_bSoundStarted = false;
    }
}

////////////////////////////////////////////////////////////////////////////
CDialogActorContext* CDialogActorContext::GetDialogActorContextByAudioCallbackData(Audio::SAudioRequestInfo const* const pAudioRequestInfo)
{
    if (pAudioRequestInfo->pUserDataOwner && pAudioRequestInfo->pUserData == CDialogActorContext::GetClassIdentifier())
    {
        EntityId const nContextID = static_cast<EntityId>(reinterpret_cast<uintptr_t>(pAudioRequestInfo->pUserDataOwner));
        CDialogSystem* pDS = CCryAction::GetCryAction()->GetDialogSystem();
        return pDS->GetActiveSessionActorContext(nContextID);
    }
    return nullptr;
}
