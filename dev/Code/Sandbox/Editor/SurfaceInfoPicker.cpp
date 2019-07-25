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

#include "StdAfx.h"
#include "SurfaceInfoPicker.h"
#include "Material/MaterialManager.h"
#include "Terrain/TerrainManager.h"
#include "Terrain/SurfaceType.h"
#include "VegetationMap.h"
#include "VegetationObject.h"
#include "Objects/EntityObject.h"
#include "Viewport.h"
#include "Plugins/ComponentEntityEditorPlugin/Objects/ComponentEntityObject.h"

#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <LmbrCentral/Rendering/GeomCacheComponentBus.h>
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>

#include <IAttachment.h>

static const float kEnoughFarDistance(5000.0f);

CSurfaceInfoPicker::CSurfaceInfoPicker()
    : m_pObjects(NULL)
    , m_pSetObjects(NULL)
    , m_PickOption(0)
{
    m_pActiveView = GetIEditor()->GetActiveView();
}

bool CSurfaceInfoPicker::PickObject(const QPoint& point,
    SRayHitInfo& outHitInfo,
    CBaseObject* pObject)
{
    Vec3 vWorldRaySrc, vWorldRayDir;
    m_pActiveView->ViewToWorldRay(point, vWorldRaySrc, vWorldRayDir);
    vWorldRaySrc = vWorldRaySrc + vWorldRayDir * 0.1f;
    vWorldRayDir = vWorldRayDir * kEnoughFarDistance;

    return PickObject(vWorldRaySrc, vWorldRayDir, outHitInfo, pObject);
}

bool CSurfaceInfoPicker::PickObject(const Vec3& vWorldRaySrc,
    const Vec3& vWorldRayDir,
    SRayHitInfo& outHitInfo,
    CBaseObject* pObject)
{
    if (pObject == NULL)
    {
        return false;
    }
    if (pObject->IsHidden() || IsFrozen(pObject))
    {
        return false;
    }

    Vec3 vDir = vWorldRayDir.GetNormalized() * kEnoughFarDistance;
    memset(&outHitInfo, 0, sizeof(outHitInfo));
    outHitInfo.fDistance = kEnoughFarDistance;

    AABB boundbox;
    pObject->GetBoundBox(boundbox);
    if (!m_pActiveView || !m_pActiveView->IsBoundsVisible(boundbox))
    {
        return false;
    }

    ObjectType objType(pObject->GetType());

    if (objType == OBJTYPE_SOLID || objType == OBJTYPE_VOLUMESOLID)
    {
        _smart_ptr<IStatObj> pStatObj = pObject->GetIStatObj();
        if (!pStatObj)
        {
            return false;
        }
        return RayIntersection(vWorldRaySrc, vWorldRayDir, NULL, NULL, pStatObj, NULL, pObject->GetWorldTM(), outHitInfo, NULL);
    }
    else if (objType == OBJTYPE_BRUSH)
    {
        return RayIntersection_CBaseObject(vWorldRaySrc, vWorldRayDir, pObject, NULL, outHitInfo);
    }
    else if (objType == OBJTYPE_PREFAB)
    {
        for (int k = 0, nChildCount(pObject->GetChildCount()); k < nChildCount; ++k)
        {
            CBaseObject* childObject(pObject->GetChild(k));
            if (childObject == NULL)
            {
                continue;
            }
            if (childObject->IsHidden() || IsFrozen(childObject))
            {
                continue;
            }
            if (RayIntersection_CBaseObject(vWorldRaySrc, vWorldRayDir, pObject->GetChild(k), NULL, outHitInfo))
            {
                return true;
            }
        }
        return false;
    }
    else if (objType == OBJTYPE_ENTITY)
    {
        CEntityObject* pEntity((CEntityObject*)pObject);
        return RayIntersection(vWorldRaySrc, vWorldRayDir, NULL, pEntity->GetIEntity(), NULL, NULL, pObject->GetWorldTM(), outHitInfo, NULL);
    }

    return false;
}

bool CSurfaceInfoPicker::PickByAABB(const QPoint& point, int nFlag, IDisplayViewport* pView, CExcludedObjects* pExcludedObjects, std::vector<CBaseObjectPtr>* pOutObjects)
{
    GetIEditor()->GetObjectManager()->GetObjects(m_objects);

    Vec3 vWorldRaySrc, vWorldRayDir;
    m_pActiveView->ViewToWorldRay(point, vWorldRaySrc, vWorldRayDir);
    vWorldRaySrc = vWorldRaySrc + vWorldRayDir * 0.1f;
    vWorldRayDir = vWorldRayDir * kEnoughFarDistance;

    bool bPicked = false;

    for (int i = 0, iCount(m_objects.size()); i < iCount; ++i)
    {
        if (pExcludedObjects && pExcludedObjects->Contains(m_objects[i]))
        {
            continue;
        }

        AABB worldObjAABB;
        m_objects[i]->GetBoundBox(worldObjAABB);

        if (pView)
        {
            float fScreenFactor = pView->GetScreenScaleFactor(m_objects[i]->GetPos());
            worldObjAABB.Expand(0.01f * Vec3(fScreenFactor, fScreenFactor, fScreenFactor));
        }

        Vec3 vHitPos;
        if (Intersect::Ray_AABB(vWorldRaySrc, vWorldRayDir, worldObjAABB, vHitPos))
        {
            if ((vHitPos - vWorldRaySrc).GetNormalized().Dot(vWorldRayDir) > 0 || worldObjAABB.IsContainPoint(vHitPos))
            {
                if (pOutObjects)
                {
                    pOutObjects->push_back(m_objects[i]);
                }
                bPicked = true;
            }
        }
    }

    return bPicked;
}

