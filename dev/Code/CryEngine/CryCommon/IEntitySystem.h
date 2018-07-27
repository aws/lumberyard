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

#ifndef CRYINCLUDE_CRYCOMMON_IENTITYSYSTEM_H
#define CRYINCLUDE_CRYCOMMON_IENTITYSYSTEM_H
#pragma once

// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the CRYENTITYDLL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// CRYENTITYDLL_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef CRYENTITYDLL_EXPORTS
    #define CRYENTITYDLL_API DLL_EXPORT
#else
    #define CRYENTITYDLL_API DLL_IMPORT
#endif

#define _DISABLE_AREASOLID_

#include <IAudioInterfacesCommonData.h>
#include <IComponent.h>
#include <ComponentType.h>
#include <IComponentFactoryRegistry.h>
#if !defined(_RELEASE) && (defined(WIN32) || defined(WIN64))
#define ENABLE_ENTITYEVENT_DEBUGGING 1
#else
#define ENABLE_ENTITYEVENT_DEBUGGING 0
#endif


// entity event listener debug output macro
#if ENABLE_ENTITYEVENT_DEBUGGING
#define ENTITY_EVENT_LISTENER_DEBUG                                            \
    {                                                                          \
        if (gEnv && gEnv->pConsole)                                            \
        {                                                                      \
            ICVar* pCvar = gEnv->pConsole->GetCVar("es_debugEntityListeners"); \
            if (pCvar && pCvar->GetIVal()) {                                   \
                CryLog("Entity Event listener %s '%p'", __FUNCTION__, this); } \
        }                                                                      \
    }

#define ENTITY_EVENT_ENTITY_DEBUG(pEntity)                                                   \
    {                                                                                        \
        if (gEnv && gEnv->pConsole)                                                          \
        {                                                                                    \
            ICVar* pCvar = gEnv->pConsole->GetCVar("es_debugEntityListeners");               \
            if (pCvar && pCvar->GetIVal()) {                                                 \
                CryLog("Event for entity '%s' pointer '%p'", pEntity->GetName(), pEntity); } \
        }                                                                                    \
    }

#define ENTITY_EVENT_ENTITY_LISTENER(pListener)                                \
    {                                                                          \
        if (gEnv && gEnv->pConsole)                                            \
        {                                                                      \
            ICVar* pCvar = gEnv->pConsole->GetCVar("es_debugEntityListeners"); \
            if (pCvar && pCvar->GetIVal()) {                                   \
                CryLog("Event listener '%p'", pListener); }                    \
        }                                                                      \
    }

#define ENTITY_EVENT_LISTENER_ADDED(pEntity, pListener)                                                                                          \
    {                                                                                                                                            \
        if (gEnv && gEnv->pConsole)                                                                                                              \
        {                                                                                                                                        \
            ICVar* pCvar = gEnv->pConsole->GetCVar("es_debugEntityListeners");                                                                   \
            if (pCvar && pCvar->GetIVal()) {                                                                                                     \
                CryLog("Adding Entity Event listener for entity '%s' pointer '%p' for listener '%p'", pEntity->GetName(), pEntity, pListener); } \
        }                                                                                                                                        \
    }

#define ENTITY_EVENT_LISTENER_REMOVED(nEntity, pListener)                                                                                     \
    {                                                                                                                                         \
        if (gEnv && gEnv->pConsole)                                                                                                           \
        {                                                                                                                                     \
            ICVar* pCvar = gEnv->pConsole->GetCVar("es_debugEntityListeners");                                                                \
            if (pCvar && pCvar->GetIVal())                                                                                                    \
            {                                                                                                                                 \
                IEntity* pEntity = GetEntity(nEntity);                                                                                        \
                CryLog("Removing Entity Event listener for entity '%s' for listener '%p'", pEntity ? pEntity->GetName() : "null", pListener); \
            }                                                                                                                                 \
        }                                                                                                                                     \
    }

#else
#define ENTITY_EVENT_LISTENER_DEBUG
#define ENTITY_EVENT_ENTITY_DEBUG(pEntity)
#define ENTITY_EVENT_ENTITY_LISTENER(pListener)
#define ENTITY_EVENT_LISTENER_ADDED(pEntity, pListener)
#define ENTITY_EVENT_LISTENER_REMOVED(nEntity, pListener)
#endif


