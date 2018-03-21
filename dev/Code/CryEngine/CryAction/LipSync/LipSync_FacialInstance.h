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


#ifndef CRYINCLUDE_CRYACTION_LIPSYNC_LIPSYNC_FACIALINSTANCE_H
#define CRYINCLUDE_CRYACTION_LIPSYNC_LIPSYNC_FACIALINSTANCE_H
#pragma once


class CLipSyncProvider_FacialInstance
    : public ILipSyncProvider
{
public:
    explicit CLipSyncProvider_FacialInstance(EntityId entityId);

    // ILipSyncProvider
    virtual void RequestLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) override;
    virtual void StartLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) override;
    virtual void PauseLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) override;
    virtual void UnpauseLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) override;
    virtual void StopLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) override;
    virtual void UpdateLipSync(IComponentAudioPtr pAudioComponent, const Audio::TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) override;
    // ~ILipSyncProvider

    void FullSerialize(TSerialize ser);
    void GetEntityPoolSignature(TSerialize signature);

private:
    void LipSyncWithSound(const Audio::TAudioControlID nAudioTriggerId, bool bStop = false);
    EntityId m_entityId;
};
DECLARE_SMART_POINTERS(CLipSyncProvider_FacialInstance);


class CLipSync_FacialInstance
    : public CGameObjectExtensionHelper<CLipSync_FacialInstance, IGameObjectExtension>
{
public:
    DECLARE_COMPONENT_TYPE("LipSync_FacialInstance", 0x7CE29F49143D4B19, 0x8017031B3E70AD0D)

    CLipSync_FacialInstance() {}
    ~CLipSync_FacialInstance() override;

    // IGameObjectExtension
    virtual void GetMemoryUsage(ICrySizer* pSizer) const override;
    virtual bool Init(IGameObject* pGameObject) override;
    virtual void PostInit(IGameObject* pGameObject) override;
    virtual void InitClient(ChannelId channelId) override;
    virtual void PostInitClient(ChannelId channelId) override;
    virtual bool ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override;
    virtual void PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override;
    virtual bool GetEntityPoolSignature(TSerialize signature) override;
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
    CLipSyncProvider_FacialInstancePtr m_pLipSyncProvider;
};

#endif // CRYINCLUDE_CRYACTION_LIPSYNC_LIPSYNC_FACIALINSTANCE_H
