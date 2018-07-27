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
#include "CryLegacy_precompiled.h"
#include "PrefabManager.h"
#include "RuntimePrefab.h"
#include <Components/IComponentRender.h>
#include <AzCore/Casting/numeric_cast.h>

//////////////////////////////////////////////////////////////////////////
CRuntimePrefab::CRuntimePrefab(EntityId id)
    : m_id(id)
    , m_sourcePrefab(nullptr)
{
    m_worldToLocal.SetIdentity();
}

//////////////////////////////////////////////////////////////////////////
CRuntimePrefab::~CRuntimePrefab()
{
    Clear();
}

//////////////////////////////////////////////////////////////////////////
void CRuntimePrefab::Clear()
{
    m_spawnedPrefabs.clear();

    for (const EntityId& id : m_spawnedIds)
    {
        if (id != 0)
        {
            gEnv->pEntitySystem->RemoveEntity(id);
        }
    }
    m_spawnedIds.clear();

    for (IRenderNode* node : m_spawnedNodes)
    {
        if (node)
        {
            IStatObj* statObj = node->GetEntityStatObj();
            node->SetEntityStatObj(0, 0, 0);
            if (statObj)
            {
                statObj->Release();
            }
            gEnv->p3DEngine->DeleteRenderNode(node);
        }
    }
    m_spawnedNodes.clear();

    m_sourcePrefab.reset();
}

//////////////////////////////////////////////////////////////////////////
void CRuntimePrefab::SpawnEntities(const std::shared_ptr<CPrefab>& prefab, const Matrix34& matSource, AABB& box)
{
    gEnv->pEntitySystem->BeginCreateEntities(prefab->m_entityParamsList.size());

    m_spawnedIds.reserve(prefab->m_entityParamsList.size());
    for (EntityParams& entityParams : prefab->m_entityParamsList)
    {
        entityParams.m_spawnParams.sLayerName = nullptr;

        // world TM
        Matrix34 worldTransform = matSource * entityParams.m_worldMatrix;

        worldTransform.OrthonormalizeFast();
        SEntitySpawnParams spawnParams = entityParams.m_spawnParams;
        spawnParams.vPosition = worldTransform.GetTranslation();
        spawnParams.qRotation = Quat(worldTransform);

        spawnParams.id = 0;
        // need to clear unremovable flag otherwise desc actors don't work in Editor game mode.
        spawnParams.nFlags &= ~ENTITY_FLAG_UNREMOVABLE;

        EntityId id = 0;
        gEnv->pEntitySystem->CreateEntity(entityParams.m_entityNode, spawnParams, id);

        IEntity* entity = gEnv->pEntitySystem->GetEntity(id);
        if (entity)
        {
            // calc bbox for the editor because we spawn separate entities
            Matrix34 entityTransform;
            entityTransform.Set(Vec3_One, Quat::CreateIdentity(), entityParams.m_spawnParams.vPosition);
            AABB entityBounds;
            entity->GetLocalBounds(entityBounds);     // seems like localbounds for the entity already have scale and rotation component included
            entityBounds.SetTransformedAABB(entityTransform, entityBounds);
            box.Add(entityBounds);
        }

        m_spawnedIds.push_back(id);
    }

    gEnv->pEntitySystem->EndCreateEntities();
}