#include <IEntity.h>
#include <IEntityClass.h>
#include <smartptr.h>
#include <Cry_Geo.h>
#include <IXml.h>

// Forward declarations.
struct ISystem;
struct IEntitySystem;
class ICrySizer;
struct IEntity;
struct SpawnParams;
struct IPhysicalEntity;
struct IBreakEventListener;
struct SRenderNodeCloneLookup;
struct EventPhysRemoveEntityParts;
struct IBreakableManager;
struct SComponentRegisteredEvents;
class CEntityLayer;
class IComponentFactoryRegistry;
struct IEntityPoolManager;


//  Summary:
//    SpecType for entity layers.
//    Add new bits to update. Do not just rename, cause values are used for saving levels.
enum ESpecType
{
    eSpecType_PC = BIT(0),
    eSpecType_XBoxOne = BIT(1),
    eSpecType_PS4 = BIT(2),
    eSpecType_All   = -1
};

// Summary:
//   Area Manager Interface.
struct IArea
{
    // <interfuscator:shuffle>
    virtual ~IArea(){}
    virtual size_t GetEntityAmount() const = 0;
    virtual const EntityId GetEntityByIdx(int index) const = 0;
    virtual void GetMinMax(Vec3** min, Vec3** max) const = 0;
    virtual int GetGroup() const = 0;
    virtual int GetPriority() const = 0;
    virtual int GetID() const = 0;
    // </interfuscator:shuffle>
};

// Summary:
//   EventListener interface for the AreaManager.
struct IAreaManagerEventListener
{
    // <interfuscator:shuffle>
    virtual ~IAreaManagerEventListener(){}
    // Summary:
    //   Callback event.
    virtual void OnAreaManagerEvent(EEntityEvent event, EntityId TriggerEntityID, IArea* pArea) = 0;
    // </interfuscator:shuffle>
};

// Summary:
//   Structure for additional AreaManager queries.
struct SAreaManagerResult
{
public:
    SAreaManagerResult()
    {
        pArea = NULL;
        fDistanceSq = 0.0f;
        vPosOnHull  = Vec3(0);
        bInside = false;
        bNear   = false;
    }

    IArea* pArea;
    float fDistanceSq;
    Vec3 vPosOnHull;
    bool bInside;
    bool bNear;
};

// Summary:
//   Structure for AudioArea AreaManager queries.
struct SAudioAreaInfo
{
    SAudioAreaInfo()
        : pArea(NULL)
        , fEnvironmentAmount(0.0f)
        , nEnvironmentID(INVALID_AUDIO_ENVIRONMENT_ID)
        ,   nEnvProvidingEntityID(INVALID_ENTITYID)
    {
    }

    IArea*  pArea;
    float       fEnvironmentAmount;
    Audio::TAudioEnvironmentID nEnvironmentID;
    EntityId nEnvProvidingEntityID;
};

struct IBSPTree3D
{
    typedef DynArray<Vec3> CFace;
    typedef DynArray<CFace> FaceList;

    virtual ~IBSPTree3D() {}
    virtual bool IsInside(const Vec3& vPos) const = 0;
    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;

    virtual size_t WriteToBuffer(void* pBuffer) const = 0;
    virtual void ReadFromBuffer(const void* pBuffer) = 0;
};

struct IAreaManager
{
    // <interfuscator:shuffle>
    virtual ~IAreaManager(){}
    virtual size_t                          GetAreaAmount() const = 0;
    virtual IArea const* const  GetArea(size_t const nAreaIndex) const = 0;
    virtual size_t GetOverlappingAreas(const AABB& bb, PodArray<IArea*>& list) const = 0;

    // Description:
    //   Additional Query based on position. Needs preallocated space to write nMaxResults to pResults.
    //   Returns the actual number of results written in rNumResults.
    // Return Value:
    //   True on success or false on error or if provided structure was too small.
    virtual bool QueryAreas(EntityId const nEntityID, Vec3 const& vPos, SAreaManagerResult* pResults, int nMaxResults, int& rNumResults) = 0;

