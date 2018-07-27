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

#ifndef CRYINCLUDE_CRYACTION_GAMEOBJECTS_INTERACTOR_H
#define CRYINCLUDE_CRYACTION_GAMEOBJECTS_INTERACTOR_H
#pragma once

#include "IGameObject.h"
#include "IInteractor.h"

class CWorldQuery;
struct IEntitySystem;

class CInteractor
    : public CGameObjectExtensionHelper<CInteractor, IInteractor>
{
public:
    CInteractor();
    ~CInteractor();

    DECLARE_COMPONENT_TYPE("CInteractor", 0x7A153143FB33464F, 0x8611DE650AEDD9F0)
    // IGameObjectExtension
    virtual bool Init(IGameObject* pGameObject);
    virtual void InitClient(ChannelId channelId) {};
    virtual void PostInit(IGameObject* pGameObject);
    virtual void PostInitClient(ChannelId channelId) {};
    virtual bool ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params);
    virtual void PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) {}
    virtual bool GetEntityPoolSignature(TSerialize signature);
    virtual void FullSerialize(TSerialize ser);
    virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags);
    virtual void PostSerialize();
    virtual void SerializeSpawnInfo(TSerialize ser, IEntity* entity) {}
    virtual ISerializableInfoPtr GetSpawnInfo() {return 0; }
    virtual void Update(SEntityUpdateContext& ctx, int slot);
    virtual void HandleEvent(const SGameObjectEvent&);
    virtual void ProcessEvent(SEntityEvent&) {};
    virtual void SetChannelId(ChannelId id) {};
    virtual void SetAuthority(bool auth) {}
    virtual void PostUpdate(float frameTime) { CRY_ASSERT(false); }
    virtual void PostRemoteSpawn() {};
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    // ~IGameObjectExtension


    // IInteractor
    virtual bool IsUsable(EntityId entityId) const;
    virtual bool IsLocked() const { return m_lockEntityId != 0; };
    virtual int GetLockIdx() const { return m_lockIdx; };
    virtual int GetLockedEntityId() const { return m_lockEntityId; };
    virtual void SetQueryMethods(char* pMethods){ m_queryMethods = pMethods; };
    virtual int GetOverEntityId() const { return m_overId; };
    virtual int GetOverSlotIdx() const { return m_overIdx; };
    //~IInteractor

private:

    struct SQueryResult
    {
        SQueryResult()
            : entityId(0)
            , slotIdx(0)
            , hitPosition(0, 0, 0)
            , hitDistance(0) {}
        EntityId entityId;
        int slotIdx;
        Vec3 hitPosition;
        float hitDistance;
    };

    typedef std::pair<IEntity*, SQueryResult> TQueryElement;
    typedef std::vector<TQueryElement> TQueryVector;
    typedef std::map<IEntityClass*, bool>   TUsableClassesMap;

    CWorldQuery* m_pQuery;

    float m_useHoverTime;
    float m_unUseHoverTime;
    float m_messageHoverTime;
    float m_longHoverTime;

    ITimer* m_pTimer;
    IEntitySystem* m_pEntitySystem;

    string m_queryMethods;

    SmartScriptTable m_pGameRules;
    HSCRIPTFUNCTION m_funcIsUsable;
    HSCRIPTFUNCTION m_funcOnNewUsable;
    HSCRIPTFUNCTION m_funcOnUsableMessage;
    HSCRIPTFUNCTION m_funcOnLongHover;

    SmartScriptTable m_areUsableEntityList;
    HSCRIPTFUNCTION m_funcAreUsable;
    TQueryVector m_frameQueryVector;
    TUsableClassesMap m_usableEntityClasses;

    EntityId m_lockedByEntityId;
    EntityId m_lockEntityId;
    int m_lockIdx;

    EntityId m_nextOverId;
    int m_nextOverIdx;
    CTimeValue m_nextOverTime;
    EntityId m_overId;
    int m_overIdx;
    CTimeValue m_overTime;
    bool m_sentMessageHover;
    bool m_sentLongHover;

    float m_lastRadius;

    static ScriptAnyValue EntityIdToScript(EntityId id);
    void PerformQueries(EntityId& id, int& idx);
    void UpdateTimers(EntityId id, int idx);

    bool PerformRayCastQuery(SQueryResult& r);
    bool PerformViewCenterQuery(SQueryResult& r);
    bool PerformDotFilteredProximityQuery(SQueryResult& r, float minDot);
    bool PerformMergedQuery(SQueryResult& r, float minDot);
    bool PerformUsableTestAndCompleteIds(IEntity* pEntity, SQueryResult& r) const;
    int PerformUsableTest(IEntity* pEntity) const;
    void PerformUsableTestOnEntities (TQueryVector& query);
    float LinePointDistanceSqr(const Line& line, const Vec3& point);

    bool IsEntityUsable(const IEntity* pEntity);
};

#endif // CRYINCLUDE_CRYACTION_GAMEOBJECTS_INTERACTOR_H
