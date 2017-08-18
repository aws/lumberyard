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
#ifndef CRYINCLUDE_GAMEDLL_ENVIRONMENT_TORNADO_H
#define CRYINCLUDE_GAMEDLL_ENVIRONMENT_TORNADO_H
#pragma once

#include <IGameObject.h>

class CFlowTornadoWander;
struct IGroundEffect;

class CTornado
    : public CGameObjectExtensionHelper<CTornado, IGameObjectExtension>
{
public:

    DECLARE_COMPONENT_TYPE("ComponentTornado", 0xD2FF944F8D9C4DDB, 0xB145E856D7935A0B);

    CTornado();
    virtual ~CTornado();

    // IGameObjectExtension
    bool Init(IGameObject* pGameObject) override;
    void InitClient(ChannelId channelId) override {}
    void PostInit(IGameObject* pGameObject) override;
    void PostInitClient(ChannelId channelId) override {}
    bool ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override;
    void PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override { }
    bool GetEntityPoolSignature(TSerialize signature) override;
    void FullSerialize(TSerialize ser) override;
    bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags) override;
    void PostSerialize() override { }
    void SerializeSpawnInfo(TSerialize ser, IEntity*) override { }
    ISerializableInfoPtr GetSpawnInfo() override { return 0; }
    void Update(SEntityUpdateContext& ctx, int updateSlot) override;
    void PostUpdate(float frameTime) override {}
    void PostRemoteSpawn() override {}
    void HandleEvent(const SGameObjectEvent&) override;
    void ProcessEvent(SEntityEvent&) override;
    void SetChannelId(ChannelId id) override { }
    void SetAuthority(bool auth) override;
    void GetMemoryUsage(ICrySizer* pSizer) const override;
    //~IGameObjectExtension

    void    SetTarget(IEntity* pTargetEntity, CFlowTornadoWander* pCallback);

    bool Reset();

protected:

    bool    UseFunnelEffect(const char* effectName);
    void    UpdateParticleEmitters();
    void    UpdateTornadoSpline();
    void    UpdateFlow();
    void    OutputDistance();

protected:
    static const NetworkAspectType POSITION_ASPECT = eEA_GameServerStatic;

    IPhysicalEntity*    m_pPhysicalEntity;

    // appearance of tornado
    IParticleEffect*    m_pFunnelEffect;
    IParticleEffect*    m_pCloudConnectEffect;
    IParticleEffect*    m_pTopEffect;
    IGroundEffect*      m_pGroundEffect;
    float                           m_cloudHeight;
    bool                            m_isOnWater;
    bool                            m_isInAir;
    float                           m_radius;
    int                             m_curMatID;

    // spline
    Vec3                            m_points[4];
    Vec3                            m_oldPoints[4];

    // wandering
    Vec3                            m_wanderDir;
    float                           m_wanderSpeed;
    Vec3                            m_currentPos;

    // target
    IEntity*                        m_pTargetEntity;    // the tornado will try to reach this entity
    CFlowTornadoWander* m_pTargetCallback;

    //
    float m_nextEntitiesCheck;

    float m_spinImpulse;
    float m_attractionImpulse;
    float m_upImpulse;

    std::vector<int> m_spinningEnts;
};

// Extracted from macro REGISTER_GAME_OBJECT();
struct CTornadoCreator
    : public IGameObjectExtensionCreatorBase
{
    IGameObjectExtensionPtr Create()
    {
        return gEnv->pEntitySystem->CreateComponent<CTornado>();
    }

    void GetGameObjectExtensionRMIData(void** ppRMI, size_t* nCount)
    {
        CTornado::GetGameObjectExtensionRMIData(ppRMI, nCount);
    }
};

#endif // CRYINCLUDE_GAMEDLL_ENVIRONMENT_TORNADO_H