    // Description:
    //   Additional Query based on position. Returns only the areas that have AudioProxies and valid AudioEnvironmentIDs.
    //   Needs preallocated space to write nMaxResults to pResults.
    //   The actual number of results filled is returned in rNumResults.
    // Return Value:
    //   True on success or false on error or if provided structure was too small.
    virtual bool QueryAudioAreas(Vec3 const& rPos, SAudioAreaInfo* pResults, size_t const nMaxResults, size_t& rNumResults) = 0;


    // Description:
    //  Query areas linked to other entities (these links are shape links)
    //  fills out a list of entities and sets outAndMaxResults to the size of results
    // Return Value:
    //  Returns true if the array was large enough, or false if there was more then outAndMaxResults attached to the entity
    virtual bool GetLinkedAreas(EntityId linkedId, EntityId* pOutArray, int& outAndMaxResults) const = 0;

    virtual void DrawLinkedAreas(EntityId linkedId) const = 0;

    // Summary:
    //   Registers EventListener to the AreaManager.
    virtual void AddEventListener(IAreaManagerEventListener* pListener) = 0;
    virtual void RemoveEventListener(IAreaManagerEventListener* pListener) = 0;

    // Description:
    // Invokes a re-compilation of the entire area grid
    virtual void SetAreasDirty() = 0;

    // Description:
    // Invokes a partial recomp of the grid (faster)
    virtual void SetAreaDirty(IArea* pArea) = 0;

    // Description:
    //  Passed in entity exits all areas. Meant for when players are killed.
    virtual void ExitAllAreas(IEntity const* const pEntity) = 0;

    // Description:
    // Puts the passed entity ID into the update list for the next update.
    virtual void MarkEntityForUpdate(EntityId const nEntityID) = 0;
    // </interfuscator:shuffle>
};

// Description:
//   Entity iterator interface. This interface is used to traverse trough all the entities in an entity system. In a way,
//   this iterator works a lot like a stl iterator.

struct IEntityIt
{
    // <interfuscator:shuffle>
    virtual ~IEntityIt(){}

    virtual void AddRef() = 0;

    // Summary:
    //   Deletes this iterator and frees any memory it might have allocated.
    virtual void Release() = 0;

    // Summary:
    //   Checks whether current iterator position is the end position.
    // Return Value:
    //   True if iterator at end position.
    virtual bool IsEnd() = 0;

    // Summary:
    //   Retrieves next entity.
    // Return Value:
    //   A pointer to the entity that the iterator points to before it goes to the next.
    virtual IEntity* Next() = 0;

    // Summary:
    //   Retrieves current entity.
    // Return Value:
    //   The entity that the iterator points to.
    virtual IEntity* This() = 0;

    // Summary:
    //   Positions the iterator at the beginning of the entity list.
    virtual void MoveFirst() = 0;
    // </interfuscator:shuffle>
};

typedef _smart_ptr<IEntityIt> IEntityItPtr;

// Description:
//   A callback interface for a class that wants to be aware when new entities are being spawned or removed. A class that implements
//   this interface will be called every time a new entity is spawned, removed, or when an entity container is to be spawned.
struct IEntitySystemSink
{
    // <interfuscator:shuffle>
    virtual ~IEntitySystemSink(){}
    // Description:
    //   This callback is called before this entity is spawned. The entity will be created and added to the list of entities,
    //   if this function returns true. Returning false will abort the spawning process.
    // Arguments:
    //   params - The parameters that will be used to spawn the entity.
    virtual bool OnBeforeSpawn(SEntitySpawnParams& params) = 0;

    // Description:
    //   This callback is called when this entity has finished spawning. The entity has been created and added to the list of entities,
    //   but has not been initialized yet.
    // Arguments:
    //   pEntity - The entity that was just spawned
    //   params  -
    virtual void OnSpawn(IEntity* pEntity, SEntitySpawnParams& params) = 0;

    // Description:
    //   Called when an entity is being removed.
    // Arguments:
    //   pEntity - The entity that is being removed. This entity is still fully valid.
    // Return Value:
    //   True to allow removal, false to deny.
    virtual bool OnRemove(IEntity* pEntity) = 0;

    // Description:
    //   Called when an entity has been reused. You should clean up when this is called.
    // Arguments:
    //   pEntity - The entity that is being reused. This entity is still fully valid and with its new EntityId.
    //   params - The new params this entity is using.
    virtual void OnReused(IEntity* pEntity, SEntitySpawnParams& params) = 0;

