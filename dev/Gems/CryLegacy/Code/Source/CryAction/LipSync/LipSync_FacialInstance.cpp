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

// Description : Automatic start of facial animation when a sound is being played back.
//               Legacy version that uses CryAnimation s FacialInstance


#include "CryLegacy_precompiled.h"

#if 0 // TODO-MERGE-RESOLVE: 20141022 - Disabling lip sync items
#include "LipSync_FacialInstance.h"

DECLARE_DEFAULT_COMPONENT_FACTORY(CLipSync_FacialInstance, CLipSync_FacialInstance)

//=============================================================================
//
// CLipSyncProvider_FacialInstance
//
//=============================================================================

CLipSyncProvider_FacialInstance::CLipSyncProvider_FacialInstance(EntityId entityId)
    : m_entityId(entityId)
{
}

void CLipSyncProvider_FacialInstance::RequestLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod)
{
    // actually facial sequence is triggered in OnSoundEvent SOUND_EVENT_ON_PLAYBACK_STARTED of the CSoundProxy
    // when playback is started, it will start facial sequence as well
}

void CLipSyncProvider_FacialInstance::StartLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod)
{
    if (lipSyncMethod != eLSM_None)
    {
        LipSyncWithSound(nAudioTriggerId);
    }
}

void CLipSyncProvider_FacialInstance::PauseLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod)
{
    if (lipSyncMethod != eLSM_None)
    {
        LipSyncWithSound(nAudioTriggerId, true);
    }
}

void CLipSyncProvider_FacialInstance::UnpauseLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod)
{
    if (lipSyncMethod != eLSM_None)
    {
        LipSyncWithSound(nAudioTriggerId);
    }
}

void CLipSyncProvider_FacialInstance::StopLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod)
{
    if (lipSyncMethod != eLSM_None)
    {
        LipSyncWithSound(nAudioTriggerId, true);
    }
}

void CLipSyncProvider_FacialInstance::UpdateLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod)
{
}

void CLipSyncProvider_FacialInstance::FullSerialize(TSerialize ser)
{
    ser.BeginGroup("LipSyncProvider_FacialInstance");
    ser.EndGroup();
}

void CLipSyncProvider_FacialInstance::GetEntityPoolSignature(TSerialize signature)
{
    signature.BeginGroup("LipSyncProvider_FacialInstance");
    signature.EndGroup();
}

void CLipSyncProvider_FacialInstance::LipSyncWithSound(const Audio::TAudioControlID nAudioTriggerId, bool bStop /*= false*/)
{
    if (IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_entityId))
    {
        if (ICharacterInstance* pChar = pEntity->GetCharacter(0))
        {
            pChar->LipSyncWithSound(nAudioTriggerId, bStop);
        }
    }
}


//=============================================================================
//
// CLipSync_FacialInstance
//
//=============================================================================

void CLipSync_FacialInstance::InjectLipSyncProvider()
{
    IEntity* pEntity = GetEntity();
    IComponentAudioPtr pAudioComponent = pEntity->GetOrCreateComponent<IComponentAudio>();
    CRY_ASSERT(pAudioComponent);
    m_pLipSyncProvider.reset(new CLipSyncProvider_FacialInstance(pEntity->GetId()));
    // Audio: add SetLipSyncProvider to interface
    //pAudioComponent->SetLipSyncProvider(m_pLipSyncProvider);
}

void CLipSync_FacialInstance::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->Add(*this);
    if (m_pLipSyncProvider)
    {
        pSizer->Add(*m_pLipSyncProvider);
    }
}

bool CLipSync_FacialInstance::Init(IGameObject* pGameObject)
{
    SetGameObject(pGameObject);
    return true;
}

void CLipSync_FacialInstance::PostInit(IGameObject* pGameObject)
{
    InjectLipSyncProvider();
}

void CLipSync_FacialInstance::InitClient(ChannelId channelId)
{
}

void CLipSync_FacialInstance::PostInitClient(ChannelId channelId)
{
}

bool CLipSync_FacialInstance::ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params)
{
    ResetGameObject();
    return true;
}

void CLipSync_FacialInstance::PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params)
{
    InjectLipSyncProvider();
}

bool CLipSync_FacialInstance::GetEntityPoolSignature(TSerialize signature)
{
    signature.BeginGroup("LipSync_FacialInstance");
    if (m_pLipSyncProvider)
    {
        m_pLipSyncProvider->GetEntityPoolSignature(signature);
    }
    signature.EndGroup();
    return true;
}

void CLipSync_FacialInstance::~CLipSync_FacialInstance()
{
    IEntity* pEntity = GetEntity();
    if (IComponentAudioPtr pAudioComponent = pEntity->GetComponent<IComponentAudio>())
    {
        // Audio: add SetLipSyncProvider to interface
        //pSoundComponent->SetLipSyncProvider(ILipSyncProviderPtr());
    }
}

void CLipSync_FacialInstance::FullSerialize(TSerialize ser)
{
    ser.BeginGroup("LipSync_FacialInstance");

    bool bLipSyncProviderIsInjected = (m_pLipSyncProvider != NULL);
    ser.Value("bLipSyncProviderIsInjected", bLipSyncProviderIsInjected);
    if (bLipSyncProviderIsInjected && !m_pLipSyncProvider)
    {
        CRY_ASSERT(ser.IsReading());
        InjectLipSyncProvider();
    }
    if (m_pLipSyncProvider)
    {
        m_pLipSyncProvider->FullSerialize(ser);
    }

    ser.EndGroup();
}

bool CLipSync_FacialInstance::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags)
{
    return true;
}

void CLipSync_FacialInstance::PostSerialize()
{
}

void CLipSync_FacialInstance::SerializeSpawnInfo(TSerialize ser, IEntity* entity)
{
}

ISerializableInfoPtr CLipSync_FacialInstance::GetSpawnInfo()
{
    return NULL;
}

void CLipSync_FacialInstance::Update(SEntityUpdateContext& ctx, int updateSlot)
{
}

void CLipSync_FacialInstance::HandleEvent(const SGameObjectEvent& event)
{
}

void CLipSync_FacialInstance::ProcessEvent(SEntityEvent& event)
{
}

void CLipSync_FacialInstance::SetChannelId(uint16 id)
{
}

void CLipSync_FacialInstance::SetAuthority(bool auth)
{
}

void CLipSync_FacialInstance::PostUpdate(float frameTime)
{
}

void CLipSync_FacialInstance::PostRemoteSpawn()
{
}

#endif // #if 0