bool CSurfaceInfoPicker::PickImpl(const QPoint& point,
    _smart_ptr<IMaterial>* ppOutLastMaterial,
    SRayHitInfo& outHitInfo,
    CExcludedObjects* pExcludedObjects,
    int nFlag)
{
    Vec3 vWorldRaySrc;
    Vec3 vWorldRayDir;
    if (!m_pActiveView)
    {
        m_pActiveView = GetIEditor()->GetActiveView();
    }
    m_pActiveView->ViewToWorldRay(point, vWorldRaySrc, vWorldRayDir);
    vWorldRaySrc = vWorldRaySrc + vWorldRayDir * 0.1f;
    vWorldRayDir = vWorldRayDir * kEnoughFarDistance;

    return PickImpl(vWorldRaySrc, vWorldRayDir, ppOutLastMaterial, outHitInfo, pExcludedObjects, nFlag);
}

bool CSurfaceInfoPicker::PickImpl(const Vec3& vWorldRaySrc, const Vec3& vWorldRayDir,
    _smart_ptr<IMaterial>* ppOutLastMaterial,
    SRayHitInfo& outHitInfo,
    CExcludedObjects* pExcludedObjects,
    int nFlag)
{
    memset(&outHitInfo, 0, sizeof(outHitInfo));
    outHitInfo.fDistance = kEnoughFarDistance;

    if (m_pSetObjects)
    {
        m_pObjects = m_pSetObjects;
    }
    else
    {
        GetIEditor()->GetObjectManager()->GetObjects(m_objects);
        m_pObjects = &m_objects;
    }

    if (pExcludedObjects)
    {
        m_ExcludedObjects = *pExcludedObjects;
    }
    else
    {
        m_ExcludedObjects.Clear();
    }

    m_pPickedObject = NULL;

    if (nFlag & ePOG_Terrain)
    {
        FindNearestInfoFromTerrain(vWorldRaySrc, vWorldRayDir, ppOutLastMaterial, outHitInfo);
    }
    if (nFlag & ePOG_BrushObject)
    {
        FindNearestInfoFromBrushObjects(vWorldRaySrc, vWorldRayDir, ppOutLastMaterial, outHitInfo);
    }
    if (nFlag & ePOG_Entity)
    {
        FindNearestInfoFromEntities(vWorldRaySrc, vWorldRayDir, ppOutLastMaterial, outHitInfo);
    }
    if (nFlag & ePOG_Vegetation)
    {
        FindNearestInfoFromVegetations(vWorldRaySrc, vWorldRayDir, ppOutLastMaterial, outHitInfo);
    }
    if (nFlag & ePOG_Prefabs)
    {
        FindNearestInfoFromPrefabs(vWorldRaySrc, vWorldRayDir, ppOutLastMaterial, outHitInfo);
    }
    if ((nFlag & ePOG_Solid) || (nFlag & ePOG_DesignerObject))
    {
        FindNearestInfoFromSolids(nFlag, vWorldRaySrc, vWorldRayDir, ppOutLastMaterial, outHitInfo);
    }

    if (!m_pSetObjects)
    {
        m_objects.clear();
    }

    m_ExcludedObjects.Clear();

    return outHitInfo.fDistance < kEnoughFarDistance;
}

void CSurfaceInfoPicker::FindNearestInfoFromBrushObjects(
    const Vec3& vWorldRaySrc,
    const Vec3& vWorldRayDir,
    _smart_ptr<IMaterial>* pOutLastMaterial,
    SRayHitInfo& outHitInfo) const
{
    for (size_t i = 0; i < m_pObjects->size(); ++i)
    {
        CBaseObject* object((*m_pObjects)[i]);
        ObjectType objType(object->GetType());
        if (object == NULL)
        {
            continue;
        }
        if (objType != OBJTYPE_BRUSH)
        {
            continue;
        }
        if (object->IsHidden() || IsFrozen(object))
        {
            continue;
        }
        if (m_ExcludedObjects.Contains(object))
        {
            continue;
        }
        if (RayIntersection_CBaseObject(vWorldRaySrc, vWorldRayDir, object, pOutLastMaterial, outHitInfo))
        {
            if (pOutLastMaterial)
            {
                AssignObjectMaterial(object, outHitInfo, pOutLastMaterial);
            }
            m_pPickedObject = object;
        }
    }
}

void CSurfaceInfoPicker::FindNearestInfoFromPrefabs(
    const Vec3& vWorldRaySrc,
    const Vec3& vWorldRayDir,
    _smart_ptr<IMaterial>* pOutLastMaterial,
    SRayHitInfo& outHitInfo) const
{
    for (size_t i = 0; i < m_pObjects->size(); ++i)
    {
        CBaseObject* object((*m_pObjects)[i]);
        if (object == NULL)
        {
            continue;
        }
        ObjectType objType(object->GetType());
        if (objType != OBJTYPE_PREFAB)
        {
            continue;
        }
        if (object->IsHidden() || IsFrozen(object))
        {
            continue;
        }
        if (m_ExcludedObjects.Contains(object))
        {
            continue;
        }
        for (int k = 0, nChildCount(object->GetChildCount()); k < nChildCount; ++k)
        {
            CBaseObject* childObject(object->GetChild(k));
            if (childObject == NULL)
            {
                continue;
            }
            if (childObject->IsHidden() || IsFrozen(childObject))
            {
                continue;
            }
            if (RayIntersection_CBaseObject(vWorldRaySrc, vWorldRayDir, object->GetChild(k), pOutLastMaterial, outHitInfo))
            {
                if (pOutLastMaterial)
                {
                    AssignObjectMaterial(childObject, outHitInfo, pOutLastMaterial);
                }
                m_pPickedObject = childObject;
            }
        }
    }
}