    // Description:
    //   Called in response to an entity event.
    // Arguments:
    //   pEntity - The entity that is being removed. This entity is still fully valid.
    //   event   -
    virtual void OnEvent(IEntity* pEntity, SEntityEvent& event) = 0;

    // Description:
    //   Collect memory informations
    // Arguments:
    //   pSizer - The Sizer class used to collect the memory informations
    virtual void GetMemoryUsage(class ICrySizer* pSizer) const{};
    // </interfuscator:shuffle>
};

// Summary:
//   Interface to the entity archetype.
struct IEntityArchetype
{
    // <interfuscator:shuffle>
    virtual ~IEntityArchetype(){}
    // Retrieve entity class of the archetype.
    virtual IEntityClass* GetClass() const = 0;
    virtual const char* GetName() const = 0;
    virtual IScriptTable* GetProperties() = 0;
    virtual XmlNodeRef GetObjectVars() = 0;
    virtual void LoadFromXML(XmlNodeRef& propertiesNode, XmlNodeRef& objectVarsNode) = 0;
    // </interfuscator:shuffle>
};

//////////////////////////////////////////////////////////////////////////
struct IEntityEventListener
{
    // <interfuscator:shuffle>
    virtual ~IEntityEventListener(){}
    virtual void OnEntityEvent(IEntity* pEntity, SEntityEvent& event) = 0;
    // </interfuscator:shuffle>
};


// Summary:
//   Structure used by proximity query in entity system.
struct SEntityProximityQuery
{
    AABB box; // Report entities within this bounding box.
    IEntityClass* pEntityClass;
    uint32 nEntityFlags;

    // Output.
    IEntity** pEntities;
    int nCount;

    SEntityProximityQuery()
    {
        nCount = 0;
        pEntities = 0;
        pEntityClass = 0;
        nEntityFlags = 0;
    }
};

// Description:
//   Interface to the system that manages the entities in the game, their creation, deletion and upkeep. The entities are kept in a map
//   indexed by their unique entity ID. The entity system updates only unbound entities every frame (bound entities are updated by their
//   parent entities), and deletes the entities marked as garbage every frame before the update. The entity system also keeps track of entities
//   that have to be drawn last and with more zbuffer resolution.
// Summary:
//   Interface to the system that manages the entities in the game.
struct IEntitySystem
{
    enum SinkEventSubscriptions
    {
        OnBeforeSpawn = BIT(0),
        OnSpawn             = BIT(1),
        OnRemove            = BIT(2),
        OnReused            = BIT(3),
        OnEvent             = BIT(4),

        AllSinkEvents   = ~0u,
    };

    enum
    {
        SinkMaxEventSubscriptionCount = 5,
    };

    // <interfuscator:shuffle>
    virtual ~IEntitySystem(){}

    virtual void RegisterCharactersForRendering() = 0;

    // Summary:
    //   Releases entity system.
    virtual void Release() = 0;

    // Description:
    //   Updates entity system and all entities before physics is called. (or as early as possible after input in the multithreaded physics case)
    //   This function executes once per frame.
    virtual void PrePhysicsUpdate() = 0;

    // Summary:
    //   Updates entity system and all entities. This function executes once a frame.
    virtual void    Update() = 0;

    // Summary:
    //   Resets whole entity system, and destroy all entities.
    virtual void    Reset() = 0;

    // Summary:
    //   Unloads whole entity system - should be called when level is unloaded.
    virtual void Unload() = 0;
    virtual void PurgeHeaps() = 0;

    // Description:
    //     Deletes any pending entities (which got marked for deletion).
    virtual void  DeletePendingEntities() = 0;


    // Description:
    //     Retrieves the entity class registry interface.
    // Return:
    //     Pointer to the valid entity class registry interface.
    virtual IEntityClassRegistry* GetClassRegistry() = 0;

    virtual IComponentFactoryRegistry* GetComponentFactoryRegistry() = 0;

    // Summary:
    //   Spawns a new entity according to the data in the Entity Descriptor.
    // Arguments:
    //   params     - Entity descriptor structure that describes what kind of entity needs to be spawned
    //   bAutoInit  - If true automatically initialize entity.
    // Return Value:
    //   The spawned entity if successful, NULL if not.
    // See Also:
    //   CEntityDesc
    virtual IEntity* SpawnEntity(SEntitySpawnParams& params, bool bAutoInit = true) = 0;

