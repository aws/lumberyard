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

#ifndef CRYINCLUDE_CRYAISYSTEM_AIOBJECTMANAGER_H
#define CRYINCLUDE_CRYAISYSTEM_AIOBJECTMANAGER_H
#pragma once



#include "IAIObjectManager.h"
#include "IEntityPoolManager.h"

#include "PoolAllocator.h"

typedef std::multimap<short, CCountedRef<CAIObject> > AIObjectOwners;
typedef std::multimap<short, CWeakRef<CAIObject> > AIObjects;

enum EAIClass
{
    eAIC_Invalid = 0,

    eAIC_FIRST = 1,

    eAIC_AIObject = 1,
    eAIC_AIActor = 2,
    eAIC_Leader = 3,
    eAIC_AIPlayer = 4,
    eAIC_PipeUser = 5,
    eAIC_Puppet = 6,
    eAIC_AIVehicle = 7,
    eAIC_AIFlyingVehicle = 8,

    eAIC_LAST = eAIC_AIVehicle
};

struct SAIObjectCreationHelper
{
    SAIObjectCreationHelper(CAIObject* pObject);
    void Serialize(TSerialize ser);
    CAIObject* RecreateObject(void* alloc = NULL);

    string name;
    EAIClass aiClass;
    tAIObjectID objectId;
};


class CAIObjectManager
    : public IAIObjectManager
    , public IEntityPoolListener
{
public:

    CAIObjectManager();
    virtual ~CAIObjectManager();

    void Init();
    void Reset(bool includingPooled = true);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //IAIObjectManager/////////////////////////////////////////////////////////////////////////////////////////////

    virtual IAIObject* CreateAIObject(const AIObjectParams& params);
    virtual void RemoveObject(tAIObjectID objectID);

    virtual IAIObject* GetAIObject(tAIObjectID aiObjectID);
    virtual IAIObject* GetAIObjectByName(unsigned short type, const char* pName) const;

    // Returns AIObject iterator for first match, see EGetFirstFilter for the filter options.
    // The parameter 'n' specifies the type, group id or species based on the selected filter.
    // It is possible to iterate over all objects by setting the filter to OBJFILTER_TYPE
    // passing zero to 'n'.
    virtual IAIObjectIter* GetFirstAIObject(EGetFirstFilter filter, short n);
    // Iterates over AI objects within specified range.
    // Parameter 'pos' and 'rad' specify the enclosing sphere, for other parameters see GetFirstAIObject.
    virtual IAIObjectIter* GetFirstAIObjectInRange(EGetFirstFilter filter, short n, const Vec3& pos, float rad, bool check2D);

    //IAIObjectManager/////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //IEntityPoolListener//////////////////////////////////////////////////////////////////////////////////////////
    virtual void OnEntityPreparedFromPool(EntityId entityId, IEntity* pEntity);
    virtual void OnEntityReturnedToPool(EntityId entityId, IEntity* pEntity);
    virtual void OnPoolDefinitionsLoaded(size_t numAI);
    virtual void OnBookmarkEntitySerialize(TSerialize serialize, void* pVEntity);
    //IEntityPoolListener//////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // callback from CObjectContainer: notify the rest of the system that the object is disappearing
    void OnObjectRemoved(CAIObject* pObject);

    //// it removes all references to this object from all objects of the specified type
    //// (MATT) Note that is does not imply deletion - vehicles use it when they lose their driver {2009/02/05}
    void RemoveObjectFromAllOfType(int nType, CAIObject* pRemovedObject);

    void CreateDummyObject(CCountedRef<CAIObject>& ref, const char* name = "", CAIObject::ESubType type = CAIObject::STP_NONE, tAIObjectID requiredID = INVALID_AIOBJECTID);
    void CreateDummyObject(CStrongRef <CAIObject>& ref, const char* name = "", CAIObject::ESubType type = CAIObject::STP_NONE, tAIObjectID requiredID = INVALID_AIOBJECTID);

    CAIObject* GetAIObjectByName(const char* pName) const;

    void ReleasePooledObject(CAIObject* pObject);
    bool IsSerializingBookmark() const { return m_serializingBookmark; }

    // todo: ideally not public
    AIObjectOwners m_Objects;// m_RootObjects or EntityObjects might be better names
    AIObjects m_mapDummyObjects;

protected:

    typedef std::map<EntityId, CCountedRef<CAIObject> > TPooledAIObjectMap;
    TPooledAIObjectMap m_pooledObjects;

    typedef class CAIVehicle TPooledAIObject;   // Currently CAIVehicle is the largest class; ensures there is always space in the pool
    stl::TPoolAllocator<TPooledAIObject>* m_pPoolAllocator;

    // set to true during the process of serializing an AI object to/from an entity pool bookmark. This allows
    //  objects owned by the current object to be serialized into the bookmark as well.
    bool m_serializingBookmark;

    size_t m_PoolBucketSize;
};

#endif // CRYINCLUDE_CRYAISYSTEM_AIOBJECTMANAGER_H
