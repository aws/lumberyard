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

// Description : Interface for Entity pool manager

#ifndef CRYINCLUDE_CRYCOMMON_IENTITYPOOLMANAGER_H
#define CRYINCLUDE_CRYCOMMON_IENTITYPOOLMANAGER_H
#pragma once


#include <ISerialize.h>

struct SEntitySpawnParams;

struct IEntityPoolListener
{
    enum ESubscriptions
    {
        PoolBookmarkCreated         = BIT(0),
        EntityPreparedFromPool  = BIT(1),
        EntityReturningToPool       = BIT(2),
        EntityReturnedToPool        = BIT(3),
        PoolDefinitionsLoaded       = BIT(4),
        BookmarkEntitySerialize = BIT(5),
    };
    enum
    {
        COUNT_SUBSCRIPTIONS = 6
    };

    // <interfuscator:shuffle>
    virtual ~IEntityPoolListener() {}

    //! Called when an entity bookmark is created on level load
    virtual void OnPoolBookmarkCreated(EntityId entityId, const SEntitySpawnParams& params, XmlNodeRef entityNode) {}

    //! Called when a bookmarked entity is prepared from the pool
    virtual void OnEntityPreparedFromPool(EntityId entityId, IEntity* pEntity) {}

    //! Called when a bookmarked entity is returning to the pool but has not been serialized to a bookmark
    virtual void OnEntityReturningToPool(EntityId entityId, IEntity* pEntity) {}

    //! Called when a bookmarked entity is returned to the pool
    virtual void OnEntityReturnedToPool(EntityId entityId, IEntity* pEntity) {}

    //! Called on loading pool definitions
    virtual void OnPoolDefinitionsLoaded(size_t numAI) {}

    //! Called when serializing a particular entity, either to or from a bookmark
    virtual void OnBookmarkEntitySerialize(TSerialize serialize, void* pVEntity) {}
    // </interfuscator:shuffle>
};

struct IEntityPoolManager
{
    struct SPreparingParams
    {
        EntityId entityId;
        tAIObjectID aiObjectId;
    };

    // <interfuscator:shuffle>
    virtual ~IEntityPoolManager() {}

    virtual void Serialize(TSerialize ser) = 0;

    //! Request entity pools be enabled
    virtual void Enable(bool bEnable) = 0;

    //! Resets all entity pools and returns all active entities
    virtual void ResetPools(bool bSaveState = true) = 0;

    //! Listener setup
    virtual void AddListener(IEntityPoolListener* pListener, const char* szWho, uint32 uSubscriptions) = 0;
    virtual void RemoveListener(IEntityPoolListener* pListener) = 0;

    //! Controls to bring a bookmarked entity into/out of existence via its assigned pool
    virtual bool PrepareFromPool(EntityId entityId, bool bPrepareNow = false) = 0;
    virtual bool ReturnToPool(EntityId entityId, bool bSaveState = true) = 0;
    virtual void ResetBookmark(EntityId entityId) = 0;

    //! Returns true if the given entity class name is set to be bookmarked by default
    virtual bool IsClassDefaultBookmarked(const char* szClassName) const = 0;

    //! Returns true if the given entityId has a bookmark (is set to be created through the pool)
    virtual bool IsEntityBookmarked(EntityId entityId) const = 0;
    virtual const char* GetBookmarkedClassName(EntityId entityId) const = 0;
    virtual const char* GetBookmarkedEntityName(EntityId entityId) const = 0;

    //! Returns true if the system is currently preparing an entity from the pool (and fills outParams)
    virtual bool IsPreparingEntity(SPreparingParams& outParams) const = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_IENTITYPOOLMANAGER_H