    // Summary:
    //   Initializes entity.
    // Description:
    //   Initialize entity if entity was spawned not initialized (with bAutoInit false in SpawnEntity).
    // Note:
    //   Used only by Editor, to setup properties & other things before initializing entity,
    //   do not use this directly.
    // Arguments:
    //   pEntity - Pointer to just spawned entity object.
    //   params  - Entity descriptor structure that describes what kind of entity needs to be spawned.
    // Return value:
    //   True if successfully initialized entity.
    virtual bool InitEntity(IEntity* pEntity, SEntitySpawnParams& params) = 0;

    // Summary:
    //   Retrieves entity from its unique id.
    // Arguments:
    //   id - The unique ID of the entity required.
    // Return value:
    //   The entity if one with such an ID exists, and NULL if no entity could be matched with the id
    virtual IEntity* GetEntity(EntityId id) const = 0;

    // Summary:
    //   Converts an original entity id to that of a cloned version of it.  This should be
    //   called on any serialized entity ids after loading is complete, to make sure they
    //   account for runtime cloning.
    // Arguments:
    //   origId - The unique ID of the original entity.
    //   refId - The id of a cloned entity from the same layer of the original entity
    //           (in this case "same layer" means the root layer and any children).
    // Return value:
    //   The id of the cloned enity, or the original entity if it wasn't cloned
    virtual EntityId GetClonedEntityId(EntityId origId, EntityId refId) const = 0;

    // Summary:
    //   Find first entity with given name.
    // Arguments:
    //   sEntityName - The name to look for.
    // Return value:
    //   The entity if found, 0 if failed.
    virtual IEntity* FindEntityByName(const char* sEntityName) const = 0;

    // Note:
    //   Might be needed to call before loading of entities to be sure we get the requested IDs.
    // Arguments:
    //   id - must not be 0.
    virtual void ReserveEntityId(const EntityId id) = 0;

    // Reserves a dynamic entity id
    virtual EntityId ReserveUnknownEntityId() = 0;

    // Summary:
    //   Removes an entity by ID.
    // Arguments:
    //   entity          - The id of the entity to be removed.
    //   bForceRemoveNow - If true forces immediately delete of entity, overwise will delete entity on next update.
    virtual void    RemoveEntity(EntityId entity, bool bForceRemoveNow = false) = 0;

    // Summary:
    //   Gets number of entities stored in entity system.
    // Return value:
    //   The number of entities.
    virtual uint32 GetNumEntities() const = 0;

    // Summary:
    //   Gets a entity iterator.
    // Description:
    //   Gets a entity iterator. This iterator interface can be used to traverse all the entities in this entity system.
    // Return value:
    //   An entityIterator.
    // See also:
    //   IEntityIt
    virtual IEntityIt* GetEntityIterator() = 0;

    // Description:
    //    Sends the same event to all entities in Entity System.
    // Arguments:
    //    event - Event to send.
    virtual void SendEventToAll(SEntityEvent& event) = 0;

    // Description:
    //    Sends the same event to the entity in Entity System using the EntityEvent system
    // Arguments:
    //    event - Event to send.
    virtual void SendEventViaEntityEvent(IEntity* piEntity, SEntityEvent& event) = 0;

    // Summary:
    //   Get all entities within proximity of the specified bounding box.
    // Note:
    //   Query is not exact, entities reported can be a few meters away from the bounding box.
    virtual int QueryProximity(SEntityProximityQuery& query) = 0;

    // Summary:
    //   Resizes the proximity grid.
    // Note:
    //   Call this when you know dimensions of the level, before entities are created.
    virtual void ResizeProximityGrid(int nWidth, int nHeight) = 0;


    // Summary:
    //   Gets all entities in specified radius.
    // Arguments:
    //   origin     -
    //   radius     -
    //   pList      -
    //   physFlags  - is one or more of PhysicalEntityFlag.
    // See also:
    //   PhysicalEntityFlag
    virtual int GetPhysicalEntitiesInBox(const Vec3& origin, float radius, IPhysicalEntity**& pList, int physFlags = (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4)) const = 0;