void CSurfaceInfoPicker::FindNearestInfoFromSolids(int nFlags,
    const Vec3& vWorldRaySrc,
    const Vec3& vWorldRayDir,
    _smart_ptr<IMaterial>* pOutLastMaterial,
    SRayHitInfo& outHitInfo) const
{
    for (size_t i = 0; i < m_pObjects->size(); ++i)
    {
        CBaseObject* object((*m_pObjects)[i]);
        if (object == NULL)
        {
            continue;
        }
        if (object->GetType() != OBJTYPE_SOLID)
        {
            continue;
        }
        if (!(nFlags & ePOG_DesignerObject) && object->GetType() == OBJTYPE_SOLID)
        {
            continue;
        }
        if (object->IsHidden() || IsFrozen(object))
        {
            continue;
        }
        if (m_ExcludedObjects.Contains(object))
        {
            continue;
        }
        _smart_ptr<IStatObj> pStatObj = object->GetIStatObj();
        if (!pStatObj)
        {
            continue;
        }

        Matrix34A worldTM(object->GetWorldTM());
        if (object->GetMaterial() && pOutLastMaterial)
        {
            if (!IsMaterialValid(object->GetMaterial()))
            {
                continue;
            }
        }

        if (RayIntersection(vWorldRaySrc, vWorldRayDir, NULL, NULL, pStatObj, NULL, worldTM, outHitInfo, pOutLastMaterial))
        {
            if (pOutLastMaterial)
            {
                AssignObjectMaterial(object, outHitInfo, pOutLastMaterial);
            }
            m_pPickedObject = object;
        }
    }
}

void CSurfaceInfoPicker::FindNearestInfoFromDecals(
    const Vec3& vWorldRaySrc,
    const Vec3& vWorldRayDir,
    _smart_ptr<IMaterial>* pOutLastMaterial,
    SRayHitInfo& outHitInfo)   const
{
    for (size_t i = 0; i < m_pObjects->size(); ++i)
    {
        CBaseObject* object((*m_pObjects)[i]);

        if (object->GetType() != OBJTYPE_DECAL)
        {
            continue;
        }

        if (object->IsHidden() || IsFrozen(object))
        {
            continue;
        }

        if (m_ExcludedObjects.Contains(object))
        {
            continue;
        }

        IRenderNode* pRenderNode(object->GetEngineNode());
        AABB localBoundBox;
        pRenderNode->GetLocalBounds(localBoundBox);

        Matrix34 worldTM(object->GetWorldTM());
        Matrix34 invWorldTM = worldTM.GetInverted();

        Vec3 localRaySrc = invWorldTM.TransformPoint(vWorldRaySrc);
        Vec3 localRayDir = invWorldTM.TransformVector(vWorldRayDir);

        Vec3 localRayOut;
        if (Intersect::Ray_AABB(localRaySrc, localRayDir, localBoundBox, localRayOut))
        {
            float distance(localRaySrc.GetDistance(localRayOut));
            if (distance < outHitInfo.fDistance)
            {
                if (pOutLastMaterial)
                {
                    *pOutLastMaterial = object->GetMaterial()->GetMatInfo();
                }
                outHitInfo.fDistance = distance;
                outHitInfo.vHitPos = worldTM.TransformPoint(localRayOut);
                outHitInfo.vHitNormal = worldTM.GetColumn2().GetNormalized();
                outHitInfo.nHitMatID = -1;
                m_pPickedObject = NULL;
            }
        }
    }
}

