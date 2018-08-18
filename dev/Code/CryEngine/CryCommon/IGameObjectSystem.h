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

#ifndef CRYINCLUDE_CRYACTION_IGAMEOBJECTSYSTEM_H
#define CRYINCLUDE_CRYACTION_IGAMEOBJECTSYSTEM_H
#pragma once

#include "IEntityClass.h"
#include "IGameFramework.h"
#include "INetwork.h"

enum EGameObjectEventFlags
{
    eGOEF_ToScriptSystem = 0x0001,
    eGOEF_ToGameObject   = 0x0002,
    eGOEF_ToExtensions   = 0x0004,
    eGOEF_LoggedPhysicsEvent = 0x0008, // was this a logged or immediate physics event

    eGOEF_ToAll          = eGOEF_ToScriptSystem | eGOEF_ToGameObject | eGOEF_ToExtensions
};

struct IGameObject;
struct SGameObjectEvent;

struct IGameObjectBoxListener
{
    virtual ~IGameObjectBoxListener(){}
    virtual void OnEnter(int id, EntityId entity) = 0;
    virtual void OnLeave(int id, EntityId entity) = 0;
    virtual void OnRemoveParent() = 0;
};

// Description:
//      A callback interface for a class that wants to be aware when new game objects are being spawned. A class that implements
//      this interface will be called every time a new game object is spawned.
struct IGameObjectSystemSink
{
    // This callback is called after this game object is initialized.
    virtual void OnAfterInit(IGameObject* object) = 0;
    virtual ~IGameObjectSystemSink(){}
};

struct IGameObjectSystem
{
    virtual ~IGameObjectSystem(){}
    // If this is set as the user data for a GameObject with Preactivated Extension
    // spawn, then it will be called back to provide additional initialization.
    struct SEntitySpawnParamsForGameObjectWithPreactivatedExtension
    {
        // If the user wants to extend this spawn parameters using this as a base class,
        // make sure to override 'm_type' member with your own typeID starting at 'eSpawnParamsType_Custom'
        enum EType
        {
            eSpawnParamsType_Default = 0,
            eSpawnParamsType_Custom,
        };

        bool (* hookFunction)(IEntity* pEntity, IGameObject*, void* pUserData);
        void* pUserData;

        SEntitySpawnParamsForGameObjectWithPreactivatedExtension()
            : m_type(eSpawnParamsType_Default)
        {
        }

        uint32 IsOfType(const uint32 type) const { return (m_type == type); };

    protected:
        uint32 m_type;
    };

    typedef uint16 ExtensionID;
    static const ExtensionID InvalidExtensionID = ~ExtensionID(0);
    typedef IGameObjectExtension*(* GameObjectExtensionFactory)();

    virtual IGameObjectSystem::ExtensionID GetID(const char* name) = 0;
    virtual const char* GetName(IGameObjectSystem::ExtensionID id) = 0;
    virtual uint32 GetExtensionSerializationPriority(IGameObjectSystem::ExtensionID id) = 0;
    virtual IGameObjectExtensionPtr Instantiate(IGameObjectSystem::ExtensionID id, IGameObject* pObject) = 0;
    virtual void BroadcastEvent(const SGameObjectEvent& evt) = 0;

    static const uint32 InvalidEventID = ~uint32(0);
    virtual void RegisterEvent(uint32 id, const char* name) = 0;
    virtual uint32 GetEventID(const char* name) = 0;
    virtual const char* GetEventName(uint32 id) = 0;

    virtual IGameObject* CreateGameObjectForEntity(EntityId entityId) = 0;

    virtual void RegisterExtension(const char* name, IGameObjectExtensionCreatorBase* pCreator, IEntityClassRegistry::SEntityClassDesc* pEntityCls) = 0;

    virtual void PostUpdate(float frameTime) = 0;
    virtual void SetPostUpdate(IGameObject* pGameObject, bool enable) = 0;

    virtual void Reset() = 0;

    virtual void SetSpawnSerializerForEntity(const EntityId entityId, TSerialize* pSerializer) = 0;
    virtual void ClearSpawnSerializerForEntity(const EntityId entityId) = 0;

    virtual void AddSink(IGameObjectSystemSink* pSink) = 0;
    virtual void RemoveSink(IGameObjectSystemSink* pSink) = 0;
};

// Summary
//   Structure used to define a game object event
struct SGameObjectEvent
{
    SGameObjectEvent(uint32 _event, uint16 _flags, IGameObjectSystem::ExtensionID _target = IGameObjectSystem::InvalidExtensionID, void* _param = 0)
    {
        this->event = _event;
        this->target = _target;
        this->flags = _flags;
        this->ptr = 0;
        this->param = _param;
    }
    uint32 event;
    IGameObjectSystem::ExtensionID target;
    uint16 flags;
    void* ptr;
    // optional parameter of event (ugly)
    union
    {
        void* param;
        bool paramAsBool;
    };
};

#endif // CRYINCLUDE_CRYACTION_IGAMEOBJECTSYSTEM_H