    // Description:
    //   Retrieves host entity from the physical entity.
    virtual IEntity* GetEntityFromPhysics(IPhysicalEntity* pPhysEntity) const = 0;

    // Description:
    //   Adds the sink of the entity system. The sink is a class which implements IEntitySystemSink.
    // Arguments:
    //   sink - Pointer to the sink, must not be 0.
    //   subscription - combination of SinkEventSubscriptions flags specifying which events to receive
    // See also:
    //   IEntitySystemSink
    virtual void AddSink(IEntitySystemSink* sink, uint32 subscriptions, uint64 onEventSubscriptions) = 0;

    // Description:
    //   Removes listening sink from the entity system. The sink is a class which implements IEntitySystemSink.
    // Arguments:
    //   sink - Pointer to the sink, must not be 0.
    // See also:
    //   IEntitySystemSink
    virtual void RemoveSink(IEntitySystemSink* sink) = 0;

    // Description:
    //    Pause all entity timers.
    // Arguments:
    //    bPause - true to pause timer, false to resume.
    virtual void    PauseTimers(bool bPause, bool bResume = false) = 0;

    // Summary:
    //   Checks whether a given entity ID is already used.
    virtual bool IsIDUsed(EntityId nID) const = 0;

    // Description:
    //   Puts the memory statistics of the entities into the given sizer object
    //   according to the specifications in interface ICrySizer.
    virtual void GetMemoryStatistics(ICrySizer* pSizer) const = 0;

    // Summary:
    //   Gets pointer to original ISystem.
    virtual ISystem* GetSystem() const = 0;

    // Description:
    //   Extract entity load parameters from XML.
    // Arguments:
    //   entityNode - XML entity node
    //   spawnParams - entity spawn parameters
    virtual bool    ExtractEntityLoadParams(XmlNodeRef& entityNode, SEntitySpawnParams& spawnParams) const = 0;

    //   Creates entity from XML. To ensure that entity links work, call BeginCreateEntities
    //   before using CreateEntity, then call EndCreateEntities when you're done to setup
    //   all the links.
    // Arguments:
    //   entityNode - XML entity node
    //   spawnParams - entity spawn parameters
    //  outusingID - resulting ID
    virtual void BeginCreateEntities(int amtToCreate) = 0;
    virtual bool CreateEntity(XmlNodeRef& entityNode, SEntitySpawnParams& pParams, EntityId& outUsingId) = 0;
    virtual void EndCreateEntities() = 0;

    // Description:
    //    Loads entities exported from Editor.
    //    bIsLoadingLevelFile indicates if the loaded entities come from the original level file
    virtual void LoadEntities(XmlNodeRef& objectsNode, bool bIsLoadingLevelFile) = 0;
    virtual void LoadEntities(XmlNodeRef& objectsNode, bool bIsLoadingLevelFile, std::vector<IEntity*>* outGlobalEntityIds, std::vector<IEntity*>* outLocalEntityIds) = 0;

    // Description:
    //   During the loading process, don't add any entities from this layer to the world,
    //   and instead cache off their creation info for later use.
    // Arguments:
    //   pLayerName - The name of the top-level layer, will include any sub-layers too.
    virtual void HoldLayerEntities(const char* pLayerName) = 0;

    // Description:
    //   Add clones of held entities for the specified layer to the world.  This can be
    //   called multiple times for the same layer.
    // Arguments:
    //   localOffset - The point the entities are transformed relative to when they are added
    //                 (typically, the center of the layer).
    //   l2w - The transform to apply to all entities in the layer.
    //   pIncludeLayers - Optionally, an array of names of sub-layers to include when cloning (should only be NULL when numIncludeLayers is 0).
    //   numIncludeLayers - Number of included layers
    virtual void CloneHeldLayerEntities(const char* pLayerName, const Vec3& localOffset, const Matrix34& l2w, const char** pIncludeLayers = NULL, int numIncludeLayers = 0) = 0;

    // Description:
    //   Releases all the held entities.
    virtual void ReleaseHeldEntities() = 0;

    // Summary:
    //    Registers Entity Event`s listeners.
    virtual void AddEntityEventListener(EntityId nEntity, EEntityEvent event, IEntityEventListener* pListener) = 0;
    virtual void RemoveEntityEventListener(EntityId nEntity, EEntityEvent event, IEntityEventListener* pListener) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Entity GUIDs
    //////////////////////////////////////////////////////////////////////////