void CSurfaceInfoPicker::FindNearestInfoFromEntities(
    const Vec3& vWorldRaySrc,
    const Vec3& vWorldRayDir,
    _smart_ptr<IMaterial>* pOutLastMaterial,
    SRayHitInfo& outHitInfo) const
{
    for (size_t i = 0; i < m_pObjects->size(); ++i)
    {
        CBaseObject* object((*m_pObjects)[i]);

        if (object == nullptr
            || !qobject_cast<CEntityObject*>(object)
            || object->IsHidden()
            || IsFrozen(object)
            || m_ExcludedObjects.Contains(object)
            )
        {
            continue;
        }

        CEntityObject* entityObject = static_cast<CEntityObject*>(object);

        // If a legacy entity doesn't have a material override, we'll get the default material for that statObj
        _smart_ptr<IMaterial> statObjDefaultMaterial = nullptr;
        // And in some cases the material we want does comes from RayIntersection(), and we will skip AssignObjectMaterial().
        _smart_ptr<IMaterial> pickedMaterial = nullptr;

        bool hit = false;

        // entityObject is a component entity...
        if (entityObject->GetType() == OBJTYPE_AZENTITY)
        {
            AZ::EntityId id;
            AzToolsFramework::ComponentEntityObjectRequestBus::EventResult(id, entityObject, &AzToolsFramework::ComponentEntityObjectRequestBus::Events::GetAssociatedEntityId);

            AZ::EBusAggregateResults<IStatObj*> statObjs;
            LmbrCentral::LegacyMeshComponentRequestBus::EventResult(statObjs, id, &LmbrCentral::LegacyMeshComponentRequestBus::Events::GetStatObj);
            
            // If the entity has a MeshComponent, it will hit here
            for (IStatObj* statObj : statObjs.values)
            {
                hit = RayIntersection(vWorldRaySrc, vWorldRayDir, nullptr, nullptr, statObj, nullptr, object->GetWorldTM(), outHitInfo, &statObjDefaultMaterial);
                if (hit)
                {
                    break;
                }
            }

            // If the entity has an ActorComponent or another component that overrides RenderNodeRequestBus, it will hit here
            if (!hit)
            {
                // There might be multiple components with render nodes on the same entity
                // This will get the highest priority one, as determined by RenderNodeRequests::GetRenderNodeRequestBusOrder
                IRenderNode* renderNode = entityObject->GetEngineNode();

                // If the renderNode exists and is physicalized, it will hit here
                hit = RayIntersection_IRenderNode(vWorldRaySrc, vWorldRayDir, renderNode, &pickedMaterial, object->GetWorldTM(), outHitInfo);
                if (!hit)
                {
                    // If the renderNode is not physicalized, such as an actor component, but still exists and has a valid material we might want to pick
                    if (renderNode && renderNode->GetMaterial())
                    {
                        // Do a hit test with anything in this entity that has overridden EditorComponentSelectionRequestsBus          
                        CComponentEntityObject* componentEntityObject = static_cast<CComponentEntityObject*>(entityObject);
                        HitContext hc;
                        hc.raySrc = vWorldRaySrc;
                        hc.rayDir = vWorldRayDir;                       
                        bool intersects = componentEntityObject->HitTest(hc);
                        
                        if (intersects)
                        {
                            // If the distance is closer than the nearest distance so far
                            if (hc.dist < outHitInfo.fDistance)
                            {
                                hit = true;
                                outHitInfo.vHitPos = hc.raySrc + hc.rayDir * hc.dist;
                                outHitInfo.fDistance = hc.dist;
                                // We don't get material/sub-material information back from HitTest, so just use the material from the render node
                                pickedMaterial = renderNode->GetMaterial(); 
                                outHitInfo.nHitMatID = 0;
                                // We don't get normal information back from HitTest, so just orient the selection disk towards the camera
                                outHitInfo.vHitNormal = vWorldRayDir.normalized();
                            }
                        }
                    }
                }
            }

            // If the entity has a SkinnedMeshComponent, it will hit here
            if (!hit)
            {
                ICharacterInstance* characterInstance = nullptr;
                LmbrCentral::SkinnedMeshComponentRequestBus::EventResult(characterInstance, id, &LmbrCentral::SkinnedMeshComponentRequestBus::Events::GetCharacterInstance);
                hit = RayIntersection(vWorldRaySrc, vWorldRayDir, nullptr, nullptr, nullptr, characterInstance, object->GetWorldTM(), outHitInfo, &pickedMaterial);
            }

            // If the entity has a GeometryCacheComponent it will hit here
            if (!hit)
            {
                IGeomCacheRenderNode* geomCacheRenderNode = nullptr;
                LmbrCentral::GeometryCacheComponentRequestBus::EventResult(geomCacheRenderNode, id, &LmbrCentral::GeometryCacheComponentRequestBus::Events::GetGeomCacheRenderNode);
                hit = RayIntersection_IGeomCacheRenderNode(vWorldRaySrc, vWorldRayDir, geomCacheRenderNode, &pickedMaterial, object->GetWorldTM(), outHitInfo);
            }
        }

        // If the entity is a legacy CryEngine entity, it will hit here
        if(!hit)
        {
            hit = RayIntersection(vWorldRaySrc, vWorldRayDir, nullptr, entityObject->GetIEntity(), nullptr, nullptr, object->GetWorldTM(), outHitInfo, &statObjDefaultMaterial);
        }

        if (hit)
        {
            if(pickedMaterial)
            {
                AssignMaterial(pickedMaterial, outHitInfo, pOutLastMaterial);
            }
            else
            {
                if (object->GetMaterial())
                {
                    // If the entity has a material override, assign the object material
                    AssignObjectMaterial(object, outHitInfo, pOutLastMaterial);
                }
                else
                {
                    // Otherwise assign the default material for that object
                    AssignMaterial(statObjDefaultMaterial, outHitInfo, pOutLastMaterial);
                }
            }

            m_pPickedObject = object;
        }
    }
}

void CSurfaceInfoPicker::FindNearestInfoFromVegetations(
    const Vec3& vWorldRaySrc,
    const Vec3& vWorldRayDir,
    _smart_ptr<IMaterial>* ppOutLastMaterial,
    SRayHitInfo& outHitInfo) const
{
    CVegetationMap* pVegetationMap = GetIEditor()->GetVegetationMap();
    if (!pVegetationMap)
    {
        return;
    }
    std::vector<CVegetationInstance*> vegetations;
    pVegetationMap->GetAllInstances(vegetations);

    for (size_t i = 0; i < vegetations.size(); ++i)
    {
        CVegetationInstance* vegetation(vegetations[i]);
        if (vegetation == NULL)
        {
            continue;
        }
        if (vegetation->object == NULL)
        {
            continue;
        }
        if (vegetation->object->IsHidden())
        {
            continue;
        }
        Matrix34A worldTM;

        float fAngle = DEG2RAD(360.0f / 255.0f * vegetation->angle);
        worldTM.SetIdentity();
        worldTM.SetRotationZ(fAngle);
        worldTM.SetTranslation(vegetation->pos);

        IStatObj* pStatObj = vegetation->object->GetObject();
        if (pStatObj == NULL)
        {
            continue;
        }

        if (RayIntersection(vWorldRaySrc, vWorldRayDir, NULL, NULL, pStatObj, NULL, worldTM, outHitInfo, ppOutLastMaterial))
        {
            CMaterial* pUserDefMaterial = vegetation->object->GetMaterial();
            if (pUserDefMaterial && ppOutLastMaterial)
            {
                *ppOutLastMaterial = pUserDefMaterial->GetMatInfo();
            }

            m_pPickedObject = NULL;
        }
    }
}