//////////////////////////////////////////////////////////////////////////
void CRuntimePrefab::SpawnBrushes(const std::shared_ptr<CPrefab>& prefab, const Matrix34& matSource, AABB& box)
{
    // brushes and decals
    m_spawnedNodes.reserve(prefab->m_brushParamsList.size());
    for (const BrushParams& brushParams : prefab->m_brushParamsList)
    {
        // world TM
        Matrix34 worldTransform = matSource * brushParams.m_worldMatrix;

        _smart_ptr<IMaterial> material = nullptr;
        if (!brushParams.m_material.empty())
        {
            material = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(brushParams.m_material);
        }

        IRenderNode* node = nullptr;
        if (brushParams.m_isDecal)
        {
            if (material != nullptr)
            {
                node = gEnv->p3DEngine->CreateRenderNode(eERType_Decal);

                node->SetRndFlags(brushParams.m_renderNodeFlags | ERF_PROCEDURAL, true);

                // set properties
                SDecalProperties decalProperties;
                switch (brushParams.m_projectionType)
                {
                case SDecalProperties::eProjectOnTerrainAndStaticObjects:
                {
                    decalProperties.m_projectionType = SDecalProperties::eProjectOnTerrainAndStaticObjects;
                    break;
                }
                case SDecalProperties::eProjectOnTerrain:
                {
                    decalProperties.m_projectionType = SDecalProperties::eProjectOnTerrain;
                    break;
                }
                case SDecalProperties::ePlanar:
                default:
                {
                    decalProperties.m_projectionType = SDecalProperties::ePlanar;
                    break;
                }
                }

                // get normalized rotation (remove scaling)
                Matrix33 rotation(worldTransform);
                if (brushParams.m_projectionType != SDecalProperties::ePlanar)
                {
                    rotation.SetRow(0, rotation.GetRow(0).GetNormalized());
                    rotation.SetRow(1, rotation.GetRow(1).GetNormalized());
                    rotation.SetRow(2, rotation.GetRow(2).GetNormalized());
                }

                decalProperties.m_pos = worldTransform.TransformPoint(Vec3(0, 0, 0));
                decalProperties.m_normal = worldTransform.TransformVector(Vec3(0, 0, 1));
                decalProperties.m_pMaterialName = material->GetName();
                decalProperties.m_radius = brushParams.m_projectionType != (SDecalProperties::ePlanar ? decalProperties.m_normal.GetLength() : 1);
                decalProperties.m_explicitRightUpFront = rotation;
                decalProperties.m_sortPrio = brushParams.m_sortPriority;
                decalProperties.m_deferred = brushParams.m_deferredDecal;
                decalProperties.m_depth = brushParams.m_depth;
                IDecalRenderNode* renderNode = static_cast<IDecalRenderNode*>(node);
                renderNode->SetDecalProperties(decalProperties);
            }
        }
        else
        {
            // brush
            node = gEnv->p3DEngine->CreateRenderNode(eERType_Brush);
            IStatObj* obj = nullptr;
            if (!brushParams.m_filename.empty())
            {
                obj = gEnv->p3DEngine->LoadStatObjUnsafeManualRef(brushParams.m_filename, nullptr, nullptr, false);
            }
            else if (!brushParams.m_binaryDesignerObject.empty())
            {
                obj = gEnv->p3DEngine->LoadDesignerObject(brushParams.m_binaryDesignerObjectVersion, &brushParams.m_binaryDesignerObject[0], (int)brushParams.m_binaryDesignerObject.size());
            }

            if (obj)
            {
                obj->AddRef();
                node->SetEntityStatObj(0, obj, 0);

                // calc bbox for the editor because we spawn separate brushes
                AABB objectBounds = obj->GetAABB();
                objectBounds.SetTransformedAABB(brushParams.m_worldMatrix, objectBounds);
                box.Add(objectBounds);
            }

            if (material != nullptr)
            {
                node->SetMaterial(material);
            }
        }

        if (node)
        {
            node->SetMatrix(worldTransform);
            node->SetRndFlags(brushParams.m_renderNodeFlags | ERF_PROCEDURAL, true);
            node->SetLodRatio(brushParams.m_lodRatio);
            node->SetViewDistanceMultiplier(aznumeric_caster(brushParams.m_viewDistRatio));

            gEnv->p3DEngine->RegisterEntity(node);
        }

        m_spawnedNodes.push_back(node);
    }
}

//////////////////////////////////////////////////////////////////////////
void CRuntimePrefab::Spawn(IEntity* entity, const std::shared_ptr<CPrefab>& prefab, const Matrix34& localTransform, AABB& box)
{
    m_sourcePrefab = prefab;

    // store the prefab that was chosen
    IScriptTable* scriptTable(entity->GetScriptTable());
    if (scriptTable)
    {
        ScriptAnyValue value;
        value.type = ANY_TSTRING;
        value.str = prefab->GetName();
        scriptTable->SetValueAny("PrefabSourceName", value);
    }

    m_worldToLocal = localTransform;

    Matrix34 worldTransform = entity->GetWorldTM() * localTransform;

    SpawnEntities(prefab, worldTransform, box);
    SpawnBrushes(prefab, worldTransform, box);
}