    // Summary:
    //   Finds entity by Entity GUID.
    virtual EntityId FindEntityByGuid(const EntityGUID& guid) const = 0;

    // Summary:
    //   Finds entity by editor GUID.  This is a special case for runtime prefabs, since
    //   they use the editor guids.  It is only valid to call in between
    //   BeginCreateEntities and EndCreateEntities.
    // Arguments:
    //   guid - The guid string (ie {ABCD1234-...}) of the entity required.
    virtual EntityId FindEntityByEditorGuid(const char* pGuid) const = 0;

    // Summary:
    //   Generates new entity id based on Entity GUID.
    virtual EntityId GenerateEntityIdFromGuid(const EntityGUID& guid) = 0;

    // Summary:
    //   Gets a pointer to access to area manager.
    virtual IAreaManager* GetAreaManager() const = 0;

    //////////////////////////////////////////////////////////////////////////
    // Description:
    //    Return the breakable manager interface.
    virtual IBreakableManager* GetBreakableManager() const = 0;

    //////////////////////////////////////////////////////////////////////////
    // Description:
    //   Returns the entity pool manager interface.
    virtual IEntityPoolManager* GetIEntityPoolManager() const = 0;

    //////////////////////////////////////////////////////////////////////////
    // Entity archetypes.
    //////////////////////////////////////////////////////////////////////////
    virtual IEntityArchetype* LoadEntityArchetype(XmlNodeRef oArchetype) = 0;
    virtual IEntityArchetype* LoadEntityArchetype(const char* sArchetype) = 0;
    virtual IEntityArchetype* CreateEntityArchetype(IEntityClass* pClass, const char* sArchetype) = 0;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Description:
    //     Serializes basic entity system members (timers etc. ) to/from a savegame;
    virtual void Serialize(TSerialize ser) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Description:
    //     Makes sure the next SpawnEntity will use the id provided (if it's in use, the current entity is deleted).
    virtual void SetNextSpawnId(EntityId id) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Description:
    //     Resets any area state for the specified entity.
    virtual void ResetAreas() = 0;

    //////////////////////////////////////////////////////////////////////////
    // Description:
    //     Unloads any area state for the specified entity.
    virtual void UnloadAreas() = 0;

    //////////////////////////////////////////////////////////////////////////
    // Description:
    //     Dumps entities in system.
    virtual void DumpEntities() = 0;

    //////////////////////////////////////////////////////////////////////////
    // Description:
    //     Do not spawn any entities unless forced to.
    virtual void LockSpawning(bool lock) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Description:
    //     Handles entity-related loading of a level
    virtual bool OnLoadLevel(const char* szLevelPath) = 0;

    // Add entity layer
    virtual CEntityLayer* AddLayer(const char* name, const char* parent, uint16 id, bool bHasPhysics, int specs, bool bDefaultLoaded) = 0;

    // Load layer infos
    virtual void LoadLayers(const char* dataFile) = 0;

    // Reorganize layer children
    virtual void LinkLayerChildren() = 0;

    // Add entity to entity layer
    virtual void AddEntityToLayer(const char* layer, EntityId id) = 0;

    // Remove entity from all layers
    virtual void RemoveEntityFromLayers(EntityId id) = 0;

    // Clear list of entity layers
    virtual void ClearLayers() = 0;

    // Enable all the default layers
    virtual void EnableDefaultLayers(bool isSerialized = true) = 0;

    // Enable entity layer
    virtual void EnableLayer(const char* layer, bool isEnable, bool isSerialized = true) = 0;

    // Is layer with given name enabled ?
    virtual bool IsLayerEnabled(const char* layer, bool bMustBeLoaded) const = 0;

    // Returns true if entity is not in a layer or the layer is enabled/serialized
    virtual bool ShouldSerializedEntity(IEntity* pEntity) = 0;

    // Register callbacks from Physics System
    virtual void RegisterPhysicCallbacks() = 0;
    virtual void UnregisterPhysicCallbacks() = 0;

    virtual void PurgeDeferredCollisionEvents(bool bForce = false) = 0;

    // Component APIs
    virtual void ComponentEnableEvent(const EntityId id, const int eventID, const bool enable) = 0;

    virtual void DebugDraw() = 0;