void CSurfaceInfoPicker::FindNearestInfoFromTerrain(
    const Vec3& vWorldRaySrc,
    const Vec3& vWorldRayDir,
    _smart_ptr<IMaterial>* pOutLastMaterial,
    SRayHitInfo& outHitInfo) const
{
    int objTypes = ent_terrain;
    int flags = (geom_colltype_ray | geom_colltype14) << rwi_colltype_bit | rwi_colltype_any | rwi_ignore_terrain_holes | rwi_stop_at_pierceable;
    ray_hit hit;
    hit.pCollider = 0;
    int col = gEnv->pPhysicalWorld->RayWorldIntersection(vWorldRaySrc, vWorldRayDir, objTypes, flags, &hit, 1);
    if (col > 0 && hit.dist < outHitInfo.fDistance && hit.bTerrain)
    {
        outHitInfo.nHitSurfaceID = hit.surface_idx;
        outHitInfo.vHitNormal = hit.n;
        outHitInfo.vHitPos = hit.pt;
        int nTerrainSurfaceType = hit.idmatOrg;
        if (nTerrainSurfaceType >= 0 && nTerrainSurfaceType < GetIEditor()->GetTerrainManager()->GetSurfaceTypeCount())
        {
            CSurfaceType* pSurfaceType = GetIEditor()->GetTerrainManager()->GetSurfaceTypePtr(nTerrainSurfaceType);
            QString mtlName = pSurfaceType ? pSurfaceType->GetMaterial() : "";
            if (!mtlName.isEmpty())
            {
                if (pOutLastMaterial)
                {
                    *pOutLastMaterial = GetIEditor()->Get3DEngine()->GetMaterialManager()->FindMaterial(mtlName.toUtf8().data());
                }
                outHitInfo.fDistance = hit.dist;
                outHitInfo.nHitMatID = -1;
            }
        }
    }
}

bool CSurfaceInfoPicker::RayIntersection_CBaseObject(
    const Vec3& vWorldRaySrc,
    const Vec3& vWorldRayDir,
    CBaseObject* pBaseObject,
    _smart_ptr<IMaterial>* pOutLastMaterial,
    SRayHitInfo& outHitInfo)
{
    if (pBaseObject == NULL)
    {
        return false;
    }
    IRenderNode* pRenderNode(pBaseObject->GetEngineNode());
    if (pRenderNode == NULL)
    {
        return false;
    }
    IStatObj* pStatObj(pRenderNode->GetEntityStatObj());
    if (pStatObj == NULL)
    {
        return false;
    }
    return RayIntersection(vWorldRaySrc, vWorldRayDir, pRenderNode, NULL, pStatObj, NULL, pBaseObject->GetWorldTM(), outHitInfo, pOutLastMaterial);
}

bool CSurfaceInfoPicker::RayIntersection(
    const Vec3& vWorldRaySrc,
    const Vec3& vWorldRayDir,
    IRenderNode* pRenderNode,
    IEntity* pEntity,
    IStatObj* pStatObj,
    ICharacterInstance* pCharacterInstance,
    const Matrix34A& WorldTM,
    SRayHitInfo& outHitInfo,
    _smart_ptr<IMaterial>* ppOutLastMaterial)
{
    SRayHitInfo hitInfo;
    bool bRayIntersection = false;
    _smart_ptr<IMaterial> pMaterial(NULL);

    bRayIntersection = RayIntersection_IEntity(vWorldRaySrc, vWorldRayDir, pEntity, &pMaterial, WorldTM, hitInfo);
    if (!bRayIntersection)
    {
        bRayIntersection = RayIntersection_IStatObj(vWorldRaySrc, vWorldRayDir, pStatObj, &pMaterial, WorldTM, hitInfo);
        if (!bRayIntersection)
        {
            bRayIntersection = RayIntersection_ICharacterInstance(vWorldRaySrc, vWorldRayDir, pCharacterInstance, &pMaterial, WorldTM, hitInfo);
            if (!bRayIntersection)
            {
                bRayIntersection = RayIntersection_IRenderNode(vWorldRaySrc, vWorldRayDir, pRenderNode, &pMaterial, WorldTM, hitInfo);
            }
        }
    }

    if (bRayIntersection)
    {
        hitInfo.fDistance = vWorldRaySrc.GetDistance(hitInfo.vHitPos);
        if (hitInfo.fDistance < outHitInfo.fDistance)
        {
            if (ppOutLastMaterial)
            {
                *ppOutLastMaterial = pMaterial;
            }
            memcpy(&outHitInfo, &hitInfo, sizeof(SRayHitInfo));
            outHitInfo.vHitNormal.Normalize();
            return true;
        }
    }
    return false;
}

