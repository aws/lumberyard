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

#ifndef CRYINCLUDE_CRYENTITYSYSTEM_ENTITYSYSTEM_H
#define CRYINCLUDE_CRYENTITYSYSTEM_ENTITYSYSTEM_H
#pragma once

#include "IEntitySystem.h"
#include <ISystem.h>
#include "ITimer.h"
#include "SaltBufferArray.h"                    // SaltBufferArray<>
#include "EntityTimeoutList.h"
#include <StlUtils.h>
#include "STLPoolAllocator.h"
#include "STLGlobalAllocator.h"

//////////////////////////////////////////////////////////////////////////
// forward declarations.
//////////////////////////////////////////////////////////////////////////
class CEntity;
struct ICVar;
struct IPhysicalEntity;
class IComponent;
class CComponentEventDistributer;
class CEntityClassRegistry;
class CComponentFactoryRegistry;
class CScriptBind_Entity;
class CPhysicsEventListener;
class CAreaManager;
class CBreakableManager;
class CEntityArchetypeManager;
class CPartitionGrid;
class CProximityTriggerSystem;
class CEntityLayer;
class CEntityLoadManager;
class CEntityPoolManager;
struct SEntityLayerGarbage;
class CGeomCacheAttachmentManager;
class CCharacterBoneAttachmentManager;
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
struct SEntityTimerEvent
{
    EntityId entityId;
    int nTimerId;
    int nMilliSeconds;
};



//////////////////////////////////////////////////////////////////////////
struct SEntityAttachment
{
    EntityId child;
    EntityId parent;
    EntityGUID parentGuid;
    Vec3 pos;
    Quat rot;
    Vec3 scale;
    bool guid;
    int flags;
    string target;

    SEntityAttachment()
        : child(0)
        , parent(0)
        , parentGuid(0)
        , pos(ZERO)
        , rot(ZERO)
        , scale(ZERO)
        , guid(false)
        , flags(0) {}
};



//////////////////////////////////////////////////////////////////////////
// Structure for extended information about loading an entity
//  Supports reusing an entity container if specified
struct SEntityLoadParams
{
    SEntitySpawnParams spawnParams;
    CEntity* pReuseEntity;
    bool bCallInit;
    int clonedLayerId;

    SEntityLoadParams();
    SEntityLoadParams(CEntity* pReuseEntity, SEntitySpawnParams& resetParams);
    ~SEntityLoadParams();

    SEntityLoadParams& operator =(const SEntityLoadParams& other);
    SEntityLoadParams(const SEntityLoadParams& other) { *this = other; }
    void UseClonedEntityNode(const XmlNodeRef sourceEntityNode, XmlNodeRef parentNode);

private:
    void AddRef();
    void RemoveRef();

    static bool CloneXmlNode(const XmlNodeRef sourceNode, XmlNodeRef destNode);
};
typedef std::vector<SEntityLoadParams> TEntityLoadParamsContainer;


typedef CSaltHandle<unsigned short, unsigned short> CEntityHandle;

