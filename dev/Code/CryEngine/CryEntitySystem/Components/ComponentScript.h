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
#ifndef CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTSCRIPT_H
#define CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTSCRIPT_H
#pragma once

// forward declarations.
class CEntityScript;
struct IScriptTable;
struct SScriptState;

#include "EntityScript.h"

//! Implementation of the script component.
//! This component handles interaction between the entity and its script.
class CComponentScript
    : public IComponentScript
{
public:
    CComponentScript();
    ~CComponentScript() override;

    struct SComponentInitializerScript
        : public SComponentInitializer
    {
        CEntityScript* m_pScript;
        SEntitySpawnParams* m_pSpawnParams;
        SComponentInitializerScript(IEntity* pEntity, CEntityScript* pScript = NULL, SEntitySpawnParams* pSpawnParams  = NULL)
            : SComponentInitializer(pEntity)
            , m_pScript(pScript)
            , m_pSpawnParams(pSpawnParams)
        {}
    };

    virtual void Initialize(const SComponentInitializer& init);

    //////////////////////////////////////////////////////////////////////////
    // IComponent implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void ProcessEvent(SEntityEvent& event);
    virtual bool InitComponent(IEntity* pEntity, SEntitySpawnParams& params);
    virtual void Reload(IEntity* pEntity, SEntitySpawnParams& params);
    virtual void Done();
    virtual void UpdateComponent(SEntityUpdateContext& ctx);
    virtual void SerializeXML(XmlNodeRef& entityNode, bool bLoading);
    virtual void Serialize(TSerialize ser);
    virtual bool NeedSerialize();
    virtual bool GetSignature(TSerialize signature);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // IComponentScript implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void SetScriptUpdateRate(float fUpdateEveryNSeconds) { m_fScriptUpdateRate = fUpdateEveryNSeconds; };
    virtual IScriptTable* GetScriptTable() const { return m_pThis; }

    virtual void CallEvent(const char* sEvent);
    virtual void CallEvent(const char* sEvent, float fValue);
    virtual void CallEvent(const char* sEvent, double fValue);
    virtual void CallEvent(const char* sEvent, bool bValue);
    virtual void CallEvent(const char* sEvent, const char* sValue);
    virtual void CallEvent(const char* sEvent, EntityId nEntityId);
    virtual void CallEvent(const char* sEvent, const Vec3& vValue);

    void CallInitEvent(bool bFromReload) override;

    virtual void SendScriptEvent(int Event, IScriptTable* pParamters, bool* pRet = NULL);
    virtual void SendScriptEvent(int Event, const char* str, bool* pRet = NULL);
    virtual void SendScriptEvent(int Event, int nParam, bool* pRet = NULL);

    virtual void ChangeScript(IEntityScript* pScript, SEntitySpawnParams* params);
    //////////////////////////////////////////////////////////////////////////

    virtual void OnCollision(IEntity* pTarget, int matId, const Vec3& pt, const Vec3& n, const Vec3& vel, const Vec3& targetVel, int partId, float mass) override;
    void OnPreparedFromPool() override;

    //////////////////////////////////////////////////////////////////////////
    // State Management public interface.
    //////////////////////////////////////////////////////////////////////////
    virtual bool GotoState(const char* sStateName);
    virtual bool GotoStateId(int nState) { return GotoState(nState); };
    bool GotoState(int nState);
    bool IsInState(const char* sStateName);
    bool IsInState(int nState);
    const char* GetState();
    int GetStateId();
    void RegisterForAreaEvents(bool bEnable) override;
    bool IsRegisteredForAreaEvents() const;

    void SerializeProperties(TSerialize ser) override;

    virtual void GetMemoryUsage(ICrySizer* pSizer) const;

private:
    SScriptState* CurrentState() { return m_pScript->GetState(m_nCurrStateId); }
    void CreateScriptTable(SEntitySpawnParams* pSpawnParams);
    void SetEventTargets(XmlNodeRef& eventTargets);
    IScriptSystem* GetIScriptSystem() const { return gEnv->pScriptSystem; }

    void SerializeTable(TSerialize ser, const char* name);
    bool HaveTable(const char* name);

private:
    IEntity* m_pEntity;
    CEntityScript* m_pScript;
    IScriptTable* m_pThis;

    float m_fScriptUpdateTimer;
    float m_fScriptUpdateRate;

    // Cache Tables.
    SmartScriptTable m_hitTable;

    uint32 m_nCurrStateId  : 8;
    uint32 m_bUpdateScript : 1;
    bool m_bEnableSoundAreaEvents : 1;
};


#endif // CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTSCRIPT_H