    // Get the layer index, -1 if not found
    virtual int GetLayerId(const char* szLayerName) const = 0;

    // Get the layer name, from an id
    virtual const char* GetLayerName(int layerId) const = 0;

    // Gets the number of children for a layer
    virtual int GetLayerChildCount(const char* szLayerName) const = 0;

    // Gets the name of the specified child layer
    virtual const char* GetLayerChild(const char* szLayerName, int childIdx) const = 0;

    // Get mask for visible layers, returns layer count
    virtual int GetVisibleLayerIDs(uint8* pLayerMask, const uint32 maxCount) const = 0;

    // Toggle layer visibility (in-game only), non recursive
    virtual void ToggleLayerVisibility(const char* layer, bool isEnabled, bool includeParent = true) = 0;

    // Toggle layer visibility for all layers containing pSearchSubstring with the exception of any with pExceptionSubstring
    virtual void ToggleLayersBySubstring(const char* pSearchSubstring, const char* pExceptionSubstring, bool isEnable) = 0;

    virtual IBSPTree3D* CreateBSPTree3D(const IBSPTree3D::FaceList& faceList) = 0;
    virtual void ReleaseBSPTree3D(IBSPTree3D*& pTree) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Components
    //////////////////////////////////////////////////////////////////////////

    //! Creates a component of given type, using the appropriate factory.
    //! The component is not registered on any entity and has not had
    //! its Initialize() called.
    //! \return The new component, or nullptr if creation failed.
    template<class TYPE>
    AZStd::shared_ptr<TYPE> CreateComponent();

    //! Creates a component of given type, using the appropriate factory.
    //! The component is then registered with the given entity and initialized.
    //! \return The new component, or nullptr if creation failed.
    template<typename TYPE>
    AZStd::shared_ptr<TYPE> CreateComponentAndRegister(const IComponent::SComponentInitializer& componentInitializer);

    // </interfuscator:shuffle>
};

class CryLegacyEntitySystemRequests
    : public AZ::EBusTraits
{
public:
    //////////////////////////////////////////////////////////////////////////
    // EBusTraits overrides
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Creates and initializes a legacy IEntitySystem instance
    virtual IEntitySystem* InitEntitySystem() = 0;

    //////////////////////////////////////////////////////////////////////////
    // Shuts down and destroys a legacy IEntitySystem instance
    virtual void ShutdownEntitySystem(IEntitySystem* entitySystem) = 0;
};
using CryLegacyEntitySystemRequestBus = AZ::EBus<CryLegacyEntitySystemRequests>;

extern "C"
{
CRYENTITYDLL_API struct IEntitySystem* CreateEntitySystem(ISystem* pISystem);
}

typedef struct IEntitySystem* (* PFNCREATEENTITYSYSTEM)(ISystem* pISystem);

//
// Implementation
//

template<class TYPE>
inline AZStd::shared_ptr<TYPE> IEntitySystem::CreateComponent()
{
    static_assert(HasTypeFunction<TYPE>::value, "Class must have DECLARE_COMPONENT_TYPE declaration to be constructable.");
    IComponentFactoryRegistry* registry = GetComponentFactoryRegistry();
    if (!registry)
    {
        CryWarning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_ERROR, "Cannot create components until ComponentFactoryRegistry is instantiated");
        return nullptr;
    }
    IComponentFactory<TYPE>* factory = registry->GetFactory<TYPE>();
    if (!factory)
    {
        CryWarning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_ERROR, "No factory found for component of type '%s'", TYPE::Type().Name());
        return nullptr;
    }
    return factory->CreateComponent();
}

template<typename TYPE>
inline AZStd::shared_ptr<TYPE> IEntitySystem::CreateComponentAndRegister(const IComponent::SComponentInitializer& componentInitializer)
{
    CRY_ASSERT(componentInitializer.m_pEntity);
    AZStd::shared_ptr<TYPE> pComponent(CreateComponent<TYPE>());
    if (!pComponent)
    {
        return nullptr;
    }

    if (!componentInitializer.m_pEntity->RegisterComponent(pComponent))
    {
        return nullptr;
    }

    pComponent->Initialize(componentInitializer);
    return pComponent;
}

#endif // CRYINCLUDE_CRYCOMMON_IENTITYSYSTEM_H