//////////////////////////////////////////////////////////////////////
class CEntitySystem
    : public IEntitySystem
{
public:
    CEntitySystem(ISystem* pSystem);
    ~CEntitySystem();

    bool Init(ISystem* pSystem);

    // interface IEntitySystem ------------------------------------------------------
    virtual void RegisterCharactersForRendering();
    virtual void Release();
    virtual void Update();
    virtual void DeletePendingEntities();
    virtual void PrePhysicsUpdate();
    virtual void Reset();
    virtual void Unload();
    virtual void PurgeHeaps();
    virtual IEntityClassRegistry* GetClassRegistry();
    IComponentFactoryRegistry* GetComponentFactoryRegistry() override;
    virtual IEntity* SpawnEntity(SEntitySpawnParams& params, bool bAutoInit = true);
    virtual bool InitEntity(IEntity* pEntity, SEntitySpawnParams& params);
    virtual IEntity* GetEntity(EntityId id) const;
    virtual EntityId GetClonedEntityId(EntityId origId, EntityId refId) const;
    virtual IEntity* FindEntityByName(const char* sEntityName) const;
    virtual void ReserveEntityId(const EntityId id);
    virtual EntityId ReserveUnknownEntityId();
    virtual void RemoveEntity(EntityId entity, bool bForceRemoveNow = false);
    virtual uint32 GetNumEntities() const;
    virtual IEntityIt* GetEntityIterator();
    virtual void SendEventViaEntityEvent(IEntity* piEntity, SEntityEvent& event);
    virtual void SendEventToAll(SEntityEvent& event);
    virtual int QueryProximity(SEntityProximityQuery& query);
    virtual void ResizeProximityGrid(int nWidth, int nHeight);
    virtual int GetPhysicalEntitiesInBox(const Vec3& origin, float radius, IPhysicalEntity**& pList, int physFlags) const;
    virtual IEntity* GetEntityFromPhysics(IPhysicalEntity* pPhysEntity) const;
    virtual void AddSink(IEntitySystemSink* pSink, uint32 subscriptions, uint64 onEventSubscriptions);
    virtual void RemoveSink(IEntitySystemSink* pSink);
    virtual void PauseTimers(bool bPause, bool bResume = false);
    virtual bool IsIDUsed(EntityId nID) const;
    virtual void GetMemoryStatistics(ICrySizer* pSizer) const;
    virtual ISystem* GetSystem() const { return m_pISystem; };
    virtual void SetNextSpawnId(EntityId id);
    virtual void ResetAreas();
    virtual void UnloadAreas();

    virtual void AddEntityEventListener(EntityId nEntity, EEntityEvent event, IEntityEventListener* pListener);
    virtual void RemoveEntityEventListener(EntityId nEntity, EEntityEvent event, IEntityEventListener* pListener);


    virtual EntityId FindEntityByGuid(const EntityGUID& guid) const;
    virtual EntityId FindEntityByEditorGuid(const char* pGuid) const;

    virtual EntityId GenerateEntityIdFromGuid(const EntityGUID& guid);

    virtual IEntityArchetype* LoadEntityArchetype(XmlNodeRef oArchetype);
    virtual IEntityArchetype* LoadEntityArchetype(const char* sArchetype);
    virtual IEntityArchetype* CreateEntityArchetype(IEntityClass* pClass, const char* sArchetype);

    virtual void Serialize(TSerialize ser);

    virtual void DumpEntities();

    virtual int GetLayerId(const char* szLayerName) const;
    virtual const char* GetLayerName(int layerId) const;
    virtual int GetLayerChildCount(const char* szLayerName) const;
    virtual const char* GetLayerChild(const char* szLayerName, int childIdx) const;

    virtual int GetVisibleLayerIDs(uint8* pLayerMask, const uint32 maxCount) const;

    virtual void ToggleLayerVisibility(const char* layer, bool isEnabled, bool includeParent = true);

    virtual void ToggleLayersBySubstring(const char* pSearchSubstring, const char* pExceptionSubstring, bool isEnable);

    virtual void LockSpawning(bool lock) {m_bLocked = lock; }

    virtual bool OnLoadLevel(const char* szLevelPath);
    void OnLevelLoadStart();

    virtual CEntityLayer* AddLayer(const char* name, const char* parent, uint16 id, bool bHasPhysics, int specs, bool bDefaultLoaded);
    virtual void LoadLayers(const char* dataFile);
    virtual void LinkLayerChildren();
    virtual void AddEntityToLayer(const char* layer, EntityId id);
    virtual void RemoveEntityFromLayers(EntityId id);
    virtual void ClearLayers();
    virtual void EnableDefaultLayers(bool isSerialized = true);
    virtual void EnableLayer(const char* layer, bool isEnable, bool isSerialized = true);
    virtual bool IsLayerEnabled(const char* layer, bool bMustBeLoaded) const;
    virtual bool ShouldSerializedEntity(IEntity* pEntity);
    virtual void RegisterPhysicCallbacks();
    virtual void UnregisterPhysicCallbacks();
    CEntityLayer* FindLayer(const char* layer);

    // ------------------------------------------------------------------------

    CEntityLayer* GetLayerForEntity(EntityId id);

    bool OnBeforeSpawn(SEntitySpawnParams& params);
    void OnEntityReused(IEntity* pEntity, SEntitySpawnParams& params);

    // Sets new entity timer event.
    void AddTimerEvent(SEntityTimerEvent& event, CTimeValue startTime = gEnv->pTimer->GetFrameStartTime());

    //////////////////////////////////////////////////////////////////////////
    // Load entities from XML.
    void LoadEntities(XmlNodeRef& objectsNode, bool bIsLoadingLevelFile);
    void LoadEntities(XmlNodeRef& objectsNode, bool bIsLoadingLevelFile, std::vector<IEntity*>* outGlobalEntityIds, std::vector<IEntity*>* outLocalEntityIds);

    virtual void HoldLayerEntities(const char* pLayerName);
    virtual void CloneHeldLayerEntities(const char* pLayerName, const Vec3& localOffset, const Matrix34& l2w, const char** pExcludeLayers = NULL, int numExcludeLayers = 0);
    virtual void ReleaseHeldEntities();

    ILINE CComponentEventDistributer* GetEventDistributer() { return m_pEventDistributer; }

    virtual bool ExtractEntityLoadParams(XmlNodeRef& entityNode, SEntitySpawnParams& spawnParams) const;
    virtual void BeginCreateEntities(int nAmtToCreate);
    virtual bool CreateEntity(XmlNodeRef& entityNode, SEntitySpawnParams& pParams, EntityId& outUsingId);
    virtual void EndCreateEntities();

    //////////////////////////////////////////////////////////////////////////
    // Called from CEntity implementation.
    //////////////////////////////////////////////////////////////////////////
    void RemoveTimerEvent(EntityId id, int nTimerId);
    void ChangeEntityNameRemoveTimerEvent(EntityId id);

    // Puts entity into active list.
    void ActivateEntity(CEntity* pEntity, bool bActivate);
    void ActivatePrePhysicsUpdateForEntity(CEntity* pEntity, bool bActivate);
    bool IsPrePhysicsActive(CEntity* pEntity);
    void OnEntityEvent(CEntity* pEntity, SEntityEvent& event);

    // Access to class that binds script to entity functions.
    // Used by Script component.
    CScriptBind_Entity* GetScriptBindEntity() { return m_pEntityScriptBinding; };

    // Access to area manager.
    IAreaManager* GetAreaManager() const { return (IAreaManager*)(m_pAreaManager); }

    // Access to breakable manager.
    virtual IBreakableManager* GetBreakableManager() const { return m_pBreakableManager; };

    // Access to entity pool manager.
    virtual IEntityPoolManager* GetIEntityPoolManager() const { return (IEntityPoolManager*)m_pEntityPoolManager; }
    CEntityPoolManager* GetEntityPoolManager() const { return m_pEntityPoolManager; }

    CEntityLoadManager* GetEntityLoadManager() const { return m_pEntityLoadManager; }

    CGeomCacheAttachmentManager* GetGeomCacheAttachmentManager() const { return m_pGeomCacheAttachmentManager; }
    CCharacterBoneAttachmentManager* GetCharacterBoneAttachmentManager() const { return m_pCharacterBoneAttachmentManager; }

    static ILINE uint16 IdToIndex(const EntityId id) { return id & 0xffff; }
    static ILINE CSaltHandle<> IdToHandle(const EntityId id) { return CSaltHandle<>(id >> 16, id & 0xffff); }
    static ILINE EntityId HandleToId(const CSaltHandle<> id) { return (((uint32)id.GetSalt()) << 16) | ((uint32)id.GetIndex()); }

    EntityId GenerateEntityId(bool bStaticId);
    bool ResetEntityId(CEntity* pEntity, EntityId newEntityId);
    void RegisterEntityGuid(const EntityGUID& guid, EntityId id);
    void UnregisterEntityGuid(const EntityGUID& guid);

    CPartitionGrid* GetPartitionGrid() { return m_pPartitionGrid; }
    CProximityTriggerSystem* GetProximityTriggerSystem() { return m_pProximityTriggerSystem;   }

    void ChangeEntityName(CEntity* pEntity, const char* sNewName);

    void RemoveEntityEventListeners(CEntity* pEntity);

    CEntity* GetCEntityFromID(EntityId id) const;
    ILINE bool HasEntity(EntityId id) const { return GetCEntityFromID(id) != 0; };

    virtual void PurgeDeferredCollisionEvents(bool bForce = false);

    void ComponentRegister(EntityId id, IComponentPtr pComponent);
    void ComponentDeregister(EntityId id, IComponentPtr pComponent);
    virtual void ComponentEnableEvent(const EntityId id, const int eventID, const bool enable);

    virtual void DebugDraw();

    virtual IBSPTree3D* CreateBSPTree3D(const IBSPTree3D::FaceList& faceList);
    virtual void ReleaseBSPTree3D(IBSPTree3D*& pTree);

private: // -----------------------------------------------------------------
    void DoPrePhysicsUpdate();
    void DoPrePhysicsUpdateFast();
    void DoUpdateLoop(float fFrameTime);

    void DeleteEntity(CEntity* pEntity);
    void UpdateDeletedEntities();
    void RemoveEntityFromActiveList(CEntity* pEntity);

    void UpdateNotSeenTimeouts();

    void OnBind(EntityId id, EntityId child);
    void OnUnbind(EntityId id, EntityId child);

    void UpdateEngineCVars();
    void UpdateTimers();
    void DebugDraw(CEntity* pEntity, float fUpdateTime);

    void DebugDrawEntityUsage();
    void DebugDrawLayerInfo();
    void DebugDrawProximityTriggers();

    void ClearEntityArray();

    void DumpEntity(IEntity* pEntity);

    void UpdateTempActiveEntities();

    // slow - to find specific problems
    void CheckInternalConsistency() const;


    //////////////////////////////////////////////////////////////////////////
    // Variables.
    //////////////////////////////////////////////////////////////////////////
    struct OnEventSink
    {
        OnEventSink(uint64 _subscriptions, IEntitySystemSink* _pSink)
            : subscriptions(_subscriptions)
            , pSink(_pSink)
        {
        }

        uint64 subscriptions;
        IEntitySystemSink* pSink;
    };

    typedef std::vector<OnEventSink, stl::STLGlobalAllocator<OnEventSink> > EntitySystemOnEventSinks;
    typedef std::vector<IEntitySystemSink*, stl::STLGlobalAllocator<IEntitySystemSink*> > EntitySystemSinks;
    typedef std::vector<CEntity*> DeletedEntities;
    typedef std::multimap<CTimeValue, SEntityTimerEvent, std::less<CTimeValue>, stl::STLPoolAllocator<std::pair<const CTimeValue, SEntityTimerEvent>, stl::PoolAllocatorSynchronizationSinglethreaded> > EntityTimersMap;
    typedef std::multimap<const char*, EntityId, stl::less_stricmp<const char*> > EntityNamesMap;
    typedef std::map<EntityId, CEntity*> EntitiesMap;
    typedef std::set<EntityId> EntitiesSet;
    typedef std::vector<SEntityTimerEvent> EntityTimersVector;

    EntitySystemSinks                               m_sinks[SinkMaxEventSubscriptionCount]; // registered sinks get callbacks for creation and removal
    EntitySystemOnEventSinks                m_onEventSinks;

    ISystem*                                               m_pISystem;
    std::vector<CEntity*>                  m_EntityArray;                   // [id.GetIndex()]=CEntity
    DeletedEntities                                 m_deletedEntities;
    std::vector<CEntity*>                       m_deferredUsedEntities;
    EntitiesMap                     m_mapActiveEntities;        // Map of currently active entities (All entities that need per frame update).
    bool                            m_tempActiveEntitiesValid; // must be set to false whenever m_mapActiveEntities is changed
    EntitiesSet                     m_mapPrePhysicsEntities; // map of entities requiring pre-physics activation

    EntityNamesMap                  m_mapEntityNames;  // Map entity name to entity ID.

    CSaltBufferArray<>                          m_EntitySaltBuffer;         // used to create new entity ids (with uniqueid=salt)
    std::vector<EntityId>           m_tempActiveEntities;   // Temporary array of active entities.

    CComponentEventDistributer*         m_pEventDistributer;
    //////////////////////////////////////////////////////////////////////////

    // Entity timers.
    EntityTimersMap                                 m_timersMap;
    EntityTimersVector                          m_currentTimers;
    bool                                                        m_bTimersPause;
    CTimeValue                                          m_nStartPause;

    // Binding entity.
    CScriptBind_Entity*                        m_pEntityScriptBinding;

    // Entity class registry.
    CEntityClassRegistry*                  m_pClassRegistry;

    CComponentFactoryRegistry*              m_pComponentFactoryRegistry;
    CPhysicsEventListener*                 m_pPhysicsEventListener;

    CAreaManager*                                  m_pAreaManager;

    CEntityLoadManager*                    m_pEntityLoadManager;
    CEntityPoolManager*                    m_pEntityPoolManager;

    // There`s a map of entity id to event listeners for each event.
    typedef std::multimap<EntityId, IEntityEventListener*> EventListenersMap;
    EventListenersMap m_eventListeners[ENTITY_EVENT_LAST];

    typedef std::map<EntityGUID, EntityId> EntityGuidMap;
    EntityGuidMap m_guidMap;
    EntityGuidMap m_genIdMap;

    IBreakableManager* m_pBreakableManager;
    CEntityArchetypeManager* m_pEntityArchetypeManager;
    CGeomCacheAttachmentManager* m_pGeomCacheAttachmentManager;
    CCharacterBoneAttachmentManager* m_pCharacterBoneAttachmentManager;

    // Partition grid used by the entity system
    CPartitionGrid* m_pPartitionGrid;
    CProximityTriggerSystem* m_pProximityTriggerSystem;

    EntityId m_idForced;

    //don't spawn any entities without being forced to
    bool        m_bLocked;

    CEntityTimeoutList m_entityTimeoutList;

    friend class CEntityItMap;
    class CCompareEntityIdsByClass;

    // helper to satisfy GCC
    static IEntityClass* GetClassForEntity(CEntity*);

    typedef std::map<string, CEntityLayer*> TLayers;
    typedef std::vector<SEntityLayerGarbage> THeaps;

    TLayers m_layers;
    THeaps m_garbageLayerHeaps;
    int m_nGeneratedFromGuid;

    //////////////////////////////////////////////////////////////////////////
    // Pool Allocators.
    //////////////////////////////////////////////////////////////////////////
public:
    bool m_bReseting;
    //////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_PROFILING_CODE
public:
    struct SLayerProfile
    {
        float fTimeMS;
        float fTimeOn;
        bool isEnable;
        CEntityLayer* pLayer;
    };

    std::vector<SLayerProfile> m_layerProfiles;
#endif //ENABLE_PROFILING_CODE
};

//////////////////////////////////////////////////////////////////////////
// Precache resources mode state.
//////////////////////////////////////////////////////////////////////////
extern bool gPrecacheResourcesMode;

#endif // CRYINCLUDE_CRYENTITYSYSTEM_ENTITYSYSTEM_H
