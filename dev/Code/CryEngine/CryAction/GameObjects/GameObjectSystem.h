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

#ifndef CRYINCLUDE_CRYACTION_GAMEOBJECTS_GAMEOBJECTSYSTEM_H
#define CRYINCLUDE_CRYACTION_GAMEOBJECTS_GAMEOBJECTSYSTEM_H
#pragma once

// FIXME: Cell SDK GCC bug workaround.
#ifndef CRYINCLUDE_IGAMEOBJECTSYSTEM_H
#include "IGameObjectSystem.h"
#endif

#include <vector>
#include <map>

class CGameObjectSystem
    : public IGameObjectSystem
{
public:
    bool Init();

    IGameObjectSystem::ExtensionID GetID(const char* name);
    const char* GetName(IGameObjectSystem::ExtensionID id);
    uint32 GetExtensionSerializationPriority(IGameObjectSystem::ExtensionID id);
    IGameObjectExtensionPtr Instantiate(IGameObjectSystem::ExtensionID id, IGameObject* pObject);
    virtual void RegisterExtension(const char* name, IGameObjectExtensionCreatorBase* pCreator, IEntityClassRegistry::SEntityClassDesc* pClsDesc);
    virtual void BroadcastEvent(const SGameObjectEvent& evt);

    virtual void RegisterEvent(uint32 id, const char* name);
    virtual uint32 GetEventID(const char* name);
    virtual const char* GetEventName(uint32 id);

    virtual IGameObject* CreateGameObjectForEntity(EntityId entityId);

    virtual void PostUpdate(float frameTime);
    virtual void SetPostUpdate(IGameObject* pGameObject, bool enable);

    virtual void Reset();

    void RegisterFactories(IGameFramework* pFW);

    IEntity* CreatePlayerProximityTrigger();
    ILINE IEntityClass* GetPlayerProximityTriggerClass() { return m_pClassPlayerProximityTrigger; }
    ILINE std::vector<IGameObjectSystem::ExtensionID>* GetActivatedExtensionsTop() { return &m_activatedExtensions_top; }

    virtual void SetSpawnSerializerForEntity(const EntityId entityId, TSerialize* pSerializer);
    virtual void ClearSpawnSerializerForEntity(const EntityId entityId);

    void GetMemoryUsage(ICrySizer* s) const;

    virtual void AddSink(IGameObjectSystemSink* pSink);
    virtual void RemoveSink(IGameObjectSystemSink* pSink);
private:
    void LoadSerializationOrderFile();
    TSerialize* GetSpawnSerializerForEntity(const EntityId entityId) const;

    std::map<string, ExtensionID> m_nameToID;

    struct SExtensionInfo
    {
        string name;
        uint32 serializationPriority; // lower values is higher priority
        IGameObjectExtensionCreatorBase* pFactory;

        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            pSizer->AddObject(name);
        }
    };
    std::vector<SExtensionInfo> m_extensionInfo;

    static IComponentPtr CreateGameObjectWithPreactivatedExtension(
        IEntity* pEntity, SEntitySpawnParams& params, void* pUserData);

    std::vector<IGameObject*> m_postUpdateObjects;
    IEntityClass* m_pClassPlayerProximityTrigger;

    struct SSpawnSerializer
    {
        SSpawnSerializer(const EntityId _entityId, TSerialize* _pSerializer)
            : entityId(_entityId)
            , pSerializer(_pSerializer)
        {
        }

        bool operator== (const SSpawnSerializer& otherSerializer) const
        {
            return (entityId == otherSerializer.entityId);
        };

        bool operator== (const EntityId& otherEntityId) const
        {
            return (entityId == otherEntityId);
        }

        EntityId    entityId;
        TSerialize* pSerializer;
    };
    typedef std::vector<SSpawnSerializer> TSpawnSerializers;
    TSpawnSerializers m_spawnSerializers;

    // event ID management
    std::map<string, uint32> m_eventNameToID;
    std::map<uint32, string> m_eventIDToName;
    //
    typedef std::list<IGameObjectSystemSink*> SinkList;
    SinkList                                              m_lstSinks;                       // registered sinks get callbacks

    std::vector<IGameObject*> m_tempObjects;

    std::vector<IGameObjectSystem::ExtensionID> m_activatedExtensions_top;
    std::vector<string> m_serializationOrderList;  // defines serialization order for extensions
};

#endif // CRYINCLUDE_CRYACTION_GAMEOBJECTS_GAMEOBJECTSYSTEM_H