bool CSurfaceInfoPicker::RayIntersection_IStatObj(
    const Vec3& vWorldRaySrc,
    const Vec3& vWorldRayDir,
    IStatObj* pStatObj,
    _smart_ptr<IMaterial>* ppOutLastMaterial,
    const Matrix34A& worldTM,
    SRayHitInfo& outHitInfo)
{
    if (pStatObj == NULL)
    {
        return false;
    }

    Vec3 localRaySrc;
    Vec3 localRayDir;
    if (!RayWorldToLocal(worldTM, vWorldRaySrc, vWorldRayDir, localRaySrc, localRayDir))
    {
        return false;
    }

    // the outHitInfo contains information about the previous closest hit and should not be cleared / replaced unless you have a better hit!
    // all of the intersection functions called (such as statobj's RayIntersection) only modify the hit info if it actually hits it closer than the current distance of the last hit.

    outHitInfo.inReferencePoint = localRaySrc;
    outHitInfo.inRay                = Ray(localRaySrc, localRayDir);
    outHitInfo.bInFirstHit  = false;
    outHitInfo.bUseCache        = false;

    _smart_ptr<IMaterial> hitMaterial = nullptr;

    Vec3 hitPosOnAABB;
    if (Intersect::Ray_AABB(outHitInfo.inRay, pStatObj->GetAABB(), hitPosOnAABB) == 0x00)
    {
        return false;
    }

    if (pStatObj->RayIntersection(outHitInfo))
    {
        if (outHitInfo.fDistance < 0)
        {
            return false;
        }
        outHitInfo.vHitPos = worldTM.TransformPoint(outHitInfo.vHitPos);
        outHitInfo.vHitNormal = worldTM.GetTransposed().GetInverted().TransformVector(outHitInfo.vHitNormal);

        // we need to set nHitSurfaceID anyway - so we need to do this regardless of whether the caller
        // has asked for detailed material info by passing in a non-null ppOutLastMaterial
        hitMaterial = pStatObj->GetMaterial();

        if (hitMaterial)
        {
            if (outHitInfo.nHitMatID >= 0)
            {
                if (hitMaterial->GetSubMtlCount() > 0 && outHitInfo.nHitMatID < hitMaterial->GetSubMtlCount())
                {
                    _smart_ptr<IMaterial> subMaterial = hitMaterial->GetSubMtl(outHitInfo.nHitMatID);
                    if (subMaterial)
                    {
                        hitMaterial = subMaterial;
                    }
                }
            }
            outHitInfo.nHitSurfaceID = hitMaterial->GetSurfaceTypeId();
        }

        if ((ppOutLastMaterial) && (hitMaterial))
        {
            *ppOutLastMaterial = hitMaterial;
        }

        return true;
    }

    return outHitInfo.fDistance < kEnoughFarDistance;
}

#if defined(USE_GEOM_CACHES)
bool CSurfaceInfoPicker::RayIntersection_IGeomCacheRenderNode(
    const Vec3& vWorldRaySrc,
    const Vec3& vWorldRayDir,
    IGeomCacheRenderNode* pGeomCacheRenderNode,
    _smart_ptr<IMaterial>* ppOutLastMaterial,
    const Matrix34A& worldTM,
    SRayHitInfo& outHitInfo)
{
    if (!pGeomCacheRenderNode)
    {
        return false;
    }

    SRayHitInfo newHitInfo = outHitInfo;
    newHitInfo.inReferencePoint = vWorldRaySrc;
    newHitInfo.inRay                = Ray(vWorldRaySrc, vWorldRayDir);
    newHitInfo.bInFirstHit  = false;
    newHitInfo.bUseCache        = false;

    if (pGeomCacheRenderNode->RayIntersection(newHitInfo))
    {
        if (newHitInfo.fDistance < 0 || newHitInfo.fDistance > kEnoughFarDistance || (outHitInfo.fDistance != 0 && newHitInfo.fDistance > outHitInfo.fDistance))
        {
            return false;
        }

        // Only override outHitInfo if the new hit is closer than the original hit
        outHitInfo = newHitInfo;
        if (ppOutLastMaterial)
        {
            _smart_ptr<IMaterial> pMaterial = pGeomCacheRenderNode->GetMaterial();
            if (pMaterial)
            {
                *ppOutLastMaterial = pMaterial;
                if (outHitInfo.nHitMatID >= 0)
                {
                    if (pMaterial->GetSubMtlCount() > 0 && outHitInfo.nHitMatID < pMaterial->GetSubMtlCount())
                    {
                        *ppOutLastMaterial = pMaterial->GetSubMtl(outHitInfo.nHitMatID);
                    }
                }
            }
        }
        return true;
    }

    return false;
}
#endif

bool CSurfaceInfoPicker::RayIntersection_IRenderNode(
    const Vec3& vWorldRaySrc,
    const Vec3& vWorldRayDir,
    IRenderNode* pRenderNode,
    _smart_ptr<IMaterial>* pOutLastMaterial,
    const Matrix34A& WorldTM,
    SRayHitInfo& outHitInfo)
{
    if (pRenderNode == NULL)
    {
        return false;
    }

    IPhysicalEntity* physics = 0;
    physics = pRenderNode->GetPhysics();
    if (physics == NULL)
    {
        return false;
    }

    pe_status_nparts nparts;
    if (physics->GetStatus(&nparts) == 0)
    {
        return false;
    }

    ray_hit hit;
    int col = GetIEditor()->GetSystem()->GetIPhysicalWorld()->RayTraceEntity(physics, vWorldRaySrc, vWorldRayDir, &hit, 0, geom_collides);
    if (col <= 0)
    {
        return false;
    }

    // Don't override outHitInfo if the previous hit was closer than the current hit
    if (hit.dist < outHitInfo.fDistance)
    {
        return false;
    }

    outHitInfo.vHitPos = hit.pt;
    outHitInfo.vHitNormal = hit.n;
    outHitInfo.fDistance = hit.dist;
    if (outHitInfo.fDistance < 0)
    {
        return false;
    }
    outHitInfo.nHitMatID = -1;
    outHitInfo.nHitSurfaceID = hit.surface_idx;
    _smart_ptr<IMaterial> pMaterial(pRenderNode->GetMaterial());

    if (pMaterial && pOutLastMaterial)
    {
        *pOutLastMaterial = pMaterial;
        if (pMaterial->GetSubMtlCount() > 0 && hit.idmatOrg >= 0 && hit.idmatOrg < pMaterial->GetSubMtlCount())
        {
            outHitInfo.nHitMatID = hit.idmatOrg;
            *pOutLastMaterial = pMaterial->GetSubMtl(hit.idmatOrg);
        }
    }

    return true;
}