//////////////////////////////////////////////////////////////////////////
void CRuntimePrefab::Spawn(const std::shared_ptr<CPrefab>& prefab)
{
    Clear();

    IEntity* entity = gEnv->pEntitySystem->GetEntity(m_id);
    if (!entity)
    {
        gEnv->pLog->LogError("Prefab Spawn: Entity ID %d not found", m_id);
        return;
    }

    IComponentRender* rendererProxy = (IComponentRender*)entity->GetOrCreateComponent<IComponentRender>().get();
    AABB localBounds(AABB::RESET);

    // expand embedded prefabs
    m_spawnedPrefabs.reserve(prefab->m_prefabParamsList.size());
    for (const PrefabParams& prefabParams : prefab->m_prefabParamsList)
    {
        if (prefabParams.m_prefab)
        {
            auto runtimePrefab = std::make_shared<CRuntimePrefab>(m_id);
            runtimePrefab->Spawn(entity, prefabParams.m_prefab, prefabParams.m_worldMatrix, localBounds);

            m_spawnedPrefabs.push_back(runtimePrefab);
        }
    }

    Spawn(entity, prefab, Matrix34::CreateIdentity(), localBounds);

    // safety check in case the entities cannot be spawned for some reason and therefore the bounding box doesnt get set properly.
    // otherwise this will cause issue when inserting into the 3d engine octree
    if (localBounds.min.x > localBounds.max.x || localBounds.min.y > localBounds.max.y || localBounds.min.z > localBounds.max.z)
    {
        localBounds = AABB(1.0f);
    }

    rendererProxy->SetLocalBounds(localBounds, true);

    // make sure all transformations are set
    Move();
}

//////////////////////////////////////////////////////////////////////////
void CRuntimePrefab::Move(const Matrix34& matOff)
{
    // entities
    int prefabReferences = 0;
    for (const EntityId& id : m_spawnedIds)
    {
        IEntity* entity = gEnv->pEntitySystem->GetEntity(id);
        if (entity)
        {
            const EntityParams& entityParams = m_sourcePrefab->m_entityParamsList[prefabReferences];
            Matrix34 worldTransform = matOff * entityParams.m_worldMatrix;
            entity->SetWorldTM(worldTransform);
        }
        prefabReferences++;
    }

    // brushes and decals
    prefabReferences = 0;
    for (IRenderNode* node : m_spawnedNodes)
    {
        if (!node)
        {
            gEnv->p3DEngine->UnRegisterEntityDirect(node);

            const BrushParams& brushParams = m_sourcePrefab->m_brushParamsList[prefabReferences];

            if (!brushParams.m_isDecal)
            {
                IStatObj* statObj = node->GetEntityStatObj(0, 0, 0);
                node->SetEntityStatObj(0, statObj, 0);
            }

            Matrix34 mat = matOff * brushParams.m_worldMatrix;
            node->SetMatrix(mat);

            gEnv->p3DEngine->RegisterEntity(node);
        }
        prefabReferences++;
    }
}

//////////////////////////////////////////////////////////////////////////
void CRuntimePrefab::Move()
{
    IEntity* entity = gEnv->pEntitySystem->GetEntity(m_id);
    assert(entity);

    Vec3 vPos = entity->GetPos();
    Quat qRot = entity->GetRotation();
    Vec3 vScale = entity->GetScale();

    Matrix34 matSource;
    matSource.Set(vScale, qRot, vPos);

    for (std::shared_ptr<CRuntimePrefab> runtimePrefab : m_spawnedPrefabs)
    {
        runtimePrefab->Move(matSource * runtimePrefab->m_worldToLocal);
    }

    Move(matSource);
}

//////////////////////////////////////////////////////////////////////////
void CRuntimePrefab::HideComponents(bool hide)
{
    // entities
    int prefabReferences = 0;
    for (const EntityId& id : m_spawnedIds)
    {
        IEntity* entity = gEnv->pEntitySystem->GetEntity(id);
        if (entity)
        {
            entity->Hide(hide);
        }
    }

    // brushes and decals
    for (IRenderNode* node : m_spawnedNodes)
    {
        if (node)
        {
            node->Hide(hide);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CRuntimePrefab::Hide(bool hide)
{
    IEntity* entity = gEnv->pEntitySystem->GetEntity(m_id);
    assert(entity);

    for (std::shared_ptr<CRuntimePrefab> runtimePrefab : m_spawnedPrefabs)
    {
        runtimePrefab->HideComponents(hide);
    }

    HideComponents(hide);
}
