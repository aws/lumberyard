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


#ifndef CRYINCLUDE_CRYACTION_LIPSYNC_LIPSYNC_TRANSITIONQUEUE_H
#define CRYINCLUDE_CRYACTION_LIPSYNC_LIPSYNC_TRANSITIONQUEUE_H
#pragma once

#if 0 // TODO-MERGE-RESOLVE: 20141022 - Disabling lip sync items



/// The CLipSyncProvider_TransitionQueue communicates with sound proxy to play synchronized facial animation directly on the transition queue
class CLipSyncProvider_TransitionQueue
    : public ILipSyncProvider
{
public:
    explicit CLipSyncProvider_TransitionQueue(EntityId entityId);

    // ILipSyncProvider
    void RequestLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) override;
    void StartLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) override;
    void PauseLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) override;
    void UnpauseLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) override;
    void StopLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) override;
    void UpdateLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) override;
    // ~ILipSyncProvider

    void FullSerialize(TSerialize ser);
    void GetEntityPoolSignature(TSerialize signature);

private:
    IEntity* GetEntity();
    ICharacterInstance* GetCharacterInstance();
    void FindMatchingAnim(const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod, ICharacterInstance& character, int* pAnimIdOut, CryCharAnimationParams* pAnimParamsOut) const;
    void FillCharAnimationParams(const bool isDefaultAnim, CryCharAnimationParams* pParams) const;
    void SynchronizeAnimationToSound(const Audio::TAudioControlID nAudioTriggerId);

private:
    enum EState
    {
        eS_Init,
        eS_Requested,
        eS_Started,
        eS_Paused,
        eS_Unpaused,
        eS_Stopped,
    };

    static uint32 s_lastAnimationToken;

    EntityId m_entityId;
    int m_nCharacterSlot;           // the lip-sync animation will be played back on the character residing in this slot of the entity
    int m_nAnimLayer;               // the lip-sync animation will be played back on this layer of the character
    string m_sDefaultAnimName;      // fallback animation to play if there is no animation matching the currently playing sound in the character's database

    EState m_state;
    bool m_isSynchronized;

    // Filled when request comes in:
    int m_requestedAnimId;
    CryCharAnimationParams m_requestedAnimParams;
    CAutoResourceCache_CAF m_cachedAnim;

    // Filled when animation is started:
    uint32 m_nCurrentAnimationToken;
    Audio::TAudioControlID m_soundId;
};
DECLARE_SMART_POINTERS(CLipSyncProvider_TransitionQueue);


class CLipSync_TransitionQueue
    : public CGameObjectExtensionHelper<CLipSync_TransitionQueue, IGameObjectExtension>
{
public:
    DECLARE_COMPONENT_TYPE("LipSync_TransitionQueue", 0xE79923D9F65E45EF, 0xA3BC0B470F7D43A6)

    CLipSync_TransitionQueue() {}

    // IGameObjectExtension
    virtual void GetMemoryUsage(ICrySizer* pSizer) const override;
    virtual bool Init(IGameObject* pGameObject) override;
    virtual void PostInit(IGameObject* pGameObject) override;
    virtual void InitClient(ChannelId channelId) override;
    virtual void PostInitClient(ChannelId channelId) override;
    virtual bool ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override;
    virtual void PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override;
    virtual bool GetEntityPoolSignature(TSerialize signature) override;
    virtual void Release() override;
    virtual void FullSerialize(TSerialize ser) override;
    virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags) override;
    virtual void PostSerialize() override;
    virtual void SerializeSpawnInfo(TSerialize ser, IEntity* entity) override;
    virtual ISerializableInfoPtr GetSpawnInfo() override;
    virtual void Update(SEntityUpdateContext& ctx, int updateSlot) override;
    virtual void HandleEvent(const SGameObjectEvent& event) override;
    virtual void ProcessEvent(SEntityEvent& event) override;
    virtual void SetChannelId(ChannelId id) override;
    virtual void SetAuthority(bool auth) override;
    virtual void PostUpdate(float frameTime) override;
    virtual void PostRemoteSpawn() override;
    // ~IGameObjectExtension

private:
    void InjectLipSyncProvider();

private:
    CLipSyncProvider_TransitionQueuePtr m_pLipSyncProvider;
};

#endif // #if false
#endif // CRYINCLUDE_CRYACTION_LIPSYNC_LIPSYNC_TRANSITIONQUEUE_H