bool CSurfaceInfoPicker::RayIntersection_IEntity(
    const Vec3& vWorldRaySrc,
    const Vec3& vWorldRayDir,
    IEntity* pEntity,
    _smart_ptr<IMaterial>* pOutLastMaterial,
    const Matrix34A& WorldTM,
    SRayHitInfo& outHitInfo)
{
    if (pEntity == NULL)
    {
        return false;
    }

    IPhysicalWorld* pPhysWorld = GetIEditor()->GetSystem()->GetIPhysicalWorld();
    IPhysicalEntity* pPhysics = 0;
    bool bHit = false;

    _smart_ptr<IMaterial> pMaterial = NULL;
    int nSlotCount = pEntity->GetSlotCount();
    int nCurSlot = 0;

    for (int i = 0; i < 2; )
    {
        if (i == 0)
        {
            if (nCurSlot >= nSlotCount)
            {
                ++i;
                continue;
            }

            SEntitySlotInfo slotInfo;
            if (pEntity->GetSlotInfo(nCurSlot++, slotInfo) == false)
            {
                continue;
            }

            if (slotInfo.pCharacter)
            {
                pPhysics = slotInfo.pCharacter->GetISkeletonPose()->GetCharacterPhysics();
                pMaterial = slotInfo.pCharacter->GetIMaterial();
            }
            else if (slotInfo.pStatObj)
            {
                if (RayIntersection_IStatObj(vWorldRaySrc, vWorldRayDir, slotInfo.pStatObj, pOutLastMaterial, WorldTM * (*slotInfo.pLocalTM), outHitInfo))
                {
                    return true;
                }
            }
#if defined(USE_GEOM_CACHES)
            else if (slotInfo.pGeomCacheRenderNode)
            {
                if (RayIntersection_IGeomCacheRenderNode(vWorldRaySrc, vWorldRayDir, slotInfo.pGeomCacheRenderNode, pOutLastMaterial, WorldTM * (*slotInfo.pLocalTM), outHitInfo))
                {
                    return true;
                }
            }
#endif
            else
            {
                continue;
            }
        }
        else
        {
            pPhysics = pEntity->GetPhysics();
            ++i;
        }
        if (pPhysics == NULL)
        {
            continue;
        }
        int type = pPhysics->GetType();
        pe_status_nparts nparts;
        if (type == PE_NONE || type == PE_PARTICLE || type == PE_ROPE || type == PE_SOFT)
        {
            continue;
        }
        else if (pPhysics->GetStatus(&nparts) == 0)
        {
            continue;
        }
        ray_hit hit;
        bHit = (pPhysWorld->RayTraceEntity(pPhysics, vWorldRaySrc, vWorldRayDir, &hit) > 0);
        if (!bHit)
        {
            continue;
        }
        outHitInfo.vHitPos = hit.pt;
        outHitInfo.vHitNormal = hit.n;
        outHitInfo.fDistance = hit.dist;
        if (outHitInfo.fDistance < 0)
        {
            bHit = false;
            continue;
        }
        outHitInfo.nHitSurfaceID = hit.surface_idx;
        outHitInfo.nHitMatID = hit.idmatOrg;
        break;
    }

    if (!bHit)
    {
        return false;
    }

    if (pOutLastMaterial)
    {
        *pOutLastMaterial = pMaterial;
        if (pMaterial)
        {
            if (outHitInfo.nHitMatID >= 0 && pMaterial->GetSubMtlCount() > 0 && outHitInfo.nHitMatID < pMaterial->GetSubMtlCount())
            {
                *pOutLastMaterial = pMaterial->GetSubMtl(outHitInfo.nHitMatID);
            }
        }
    }

    return true;
}

