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

#include "CryLegacy_precompiled.h"
#include "ComponentAudio.h"
#include <IAudioSystem.h>
#include <ICryAnimation.h>
#include "Entity.h"

#include <IEntity.h>
#include <IEntityHelper.h>

#include "Components/IComponentArea.h"
#include "Components/IComponentSerialization.h"

const Audio::SATLWorldPosition CComponentAudio::s_oNULLOffset;
CComponentAudio::TAudioProxyPair CComponentAudio::s_oNULLAudioProxyPair(INVALID_AUDIO_PROXY_ID, static_cast<Audio::IAudioProxy*>(nullptr));

//////////////////////////////////////////////////////////////////////////
inline bool IsAudioAnimEvent(const char* eventName)
{
    return eventName
        && (azstricmp("audio_trigger", eventName) == 0 || azstricmp("sound", eventName) == 0);
}

DECLARE_DEFAULT_COMPONENT_FACTORY(CComponentAudio, IComponentAudio)

//////////////////////////////////////////////////////////////////////////
CComponentAudio::CComponentAudio()
    : m_pEntity(nullptr)
    , m_nAudioProxyIDCounter(INVALID_AUDIO_PROXY_ID)
    , m_nAudioEnvironmentID(INVALID_AUDIO_ENVIRONMENT_ID)
    , m_nFlags(eEACF_CAN_MOVE_WITH_ENTITY)
    , m_fFadeDistance(0.0f)
    , m_fEnvironmentFadeDistance(0.0f)
{
}

//////////////////////////////////////////////////////////////////////////
CComponentAudio::~CComponentAudio()
{
    m_pEntity = nullptr;
    std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SReleaseAudioProxy());
    m_mapAuxAudioProxies.clear();
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::Initialize(const SComponentInitializer& init)
{
    m_pEntity = init.m_pEntity;
    m_pEntity->GetComponent<IComponentSerialization>()->Register<CComponentAudio>(SerializationOrder::Audio, *this, &CComponentAudio::Serialize, &CComponentAudio::SerializeXML, &CComponentAudio::NeedSerialize, &CComponentAudio::GetSignature);
    assert(m_mapAuxAudioProxies.empty());

    // Creating the default AudioProxy.
    CreateAuxAudioProxy();
    UpdateHideStatus();
    SetObstructionCalcType(Audio::eAOOCT_IGNORE);
    OnMove();
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::Reload(IEntity* pEntity, SEntitySpawnParams& params)
{
    m_pEntity = pEntity;
    m_fFadeDistance = 0.0f;
    m_fEnvironmentFadeDistance = 0.0f;
    m_nAudioEnvironmentID = INVALID_AUDIO_ENVIRONMENT_ID;

    UpdateHideStatus();

    std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SResetAudioProxy());

#if defined(INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE)
    std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SInitializeAudioProxy(m_pEntity->GetName()));
#else
    std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SInitializeAudioProxy(nullptr));
