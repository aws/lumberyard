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
#ifndef CRYINCLUDE_GAMEDLL_ENVIRONMENT_SNOW_H
#define CRYINCLUDE_GAMEDLL_ENVIRONMENT_SNOW_H
#pragma once

#include <IGameObject.h>

class CSnow
    : public CGameObjectExtensionHelper<CSnow, IGameObjectExtension>
{
public:

    DECLARE_COMPONENT_TYPE("ComponentSnow", 0x5449E14CA41C42A9, 0x92DFBD2544E28392);

    CSnow();
    virtual ~CSnow();

    // IGameObjectExtension
    bool Init(IGameObject* pGameObject) override;
    void InitClient(ChannelId channelId) override { }
    void PostInit(IGameObject* pGameObject) override;
    void PostInitClient(ChannelId channelId) override { }
    bool ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override;
    void PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override { }
    bool GetEntityPoolSignature(TSerialize signature) override;
    bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags) override { return true; }
    void FullSerialize(TSerialize ser) override;
    void PostSerialize() override { }
    void SerializeSpawnInfo(TSerialize, IEntity*) override { }
    ISerializableInfoPtr GetSpawnInfo() override { return 0; }
    void Update(SEntityUpdateContext& ctx, int updateSlot) override;
    void PostUpdate(float frameTime) override {}
    void PostRemoteSpawn() override {}
    void HandleEvent(const SGameObjectEvent&) override;
    void ProcessEvent(SEntityEvent&) override;
    void SetChannelId(ChannelId id) override { }
    void SetAuthority(bool auth) override;
    void GetMemoryUsage(ICrySizer* pSizer) const override { pSizer->Add(*this); }

    //~IGameObjectExtension

    bool Reset();

protected:

    bool    m_bEnabled;
    float   m_fRadius;

    // Surface params.
    float   m_fSnowAmount;
    float   m_fFrostAmount;
    float   m_fSurfaceFreezing;

    // Snowfall params.
    int     m_nSnowFlakeCount;
    float   m_fSnowFlakeSize;
    float   m_fSnowFallBrightness;
    float   m_fSnowFallGravityScale;
    float   m_fSnowFallWindScale;
    float   m_fSnowFallTurbulence;
    float   m_fSnowFallTurbulenceFreq;

private:
    CSnow(const CSnow&);
    CSnow& operator = (const CSnow&);
};

// Extracted from macro REGISTER_GAME_OBJECT();
struct CSnowCreator
    : public IGameObjectExtensionCreatorBase
{
    IGameObjectExtensionPtr Create()
    {
        return gEnv->pEntitySystem->CreateComponent<CSnow>();
    }

    void GetGameObjectExtensionRMIData(void** ppRMI, size_t* nCount)
    {
        CSnow::GetGameObjectExtensionRMIData(ppRMI, nCount);
    }
};

#endif // CRYINCLUDE_GAMEDLL_ENVIRONMENT_SNOW_H
