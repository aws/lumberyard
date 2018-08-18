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

// Description : Handles management for loading of entities


#include "CryLegacy_precompiled.h"
#include "EntityLoadManager.h"
#include "EntityPoolManager.h"
#include "EntitySystem.h"
#include "Entity.h"
#include "EntityLayer.h"

#include "INetwork.h"
#include <Components/IComponentArea.h>
#include <Components/IComponentRope.h>
#include <Components/IComponentClipVolume.h>
#include <Components/IComponentFlowGraph.h>
#include "Components/IComponentSerialization.h"

//////////////////////////////////////////////////////////////////////////
CEntityLoadManager::CEntityLoadManager(CEntitySystem* pEntitySystem)
    : m_pEntitySystem(pEntitySystem)
{
    assert(m_pEntitySystem);
}

//////////////////////////////////////////////////////////////////////////
CEntityLoadManager::~CEntityLoadManager()
{
    stl::free_container(m_queuedAttachments);
    stl::free_container(m_queuedFlowgraphs);
}

//////////////////////////////////////////////////////////////////////////
void CEntityLoadManager::Reset()
{
    m_clonedLayerIds.clear();
}

//////////////////////////////////////////////////////////////////////////
bool CEntityLoadManager::LoadEntities(XmlNodeRef& entitiesNode, bool bIsLoadingLevelFile, std::vector<IEntity*>* outGlobalEntityIds, std::vector<IEntity*>* outLocalEntityIds)
{
    bool bResult = false;

    if (entitiesNode && ReserveEntityIds(entitiesNode))
    {
        PrepareBatchCreation(entitiesNode->getChildCount());

        bResult = ParseEntities(entitiesNode, bIsLoadingLevelFile, outGlobalEntityIds, outLocalEntityIds);

        OnBatchCreationCompleted();
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
void CEntityLoadManager::SetupHeldLayer(const char* pLayerName)
{
    // Look up the layer with this name.  Note that normally only layers that can be
    // dynamically loaded will be in here, but we have a hack in ObjectLayerManager to make
    // one of these for every layer.
    CEntityLayer* pLayer = m_pEntitySystem->FindLayer(pLayerName);

    // Walk up the layer tree until we find the top level parent.  That's what we group held
    // entities by.
    while (pLayer != NULL)
    {
        const string& parentName = pLayer->GetParentName();

        if (parentName.empty())
        {
            break;
        }

        pLayer = m_pEntitySystem->FindLayer(parentName.c_str());
    }

    if (!gEnv->IsEditor())
    {
        CRY_ASSERT_MESSAGE(pLayer != NULL, "All layers should be in the entity system, level may need to be reexported");
    }

    if (pLayer != NULL)
    {
        int heldLayerIdx = -1;

        // Go through our held layers, looking for one with the parent name
        for (size_t i = 0; i < m_heldLayers.size(); ++i)
        {
            if (m_heldLayers[i].m_layerName == pLayer->GetName())
            {
                heldLayerIdx = i;
                break;
            }
        }

        // Crc the original layer name and add it to the map.  If the layer is held we
        // store the index, or -1 if it isn't held.
        uint32 layerCrc = CCrc32::Compute(pLayerName);
        m_layerNameMap[layerCrc] = heldLayerIdx;
    }
}

bool CEntityLoadManager::IsHeldLayer(XmlNodeRef& entityNode)
{
    if (m_heldLayers.empty())
    {
        return false;
    }

    const char* pLayerName = entityNode->getAttr("Layer");

    uint32 layerCrc = CCrc32::Compute(pLayerName);

    std::map<uint32, int>::iterator it = m_layerNameMap.find(layerCrc);

    // First time we've seen this layer, cache off info for it
    if (it == m_layerNameMap.end())
    {
        SetupHeldLayer(pLayerName);
        it = m_layerNameMap.find(layerCrc);

        if (it == m_layerNameMap.end())
        {
            return false;
        }
    }

    int heldLayerIdx = it->second;

    // If this is a held layer, cache off the creation info and return true (don't add)
    if (heldLayerIdx != -1)
    {
        m_heldLayers[heldLayerIdx].m_entities.push_back(entityNode);
        return true;
    }
    else
    {
        return false;
    }
}

void CEntityLoadManager::HoldLayerEntities(const char* pLayerName)
{
    m_heldLayers.resize(m_heldLayers.size() + 1);
    SHeldLayer& heldLayer = m_heldLayers[m_heldLayers.size() - 1];

    heldLayer.m_layerName = pLayerName;
}

void CEntityLoadManager::CloneHeldLayerEntities(const char* pLayerName, const Vec3& localOffset, const Matrix34& l2w, const char** pIncludeLayers, int numIncludeLayers)
{
    int heldLayerIdx = -1;

    // Get the index of the held layer
    for (size_t i = 0; i < m_heldLayers.size(); ++i)
    {
        if (m_heldLayers[i].m_layerName == pLayerName)
        {
            heldLayerIdx = i;
            break;
        }
    }

    if (heldLayerIdx == -1)
    {
        CRY_ASSERT_MESSAGE(0, "Trying to add a layer that wasn't held");
        return;
    }

    // Allocate a map for storing the mapping from the original entity ids to the cloned ones
    int cloneIdx = (int)m_clonedLayerIds.size();
    m_clonedLayerIds.resize(cloneIdx + 1);
    TClonedIds& clonedIds = m_clonedLayerIds[cloneIdx];

    // Get all the held entities for this layer
    std::vector<XmlNodeRef>& entities = m_heldLayers[heldLayerIdx].m_entities;

    CEntityPoolManager* pEntityPoolManager = m_pEntitySystem->GetEntityPoolManager();
    assert(pEntityPoolManager);
    const bool bEnablePoolUse = pEntityPoolManager->IsUsingPools();

    const int nSize = entities.size();
    for (int i = 0; i < nSize; i++)
    {
        XmlNodeRef entityNode = entities[i];

        const char* layerName = entityNode->getAttr("Layer");

        bool isIncluded = (numIncludeLayers > 0) ? false : true;

        // Check if this layer is in our include list
        for (int j = 0; j < numIncludeLayers; ++j)
        {
            if (strcmp(layerName, pIncludeLayers[j]) == 0)
            {
                isIncluded = true;
                break;
            }
        }

        if (!isIncluded)
        {
            continue;
        }

        //////////////////////////////////////////////////////////////////////////
        //
        // Copy/paste from CEntityLoadManager::ParseEntities
        //
        INDENT_LOG_DURING_SCOPE (true, "Parsing entity '%s'", entityNode->getAttr("Name"));

        bool bSuccess = false;
        SEntityLoadParams loadParams;
        if (ExtractEntityLoadParams(entityNode, loadParams, true))
        {
            // ONLY REAL CHANGES ////////////////////////////////////////////////////
            Matrix34 local(Matrix33(loadParams.spawnParams.qRotation));
            local.SetTranslation(loadParams.spawnParams.vPosition - localOffset);
            Matrix34 world = l2w * local;

            // If this entity has a parent, keep the transform local
            EntityId parentId;
            if (entityNode->getAttr("ParentId", parentId))
            {
                local.SetTranslation(loadParams.spawnParams.vPosition);
                world = local;
            }

            loadParams.spawnParams.vPosition = world.GetTranslation();
            loadParams.spawnParams.qRotation = Quat(world);

            EntityId origId = loadParams.spawnParams.id;
            loadParams.spawnParams.id = m_pEntitySystem->GenerateEntityId(true);

            loadParams.clonedLayerId = cloneIdx;


            //////////////////////////////////////////////////////////////////////////

            if (bEnablePoolUse && loadParams.spawnParams.bCreatedThroughPool)
            {
                CEntityPoolManager* pPoolManager = m_pEntitySystem->GetEntityPoolManager();
                bSuccess = (pPoolManager && pPoolManager->AddPoolBookmark(loadParams));
            }

            // Default to just creating the entity
            if (!bSuccess)
            {
                EntityId usingId = 0;

                // if we just want to reload this entity's properties
                if (entityNode->haveAttr("ReloadProperties"))
                {
                    EntityId id;

                    entityNode->getAttr("EntityId", id);
                    loadParams.pReuseEntity = m_pEntitySystem->GetCEntityFromID(id);
                }

                bSuccess = CreateEntity(loadParams, usingId, true);
            }

            if (!bSuccess)
            {
                string sName = entityNode->getAttr("Name");
                EntityWarning("CEntityLoadManager::ParseEntities : Failed when parsing entity \'%s\'", sName.empty() ? "Unknown" : sName.c_str());
            }
            else
            {
                // If we successfully cloned the entity, save the mapping from original id to cloned
                clonedIds[origId] = loadParams.spawnParams.id;
                m_clonedEntitiesTemp.push_back(loadParams.spawnParams.id);
            }
        }
        //
        // End copy/paste
        //
        //////////////////////////////////////////////////////////////////////////
    }

    // All attachment parent ids will be for the source copy, so we need to remap that id
    // to the cloned one.
    TQueuedAttachments::iterator itQueuedAttachment = m_queuedAttachments.begin();
    TQueuedAttachments::iterator itQueuedAttachmentEnd = m_queuedAttachments.end();
    for (; itQueuedAttachment != itQueuedAttachmentEnd; ++itQueuedAttachment)
    {
        SEntityAttachment& entityAttachment = *itQueuedAttachment;
        entityAttachment.parent = m_pEntitySystem->GetClonedEntityId(entityAttachment.parent, entityAttachment.child);
    }

    // Now that all the cloned ids are ready, go though all the entities we just cloned
    // and update the ids for any entity links.
    for (std::vector<EntityId>::iterator it = m_clonedEntitiesTemp.begin(); it != m_clonedEntitiesTemp.end(); ++it)
    {
        IEntity* pEntity = m_pEntitySystem->GetEntity(*it);
        if (pEntity != NULL)
        {
            IEntityLink* pLink = pEntity->GetEntityLinks();
            while (pLink != NULL)
            {
                pLink->entityId = m_pEntitySystem->GetClonedEntityId(pLink->entityId, *it);
                pLink = pLink->next;
            }
        }
    }
    m_clonedEntitiesTemp.clear();

    OnBatchCreationCompleted();
}

void CEntityLoadManager::ReleaseHeldEntities()
{
    m_heldLayers.clear();
    m_layerNameMap.clear();
}

//////////////////////////////////////////////////////////////////////////
//bool CEntityLoadManager::CreateEntities(TEntityLoadParamsContainer &container)
//{
//  bool bResult = container.empty();
//
//  if (!bResult)
//  {
//      PrepareBatchCreation(container.size());
//
//      bResult = true;
//      TEntityLoadParamsContainer::iterator itLoadParams = container.begin();
//      TEntityLoadParamsContainer::iterator itLoadParamsEnd = container.end();
//      for (; itLoadParams != itLoadParamsEnd; ++itLoadParams)
//      {
//          bResult &= CreateEntity(*itLoadParams);
//      }
//
//      OnBatchCreationCompleted();
//  }
//
//  return bResult;
//}

//////////////////////////////////////////////////////////////////////////
bool CEntityLoadManager::ReserveEntityIds(XmlNodeRef& entitiesNode)
{
    assert(entitiesNode);

    bool bResult = false;

    // Reserve the Ids to coop with dynamic entity spawning that may happen during this stage
    const int iChildCount = (entitiesNode ? entitiesNode->getChildCount() : 0);
    for (int i = 0; i < iChildCount; ++i)
    {
        XmlNodeRef entityNode = entitiesNode->getChild(i);
        if (entityNode && entityNode->isTag("Entity"))
        {
            EntityId entityId;
            EntityGUID guid;
            if (entityNode->getAttr("EntityId", entityId))
            {
                m_pEntitySystem->ReserveEntityId(entityId);
                bResult = true;
            }
            else if (entityNode->getAttr("EntityGuid", guid))
            {
                bResult = true;
            }
            else
            {
                // entity has no ID assigned
                bResult = true;
            }
        }
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityLoadManager::CanParseEntity(XmlNodeRef& entityNode, std::vector<IEntity*>* outGlobalEntityIds)
{
    assert(entityNode);

    bool bResult = true;
    if (!entityNode)
    {
        return bResult;
    }

    int nMinSpec = -1;
    if (entityNode->getAttr("MinSpec", nMinSpec) && nMinSpec > 0)
    {
        static ICVar* e_obj_quality(gEnv->pConsole->GetCVar("e_ObjQuality"));
        int obj_quality = (e_obj_quality ? e_obj_quality->GetIVal() : 0);

        // If the entity minimal spec is higher then the current server object quality this entity will not be loaded.
        bResult = (obj_quality >= nMinSpec || obj_quality == 0);
    }

    if (bResult)
    {
        const char* pLayerName = entityNode->getAttr("Layer");
        CEntityLayer* pLayer = m_pEntitySystem->FindLayer(pLayerName);

        if (pLayer)
        {
            bResult = !pLayer->IsSkippedBySpec();
        }
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityLoadManager::ParseEntities(XmlNodeRef& entitiesNode, bool bIsLoadingLevelFile, std::vector<IEntity*>* outGlobalEntityIds, std::vector<IEntity*>* outLocalEntityIds)
{
#if !defined(SYS_ENV_AS_STRUCT)
    assert(gEnv);
    PREFAST_ASSUME(gEnv);
#endif

    assert(entitiesNode);

    bool bResult = true;

    CEntityPoolManager* pEntityPoolManager = m_pEntitySystem->GetEntityPoolManager();
    assert(pEntityPoolManager);
    const bool bEnablePoolUse = pEntityPoolManager->IsUsingPools();

    const int iChildCount = entitiesNode->getChildCount();

    CryLog ("Parsing %u entities...", iChildCount);
    INDENT_LOG_DURING_SCOPE();

    for (int i = 0; i < iChildCount; ++i)
    {
        //Update loading screen and important tick functions
        SYNCHRONOUS_LOADING_TICK();

        XmlNodeRef entityNode = entitiesNode->getChild(i);
        if (entityNode && entityNode->isTag("Entity") && CanParseEntity(entityNode, outGlobalEntityIds))
        {
            // Create entities only if they are not in an held layer and we are not in editor game mode.
            if (!IsHeldLayer(entityNode) && !gEnv->IsEditorGameMode())
            {
                INDENT_LOG_DURING_SCOPE (true, "Parsing entity '%s'", entityNode->getAttr("Name"));

                bool bSuccess = false;
                SEntityLoadParams loadParams;
                if (ExtractEntityLoadParams(entityNode, loadParams, true))
                {
                    if (bEnablePoolUse && loadParams.spawnParams.bCreatedThroughPool)
                    {
                        CEntityPoolManager* pPoolManager = m_pEntitySystem->GetEntityPoolManager();
                        bSuccess = (pPoolManager && pPoolManager->AddPoolBookmark(loadParams));
                    }

                    // Default to just creating the entity
                    if (!bSuccess)
                    {
                        EntityId usingId = 0;

                        // if we just want to reload this entity's properties
                        if (entityNode->haveAttr("ReloadProperties"))
                        {
                            EntityId id;

                            entityNode->getAttr("EntityId", id);
                            loadParams.pReuseEntity = m_pEntitySystem->GetCEntityFromID(id);
                        }

                        bSuccess = CreateEntity(loadParams, usingId, bIsLoadingLevelFile);
                    }
                }

                if (!bSuccess)
                {
                    string sName = entityNode->getAttr("Name");
                    EntityWarning("CEntityLoadManager::ParseEntities : Failed when parsing entity \'%s\'", sName.empty() ? "Unknown" : sName.c_str());
                }
                bResult &= bSuccess;
            }
        }
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityLoadManager::ExtractEntityLoadParams(XmlNodeRef& entityNode, SEntityLoadParams& outLoadParams, bool bWarningMsg) const
{
    assert(entityNode);

    bool bResult = true;

    const char* sEntityClass = entityNode->getAttr("EntityClass");
    const char* sEntityName = entityNode->getAttr("Name");
    IEntityClass* pClass = m_pEntitySystem->GetClassRegistry()->FindClass(sEntityClass);
    if (pClass)
    {
        SEntitySpawnParams& spawnParams = outLoadParams.spawnParams;
        outLoadParams.spawnParams.entityNode = entityNode;

        // Load spawn parameters from xml node.
        spawnParams.pClass = pClass;
        spawnParams.sName = sEntityName;
        spawnParams.sLayerName = entityNode->getAttr("Layer");

        // Entities loaded from the xml cannot be fully deleted in single player.
        if (!gEnv->bMultiplayer)
        {
            spawnParams.nFlags |= ENTITY_FLAG_UNREMOVABLE;
        }

        Vec3 pos(Vec3_Zero);
        Quat rot(Quat::CreateIdentity());
        Vec3 scale(Vec3_One);

        entityNode->getAttr("Pos", pos);
        entityNode->getAttr("Rotate", rot);
        entityNode->getAttr("Scale", scale);

        /*Ang3 vAngles;
        if (entityNode->getAttr("Angles", vAngles))
        {
            spawnParams.qRotation.SetRotationXYZ(vAngles);
        }*/

        spawnParams.vPosition = pos;
        spawnParams.qRotation = rot;
        spawnParams.vScale = scale;

        spawnParams.id = 0;
        entityNode->getAttr("EntityId", spawnParams.id);
        entityNode->getAttr("EntityGuid", spawnParams.guid);

        // Get flags.
        //bool bRecvShadow = true; // true by default (do not change, it must be coordinated with editor export)
        bool bGoodOccluder = false; // false by default (do not change, it must be coordinated with editor export)
        bool bOutdoorOnly = false;
        bool bNoDecals = false;
        int  nCastShadowMinSpec = CONFIG_LOW_SPEC;

        entityNode->getAttr("CastShadowMinSpec", nCastShadowMinSpec);
        //entityNode->getAttr("RecvShadow", bRecvShadow);
        entityNode->getAttr("GoodOccluder", bGoodOccluder);
        entityNode->getAttr("OutdoorOnly", bOutdoorOnly);
        entityNode->getAttr("NoDecals", bNoDecals);

        static ICVar* pObjShadowCastSpec = gEnv->pConsole->GetCVar("e_ObjShadowCastSpec");
        if (nCastShadowMinSpec <= pObjShadowCastSpec->GetIVal())
        {
            spawnParams.nFlags |= ENTITY_FLAG_CASTSHADOW;
        }

        //if (bRecvShadow)
        //spawnParams.nFlags |= ENTITY_FLAG_RECVSHADOW;
        if (bGoodOccluder)
        {
            spawnParams.nFlags |= ENTITY_FLAG_GOOD_OCCLUDER;
        }
        if (bOutdoorOnly)
        {
            spawnParams.nFlags |= ENTITY_FLAG_OUTDOORONLY;
        }
        if (bNoDecals)
        {
            spawnParams.nFlags |= ENTITY_FLAG_NO_DECALNODE_DECALS;
        }

        const char* sArchetypeName = entityNode->getAttr("Archetype");
        if (sArchetypeName && sArchetypeName[0])
        {
            MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Other, 0, "%s", sArchetypeName);
            spawnParams.pArchetype = m_pEntitySystem->LoadEntityArchetype(sArchetypeName);

            if (!spawnParams.pArchetype)
            {
                EntityWarning("Archetype %s used by entity %s cannot be found! Entity cannot be loaded.", sArchetypeName, spawnParams.sName);
                bResult = false;
            }
        }

        entityNode->getAttr("CreatedThroughPool", spawnParams.bCreatedThroughPool);
        if (!spawnParams.bCreatedThroughPool)
        {
            // Check if forced via its class
            CEntityPoolManager* pPoolManager = m_pEntitySystem->GetEntityPoolManager();
            spawnParams.bCreatedThroughPool = (pPoolManager && pPoolManager->IsClassForcedBookmarked(pClass));
        }
    }
    else    // No entity class found!
    {
        if (bWarningMsg)
        {
            EntityWarning("Entity class %s used by entity %s cannot be found! Entity cannot be loaded.", sEntityClass, sEntityName);
        }
        bResult = false;
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityLoadManager::ExtractEntityLoadParams(XmlNodeRef& entityNode, SEntitySpawnParams& spawnParams) const
{
    SEntityLoadParams loadParams;
    bool bRes = ExtractEntityLoadParams(entityNode, loadParams, false);
    spawnParams = loadParams.spawnParams;
    return(bRes);
}

//////////////////////////////////////////////////////////////////////////
bool CEntityLoadManager::CreateEntity(XmlNodeRef& entityNode, SEntitySpawnParams& pParams, EntityId& outUsingId)
{
    SEntityLoadParams loadParams;
    loadParams.spawnParams = pParams;
    loadParams.spawnParams.entityNode = entityNode;

    if ((loadParams.spawnParams.id == 0) &&
        ((loadParams.spawnParams.pClass->GetFlags() & ECLF_DO_NOT_SPAWN_AS_STATIC) == 0))
    {
        // If ID is not set we generate a static ID.
        loadParams.spawnParams.id = m_pEntitySystem->GenerateEntityId(true);
    }
    return(CreateEntity(loadParams, outUsingId, true));
}

//////////////////////////////////////////////////////////////////////////
bool CEntityLoadManager::CreateEntity(SEntityLoadParams& loadParams, EntityId& outUsingId, bool bIsLoadingLevellFile)
{
    MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Entity, 0, "Entity %s", loadParams.spawnParams.pClass->GetName());

    bool bResult = true;
    outUsingId = 0;

    XmlNodeRef& entityNode = loadParams.spawnParams.entityNode;
    SEntitySpawnParams& spawnParams = loadParams.spawnParams;

    uint32 entityGuid = 0;
    if (entityNode)
    {
        // Only runtime prefabs should have GUID id's
        const char* entityGuidStr = entityNode->getAttr("Id");
        if (entityGuidStr[0] != '\0')
        {
            entityGuid = CCrc32::ComputeLowercase(entityGuidStr);
        }
    }

    IEntity* pSpawnedEntity = NULL;
    bool bWasSpawned = false;
    if (loadParams.pReuseEntity)
    {
        // Attempt to reload
        pSpawnedEntity = (loadParams.pReuseEntity->ReloadEntity(loadParams) ? loadParams.pReuseEntity : NULL);
    }
    else if (m_pEntitySystem->OnBeforeSpawn(spawnParams))
    {
        // Create a new one
        pSpawnedEntity = m_pEntitySystem->SpawnEntity(spawnParams, false);
        bWasSpawned = true;
    }

    if (bResult && pSpawnedEntity)
    {
        m_pEntitySystem->AddEntityToLayer(spawnParams.sLayerName, pSpawnedEntity->GetId());

        pSpawnedEntity->SetLoadedFromLevelFile(bIsLoadingLevellFile);
        pSpawnedEntity->SetCloneLayerId(loadParams.clonedLayerId);

        const char* szMtlName(NULL);

        if (spawnParams.pArchetype)
        {
            IScriptTable*   pArchetypeProperties = spawnParams.pArchetype->GetProperties();
            if (pArchetypeProperties)
            {
                pArchetypeProperties->GetValue("PrototypeMaterial", szMtlName);
            }
        }

        if (entityNode)
        {
            // Create needed proxies
            if (entityNode->findChild("Area"))
            {
                pSpawnedEntity->GetOrCreateComponent<IComponentArea>();
            }
            if (entityNode->findChild("Rope"))
            {
                pSpawnedEntity->GetOrCreateComponent<IComponentRope>();
            }
            if (entityNode->findChild("ClipVolume"))
            {
                pSpawnedEntity->GetOrCreateComponent<IComponentClipVolume>();
            }

            if (spawnParams.pClass)
            {
                const char* pClassName = spawnParams.pClass->GetName();
                if (pClassName && !strcmp(pClassName, "Light"))
                {
                    IComponentRenderPtr pRP = pSpawnedEntity->GetOrCreateComponent<IComponentRender>();
                    if (pRP)
                    {
                        if (IComponentSerializationPtr serializationComponent = pSpawnedEntity->GetComponent<IComponentSerialization>())
                        {
                            serializationComponent->SerializeXML(entityNode, true);
                        }

                        int nMinSpec = -1;
                        if (entityNode->getAttr("MinSpec", nMinSpec) && nMinSpec >= 0)
                        {
                            pRP->GetRenderNode()->SetMinSpec(nMinSpec);
                        }
                    }
                }
            }

            // If we have an instance material, we use it...
            if (entityNode->haveAttr("Material"))
            {
                szMtlName = entityNode->getAttr("Material");
            }

            // Prepare the entity from Xml if it was just spawned
            if (pSpawnedEntity && bWasSpawned)
            {
                if (IEntityPropertyHandler* pPropertyHandler = pSpawnedEntity->GetClass()->GetPropertyHandler())
                {
                    pPropertyHandler->LoadEntityXMLProperties(pSpawnedEntity, entityNode);
                }

                if (IEntityEventHandler* pEventHandler = pSpawnedEntity->GetClass()->GetEventHandler())
                {
                    pEventHandler->LoadEntityXMLEvents(pSpawnedEntity, entityNode);
                }

                // Serialize script component.
                if (IComponentSerializationPtr serializationComponent = pSpawnedEntity->GetComponent<IComponentSerialization>())
                {
                    serializationComponent->SerializeXMLOnly(entityNode, true, { CComponentScript::Type() });
                }
            }
        }

        // If any material has to be set...
        if (szMtlName && *szMtlName != 0)
        {
            // ... we load it...
            _smart_ptr<IMaterial> pMtl = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(szMtlName);
            if (pMtl)
            {
                // ... and set it...
                pSpawnedEntity->SetMaterial(pMtl);
            }
        }

        if (bWasSpawned)
        {
            const bool bInited = m_pEntitySystem->InitEntity(pSpawnedEntity, spawnParams);
            if (!bInited)
            {
                // Failed to initialise an entity, need to bail or we'll crash
                return true;
            }
        }
        else
        {
            m_pEntitySystem->OnEntityReused(pSpawnedEntity, spawnParams);

            if (pSpawnedEntity && loadParams.bCallInit)
            {
                IComponentScriptPtr scriptComponent = pSpawnedEntity->GetComponent<IComponentScript>();
                if (scriptComponent)
                {
                    scriptComponent->CallInitEvent(true);
                }
            }
        }

        if (entityNode)
        {
            //////////////////////////////////////////////////////////////////////////
            // Load geom entity (Must be before serializing proxies.
            //////////////////////////////////////////////////////////////////////////
            if (spawnParams.pClass->GetFlags() & ECLF_DEFAULT)
            {
                // Check if it have geometry.
                const char* sGeom = entityNode->getAttr("Geometry");
                if (sGeom[0] != 0)
                {
                    // check if character.
                    const char* ext = PathUtil::GetExt(sGeom);
                    if (_stricmp(ext, CRY_SKEL_FILE_EXT) == 0 || _stricmp(ext, CRY_CHARACTER_DEFINITION_FILE_EXT) == 0 || _stricmp(ext, CRY_ANIM_GEOMETRY_FILE_EXT) == 0)
                    {
                        pSpawnedEntity->LoadCharacter(0, sGeom, IEntity::EF_AUTO_PHYSICALIZE);
                    }
                    else
                    {
                        pSpawnedEntity->LoadGeometry(0, sGeom, 0, IEntity::EF_AUTO_PHYSICALIZE);
                    }
                }
            }
            //////////////////////////////////////////////////////////////////////////

            // Serialize all entity components except Script components after initialization.
            if (pSpawnedEntity)
            {
                pSpawnedEntity->SerializeXML_ExceptScriptComponent(entityNode, true);
            }

            const char* attachmentType = entityNode->getAttr("AttachmentType");
            const char* attachmentTarget = entityNode->getAttr("AttachmentTarget");

            int flags = 0;
            if (strcmp(attachmentType, "GeomCacheNode") == 0)
            {
                flags |= IEntity::ATTACHMENT_GEOMCACHENODE;
            }
            else if (strcmp(attachmentType, "CharacterBone") == 0)
            {
                flags |= IEntity::ATTACHMENT_CHARACTERBONE;
            }

            // Add attachment to parent.
            if (entityGuid == 0)
            {
                EntityId nParentId = 0;
                if (entityNode->getAttr("ParentId", nParentId))
                {
                    AddQueuedAttachment(nParentId, 0, spawnParams.id, spawnParams.vPosition, spawnParams.qRotation, spawnParams.vScale, false, flags, attachmentTarget);
                }
            }
            else
            {
                const char* pParentGuid = entityNode->getAttr("Parent");
                if (pParentGuid[0] != '\0')
                {
                    uint32 parentGuid = CCrc32::ComputeLowercase(pParentGuid);
                    AddQueuedAttachment((EntityId)parentGuid, 0, spawnParams.id, spawnParams.vPosition, spawnParams.qRotation, spawnParams.vScale, true, flags, attachmentTarget);
                }
            }

            // check for a flow graph
            // only store them for later serialization as the FG proxy relies
            // on all EntityGUIDs already loaded
            if (entityNode->findChild("FlowGraph"))
            {
                AddQueuedFlowgraph(pSpawnedEntity, entityNode);
            }

            // Load entity links.
            XmlNodeRef linksNode = entityNode->findChild("EntityLinks");
            if (linksNode)
            {
                const int iChildCount = linksNode->getChildCount();
                for (int i = 0; i < iChildCount; ++i)
                {
                    XmlNodeRef linkNode = linksNode->getChild(i);
                    if (linkNode)
                    {
                        if (entityGuid == 0)
                        {
                            EntityId targetId = 0;
                            linkNode->getAttr("TargetId", targetId);

                            const char* sLinkName = linkNode->getAttr("Name");
                            Quat relRot(IDENTITY);
                            Vec3 relPos(IDENTITY);

                            pSpawnedEntity->AddEntityLink(sLinkName, targetId);
                        }
                        else
                        {
                            // If this is a runtime prefab we're spawning, queue the entity
                            // link for later, since it has a guid target id we need to look up.
                            AddQueuedEntityLink(pSpawnedEntity, linkNode);
                        }
                    }
                }
            }

            // Hide entity in game. Done after potential render component is created, so it catches the Hide
            if (bWasSpawned)
            {
                bool bHiddenInGame = false;
                entityNode->getAttr("HiddenInGame", bHiddenInGame);
                if (bHiddenInGame)
                {
                    pSpawnedEntity->Hide(true);
                }
            }

            int nMinSpec = -1;
            if (entityNode->getAttr("MinSpec", nMinSpec) && nMinSpec >= 0)
            {
                if (IComponentRenderPtr pRenderComponent = pSpawnedEntity->GetComponent<IComponentRender>())
                {
                    pRenderComponent->GetRenderNode()->SetMinSpec(nMinSpec);
                }
            }
        }
    }

    if (!bResult)
    {
        EntityWarning("[CEntityLoadManager::CreateEntity] Entity Load Failed: %s (%s)", spawnParams.sName, spawnParams.pClass->GetName());
    }

    outUsingId = (pSpawnedEntity ? pSpawnedEntity->GetId() : 0);

    if (outUsingId != 0 && entityGuid != 0)
    {
        m_guidToId[entityGuid] = outUsingId;
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
void CEntityLoadManager::PrepareBatchCreation(int nSize)
{
    m_queuedAttachments.clear();
    m_queuedFlowgraphs.clear();

    m_queuedAttachments.reserve(nSize);
    m_queuedFlowgraphs.reserve(nSize);
    m_guidToId.clear();
}

//////////////////////////////////////////////////////////////////////////
void CEntityLoadManager::AddQueuedAttachment(EntityId nParent, EntityGUID nParentGuid, EntityId nChild, const Vec3& pos, const Quat& rot, const Vec3& scale, bool guid, const int flags, const char* target)
{
    SEntityAttachment entityAttachment;
    entityAttachment.child = nChild;
    entityAttachment.parent = nParent;
    entityAttachment.parentGuid = nParentGuid;
    entityAttachment.pos = pos;
    entityAttachment.rot = rot;
    entityAttachment.scale = scale;
    entityAttachment.guid = guid;
    entityAttachment.flags = flags;
    entityAttachment.target = target;

    m_queuedAttachments.push_back(entityAttachment);
}

//////////////////////////////////////////////////////////////////////////
void CEntityLoadManager::AddQueuedFlowgraph(IEntity* pEntity, XmlNodeRef& pNode)
{
    SQueuedFlowGraph f;
    f.pEntity = pEntity;
    f.pNode = pNode;

    m_queuedFlowgraphs.push_back(f);
}

//////////////////////////////////////////////////////////////////////////
void CEntityLoadManager::AddQueuedEntityLink(IEntity* pEntity, XmlNodeRef& pNode)
{
    SQueuedFlowGraph f;
    f.pEntity = pEntity;
    f.pNode = pNode;

    m_queuedEntityLinks.push_back(f);
}

//////////////////////////////////////////////////////////////////////////
void CEntityLoadManager::OnBatchCreationCompleted()
{
    CEntityPoolManager* pEntityPoolManager = m_pEntitySystem->GetEntityPoolManager();
    assert(pEntityPoolManager);

    // Load attachments
    TQueuedAttachments::iterator itQueuedAttachment = m_queuedAttachments.begin();
    TQueuedAttachments::iterator itQueuedAttachmentEnd = m_queuedAttachments.end();
    for (; itQueuedAttachment != itQueuedAttachmentEnd; ++itQueuedAttachment)
    {
        const SEntityAttachment& entityAttachment = *itQueuedAttachment;

        IEntity* pChild = m_pEntitySystem->GetEntity(entityAttachment.child);
        if (pChild)
        {
            EntityId parentId = entityAttachment.parent;
            if (entityAttachment.guid)
            {
                parentId = m_guidToId[(uint32)entityAttachment.parent];
            }
            IEntity* pParent = m_pEntitySystem->GetEntity(parentId);
            if (pParent)
            {
                SChildAttachParams attachParams(entityAttachment.flags, entityAttachment.target.c_str());
                pParent->AttachChild(pChild, attachParams);
                pChild->SetLocalTM(Matrix34::Create(entityAttachment.scale, entityAttachment.rot, entityAttachment.pos));
            }
            else if (pEntityPoolManager->IsEntityBookmarked(entityAttachment.parent))
            {
                pEntityPoolManager->AddAttachmentToBookmark(entityAttachment.parent, entityAttachment);
            }
        }
    }
    m_queuedAttachments.clear();

    // Load flowgraphs
    TQueuedFlowgraphs::iterator itQueuedFlowgraph = m_queuedFlowgraphs.begin();
    TQueuedFlowgraphs::iterator itQueuedFlowgraphEnd = m_queuedFlowgraphs.end();
    for (; itQueuedFlowgraph != itQueuedFlowgraphEnd; ++itQueuedFlowgraph)
    {
        SQueuedFlowGraph& f = *itQueuedFlowgraph;

        if (f.pEntity)
        {
            IComponentPtr component = f.pEntity->GetOrCreateComponent<IComponentFlowGraph>();
            if (component)
            {
                if (IComponentSerializationPtr serializationComponent = f.pEntity->GetComponent<IComponentSerialization>())
                {
                    serializationComponent->SerializeXML(f.pNode, true);
                }
            }
        }
    }
    m_queuedFlowgraphs.clear();

    // Load entity links
    TQueuedFlowgraphs::iterator itQueuedEntityLink = m_queuedEntityLinks.begin();
    TQueuedFlowgraphs::iterator itQueuedEntityLinkEnd = m_queuedEntityLinks.end();
    for (; itQueuedEntityLink != itQueuedEntityLinkEnd; ++itQueuedEntityLink)
    {
        SQueuedFlowGraph& f = *itQueuedEntityLink;

        if (f.pEntity)
        {
            const char* targetGuidStr = f.pNode->getAttr("TargetId");
            if (targetGuidStr[0] != '\0')
            {
                EntityId targetId = FindEntityByEditorGuid(targetGuidStr);

                const char* sLinkName = f.pNode->getAttr("Name");
                Quat relRot(IDENTITY);
                Vec3 relPos(IDENTITY);

                f.pEntity->AddEntityLink(sLinkName, targetId, 0);
            }
        }
    }
    stl::free_container(m_queuedEntityLinks);
    stl::free_container(m_guidToId);
}

//////////////////////////////////////////////////////////////////////////
void CEntityLoadManager::ResolveLinks()
{
}

//////////////////////////////////////////////////////////////////////////
EntityId CEntityLoadManager::GetClonedId(int clonedLayerId, EntityId originalId)
{
    if (clonedLayerId >= 0 && clonedLayerId < (int)m_clonedLayerIds.size())
    {
        TClonedIds& clonedIds = m_clonedLayerIds[clonedLayerId];
        return clonedIds[originalId];
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
EntityId CEntityLoadManager::FindEntityByEditorGuid(const char* pGuid) const
{
    uint32 guidCrc = CCrc32::ComputeLowercase(pGuid);
    TGuidToId::const_iterator it = m_guidToId.find(guidCrc);
    if (it != m_guidToId.end())
    {
        return it->second;
    }

    return INVALID_ENTITYID;
}