bool CSurfaceInfoPicker::RayIntersection_ICharacterInstance(
    const Vec3& vWorldRaySrc,
    const Vec3& vWorldRayDir,
    ICharacterInstance* pCharacterInstance,
    _smart_ptr<IMaterial>* pOutLastMaterial,
    const Matrix34A& worldTM,
    SRayHitInfo& outHitInfo)
{
    if (pCharacterInstance == NULL)
    {
        return false;
    }
    
    // I would like optimize by doing a bounding box check on the whole character first, but
    // in my test case the reported bounding box was smaller than the character and picks were
    // incorrectly failing.

    bool hitSomething = false;
    SRayHitInfo closestHitInfo;
    _smart_ptr<IMaterial> closestHitMaterial = nullptr;

    const int numAttachments = pCharacterInstance->GetIAttachmentManager() ? pCharacterInstance->GetIAttachmentManager()->GetAttachmentCount() : 0;
    for (int attachmentIndex = 0; attachmentIndex < numAttachments; ++attachmentIndex)
    {
        IAttachment* attachment = pCharacterInstance->GetIAttachmentManager()->GetInterfaceByIndex(attachmentIndex);
        AZ_Assert(attachment, "Attachment not found at index %d", attachmentIndex);

        IAttachmentObject* attachmentObject = attachment->GetIAttachmentObject();
        if (!attachmentObject)
        {
            continue;
        }

        const Matrix34A attachmentWorldTM(attachment->GetAttWorldAbsolute());

        Vec3 localRaySrc;
        Vec3 localRayDir;
        Vec3 hitPosOnAttachmentAABB;

        if (!RayWorldToLocal(attachmentWorldTM, vWorldRaySrc, vWorldRayDir, localRaySrc, localRayDir))
        {
            continue;
        }

        bool hitCurrent = false;
        SRayHitInfo currentHitInfo;
        currentHitInfo.fDistance = FLT_MAX; // This is necessary because RayIntersection(..., IStatObj*, ...) will otherwise return true when fDistance is 0.


        // Test for ray intersection...
        if (attachment->GetType() == CA_SKIN)
        {
            IAttachmentSkin* skinAttachment = attachment->GetIAttachmentSkin();
            if (skinAttachment &&
                skinAttachment->GetISkin() &&
                skinAttachment->GetISkin()->GetIRenderMesh(0))
            {
                IRenderMesh* attachmentRenderMesh = skinAttachment->GetISkin()->GetIRenderMesh(0);
                            
                AABB bbox;
                attachmentRenderMesh->GetBBox(bbox.min, bbox.max);

                Ray pickRay(localRaySrc, localRayDir);

                if (Intersect::Ray_AABB(pickRay, bbox, hitPosOnAttachmentAABB))
                {
                    SRayHitInfo renderMeshHitInfo;
                    renderMeshHitInfo.inReferencePoint = localRaySrc;
                    renderMeshHitInfo.inRay = pickRay;

                    if (GetIEditor()->Get3DEngine()->RenderMeshRayIntersection(attachmentRenderMesh, renderMeshHitInfo))
                    {
                        hitCurrent = true;

                        currentHitInfo = renderMeshHitInfo;
                        currentHitInfo.vHitPos = attachmentWorldTM.TransformPoint(currentHitInfo.vHitPos);
                        currentHitInfo.vHitNormal = attachmentWorldTM.GetTransposed().GetInverted().TransformVector(currentHitInfo.vHitNormal);
                    }
                }
            }
        }
        else
        {
            hitCurrent = RayIntersection_IStatObj(vWorldRaySrc, vWorldRayDir, attachmentObject->GetIStatObj(), nullptr, attachmentWorldTM, currentHitInfo);
        }
        
        // If the current attachement got hit, the hit distance is closer than both the original hit distance and the closest hit from all previously tested attachements
        if (hitCurrent && currentHitInfo.fDistance < outHitInfo.fDistance && (currentHitInfo.fDistance < closestHitInfo.fDistance || !hitSomething))
        {
            hitSomething = true;
            closestHitInfo = currentHitInfo;
            closestHitMaterial = attachmentObject->GetReplacementMaterial() ? attachmentObject->GetReplacementMaterial() : attachmentObject->GetBaseMaterial();
        }
    }


    if (hitSomething)
    {
        outHitInfo = closestHitInfo;
        if (pOutLastMaterial)
        {
            *pOutLastMaterial = closestHitMaterial;
        }
    }

    return hitSomething;
}

bool CSurfaceInfoPicker::RayWorldToLocal(
    const Matrix34A& WorldTM,
    const Vec3& vWorldRaySrc,
    const Vec3& vWorldRayDir,
    Vec3& outRaySrc,
    Vec3& outRayDir)
{
    if (!WorldTM.IsValid())
    {
        return false;
    }
    Matrix34A invertedM(WorldTM.GetInverted());
    if (!invertedM.IsValid())
    {
        return false;
    }
    outRaySrc = invertedM.TransformPoint(vWorldRaySrc);
    outRayDir = invertedM.TransformVector(vWorldRayDir).GetNormalized();
    return true;
}

bool CSurfaceInfoPicker::IsMaterialValid(CMaterial* pMaterial)
{
    if (pMaterial == NULL)
    {
        return false;
    }
    return !(pMaterial->GetMatInfo()->GetFlags() & MTL_FLAG_NODRAW);
}

void CSurfaceInfoPicker::AssignObjectMaterial(CBaseObject* pObject, const SRayHitInfo& outHitInfo, _smart_ptr<IMaterial>* pOutMaterial)
{
    CMaterial* material = pObject->GetMaterial();
    if (material)
    {
        if (material->GetMatInfo())
        {
            if (pOutMaterial)
            {
                *pOutMaterial = material->GetMatInfo();
                if (*pOutMaterial)
                {
                    if (outHitInfo.nHitMatID >= 0 && (*pOutMaterial)->GetSubMtlCount() > 0 && outHitInfo.nHitMatID < (*pOutMaterial)->GetSubMtlCount())
                    {
                        *pOutMaterial = (*pOutMaterial)->GetSubMtl(outHitInfo.nHitMatID);
                    }
                }
            }
        }
    }
}

void CSurfaceInfoPicker::AssignMaterial(_smart_ptr<IMaterial> pMaterial, const SRayHitInfo& outHitInfo, _smart_ptr<IMaterial>* pOutMaterial)
{
    if (pOutMaterial)
    {
        *pOutMaterial = pMaterial;
        if (*pOutMaterial)
        {
            if (outHitInfo.nHitMatID >= 0 && (*pOutMaterial)->GetSubMtlCount() > 0 && outHitInfo.nHitMatID < (*pOutMaterial)->GetSubMtlCount())
            {
                *pOutMaterial = (*pOutMaterial)->GetSubMtl(outHitInfo.nHitMatID);
            }
        }
    }
}

void CSurfaceInfoPicker::SetActiveView(IDisplayViewport* view)
{
    if (view)
    {
        m_pActiveView = view;
    }
    else
    {
        m_pActiveView = GetIEditor()->GetActiveView();
    }
}