#endif // INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE

    SetObstructionCalcType(Audio::eAOOCT_IGNORE);
    OnMove();
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::OnMove()
{
    const Matrix34& tm = m_pEntity->GetWorldTM();

    if (tm.IsValid())
    {
        const Audio::SATLWorldPosition oPosition(tm);

        //  Update all proxies with the eEACF_CAN_MOVE_WITH_ENTITY flag set.
        std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SRepositionAudioProxy(oPosition));

        if ((m_pEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) != 0)
        {
            Audio::SAudioRequest oRequest;
            // Don't set the audio object ID, this will go to the default listener unless
            // the listener is being overridden by another.
            oRequest.nFlags = Audio::eARF_PRIORITY_NORMAL;
            oRequest.pOwner = this;

            Audio::SAudioListenerRequestData<Audio::eALRT_SET_POSITION> oRequestData(oPosition);

            // Make sure direction and up vector are normalized
            oRequestData.oNewPosition.NormalizeForwardVec();
            oRequestData.oNewPosition.NormalizeUpVec();

            oRequest.pData = &oRequestData;

            Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, oRequest);

            // As this is an audio listener add its entity to the AreaManager for raising audio relevant events.
            gEnv->pEntitySystem->GetAreaManager()->MarkEntityForUpdate(m_pEntity->GetId());
        }
    }

    // Audio: voice attachement lookup needs to be done by code creating CComponentAudio for voice line on character
    //if (m_nAttachmentIndex != -1)
    //{
    //  if (ICharacterInstance* const pCharacter = m_pEntity->GetCharacter(0))
    //  {
    //      if (m_nAttachmentIndex != -1)
    //      {
    //          if (IAttachmentManager const* const pAttachmentManager = pCharacter->GetIAttachmentManager())
    //          {
    //              if (IAttachment const* const pAttachment = pAttachmentManager->GetInterfaceByIndex(m_nAttachmentIndex))
    //              {
    //                  tm = Matrix34(pAttachment->GetAttWorldAbsolute());
    //              }
    //          }
    //      }
    //      else if (m_nBoneHead != -1)
    //      {
    //          // re-query SkeletonPose to prevent crash on removed Character
    //          if (ISkeletonPose* const pSkeletonPose = pCharacter->GetISkeletonPose())
    //          {
    //              tm = tm * Matrix34(pSkeletonPose->GetAbsJointByID(m_nBoneHead));
    //          }
    //      }
    //  }
    //}
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::OnListenerMoveInside(const IEntity* const pEntity)
{
    m_pEntity->SetPos(pEntity->GetWorldPos());
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::OnMoveInside(const IEntity* const pEntity)
{
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::OnListenerExclusiveMoveInside(const IEntity* const __restrict pEntity, const IEntity* const __restrict pAreaHigh, const IEntity* const __restrict pAreaLow, const float fFade)
{
    const IComponentArea* const __restrict pAreaComponentLow = pAreaLow->GetComponent<IComponentArea>().get();
    IComponentArea* const __restrict pAreaComponentHigh = const_cast<IComponentArea* const __restrict>(pAreaHigh->GetComponent<IComponentArea>().get());

    if (pAreaComponentLow && pAreaComponentHigh)
    {
        Vec3 OnHighHull3d(ZERO);
        const Vec3 oPos(pEntity->GetWorldPos());
        const EntityId nEntityID = pEntity->GetId();
        const bool bInsideLow = pAreaComponentLow->CalcPointWithin(nEntityID, oPos);

        if (bInsideLow)
        {
            const float fDistSq = pAreaComponentHigh->ClosestPointOnHullDistSq(nEntityID, oPos, OnHighHull3d);
            m_pEntity->SetPos(OnHighHull3d);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::OnExclusiveMoveInside(const IEntity* const __restrict pEntity, const IEntity* const __restrict pEntityAreaHigh, const IEntity* const __restrict pEntityAreaLow, const float fEnvironmentFade)
{
    SetEnvironmentAmountInternal(pEntity, fEnvironmentFade);
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::OnListenerEnter(const IEntity* const pEntity)
{
    m_pEntity->SetPos(pEntity->GetWorldPos());
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::OnEnter(const IEntity* const pEntity)
{
    SetEnvironmentAmountInternal(pEntity, 1.0f);
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::OnLeaveNear(const IEntity* const pEntity)
{
    SetEnvironmentAmountInternal(pEntity, 0.0f);
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::OnAreaCrossing()
{
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::OnListenerMoveNear(const IEntity* const __restrict pEntity, const IEntity* const __restrict pArea)
{
    IComponentArea* const pAreaComponent = const_cast<IComponentArea*>(pArea->GetComponent<IComponentArea>().get());

    if (pAreaComponent)
    {
        Vec3 OnHull3d(ZERO);
        const float fDistSq = pAreaComponent->CalcPointNearDistSq(pEntity->GetId(), pEntity->GetWorldPos(), OnHull3d);
        m_pEntity->SetPos(OnHull3d);
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::OnMoveNear(const IEntity* const __restrict pEntity, const IEntity* const __restrict pArea)
{
    if (m_nAudioEnvironmentID == INVALID_AUDIO_ENVIRONMENT_ID)
    {
        return;
    }

    IComponentAudio* const pAudioComponent = const_cast<IComponentAudio*>(pEntity->GetComponent<IComponentAudio>().get());
    IComponentArea* const pAreaComponent = const_cast<IComponentArea*>(pArea->GetComponent<IComponentArea>().get());

    if (pAudioComponent && pAreaComponent && (m_nAudioEnvironmentID != INVALID_AUDIO_ENVIRONMENT_ID))
    {
        // Only set the Audio Environment Amount on the entities that already have an AudioProxy.
        // Passing INVALID_AUDIO_PROXY_ID to address all auxiliary AudioProxies on pAudioComponent.
        Vec3 OnHull3d(ZERO);
        const float fDist = sqrt(pAreaComponent->CalcPointNearDistSq(pEntity->GetId(), pEntity->GetWorldPos(), OnHull3d));

        if ((fDist > m_fEnvironmentFadeDistance) || (m_fEnvironmentFadeDistance < FLT_EPSILON))
        {
            pAudioComponent->SetEnvironmentAmount(m_nAudioEnvironmentID, 0.0f, INVALID_AUDIO_PROXY_ID);
        }
        else
        {
            pAudioComponent->SetEnvironmentAmount(m_nAudioEnvironmentID, 1.0f - (fDist / m_fEnvironmentFadeDistance), INVALID_AUDIO_PROXY_ID);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::ProcessEvent(SEntityEvent& event)
{
    if (m_pEntity)
    {
        switch (event.event)
        {
        case ENTITY_EVENT_XFORM:
        {
            const int nFlags = (int)event.nParam[0];

            if ((nFlags & (ENTITY_XFORM_POS | ENTITY_XFORM_ROT)) != 0)
            {
                OnMove();
            }
            break;
        }
        case ENTITY_EVENT_ENTERAREA:
        {
            if ((m_pEntity->GetFlags() & ENTITY_FLAG_VOLUME_SOUND) != 0)
            {
                const auto nEntityID = static_cast<EntityId>(event.nParam[0]);     // Entering entity!
                IEntity* const pEntity = gEnv->pEntitySystem->GetEntity(nEntityID);

                if (pEntity && (pEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) != 0)
                {
                    OnListenerEnter(pEntity);
                }
                else if (pEntity && (m_nAudioEnvironmentID != INVALID_AUDIO_ENVIRONMENT_ID))
                {
                    OnEnter(pEntity);
                }
            }
            break;
        }
        case ENTITY_EVENT_LEAVEAREA:
        {
            break;
        }
        case ENTITY_EVENT_CROSS_AREA:
        {
            if ((m_pEntity->GetFlags() & ENTITY_FLAG_VOLUME_SOUND) != 0)
            {
                const auto nEntityID = static_cast<EntityId>(event.nParam[0]);     // Crossing entity!
                IEntity* const pEntity = gEnv->pEntitySystem->GetEntity(nEntityID);

                if (pEntity && (pEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) != 0)
                {
                    OnAreaCrossing();
                }
            }
            break;
        }
        case ENTITY_EVENT_MOVENEARAREA:
        case ENTITY_EVENT_ENTERNEARAREA:
        {
            if ((m_pEntity->GetFlags() & ENTITY_FLAG_VOLUME_SOUND) != 0)
            {
                const auto nEntityID = static_cast<EntityId>(event.nParam[0]);     // Near entering/moving entity!
                IEntity* const pEntity = gEnv->pEntitySystem->GetEntity(nEntityID);

                const auto nAreaEntityID = static_cast<EntityId>(event.nParam[2]);
                IEntity* const pArea = gEnv->pEntitySystem->GetEntity(nAreaEntityID);

                if (pEntity && pArea)
                {
                    if ((pEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) != 0)
                    {
                        // This entity is an audio listener.
                        OnListenerMoveNear(pEntity, pArea);
                    }
                    else if (m_nAudioEnvironmentID != INVALID_AUDIO_ENVIRONMENT_ID)
                    {
                        OnMoveNear(pEntity, pArea);
                    }
                }
            }
            break;
        }
        case ENTITY_EVENT_LEAVENEARAREA:
        {
            if ((m_pEntity->GetFlags() & ENTITY_FLAG_VOLUME_SOUND) != 0)
            {
                const auto nEntityID = static_cast<EntityId>(event.nParam[0]);     // Leaving entity!
                IEntity* const pEntity = gEnv->pEntitySystem->GetEntity(nEntityID);

                if (pEntity && (pEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) == 0)
                {
                    OnLeaveNear(pEntity);
                }
            }
            break;
        }
        case ENTITY_EVENT_MOVEINSIDEAREA:
        {
            if ((m_pEntity->GetFlags() & ENTITY_FLAG_VOLUME_SOUND) != 0)
            {
                const auto nEntityID = static_cast<EntityId>(event.nParam[0]);     // Inside moving entity!
                IEntity* const __restrict pEntity = gEnv->pEntitySystem->GetEntity(nEntityID);

                if (pEntity)
                {
                    const auto nAreaID1 = static_cast<EntityId>(event.nParam[2]);     // AreaEntityID (low)
                    const auto nAreaID2 = static_cast<EntityId>(event.nParam[3]);     // AreaEntityID (high)

                    IEntity* const __restrict pArea1 = gEnv->pEntitySystem->GetEntity(nAreaID1);
                    IEntity* const __restrict pArea2 = gEnv->pEntitySystem->GetEntity(nAreaID2);

                    if (pArea1)
                    {
                        if (pArea2)
                        {
                            if ((pEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) != 0)
                            {
                                OnListenerExclusiveMoveInside(pEntity, pArea2, pArea1, event.fParam[0]);
                            }
                            else
                            {
                                OnExclusiveMoveInside(pEntity, pArea2, pArea1, event.fParam[1]);
                            }
                        }
                        else
                        {
                            if ((pEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) != 0)
                            {
                                OnListenerMoveInside(pEntity);
                            }
                            else
                            {
                                OnMoveInside(pEntity);
                            }
                        }
                    }
                }
            }
            break;
        }
        case ENTITY_EVENT_ANIM_EVENT:
        {
            auto pAnimEvent = reinterpret_cast<const AnimEventInstance*>(event.nParam[0]);
            auto pCharacter = reinterpret_cast<ICharacterInstance*>(event.nParam[1]);
            const char* eventName = (pAnimEvent ? pAnimEvent->m_EventName : nullptr);
            if (IsAudioAnimEvent(eventName))
            {
                Vec3 offset(ZERO);
                if (pAnimEvent->m_BonePathName && pAnimEvent->m_BonePathName[0])
                {
                    if (pCharacter)
                    {
                        IDefaultSkeleton& rIDefaultSkeleton = pCharacter->GetIDefaultSkeleton();
                        int id = rIDefaultSkeleton.GetJointIDByName(pAnimEvent->m_BonePathName);
                        if (id >= 0)
                        {
                            ISkeletonPose* pSkeletonPose = pCharacter->GetISkeletonPose();
                            QuatT boneQuat(pSkeletonPose->GetAbsJointByID(id));
                            offset = boneQuat.t;
                        }
                    }
                }

                Audio::TAudioControlID controlId = INVALID_AUDIO_CONTROL_ID;
                Audio::AudioSystemRequestBus::BroadcastResult(controlId, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, pAnimEvent->m_CustomParameter);
                if (controlId != INVALID_AUDIO_CONTROL_ID)
                {
                    ExecuteTrigger(controlId, eLSM_None);
                }
            }
            break;
        }
        case ENTITY_EVENT_HIDE:
        case ENTITY_EVENT_UNHIDE:
        {
            UpdateHideStatus();
            break;
        }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CComponentAudio::GetSignature(TSerialize signature)
{
    // CComponentAudio is not relevant to signature as it is always created again if needed
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::Serialize(TSerialize ser)
{
}

//////////////////////////////////////////////////////////////////////////
bool CComponentAudio::ExecuteTrigger(
    const Audio::TAudioControlID nTriggerID,
    const ELipSyncMethod eLipSyncMethod,
    const Audio::TAudioProxyID nAudioProxyLocalID /* = DEFAULT_AUDIO_PROXY_ID */,
    const Audio::SAudioCallBackInfos& rCallBackInfos /* = SAudioCallBackInfos::GetEmptyObject() */)
{
    if (m_pEntity)
    {
        const Matrix34& tm = m_pEntity->GetWorldTM();
    #if defined(INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE)
        if (tm.GetTranslation() == Vec3_Zero)
        {
            gEnv->pSystem->Warning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "<Audio> Trying to execute an audio trigger at (0,0,0) position in the entity %s. Entity may not be initialized correctly!", m_pEntity->GetEntityTextDescription());
        }
    #endif // INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE

        if ((m_nFlags & eEACF_HIDDEN) == 0 || (m_pEntity->GetFlags() & ENTITY_FLAG_UPDATE_HIDDEN) != 0)
        {
            const Audio::SATLWorldPosition oPosition(tm);

            if (nAudioProxyLocalID != INVALID_AUDIO_PROXY_ID)
            {
                const TAudioProxyPair& rAudioProxyPair = GetAuxAudioProxyPair(nAudioProxyLocalID);

                if (rAudioProxyPair.first != INVALID_AUDIO_PROXY_ID)
                {
                    (SRepositionAudioProxy(oPosition))(rAudioProxyPair);
                    rAudioProxyPair.second.pIAudioProxy->ExecuteTrigger(nTriggerID, eLipSyncMethod, rCallBackInfos);
                    return true;
                }
            #if defined(INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE)
                else
                {
                    const char* audioControlName = nullptr;
                    Audio::AudioSystemRequestBus::BroadcastResult(audioControlName, &Audio::AudioSystemRequestBus::Events::GetAudioControlName, Audio::eACT_TRIGGER, nTriggerID);
                    gEnv->pSystem->Warning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "<Audio> Could not find AuxAudioProxy with id '%u' on entity '%s' to ExecuteTrigger '%s'", nAudioProxyLocalID, m_pEntity->GetEntityTextDescription(), audioControlName);
                }
            #endif // INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE
            }
            else
            {
                for (TAuxAudioProxies::iterator it = m_mapAuxAudioProxies.begin(); it != m_mapAuxAudioProxies.end(); ++it)
                {
                    (SRepositionAudioProxy(oPosition))(*it);
                    it->second.pIAudioProxy->ExecuteTrigger(nTriggerID, eLipSyncMethod, rCallBackInfos);
                }
                return !m_mapAuxAudioProxies.empty();
            }

            // Audio: lip sync on EntityAudioProxy in executetrigger
            //if (lipSyncMethod != eLSM_None)
            //{
            //  // If voice is playing inform provider (if present) about it to apply lip-sync.
            //  if (m_pLipSyncProvider)
            //  {
            //      m_currentLipSyncId = pSound->GetId();
            //      m_currentLipSyncMethod = lipSyncMethod;
            //      m_pLipSyncProvider->RequestLipSync(this, m_currentLipSyncId, m_currentLipSyncMethod);
            //  }
            //}
        }
        else
        {
            gEnv->pSystem->Warning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "<Audio> Trying to execute an audio trigger on %s which is hidden!", m_pEntity->GetEntityTextDescription());
        }
    }
    else
    {
        gEnv->pSystem->Warning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "<Audio> Trying to execute an audio trigger on a ComponentAudio without a valid entity!");
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::StopTrigger(const Audio::TAudioControlID nTriggerID, const Audio::TAudioProxyID nAudioProxyLocalID /* = DEFAULT_AUDIO_PROXY_ID */)
{
    if (nAudioProxyLocalID != INVALID_AUDIO_PROXY_ID)
    {
        const TAudioProxyPair& rAudioProxyPair = GetAuxAudioProxyPair(nAudioProxyLocalID);

        if (rAudioProxyPair.first != INVALID_AUDIO_PROXY_ID)
        {
            (SStopTrigger(nTriggerID))(rAudioProxyPair);
        }
    }
    else
    {
        std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SStopTrigger(nTriggerID));
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::SetSwitchState(const Audio::TAudioControlID nSwitchID, const Audio::TAudioSwitchStateID nStateID, const Audio::TAudioProxyID nAudioProxyLocalID /* = DEFAULT_AUDIO_PROXY_ID */)
{
    if (nAudioProxyLocalID != INVALID_AUDIO_PROXY_ID)
    {
        const TAudioProxyPair& rAudioProxyPair = GetAuxAudioProxyPair(nAudioProxyLocalID);

        if (rAudioProxyPair.first != INVALID_AUDIO_PROXY_ID)
        {
            (SSetSwitchState(nSwitchID, nStateID))(rAudioProxyPair);
        }
    }
    else
    {
        std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SSetSwitchState(nSwitchID, nStateID));
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::SetRtpcValue(const Audio::TAudioControlID nRtpcID, const float fValue, const Audio::TAudioProxyID nAudioProxyLocalID /* = DEFAULT_AUDIO_PROXY_ID */)
{
    if (nAudioProxyLocalID != INVALID_AUDIO_PROXY_ID)
    {
        const TAudioProxyPair& rAudioProxyPair = GetAuxAudioProxyPair(nAudioProxyLocalID);

        if (rAudioProxyPair.first != INVALID_AUDIO_PROXY_ID)
        {
            (SSetRtpcValue(nRtpcID, fValue))(rAudioProxyPair);
        }
    }
    else
    {
        std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SSetRtpcValue(nRtpcID, fValue));
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::ResetRtpcValues(const Audio::TAudioProxyID nAudioProxyLocalID /* = DEFAULT_AUDIO_PROXY_ID */)
{
    if (nAudioProxyLocalID != INVALID_AUDIO_PROXY_ID)
    {
        const TAudioProxyPair& rAudioProxyPair = GetAuxAudioProxyPair(nAudioProxyLocalID);

        if (rAudioProxyPair.first != INVALID_AUDIO_PROXY_ID)
        {
            SResetRtpcValues resetFunctor;
            (resetFunctor)(rAudioProxyPair);
        }
    }
    else
    {
        std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SResetRtpcValues());
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::SetObstructionCalcType(const Audio::EAudioObjectObstructionCalcType eObstructionType, const Audio::TAudioProxyID nAudioProxyLocalID /* = DEFAULT_AUDIO_PROXY_ID */)
{
    if (nAudioProxyLocalID != INVALID_AUDIO_PROXY_ID)
    {
        const TAudioProxyPair& rAudioProxyPair = GetAuxAudioProxyPair(nAudioProxyLocalID);

        if (rAudioProxyPair.first != INVALID_AUDIO_PROXY_ID)
        {
            (SSetObstructionCalcType(eObstructionType))(rAudioProxyPair);
        }
    }
    else
    {
        std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SSetObstructionCalcType(eObstructionType));
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::SetEnvironmentAmount(const Audio::TAudioEnvironmentID nEnvironmentID, const float fAmount, const Audio::TAudioProxyID nAudioProxyLocalID /* = DEFAULT_AUDIO_PROXY_ID */)
{
    if (nAudioProxyLocalID != INVALID_AUDIO_PROXY_ID)
    {
        const TAudioProxyPair& rAudioProxyPair = GetAuxAudioProxyPair(nAudioProxyLocalID);

        if (rAudioProxyPair.first != INVALID_AUDIO_PROXY_ID)
        {
            SSetEnvironmentAmount(nEnvironmentID, fAmount)(rAudioProxyPair);
        }
    }
    else
    {
        std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SSetEnvironmentAmount(nEnvironmentID, fAmount));
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::SetCurrentEnvironments(const Audio::TAudioProxyID nAudioProxyLocalID /* = DEFAULT_AUDIO_PROXY_ID */)
{
    if (nAudioProxyLocalID != INVALID_AUDIO_PROXY_ID)
    {
        const TAudioProxyPair& rAudioProxyPair = GetAuxAudioProxyPair(nAudioProxyLocalID);

        if (rAudioProxyPair.first != INVALID_AUDIO_PROXY_ID)
        {
            SSetCurrentEnvironments(m_pEntity->GetId())(rAudioProxyPair);
        }
    }
    else
    {
        std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SSetCurrentEnvironments(m_pEntity->GetId()));
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::AuxAudioProxiesMoveWithEntity(const bool bCanMoveWithEntity, const Audio::TAudioProxyID nAudioProxyLocalID)
{
    if (nAudioProxyLocalID != INVALID_AUDIO_PROXY_ID)
    {
        TAudioProxyPair& rAudioProxyPair = GetAuxAudioProxyPair(nAudioProxyLocalID);

        if (rAudioProxyPair.first != INVALID_AUDIO_PROXY_ID)
        {
            SSetFlag(eEACF_CAN_MOVE_WITH_ENTITY, bCanMoveWithEntity)(rAudioProxyPair);
        }
    }
    else
    {
        std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SSetFlag(eEACF_CAN_MOVE_WITH_ENTITY, bCanMoveWithEntity));
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::AddAsListenerToAuxAudioProxy(const Audio::TAudioProxyID nAudioProxyLocalID, Audio::AudioRequestCallbackType func, Audio::EAudioRequestType requestType /* = eART_AUDIO_ALL_REQUESTS */, Audio::TATLEnumFlagsType specificRequestMask /* = ALL_AUDIO_REQUEST_SPECIFIC_TYPE_FLAGS */)
{
    const TAuxAudioProxies::const_iterator Iter(m_mapAuxAudioProxies.find(nAudioProxyLocalID));

    if (Iter != m_mapAuxAudioProxies.end())
    {
        Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::AddRequestListener,
            func,
            Iter->second.pIAudioProxy,
            requestType,
            specificRequestMask);
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::RemoveAsListenerFromAuxAudioProxy(const Audio::TAudioProxyID nAudioProxyLocalID, Audio::AudioRequestCallbackType func)
{
    const TAuxAudioProxies::const_iterator Iter(m_mapAuxAudioProxies.find(nAudioProxyLocalID));

    if (Iter != m_mapAuxAudioProxies.end())
    {
        Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::RemoveRequestListener, func, Iter->second.pIAudioProxy);
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::StopAllTriggers()
{
    std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SStopAllTriggers());
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::SetAuxAudioProxyOffset(const Audio::SATLWorldPosition& rOffset, const Audio::TAudioProxyID nAudioProxyLocalID /* = DEFAULT_AUDIO_PROXY_ID */)
{
    if (nAudioProxyLocalID != INVALID_AUDIO_PROXY_ID)
    {
        TAudioProxyPair& rAudioProxyPair = GetAuxAudioProxyPair(nAudioProxyLocalID);

        if (rAudioProxyPair.first != INVALID_AUDIO_PROXY_ID)
        {
            SSetAuxAudioProxyOffset(rOffset, m_pEntity->GetWorldTM())(rAudioProxyPair);
        }
    }
    else
    {
        std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SSetAuxAudioProxyOffset(rOffset, m_pEntity->GetWorldTM()));
    }
}

//////////////////////////////////////////////////////////////////////////
const Audio::SATLWorldPosition& CComponentAudio::GetAuxAudioProxyOffset(const Audio::TAudioProxyID nAudioProxyLocalID /* = DEFAULT_AUDIO_PROXY_ID */)
{
    const TAuxAudioProxies::const_iterator Iter(m_mapAuxAudioProxies.find(nAudioProxyLocalID));

    if (Iter != m_mapAuxAudioProxies.end())
    {
        return Iter->second.oOffset;
    }

    return s_oNULLOffset;
}

//////////////////////////////////////////////////////////////////////////
Audio::TAudioProxyID CComponentAudio::CreateAuxAudioProxy()
{
    Audio::TAudioProxyID nAudioProxyLocalID = INVALID_AUDIO_PROXY_ID;
    Audio::IAudioProxy* pIAudioProxy = nullptr;
    Audio::AudioSystemRequestBus::BroadcastResult(pIAudioProxy, &Audio::AudioSystemRequestBus::Events::GetFreeAudioProxy);

    if (pIAudioProxy)
    {
    #if defined(INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE)
        if (m_nAudioProxyIDCounter == std::numeric_limits<Audio::TAudioProxyID>::max())
        {
            CryFatalError("<Audio> Exceeded numerical limits during CComponentAudio::CreateAudioProxy!");
        }
        else if (!m_pEntity)
        {
            CryFatalError("<Audio> NULL entity pointer during CComponentAudio::CreateAudioProxy!");
        }

        CryFixedStringT<MAX_AUDIO_OBJECT_NAME_LENGTH> sFinalName(m_pEntity->GetName());
        const size_t nNumAuxAudioProxies = m_mapAuxAudioProxies.size();

        if (nNumAuxAudioProxies > 0)
        {
            // First AuxAudioProxy is not explicitly identified, it keeps the entity's name.
            // All additionally AuxaudioProxies however are being explicitly identified.
            sFinalName.Format("%s_auxaudioproxy_#%" PRISIZE_T, m_pEntity->GetName(), nNumAuxAudioProxies + 1);
        }

        pIAudioProxy->Initialize(sFinalName.c_str());
    #else
        pIAudioProxy->Initialize(nullptr);
    #endif // INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE

        pIAudioProxy->SetPosition(m_pEntity->GetWorldPos());
        pIAudioProxy->SetObstructionCalcType(Audio::eAOOCT_IGNORE);
        pIAudioProxy->SetCurrentEnvironments(m_pEntity->GetId());

        m_mapAuxAudioProxies.insert(TAudioProxyPair(++m_nAudioProxyIDCounter, SAudioProxyWrapper(pIAudioProxy)));
        nAudioProxyLocalID = m_nAudioProxyIDCounter;
    }

    return nAudioProxyLocalID;
}

//////////////////////////////////////////////////////////////////////////
bool CComponentAudio::RemoveAuxAudioProxy(const Audio::TAudioProxyID nAudioProxyLocalID)
{
    bool bSuccess = false;

    if (nAudioProxyLocalID != DEFAULT_AUDIO_PROXY_ID)
    {
        const TAuxAudioProxies::const_iterator Iter(m_mapAuxAudioProxies.find(nAudioProxyLocalID));

        if (Iter != m_mapAuxAudioProxies.end())
        {
            Iter->second.pIAudioProxy->Release();
            m_mapAuxAudioProxies.erase(Iter);
            bSuccess = true;
        }
        else
        {
            gEnv->pSystem->Warning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "<Audio> AuxAudioProxy with ID '%u' not found during CComponentAudio::RemoveAuxAudioProxy (%s)!", nAudioProxyLocalID, m_pEntity->GetEntityTextDescription());
            assert(false);
        }
    }
    else
    {
        gEnv->pSystem->Warning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_ERROR, VALIDATOR_FLAG_AUDIO, 0, "<Audio> Trying to remove the default AudioProxy during CComponentAudio::RemoveAuxAudioProxy (%s)!", m_pEntity->GetEntityTextDescription());
        assert(false);
    }

    return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
CComponentAudio::TAudioProxyPair& CComponentAudio::GetAuxAudioProxyPair(const Audio::TAudioProxyID nAudioProxyLocalID)
{
    const TAuxAudioProxies::iterator Iter(m_mapAuxAudioProxies.find(nAudioProxyLocalID));

    if (Iter != m_mapAuxAudioProxies.end())
    {
        return *Iter;
    }

    return s_oNULLAudioProxyPair;
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::SetEnvironmentAmountInternal(const IEntity* const pEntity, const float fAmount) const
{
    IComponentAudio* const pAudioComponent = const_cast<IComponentAudio*>(pEntity->GetComponent<IComponentAudio>().get());

    if (pAudioComponent && (m_nAudioEnvironmentID != INVALID_AUDIO_ENVIRONMENT_ID))
    {
        // Only set the audio-environment-amount on the entities that already have an AudioProxy.
        // Passing INVALID_AUDIO_PROXY_ID to address all auxiliary AudioProxies on pAudioComponent.
        pAudioComponent->SetEnvironmentAmount(m_nAudioEnvironmentID, fAmount, INVALID_AUDIO_PROXY_ID);
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentAudio::UpdateHideStatus()
{
    if (m_pEntity->IsHidden())
    {
        m_nFlags |= eEACF_HIDDEN;
    }
    else
    {
        m_nFlags &= ~eEACF_HIDDEN;
    }
}

///////////////////////////////////////////////////////////////////////////
void CComponentAudio::Done()
{
    StopAllTriggers();
    std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SResetAudioProxy());
    m_pEntity = nullptr;
}

