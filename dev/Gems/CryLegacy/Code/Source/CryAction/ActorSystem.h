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

// Description : Manages and controls player spawns.


#ifndef CRYINCLUDE_CRYACTION_ACTORSYSTEM_H
#define CRYINCLUDE_CRYACTION_ACTORSYSTEM_H
#pragma once

#include "IActorSystem.h"
#include "IGameFramework.h"
#include "ItemParams.h"
#include <IEntityPoolManager.h>

typedef std::map<EntityId, IActor*>        TActorMap;

struct IScriptTable;

class CActorSystem
    : public IActorSystem
    , public IEntityPoolListener
    , public IEntitySystemSink
{
    struct DemoSpectatorSystem
    {
        EntityId                m_originalActor;
        EntityId                m_currentActor;

        void SwitchSpectator(TActorMap* pActors, EntityId id = 0);
    } m_demoSpectatorSystem;

public:
    CActorSystem(ISystem* pSystem, IEntitySystem* pEntitySystem);
    virtual ~CActorSystem();

    void Release() { delete this; };

    virtual void Reset();
    virtual void Reload();

    // IActorSystem
    virtual IActor* GetActor(EntityId entityId);
    virtual IActor* CreateActor(ChannelId channelId, const char* name, const char* actorClass, const Vec3& pos, const Quat& rot, const Vec3& scale, EntityId id, TSerialize* serializer);
    virtual IActor* GetActorByChannelId(ChannelId channelId);

    virtual int GetActorCount() const { return (int)m_actors.size(); };
    virtual IActorIteratorPtr CreateActorIterator();

    virtual void SwitchDemoSpectator(EntityId id = 0);
    virtual IActor* GetCurrentDemoSpectator();
    virtual IActor* GetOriginalDemoSpectator();

    virtual void Scan(const char* folderName);
    virtual const IItemParamsNode* GetActorParams(const char* actorClass) const;

    virtual bool IsActorClass(IEntityClass* pClass) const;

    // ~IActorSystem

    // IEntityPoolListener
    virtual void OnEntityReturnedToPool(EntityId entityId, IEntity* pEntity);
    virtual void OnEntityPreparedFromPool(EntityId entityId, IEntity* pEntity);
    // ~IEntityPoolListener

    // IEntitySystemSink
    virtual bool OnBeforeSpawn(SEntitySpawnParams& params);
    virtual void OnSpawn(IEntity* pEntity, SEntitySpawnParams& params);
    virtual bool OnRemove(IEntity* pEntity);
    virtual void OnReused(IEntity* pEntity, SEntitySpawnParams& params);
    virtual void OnEvent(IEntity* pEntity, SEntityEvent& event);
    virtual void GetMemoryUsage(class ICrySizer* pSizer) const;
    // ~IEntitySystemSink

    void RegisterActorClass(const char* name, IGameFramework::IActorCreator*, bool isAI);
    void AddActor(EntityId entityId, IActor* pProxy);
    void RemoveActor(EntityId entityId);

    void GetMemoryStatistics(ICrySizer* s);

    virtual bool ScanXML(const XmlNodeRef& root, const char* xmlFile);

private:

    static bool HookCreateActor(IEntity*, IGameObject*, void*);

    static void ActorSystemErrorMessage(const char* fileName, const char* errorInfo, bool displayErrorDialog);

    struct SSpawnUserData
    {
        SSpawnUserData(const char* cls, ChannelId channel)
            : className(cls)
            , channelId(channel) {}
        const char* className;
        ChannelId channelId;
    };

    struct SActorUserData
    {
        SActorUserData(const char* cls, CActorSystem* pNewActorSystem)
            : className(cls)
            , pActorSystem(pNewActorSystem) {};
        CActorSystem* pActorSystem;
        string className;
    };

    struct SActorClassDesc
    {
        IEntityClass* pEntityClass;
        IGameFramework::IActorCreator* pCreator;

        SActorClassDesc()
            : pEntityClass()
            , pCreator() {}
    };

    class CActorIterator
        : public IActorIterator
    {
    public:
        CActorIterator(CActorSystem* pAS)
        {
            m_pAS = pAS;
            m_cur = m_pAS->m_actors.begin();
            m_nRefs = 0;
        }
        void AddRef()
        {
            ++m_nRefs;
        }
        void Release()
        {
            if (--m_nRefs <= 0)
            {
                assert (std::find (m_pAS->m_iteratorPool.begin(),
                        m_pAS->m_iteratorPool.end(), this) == m_pAS->m_iteratorPool.end());
                // Call my own destructor before I push to the pool - avoids tripping up the STLP debugging {2008/12/09})
                this->~CActorIterator();
                m_pAS->m_iteratorPool.push_back(this);
            }
        }
        IActor* Next()
        {
            if (m_cur == m_pAS->m_actors.end())
            {
                return 0;
            }
            IActor* pActor = m_cur->second;
            ++m_cur;
            return pActor;
        }
        size_t Count()
        {
            return m_pAS->m_actors.size();
        }
        CActorSystem* m_pAS;
        TActorMap::iterator m_cur;
        int m_nRefs;
    };

    typedef struct SActorParamsDesc
    {
        SActorParamsDesc()
            : params(0) {};
        ~SActorParamsDesc()
        {
            if (params)
            {
                params->Release();
            }
        };

        CItemParamsNode* params;
    } SItemParamsDesc;

    typedef std::map<string, SActorClassDesc> TActorClassMap;
    typedef std::map<string, SItemParamsDesc> TActorParamsMap;

    ISystem* m_pSystem;
    IEntitySystem* m_pEntitySystem;

    TActorMap                   m_actors;
    TActorClassMap      m_classes;

    std::vector<CActorIterator*> m_iteratorPool;

    TActorParamsMap     m_actorParams;
    string m_actorParamsFolder;
};


#endif // CRYINCLUDE_CRYACTION_ACTORSYSTEM_H
