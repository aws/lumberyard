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
#ifndef _ENVIRONMENT_LIGHTNINGARC_H_
#define _ENVIRONMENT_LIGHTNINGARC_H_
#pragma once

#include <IGameObject.h>

class CLightningArc
    : public CGameObjectExtensionHelper<CLightningArc, IGameObjectExtension>
{
public:
    DECLARE_COMPONENT_TYPE("LightningArc", 0xF95C65B58590443C, 0xAFEE70FDAFF418D3)

    CLightningArc();
    virtual ~CLightningArc();

    void GetMemoryUsage(ICrySizer* pSizer) const override;
    bool Init(IGameObject* pGameObject) override;
    void PostInit(IGameObject* pGameObject) override;
    void InitClient(ChannelId channelId) override;
    void PostInitClient(ChannelId channelId) override;
    bool ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override;
    void PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override;
    bool GetEntityPoolSignature(TSerialize signature) override;
    void FullSerialize(TSerialize ser) override;
    bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags) override;
    void PostSerialize() override;
    void SerializeSpawnInfo(TSerialize ser, IEntity* entity) override;
    ISerializableInfoPtr GetSpawnInfo() override;
    void Update(SEntityUpdateContext& ctx, int updateSlot) override;
    void HandleEvent(const SGameObjectEvent& event) override;
    void ProcessEvent(SEntityEvent& event) override;
    void SetChannelId(ChannelId id) override;
    void SetAuthority(bool auth) override;
    const void* GetRMIBase() const override;
    void PostUpdate(float frameTime) override;
    void PostRemoteSpawn() override;

    void TriggerSpark();
    void Enable(bool enable);
    void ReadLuaParameters();

private:
    void Reset(bool jumpingIntoGame);

    const char* m_lightningPreset;
    float m_delay;
    float m_delayVariation;
    float m_timer;
    bool m_enabled;
    bool m_inGameMode;
};

// Extracted from macro REGISTER_GAME_OBJECT();
struct CLightningArcCreator
    : public IGameObjectExtensionCreatorBase
{
    IGameObjectExtensionPtr Create()
    {
        return gEnv->pEntitySystem->CreateComponent<CLightningArc>();
    }

    void GetGameObjectExtensionRMIData(void** ppRMI, size_t* nCount)
    {
        CLightningArc::GetGameObjectExtensionRMIData(ppRMI, nCount);
    }
};

#endif//_ENVIRONMENT_LIGHTNINGARC_H_
