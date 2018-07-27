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

// Description : Octree parts used on Jobs to prevent scanning too many files for micro functions

#include "StdAfx.h"

#include "StatObj.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include "terrain_sector.h"
#include "CullBuffer.h"
#include "3dEngine.h"
#include "IndexedMesh.h"
#include "Brush.h"
#include "Vegetation.h"
#include "terrain.h"
#include "ObjectsTree.h"
#include "ICryAnimation.h"

#include "DecalRenderNode.h"
#include "Brush.h"
#include "BreakableGlassRenderNode.h"
#include "FogVolumeRenderNode.h"
#include "terrain_water.h"
#include "RopeRenderNode.h"
#include "MergedMeshRenderNode.h"
#include "LightEntity.h"
#include "RoadRenderNode.h"
#include "WaterVolumeRenderNode.h"
#include "DistanceCloudRenderNode.h"
#include "Environment/OceanEnvironmentBus.h"

#define CHECK_OBJECTS_BOX_WARNING_SIZE (1.0e+10f)
#define fNodeMinSize (8.f)
#define fObjectToNodeSizeRatio (1.f / 8.f)
#define fMinShadowCasterViewDist (8.f)

namespace LegacyInternal
{
    // File scoped LegacyJobExecutor instance used to run all RenderContent jobs
    static AZ::LegacyJobExecutor* s_renderContentJobExecutor;
};

//////////////////////////////////////////////////////////////////////////
COctreeNode::COctreeNode(int nSID, const AABB& box, CVisArea* pVisArea, COctreeNode* pParent)
{
    m_pRNTmpData = NULL;
    memset(this, 0, sizeof(*this));
    m_nSID = nSID;
    m_vNodeCenter = box.GetCenter();
    m_vNodeAxisRadius = box.GetSize() * 0.5f;
    m_objectsBox.min = box.max;
    m_objectsBox.max = box.min;

#if !defined(_RELEASE)
    // Check if bounding box is crazy
#define CHECK_OBJECTS_BOX_WARNING_SIZE (1.0e+10f)
    if (GetCVars()->e_CheckOctreeObjectsBoxSize != 0) // pParent is checked as silly sized things are added to the root (e.g. the sun)
    {
        if (pParent && !m_objectsBox.IsReset() && (m_objectsBox.min.len() > CHECK_OBJECTS_BOX_WARNING_SIZE || m_objectsBox.max.len() > CHECK_OBJECTS_BOX_WARNING_SIZE))
        {
            CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_ERROR_DBGBRK, "COctreeNode being created with a huge m_objectsBox: [%f %f %f] -> [%f %f %f]\n", m_objectsBox.min.x, m_objectsBox.min.y, m_objectsBox.min.z, m_objectsBox.max.x, m_objectsBox.max.y, m_objectsBox.max.z);
        }
    }
#endif

    SetTerrainNode(m_nSID >= 0 && GetTerrain() ? GetTerrain()->FindMinNodeContainingBox(box) : NULL);
    m_pVisArea = pVisArea;
    m_pParent = pParent;

    //  for(int n=0; n<2 && m_pTerrainNode && m_pTerrainNode->m_pParent; n++)
    //  m_pTerrainNode = m_pTerrainNode->m_pParent;
    m_fpSunDirX = 63;
    m_fpSunDirZ = 0;
    m_fpSunDirYs = 0;

    m_pStaticInstancingInfo = nullptr;
    m_bStaticInstancingIsDirty = false;
}

//////////////////////////////////////////////////////////////////////////
COctreeNode* COctreeNode::Create(int nSID, const AABB& box, struct CVisArea* pVisArea, COctreeNode* pParent)
{
    MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Terrain, EMemStatContextFlags::MSF_Instance, "Octree node");
    return new COctreeNode(nSID, box, pVisArea, pParent);
}

//////////////////////////////////////////////////////////////////////////
bool COctreeNode::HasObjects()
{
    for (int l = 0; l < eRNListType_ListsNum; l++)
    {
        if (m_arrObjects[l].m_pFirstNode)
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void COctreeNode::RenderContent(int nRenderMask, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter, const CCamera* pCam)
{
    if (GetCVars()->e_StatObjBufferRenderTasks == 1 && passInfo.IsGeneralPass())
    {
        GetObjManager()->AddCullJobProducer();
    }

    if (!LegacyInternal::s_renderContentJobExecutor)
    {
        LegacyInternal::s_renderContentJobExecutor = new AZ::LegacyJobExecutor;
    }

    LegacyInternal::s_renderContentJobExecutor->StartJob(
        [this, nRenderMask, passInfo, rendItemSorter, pCam]
        {
            this->RenderContentJobEntry(nRenderMask, passInfo, rendItemSorter, pCam);
        }
    );
}

//////////////////////////////////////////////////////////////////////////
void COctreeNode::Shutdown()
{
    delete LegacyInternal::s_renderContentJobExecutor;
    LegacyInternal::s_renderContentJobExecutor = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void COctreeNode::RenderContentJobEntry(int nRenderMask, const SRenderingPassInfo& passInfo, SRendItemSorter rendItemSorter, const CCamera* pCam)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ThreeDEngine);

    SSectorTextureSet* pTerrainTexInfo = NULL;

    if (GetCVars()->e_VegetationUseTerrainColor)
    {
        GetObjManager()->FillTerrainTexInfo(this, m_fNodeDistance, pTerrainTexInfo, m_objectsBox);
    }

    if (m_arrObjects[eRNListType_Vegetation].m_pFirstNode && passInfo.RenderVegetation())
    {
        RenderVegetations(&m_arrObjects[eRNListType_Vegetation], *pCam, nRenderMask, m_bNodeCompletelyInFrustum != 0, pTerrainTexInfo, passInfo, rendItemSorter);
    }

    if (/*GetCVars()->e_SceneMerging!=3 && */ m_arrObjects[eRNListType_Brush].m_pFirstNode && passInfo.RenderBrushes())
    {
        RenderBrushes(&m_arrObjects[eRNListType_Brush], *pCam, m_bNodeCompletelyInFrustum != 0, pTerrainTexInfo, passInfo, rendItemSorter);
    }

    if (m_arrObjects[eRNListType_DecalsAndRoads].m_pFirstNode && (passInfo.RenderDecals() || passInfo.RenderRoads()))
    {
        RenderDecalsAndRoads(&m_arrObjects[eRNListType_DecalsAndRoads], *pCam, nRenderMask, m_bNodeCompletelyInFrustum != 0, pTerrainTexInfo, passInfo, rendItemSorter);
    }

    if (m_arrObjects[eRNListType_Unknown].m_pFirstNode)
    {
        RenderCommonObjects(&m_arrObjects[eRNListType_Unknown], *pCam, nRenderMask, m_bNodeCompletelyInFrustum != 0, pTerrainTexInfo, passInfo, rendItemSorter);
    }

    if (GetCVars()->e_StatObjBufferRenderTasks == 1 && passInfo.IsGeneralPass())
    {
        GetObjManager()->RemoveCullJobProducer();
    }
}

///////////////////////////////////////////////////////////////////////////////
void COctreeNode::RenderVegetations(TDoublyLinkedList<IRenderNode>* lstObjects, const CCamera& rCam, int nRenderMask, bool bNodeCompletelyInFrustum, SSectorTextureSet* pTerrainTexInfo, const SRenderingPassInfo& passInfo, SRendItemSorter& rendItemSorter)
{
    FUNCTION_PROFILER_3DENGINE;

    CVars* pCVars = GetCVars();

    AABB objBox;
    const Vec3 vCamPos = rCam.GetPosition();

    bool bCheckPerObjectOcclusion = m_vNodeAxisRadius.len2() > pCVars->e_CoverageBufferCullIndividualBrushesMaxNodeSize * pCVars->e_CoverageBufferCullIndividualBrushesMaxNodeSize;

    if (GetCVars()->e_StaticInstancing == 3 && m_pStaticInstancingInfo)
    {
        DrawBBox(m_objectsBox);
    }

    if (!m_lstVegetationsForRendering.Count())
    {
        for (IRenderNode* pObj = m_arrObjects[eRNListType_Vegetation].m_pFirstNode, * pNext; pObj; pObj = pNext)
        {
            pNext = pObj->m_pNext;

            CVegetation* pInst = (CVegetation*)pObj;

            if (pObj->m_dwRndFlags & ERF_HIDDEN)
            {
                continue;
            }

            if (pObj->m_dwRndFlags & ERF_STATIC_INSTANCING)
            {
                continue;
            }

            m_lstVegetationsForRendering.Add(pInst);
        }
    }

    int32_t lastElement = m_lstVegetationsForRendering.Count() - 1;

    for (int i = 0; i < m_lstVegetationsForRendering.Count(); i++)
    {
        rendItemSorter.IncreaseObjectCounter();
        CVegetation* pObj = (CVegetation*)m_lstVegetationsForRendering[i];

        if (pObj->m_dwRndFlags & ERF_HIDDEN)
        {
            continue;
        }

#if !defined(_RELEASE)
        if (GetCVars()->e_StaticInstancing == 2 && !pObj->m_pInstancingInfo)
        {
            continue;
        }
        if (GetCVars()->e_StaticInstancing == 3 && !pObj->m_pInstancingInfo)
        {
            continue;
        }
        if (GetCVars()->e_StaticInstancing == 4 && pObj->m_pInstancingInfo)
        {
            continue;
        }
#endif

        if (pObj->m_pInstancingInfo)
        {
            objBox = pObj->m_pInstancingInfo->m_aabbBox;
        }
        else
        {
            pObj->FillBBox_NonVirtual(objBox);
        }

        if (bNodeCompletelyInFrustum || rCam.IsAABBVisible_FM(objBox))
        {
            float fEntDistanceSq = Distance::Point_AABBSq(vCamPos, objBox) * sqr(passInfo.GetZoomFactor());

            if (fEntDistanceSq < sqr(pObj->m_fWSMaxViewDist))
            {
                float fEntDistance = sqrt_tpl(fEntDistanceSq);

                if (pCVars->e_StatObjBufferRenderTasks == 1 && passInfo.IsGeneralPass())
                {
                    // if object is visible, write to output queue for main thread processing
                    if (!bCheckPerObjectOcclusion || pObj->m_pInstancingInfo || GetObjManager()->CheckOcclusion_TestAABB(objBox, fEntDistance))
                    {
                        GetObjManager()->PushIntoCullOutputQueue(SCheckOcclusionOutput::CreateVegetationOutput(pObj, objBox, fEntDistance, pTerrainTexInfo, bCheckPerObjectOcclusion, rendItemSorter));
                    }
                }
                else
                {
                    GetObjManager()->RenderVegetation(pObj, objBox, fEntDistance, pTerrainTexInfo, bCheckPerObjectOcclusion, passInfo, rendItemSorter);
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void COctreeNode::RenderBrushes(TDoublyLinkedList<IRenderNode>* lstObjects, const CCamera& rCam, bool bNodeCompletelyInFrustum, SSectorTextureSet* pTerrainTexInfo, const SRenderingPassInfo& passInfo, SRendItemSorter& rendItemSorter)
{
    FUNCTION_PROFILER_3DENGINE;

    CVars* pCVars = GetCVars();
    const Vec3 vCamPos = rCam.GetPosition();

    float cullMaxNodeSize = static_cast<float>(pCVars->e_CoverageBufferCullIndividualBrushesMaxNodeSize);
    bool bCheckPerObjectOcclusion = GetNodeRadius2() > cullMaxNodeSize * cullMaxNodeSize;

    for (CBrush* pObj = (CBrush*)lstObjects->m_pFirstNode, * pNext; pObj; pObj = pNext)
    {
        rendItemSorter.IncreaseObjectCounter();
        pNext = (CBrush*)pObj->m_pNext;

        if (pObj->m_pNext)
        {
            cryPrefetchT0SSE(pObj->m_pNext);
        }

        IF (pObj->m_dwRndFlags & ERF_HIDDEN, 0)
        {
            continue;
        }
        AABB objBox;
        memcpy(&objBox, &pObj->m_WSBBox, sizeof(AABB));

        if (bNodeCompletelyInFrustum || rCam.IsAABBVisible_FM(objBox))
        {
            float fEntDistance = sqrt_tpl(Distance::Point_AABBSq(vCamPos, objBox)) * passInfo.GetZoomFactor();
            assert(fEntDistance >= 0 && _finite(fEntDistance));
            if (fEntDistance < pObj->m_fWSMaxViewDist)
            {
                if (pCVars->e_StatObjBufferRenderTasks == 1 && passInfo.IsGeneralPass())
                {
                    // if object is visible, start CBrush::Render Job
                    if (!bCheckPerObjectOcclusion || GetObjManager()->CheckOcclusion_TestAABB(objBox, fEntDistance))
                    {
                        GetObjManager()->RenderBrush((CBrush*)pObj, pTerrainTexInfo, objBox, fEntDistance, m_pVisArea, bCheckPerObjectOcclusion, passInfo, rendItemSorter);
                    }
                }
                else
                {
                    GetObjManager()->RenderBrush(pObj, pTerrainTexInfo, pObj->m_WSBBox, fEntDistance, m_pVisArea, bCheckPerObjectOcclusion, passInfo, rendItemSorter);
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void COctreeNode::RenderDecalsAndRoads(TDoublyLinkedList<IRenderNode>* lstObjects, const CCamera& rCam, int nRenderMask, bool bNodeCompletelyInFrustum, SSectorTextureSet* pTerrainTexInfo, const SRenderingPassInfo& passInfo, SRendItemSorter& rendItemSorter)
{
    FUNCTION_PROFILER_3DENGINE;

    CVars* pCVars = GetCVars();
    AABB objBox;
    const Vec3 vCamPos = rCam.GetPosition();

    bool bCheckPerObjectOcclusion = m_vNodeAxisRadius.len2() > pCVars->e_CoverageBufferCullIndividualBrushesMaxNodeSize * pCVars->e_CoverageBufferCullIndividualBrushesMaxNodeSize;

    for (IRenderNode* pObj = lstObjects->m_pFirstNode, * pNext; pObj; pObj = pNext)
    {
        rendItemSorter.IncreaseObjectCounter();
        pNext = pObj->m_pNext;

        if (pObj->m_pNext)
        {
            cryPrefetchT0SSE(pObj->m_pNext);
        }

        IF (pObj->m_dwRndFlags & ERF_HIDDEN, 0)
        {
            continue;
        }

        pObj->FillBBox(objBox);

        if (bNodeCompletelyInFrustum || rCam.IsAABBVisible_FM(objBox))
        {
            float fEntDistance = sqrt_tpl(Distance::Point_AABBSq(vCamPos, objBox)) * passInfo.GetZoomFactor();
            assert(fEntDistance >= 0 && _finite(fEntDistance));
            if (fEntDistance < pObj->m_fWSMaxViewDist)
            {
#if !defined(_RELEASE)
                EERType rnType = pObj->GetRenderNodeType();
                if (!passInfo.RenderDecals() && rnType == eERType_Decal)
                {
                    continue;
                }
                if (!passInfo.RenderRoads() && rnType == eERType_Road)
                {
                    continue;
                }
#endif // _RELEASE

                if (pCVars->e_StatObjBufferRenderTasks == 1 && passInfo.IsGeneralPass())
                {
                    // if object is visible, write to output queue for main thread processing
                    if (GetObjManager()->CheckOcclusion_TestAABB(objBox, fEntDistance))
                    {
                        GetObjManager()->PushIntoCullOutputQueue(SCheckOcclusionOutput::CreateDecalsAndRoadsOutput(pObj, objBox, fEntDistance, bCheckPerObjectOcclusion, rendItemSorter));
                    }
                }
                else
                {
                    GetObjManager()->RenderDecalAndRoad(pObj, objBox, fEntDistance, bCheckPerObjectOcclusion, passInfo, rendItemSorter);
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void COctreeNode::RenderCommonObjects(TDoublyLinkedList<IRenderNode>* lstObjects, const CCamera& rCam, int nRenderMask, bool bNodeCompletelyInFrustum, SSectorTextureSet* pTerrainTexInfo, const SRenderingPassInfo& passInfo, SRendItemSorter& rendItemSorter)
{
    FUNCTION_PROFILER_3DENGINE;

    CVars* pCVars = GetCVars();
    AABB objBox;
    const Vec3 vCamPos = rCam.GetPosition();

    for (IRenderNode* pObj = lstObjects->m_pFirstNode, * pNext; pObj; pObj = pNext)
    {
        rendItemSorter.IncreaseObjectCounter();
        pNext = pObj->m_pNext;

        if (pObj->m_pNext)
        {
            cryPrefetchT0SSE(pObj->m_pNext);
        }

        IF (pObj->m_dwRndFlags & ERF_HIDDEN, 0)
        {
            continue;
        }

        pObj->FillBBox(objBox);
        EERType rnType = pObj->GetRenderNodeType();

        if (bNodeCompletelyInFrustum || rCam.IsAABBVisible_FM(objBox))
        {
            float fEntDistance = sqrt_tpl(Distance::Point_AABBSq(vCamPos, objBox)) * passInfo.GetZoomFactor();
            assert(fEntDistance >= 0 && _finite(fEntDistance));
            if (fEntDistance < pObj->m_fWSMaxViewDist)
            {
                if (nRenderMask & OCTREENODE_RENDER_FLAG_OBJECTS_ONLY_ENTITIES)
                {
                    if (rnType != eERType_RenderComponent && rnType != eERType_DynamicMeshRenderComponent && rnType != eERType_SkinnedMeshRenderComponent)
                    {
                        if (rnType == eERType_Light)
                        {
                            CLightEntity* pEnt = (CLightEntity*)pObj;
                            if (!pEnt->GetEntityVisArea() && pEnt->GetEntityTerrainNode() && !(pEnt->m_light.m_Flags & DLF_THIS_AREA_ONLY))
                            { // not "this area only" outdoor light affects everything
                            }
                            else
                            {
                                continue;
                            }
                        }
                        else
                        {
                            continue;
                        }
                    }
                }

                if (rnType == eERType_Light)
                {
                    bool bLightVisible = true;
                    CLightEntity* pLightEnt = (CLightEntity*)pObj;

                    // first check against camera view frustum
                    CDLight* pLight = &pLightEnt->m_light;
                    if (pLight->m_Flags & DLF_DEFERRED_CUBEMAPS)
                    {
                        OBB obb(OBB::CreateOBBfromAABB(Matrix33(pLight->m_ObjMatrix), AABB(-pLight->m_ProbeExtents, pLight->m_ProbeExtents)));
                        bLightVisible = passInfo.GetCamera().IsOBBVisible_F(pLight->m_Origin, obb);
                    }
                    else if (pLightEnt->m_light.m_Flags & DLF_AREA_LIGHT)
                    {
                        // OBB test for area lights.
                        Vec3 vBoxMax(pLight->m_fBaseRadius, pLight->m_fBaseRadius + pLight->m_fAreaWidth, pLight->m_fBaseRadius + pLight->m_fAreaHeight);
                        Vec3 vBoxMin(-0.1f, -(pLight->m_fBaseRadius + pLight->m_fAreaWidth), -(pLight->m_fBaseRadius + pLight->m_fAreaHeight));

                        OBB obb(OBB::CreateOBBfromAABB(Matrix33(pLight->m_ObjMatrix), AABB(vBoxMin, vBoxMax)));
                        bLightVisible = rCam.IsOBBVisible_F(pLight->m_Origin, obb);
                    }
                    else
                    {
                        bLightVisible = rCam.IsSphereVisible_F(Sphere(pLight->m_BaseOrigin, pLight->m_fBaseRadius));
                    }

                    if (!bLightVisible)
                    {
                        continue;
                    }
                }

                if (pCVars->e_StatObjBufferRenderTasks == 1 && passInfo.IsGeneralPass())
                {
                    // if object is visible, write to output queue for main thread processing
                    if (rnType == eERType_DistanceCloud || GetObjManager()->CheckOcclusion_TestAABB(objBox, fEntDistance))
                    {
                        if (pObj->CanExecuteRenderAsJob())
                        {
                            GetObjManager()->RenderObject(pObj, objBox, fEntDistance, eERType_RenderComponent, passInfo, rendItemSorter);
                        }
                        else
                        {
                            GetObjManager()->PushIntoCullOutputQueue(SCheckOcclusionOutput::CreateCommonObjectOutput(pObj, objBox, fEntDistance, pTerrainTexInfo, rendItemSorter));
                        }
                    }
                }
                else
                {
                    GetObjManager()->RenderObject(pObj, objBox, fEntDistance, rnType, passInfo, rendItemSorter);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void COctreeNode::UnlinkObject(IRenderNode* pObj)
{
    ERNListType eListType = pObj->GetRenderNodeListId(pObj->GetRenderNodeType());

    assert(eListType >= 0 && eListType < eRNListType_ListsNum);
    TDoublyLinkedList<IRenderNode>& rList = m_arrObjects[eListType];

    assert(pObj != pObj->m_pPrev && pObj != pObj->m_pNext);
    assert(!rList.m_pFirstNode || !rList.m_pFirstNode->m_pPrev);
    assert(!rList.m_pLastNode || !rList.m_pLastNode->m_pNext);

    if (pObj->m_pNext || pObj->m_pPrev || pObj == rList.m_pLastNode || pObj == rList.m_pFirstNode)
    {
        rList.remove(pObj);
    }

    assert(!rList.m_pFirstNode || !rList.m_pFirstNode->m_pPrev);
    assert(!rList.m_pLastNode || !rList.m_pLastNode->m_pNext);
    assert(pObj != pObj->m_pPrev && pObj != pObj->m_pNext);
    assert(!pObj->m_pNext && !pObj->m_pPrev);
}

bool COctreeNode::DeleteObject(IRenderNode* pObj)
{
    FUNCTION_PROFILER_3DENGINE;

    if (pObj->m_pOcNode && pObj->m_pOcNode != this)
    {
        return ((COctreeNode*)(pObj->m_pOcNode))->DeleteObject(pObj);
    }

    UnlinkObject(pObj);

    for (int i = 0; i < m_lstCasters.Count(); i++)
    {
        if (m_lstCasters[i].pNode == pObj)
        {
            m_lstCasters.Delete(i);
            break;
        }
    }

    if (pObj->GetRenderNodeType() == eERType_Vegetation)
    {
        m_lstVegetationsForRendering.Delete(pObj);
    }

    bool bSafeToUse = Get3DEngine()->IsObjectTreeReady();

    pObj->m_pOcNode = NULL;
    pObj->m_nSID = -1;

    if (bSafeToUse && IsEmpty() && m_arrEmptyNodes.Find(this) < 0)
    {
        m_arrEmptyNodes.Add(this);
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void COctreeNode::InsertObject(IRenderNode* pObj, const AABB& objBox, const float fObjRadiusSqr, const Vec3& vObjCenter)
{
    FUNCTION_PROFILER_3DENGINE;

    COctreeNode* pCurrentNode = this;

    EERType eType = pObj->GetRenderNodeType();
    const uint32 renderFlags = (pObj->GetRndFlags() & (ERF_GOOD_OCCLUDER | ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS));

    const bool bTypeLight = (eType == eERType_Light);
    const float fViewDistRatioVegetation = GetCVars()->e_ViewDistRatioVegetation;
    const float fWSMaxViewDist = pObj->m_fWSMaxViewDist;
    const bool bTypeRoad  =   (eType == eERType_Road);

    Vec3 vObjectCentre = vObjCenter;

    while (true)
    {
        PrefetchLine(&pCurrentNode->m_arrChilds[0], 0);

#if !defined(_RELEASE)
        if (GetCVars()->e_CheckOctreeObjectsBoxSize != 0) // pCurrentNode->m_pParent is checked as silly sized things are added to the root (e.g. the sun)
        {
            if (pCurrentNode->m_pParent && !objBox.IsReset() && (objBox.min.len() > CHECK_OBJECTS_BOX_WARNING_SIZE || objBox.max.len() > CHECK_OBJECTS_BOX_WARNING_SIZE))
            {
                CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_ERROR_DBGBRK, "Huge object being added to a COctreeNode, name: '%s', objBox: [%f %f %f] -> [%f %f %f]\n", pObj->GetName(), objBox.min.x, objBox.min.y, objBox.min.z, objBox.max.x, objBox.max.y, objBox.max.z);
            }
        }
#endif

        // parent bbox includes all children
        pCurrentNode->m_objectsBox.Add(objBox);

        pCurrentNode->m_fObjectsMaxViewDist = max(pCurrentNode->m_fObjectsMaxViewDist, fWSMaxViewDist);

        pCurrentNode->m_renderFlags |= renderFlags;

        pCurrentNode->m_bHasLights  |= (bTypeLight);
        pCurrentNode->m_bHasRoads       |= (bTypeRoad);

        if (pCurrentNode->m_vNodeAxisRadius.x * 2.0f > fNodeMinSize) // store voxels and roads in root
        {
            float nodeRadius = sqrt(pCurrentNode->GetNodeRadius2());

            if (fObjRadiusSqr < sqr(nodeRadius * fObjectToNodeSizeRatio))
            {
                if (fWSMaxViewDist < nodeRadius * fViewDistRatioVegetation)
                {
                    int nChildId =
                        ((vObjCenter.x > pCurrentNode->m_vNodeCenter.x) ? 4 : 0) |
                        ((vObjCenter.y > pCurrentNode->m_vNodeCenter.y) ? 2 : 0) |
                        ((vObjCenter.z > pCurrentNode->m_vNodeCenter.z) ? 1 : 0);

                    if (!pCurrentNode->m_arrChilds[nChildId])
                    {
                        pCurrentNode->m_arrChilds[nChildId] = COctreeNode::Create(pCurrentNode->m_nSID, pCurrentNode->GetChildBBox(nChildId), pCurrentNode->m_pVisArea, pCurrentNode);
                    }

                    pCurrentNode = pCurrentNode->m_arrChilds[nChildId];

                    continue;
                }
            }
        }

        break;
    }

    //disable as it leads to some corruption on 360
    //  PrefetchLine(&pObj->m_pOcNode, 0);  //Writing to m_pOcNode was a common L2 cache miss

    pCurrentNode->LinkObject(pObj, eType);

    pObj->m_pOcNode = pCurrentNode;
    pObj->m_nSID = pCurrentNode->m_nSID;

    // only mark octree nodes as not compiled during loading and in the editor
    // otherwise update node (and parent node) flags on per added object basis
    if (m_bLevelLoadingInProgress || gEnv->IsEditor())
    {
        pCurrentNode->SetCompiled(pCurrentNode->IsCompiled() & (eType == eERType_Light));
    }
    else
    {
        pCurrentNode->UpdateObjects(pObj);
    }

    pCurrentNode->m_nManageVegetationsFrameId = 0;
}

//////////////////////////////////////////////////////////////////////////
AABB COctreeNode::GetChildBBox(int nChildId)
{
    int x = (nChildId / 4);
    int y = (nChildId - x * 4) / 2;
    int z = (nChildId - x * 4 - y * 2);
    const Vec3& vSize = m_vNodeAxisRadius;
    Vec3 vOffset = vSize;
    vOffset.x *= x;
    vOffset.y *= y;
    vOffset.z *= z;
    AABB childBox;
    childBox.min = m_vNodeCenter - vSize + vOffset;
    childBox.max = childBox.min + vSize;
    return childBox;
}

//////////////////////////////////////////////////////////////////////////
bool COctreeNode::IsEmpty()
{
    if (m_pParent)
    {
        if (!m_arrChilds[0] && !m_arrChilds[1] && !m_arrChilds[2] && !m_arrChilds[3])
        {
            if (!m_arrChilds[4] && !m_arrChilds[5] && !m_arrChilds[6] && !m_arrChilds[7])
            {
                if (!HasObjects())
                {
                    return true;
                }
            }
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool COctreeNode::IsRightNode(const AABB& objBox, const float fObjRadiusSqr, float fObjMaxViewDist)
{
    const AABB& nodeBox = GetNodeBox();
    if (!Overlap::Point_AABB(objBox.GetCenter(), nodeBox))
    {
        if (m_pParent)
        {
            return false; // fail if center is not inside or node bbox
        }
    }
    if (2 != Overlap::AABB_AABB_Inside(objBox, m_objectsBox))
    {
        return false; // fail if not completely inside of objects bbox
    }
    float fNodeRadiusRated = GetNodeRadius2() * sqr(fObjectToNodeSizeRatio);

    if (fObjRadiusSqr > fNodeRadiusRated * 4.f)
    {
        if (m_pParent)
        {
            return false; // fail if object is too big and we need to register it some of parents
        }
    }
    if (m_vNodeAxisRadius.x * 2.0f > fNodeMinSize)
    {
        if (fObjRadiusSqr < fNodeRadiusRated)
        {
            //      if(fObjMaxViewDist < m_fNodeRadius*GetCVars()->e_ViewDistRatioVegetation*fObjectToNodeSizeRatio)
            return false; // fail if object is too small and we need to register it some of childs
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
void COctreeNode::LinkObject(IRenderNode* pObj, EERType eERType, bool bPushFront)
{
    ERNListType eListType = pObj->GetRenderNodeListId(eERType);

    TDoublyLinkedList<IRenderNode>& rList = m_arrObjects[eListType];

    assert(pObj != pObj->m_pPrev && pObj != pObj->m_pNext);
    assert(!pObj->m_pNext && !pObj->m_pPrev);
    assert(!rList.m_pFirstNode || !rList.m_pFirstNode->m_pPrev);
    assert(!rList.m_pLastNode || !rList.m_pLastNode->m_pNext);

    if (bPushFront)
    {
        rList.insertBeginning(pObj);
    }
    else
    {
        rList.insertEnd(pObj);
    }

    assert(!rList.m_pFirstNode || !rList.m_pFirstNode->m_pPrev);
    assert(!rList.m_pLastNode || !rList.m_pLastNode->m_pNext);
    assert(pObj != pObj->m_pPrev && pObj != pObj->m_pNext);
}

//////////////////////////////////////////////////////////////////////////
void COctreeNode::UpdateObjects(IRenderNode* pObj)
{
    FUNCTION_PROFILER_3DENGINE;

    float fObjMaxViewDistance = 0;
    size_t numCasters = 0;
    IObjManager* pObjManager = GetObjManager();

    bool bVegetHasAlphaTrans = false;
    int nFlags = pObj->GetRndFlags();
    EERType eRType = pObj->GetRenderNodeType();

    IF (nFlags & ERF_HIDDEN, 0)
    {
        return;
    }

    const Vec3& sunDir = Get3DEngine()->GetSunDirNormalized();
    uint32 sunDirX = (uint32)(sunDir.x * 63.5f + 63.5f);
    uint32 sunDirZ = (uint32)(sunDir.z * 63.5f + 63.5f);
    uint32 sunDirYs = (uint32)(sunDir.y < 0.0f ? 1 : 0);

    pObj->m_nInternalFlags &= ~(IRenderNode::REQUIRES_FORWARD_RENDERING | IRenderNode::REQUIRES_NEAREST_CUBEMAP);

    if (eRType == eERType_Vegetation)
    {
        CVegetation* pInst = (CVegetation*)pObj;
        pInst->UpdateRndFlags();

        StatInstGroup& vegetGroup = pInst->GetStatObjGroup();
        if (vegetGroup.pStatObj && vegetGroup.bUseAlphaBlending)
        {
            bVegetHasAlphaTrans = true;
        }

        bool bUseTerrainColor((vegetGroup.bUseTerrainColor && GetCVars()->e_VegetationUseTerrainColor) || GetCVars()->e_VegetationUseTerrainColor == 2);
        if (bUseTerrainColor)
        {
            pInst->UpdateSunDotTerrain();
        }
    }

    // update max view distances
    const float fNewMaxViewDist = pObj->GetMaxViewDist();
    pObj->m_fWSMaxViewDist = fNewMaxViewDist;

    if (eRType != eERType_Light && eRType != eERType_Cloud && eRType != eERType_FogVolume && eRType != eERType_Decal && eRType != eERType_Road && eRType != eERType_DistanceCloud)
    {
        if (eRType == eERType_ParticleEmitter)
        {
            pObj->m_nInternalFlags |= IRenderNode::REQUIRES_FORWARD_RENDERING | IRenderNode::REQUIRES_NEAREST_CUBEMAP;
        }
        else
        {
            CMatInfo* pMatInfo = (CMatInfo*)pObj->GetMaterial().get();
            if (pMatInfo)
            {
                if (bVegetHasAlphaTrans || pMatInfo->IsForwardRenderingRequired())
                {
                    pObj->m_nInternalFlags |= IRenderNode::REQUIRES_FORWARD_RENDERING;
                }

                if (pMatInfo->IsNearestCubemapRequired())
                {
                    pObj->m_nInternalFlags |= IRenderNode::REQUIRES_NEAREST_CUBEMAP;
                }
            }

            if (eRType == eERType_RenderComponent || eRType == eERType_DynamicMeshRenderComponent || eRType == eERType_SkinnedMeshRenderComponent)
            {
                int nSlotCount = pObj->GetSlotCount();

                for (int s = 0; s < nSlotCount; s++)
                {
                    if (CMatInfo* pMat = (CMatInfo*)pObj->GetEntitySlotMaterial(s).get())
                    {
                        if (pMat->IsForwardRenderingRequired())
                        {
                            pObj->m_nInternalFlags |= IRenderNode::REQUIRES_FORWARD_RENDERING;
                        }
                        if (pMat->IsNearestCubemapRequired())
                        {
                            pObj->m_nInternalFlags |= IRenderNode::REQUIRES_NEAREST_CUBEMAP;
                        }
                    }

                    if (IStatObj* pStatObj = pObj->GetEntityStatObj(s))
                    {
                        if (CMatInfo* pMat = (CMatInfo*)pStatObj->GetMaterial().get())
                        {
                            if (pMat->IsForwardRenderingRequired())
                            {
                                pObj->m_nInternalFlags |= IRenderNode::REQUIRES_FORWARD_RENDERING;
                            }
                            if (pMat->IsNearestCubemapRequired())
                            {
                                pObj->m_nInternalFlags |= IRenderNode::REQUIRES_NEAREST_CUBEMAP;
                            }
                        }
                    }
                }

                if (!(pObj->m_nInternalFlags & (IRenderNode::REQUIRES_FORWARD_RENDERING | IRenderNode::REQUIRES_NEAREST_CUBEMAP)))
                {
                    CompileCharacter(pObj->GetEntityCharacter(0), pObj->m_nInternalFlags);
                }
            }
        }
    }

    bool bUpdateParentShadowFlags = false;
    bool bUpdateParentOcclusionFlags = false;

    // fill shadow casters list
    const bool bHasPerObjectShadow = GetCVars()->e_ShadowsPerObject && Get3DEngine()->GetPerObjectShadow(pObj);
    if (nFlags & ERF_CASTSHADOWMAPS && fNewMaxViewDist > fMinShadowCasterViewDist && eRType != eERType_Light && !bHasPerObjectShadow)
    {
        bUpdateParentShadowFlags = true;

        float fMaxCastDist = fNewMaxViewDist * GetCVars()->e_ShadowsCastViewDistRatio;
        m_lstCasters.Add(SCasterInfo(pObj, fMaxCastDist, eRType));

        if (pObj->GetRenderNodeType() == eERType_Vegetation && ((CVegetation*)pObj)->m_pInstancingInfo)
        {
            AABB objBox = ((CVegetation*)pObj)->m_pInstancingInfo->m_aabbBox;
            m_lstCasters.Last().objSphere.center = objBox.GetCenter();
            m_lstCasters.Last().objSphere.radius = objBox.GetRadius();
        }
    }

    fObjMaxViewDistance = max(fObjMaxViewDistance, fNewMaxViewDist);

    // traverse the octree upwards and fill in new flags
    COctreeNode* pNode = this;
    bool bContinue = false;
    do
    {
        // update max view dist
        if (pNode->m_fObjectsMaxViewDist < fObjMaxViewDistance)
        {
            pNode->m_fObjectsMaxViewDist = fObjMaxViewDistance;
            bContinue = true;
        }

        // update shadow flags
        if (bUpdateParentShadowFlags && (pNode->m_renderFlags & ERF_CASTSHADOWMAPS) == 0)
        {
            pNode->m_renderFlags |= ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS;
            bContinue = true;
        }

        pNode = pNode->m_pParent;
    } while (pNode != NULL && bContinue);

    m_fpSunDirX = sunDirX;
    m_fpSunDirZ = sunDirZ;
    m_fpSunDirYs = sunDirYs;
}

//////////////////////////////////////////////////////////////////////////
void COctreeNode::CompileCharacter(ICharacterInstance* pChar, unsigned char& nInternalFlags)
{
    const uint32 nCompileMask = IRenderNode::REQUIRES_FORWARD_RENDERING | IRenderNode::REQUIRES_NEAREST_CUBEMAP;
    if (pChar)
    {
        CMatInfo* pCharMaterial = (CMatInfo*)pChar->GetIMaterial().get();

        if (pCharMaterial)
        {
            uint8 nInternalFlagsNew = pCharMaterial->IsForwardRenderingRequired() ? IRenderNode::REQUIRES_FORWARD_RENDERING : 0;
            nInternalFlagsNew |= pCharMaterial->IsNearestCubemapRequired() ? IRenderNode::REQUIRES_NEAREST_CUBEMAP : 0;
            nInternalFlags |= nInternalFlagsNew;

            if (nInternalFlagsNew == nCompileMask) // can do trivial return (all flags used)
            {
                return;
            }
        }

        if (IAttachmentManager* pAttMan = pChar->GetIAttachmentManager())
        {
            int nCount = pAttMan->GetAttachmentCount();
            for (int i = 0; i < nCount; i++)
            {
                if (IAttachment* pAtt = pAttMan->GetInterfaceByIndex(i))
                {
                    if (IAttachmentObject* pAttObj = pAtt->GetIAttachmentObject())
                    {
                        ICharacterInstance* pCharInstance = pAttObj->GetICharacterInstance();
                        if (pCharInstance)
                        {
                            CompileCharacter(pCharInstance, nInternalFlags);

                            if (nInternalFlags == nCompileMask)     // can do trivial return (all flags used)
                            {
                                return;
                            }
                        }

                        if (IStatObj* pStatObj = pAttObj->GetIStatObj())
                        {
                            CMatInfo* pMat = (CMatInfo*)pAttObj->GetBaseMaterial().get();
                            if (pMat == 0)
                            {
                                pMat = (CMatInfo*)pStatObj->GetMaterial().get();
                            }

                            if (pMat)
                            {
                                uint8 nInternalFlagsNew = pMat->IsForwardRenderingRequired() ? IRenderNode::REQUIRES_FORWARD_RENDERING : 0;
                                nInternalFlagsNew |= pMat->IsNearestCubemapRequired() ? IRenderNode::REQUIRES_NEAREST_CUBEMAP : 0;
                                nInternalFlags |= nInternalFlagsNew;

                                if (nInternalFlagsNew == nCompileMask) // can do trivial return (all flags used)
                                {
                                    return;
                                }
                            }
                        }

                        if (IAttachmentSkin* pAttachmentSkin = pAttObj->GetIAttachmentSkin())
                        {
                            ISkin* pSkin = pAttachmentSkin->GetISkin();
                            if (pSkin)
                            {
                                CMatInfo* pMat = (CMatInfo*)pAttObj->GetBaseMaterial().get();
                                if (pMat == 0)
                                {
                                    pMat = (CMatInfo*)pSkin->GetIMaterial(0).get();
                                }

                                if (pMat)
                                {
                                    uint8 nInternalFlagsNew = pMat->IsForwardRenderingRequired() ? IRenderNode::REQUIRES_FORWARD_RENDERING : 0;
                                    nInternalFlagsNew |= pMat->IsNearestCubemapRequired() ? IRenderNode::REQUIRES_NEAREST_CUBEMAP : 0;
                                    nInternalFlags |= nInternalFlagsNew;

                                    if (nInternalFlagsNew == nCompileMask)  // can do trivial return (all flags used)
                                    {
                                        return;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
int16 CObjManager::GetNearestCubeProbe(IVisArea* pVisArea, const AABB& objBox, bool bSpecular)
{
    // Only used for alpha blended geometry - but still should be optimized further
    float fMinDistance = FLT_MAX;
    int nMaxPriority = -1;
    CLightEntity* pNearestLight = NULL;
    int16 nDefaultId = Get3DEngine()->GetBlackCMTexID();

    if (!pVisArea)
    {
        if (Get3DEngine()->IsObjectTreeReady())
        {
            Get3DEngine()->GetObjectTree()->GetNearestCubeProbe(fMinDistance, nMaxPriority, pNearestLight, &objBox);
        }
    }
    else
    {
        Get3DEngine()->GetVisAreaManager()->GetNearestCubeProbe(fMinDistance, nMaxPriority, pNearestLight, &objBox);
    }

    if (pNearestLight)
    {
        ITexture* pTexCM = bSpecular ? pNearestLight->m_light.GetSpecularCubemap() : pNearestLight->m_light.GetDiffuseCubemap();
        // Return cubemap ID or -1 if invalid
        return (pTexCM && pTexCM->GetTextureType() >= eTT_Cube) ? pTexCM->GetTextureID() : nDefaultId;
    }

    // No cubemap found
    return nDefaultId;
}

//////////////////////////////////////////////////////////////////////////
bool CObjManager::IsAfterWater(const Vec3& vPos, const SRenderingPassInfo& passInfo)
{
    // considered "after water" if any of the following is true:
    //    - Position & camera are on the same side of the water surface on recursive level 0
    //    - Position & camera are on opposite sides of the water surface on recursive level 1

    float fWaterLevel;
    if (OceanToggle::IsActive())
    {
        if (!OceanRequest::OceanIsEnabled())
        {
            return true;
        }
        fWaterLevel = OceanRequest::GetOceanLevel();
    }
    else
    {
        fWaterLevel = GetTerrain() ? GetTerrain()->GetWaterLevel() : WATER_LEVEL_UNKNOWN;
    }

    return (0.5f - passInfo.GetRecursiveLevel()) * (0.5f - passInfo.IsCameraUnderWater()) * (vPos.z - fWaterLevel) > 0;
}

//////////////////////////////////////////////////////////////////////////
void CObjManager::RenderObjectDebugInfo(IRenderNode* pEnt, float fEntDistance,  const SRenderingPassInfo& passInfo)
{
    if (!passInfo.IsGeneralPass())
    {
        return;
    }

    m_arrRenderDebugInfo.push_back(SObjManRenderDebugInfo(pEnt, fEntDistance));
}

//////////////////////////////////////////////////////////////////////////
void CObjManager::FillTerrainTexInfo(IOctreeNode* pOcNode, float fEntDistance, struct SSectorTextureSet*& pTerrainTexInfo, const AABB& objBox)
{
    IVisArea* pVisArea = pOcNode->m_pVisArea;
    CTerrainNode* pTerrainNode = pOcNode->GetTerrainNode();

    if ((!pVisArea || pVisArea->IsAffectedByOutLights()) && pTerrainNode)
    { // provide terrain texture info
        AABB boxCenter;
        boxCenter.min = boxCenter.max = objBox.GetCenter();

        if (CTerrainNode* pTerNode = pTerrainNode)
        {
            if (pTerNode = pTerNode->FindMinNodeContainingBox(boxCenter))
            {
                SSectorTextureSet* terrainInfo = &pTerNode->m_TextureSet;
                float terrainNodeSize = pTerNode->m_LocalAABB.max.x - pTerNode->m_LocalAABB.min.x;
                while (fEntDistance * 2.f * 8.f > terrainNodeSize && pTerNode->m_Parent)
                {
                    if (pTerNode->m_TextureSet.nTex0)
                    {
                        terrainInfo = &pTerNode->m_TextureSet;
                    }
                    pTerNode = pTerNode->m_Parent;
                    terrainNodeSize = pTerNode->m_LocalAABB.max.x - pTerNode->m_LocalAABB.min.x;
                }

                pTerrainTexInfo = terrainInfo;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjManager::RenderBrush(CBrush* pEnt,
    const SSectorTextureSet* pTerrainTexInfo,
    const AABB& objBox,
    float fEntDistance,
    CVisArea* pVisArea, bool nCheckOcclusion,
    const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter)
{
    //  FUNCTION_PROFILER_3DENGINE;
    const CVars* pCVars = GetCVars();

#ifdef _DEBUG
    const char* szName = pEnt->GetName();
    const char* szClassName = pEnt->GetEntityClassName();
#endif // _DEBUG

#if !defined(_RELEASE)
    if (GetCVars()->e_CoverageBufferShowOccluder)
    {
        if (GetCVars()->e_CoverageBufferShowOccluder == 1 && (ERF_GOOD_OCCLUDER & ~pEnt->m_dwRndFlags))
        {
            return;
        }
        if (GetCVars()->e_CoverageBufferShowOccluder == 2 && (ERF_GOOD_OCCLUDER & pEnt->m_dwRndFlags))
        {
            return;
        }
    }
#endif
    if (pEnt->m_dwRndFlags & ERF_HIDDEN)
    {
        return;
    }

    // check cvars
    assert(passInfo.RenderBrushes());

    // check-allocate RNTmpData for visible objects
    Get3DEngine()->CheckCreateRNTmpData(&pEnt->m_pRNTmpData, pEnt, passInfo);
    pEnt->SetDrawFrame(passInfo.GetFrameID(), passInfo.GetRecursiveLevel());


    if (nCheckOcclusion && pEnt->m_pOcNode)
    {
        if (GetObjManager()->IsBoxOccluded(objBox, fEntDistance * passInfo.GetInverseZoomFactor(), &pEnt->m_pRNTmpData->userData.m_OcclState,
                pEnt->m_pOcNode->m_pVisArea != NULL, eoot_OBJECT, passInfo))
        {
            return;
        }
    }
    assert(pEnt && pEnt->m_pRNTmpData);
    if (!pEnt || !pEnt->m_pRNTmpData)
    {
        return;
    }

    //////////////////////////////////////////////////////////////////////////
    const CLodValue lodValue = pEnt->ComputeLod(pEnt->m_pRNTmpData->userData.nWantedLod, passInfo);
    pEnt->Render(lodValue, passInfo, pTerrainTexInfo, gEnv->pRenderer->GetGenerateRendItemJobExecutor(passInfo.ThreadID()), rendItemSorter);
}

//////////////////////////////////////////////////////////////////////////
void CObjManager::RemoveCullJobProducer()
{
    m_CheckOcclusionOutputQueue.RemoveProducer();
}

//////////////////////////////////////////////////////////////////////////
void CObjManager::AddCullJobProducer()
{
    m_CheckOcclusionOutputQueue.AddProducer();
}

//////////////////////////////////////////////////////////////////////////
bool CObjManager::CheckOcclusion_TestAABB(const AABB& rAABB, float fEntDistance)
{
    return m_CullThread.TestAABB(rAABB, fEntDistance);
}


//////////////////////////////////////////////////////////////////////////
bool CObjManager::CheckOcclusion_TestQuad(const Vec3& vCenter, const Vec3& vAxisX, const Vec3& vAxisY)
{
    return m_CullThread.TestQuad(vCenter, vAxisX, vAxisY);
}

#ifndef _RELEASE
//////////////////////////////////////////////////////////////////////////
void CObjManager::CoverageBufferDebugDraw()
{
    m_CullThread.CoverageBufferDebugDraw();
}
#endif

//////////////////////////////////////////////////////////////////////////
bool CObjManager::LoadOcclusionMesh(const char* pFileName)
{
    return m_CullThread.LoadLevel(pFileName);
}

//////////////////////////////////////////////////////////////////////////
void CObjManager::PushIntoCullQueue(const SCheckOcclusionJobData& rCheckOcclusionData)
{
#if !defined(_RELEASE)
    IF (!m_CullThread.IsActive(), 0)
    {
        __debugbreak();
    }
    IF (rCheckOcclusionData.type == SCheckOcclusionJobData::QUIT, 0)
    {
        m_CullThread.SetActive(false);
    }
#endif
    m_CheckOcclusionQueue.Push(rCheckOcclusionData);
}

//////////////////////////////////////////////////////////////////////////
void CObjManager::PopFromCullQueue(SCheckOcclusionJobData* pCheckOcclusionData)
{
    m_CheckOcclusionQueue.Pop(pCheckOcclusionData);
}

//////////////////////////////////////////////////////////////////////////
void CObjManager::PushIntoCullOutputQueue(const SCheckOcclusionOutput& rCheckOcclusionOutput)
{
    m_CheckOcclusionOutputQueue.Push(rCheckOcclusionOutput);
}

//////////////////////////////////////////////////////////////////////////
bool CObjManager::PopFromCullOutputQueue(SCheckOcclusionOutput* pCheckOcclusionOutput)
{
    return m_CheckOcclusionOutputQueue.Pop(pCheckOcclusionOutput);
}

///////////////////////////////////////////////////////////////////////////////
uint8 CObjManager::GetDissolveRef(float fDist, float fMVD)
{
    float fDissolveDist = 1.0f / CLAMP(0.1f * fMVD, GetFloatCVar(e_DissolveDistMin), GetFloatCVar(e_DissolveDistMax));

    return (uint8)SATURATEB((1.0f + (fDist - fMVD) * fDissolveDist) * 255.f);
}

///////////////////////////////////////////////////////////////////////////////
float CObjManager::GetLodDistDissolveRef(SLodDistDissolveTransitionState* pState, float curDist, int nNewLod, const SRenderingPassInfo& passInfo)
{
    float fDissolveDistbandClamped = min(GetFloatCVar(e_DissolveDistband), curDist * .4f) + 0.001f;

    if (!pState->fStartDist)
    {
        pState->fStartDist = curDist;
        pState->nOldLod = nNewLod;
        pState->nNewLod = nNewLod;

        pState->bFarside = (pState->nNewLod < pState->nOldLod && pState->nNewLod != -1) || pState->nOldLod == -1;
    }
    else if (pState->nNewLod != nNewLod)
    {
        pState->nNewLod = nNewLod;
        pState->fStartDist = curDist;

        pState->bFarside = (pState->nNewLod < pState->nOldLod && pState->nNewLod != -1) || pState->nOldLod == -1;
    }
    else if ((pState->nOldLod != pState->nNewLod))
    {
        // transition complete
        if (
            (!pState->bFarside && curDist - pState->fStartDist > fDissolveDistbandClamped) ||
            (pState->bFarside && pState->fStartDist - curDist > fDissolveDistbandClamped)
            )
        {
            pState->nOldLod = pState->nNewLod;
        }
        // with distance based transitions we can always 'fail' back to the previous LOD.
        else if (
            (!pState->bFarside && curDist < pState->fStartDist) ||
            (pState->bFarside && curDist > pState->fStartDist)
            )
        {
            pState->nNewLod = pState->nOldLod;
        }
    }

    if (pState->nOldLod == pState->nNewLod)
    {
        return 0.f;
    }
    else
    {
        if (pState->bFarside)
        {
            return SATURATE(((pState->fStartDist - curDist) * (1.f / fDissolveDistbandClamped)));
        }
        else
        {
            return SATURATE(((curDist - pState->fStartDist) * (1.f / fDissolveDistbandClamped)));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
int CObjManager::GetObjectLOD(const IRenderNode* pObj, float fDistance)
{
    SFrameLodInfo frameLodInfo = Get3DEngine()->GetFrameLodInfo();
    int resultLod = MAX_STATOBJ_LODS_NUM - 1;

    bool bLodFaceArea = GetCVars()->e_LodFaceArea != 0;
    if (bLodFaceArea)
    {
        float distances[SMeshLodInfo::s_nMaxLodCount];
        bLodFaceArea = pObj->GetLodDistances(frameLodInfo, distances);
        if (bLodFaceArea)
        {
            for (uint i = 0; i < MAX_STATOBJ_LODS_NUM - 1; ++i)
            {
                if (fDistance < distances[i])
                {
                    resultLod = i;
                    break;
                }
            }
        }
    }

    if (!bLodFaceArea)
    {
        const float fLodRatioNorm = pObj->GetLodRatioNormalized();
        const float fRadius = pObj->GetBBox().GetRadius();
        resultLod = (int)(fDistance * (fLodRatioNorm * fLodRatioNorm) / (max(frameLodInfo.fLodRatio * min(fRadius, GetFloatCVar(e_LodCompMaxSize)), 0.001f)));
    }

    return resultLod;
}

///////////////////////////////////////////////////////////////////////////////
void COctreeNode::GetObjectsByFlags(uint dwFlags, PodArray<IRenderNode*>& lstObjects)
{
    unsigned int nCurrentObject(eRNListType_First);
    for (nCurrentObject = eRNListType_First; nCurrentObject < eRNListType_ListsNum; ++nCurrentObject)
    {
        for (IRenderNode* pObj = m_arrObjects[nCurrentObject].m_pFirstNode; pObj; pObj = pObj->m_pNext)
        {
            if ((pObj->GetRndFlags() & dwFlags) == dwFlags)
            {
                lstObjects.Add(pObj);
            }
        }
    }

    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            m_arrChilds[i]->GetObjectsByFlags(dwFlags, lstObjects);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void COctreeNode::GetObjectsByType(PodArray<IRenderNode*>& lstObjects, EERType objType, const AABB* pBBox)
{
    if (objType == eERType_Light && !m_bHasLights)
    {
        return;
    }

    if (pBBox && !Overlap::AABB_AABB(*pBBox, GetObjectsBBox()))
    {
        return;
    }

    ERNListType eListType = IRenderNode::GetRenderNodeListId(objType);

    for (IRenderNode* pObj = m_arrObjects[eListType].m_pFirstNode; pObj; pObj = pObj->m_pNext)
    {
        if (pObj->GetRenderNodeType() == objType)
        {
            AABB box;
            pObj->FillBBox(box);
            if (!pBBox || Overlap::AABB_AABB(*pBBox, box))
            {
                lstObjects.Add(pObj);
            }
        }
    }

    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            m_arrChilds[i]->GetObjectsByType(lstObjects, objType, pBBox);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void COctreeNode::GetNearestCubeProbe(float& fMinDistance, int& nMaxPriority, CLightEntity*& pNearestLight, const AABB* pBBox)
{
    if (!m_bHasLights)
    {
        return;
    }

    if (!pBBox || pBBox && !Overlap::AABB_AABB(*pBBox, GetObjectsBBox()))
    {
        return;
    }

    Vec3 vCenter = pBBox->GetCenter();

    ERNListType eListType = IRenderNode::GetRenderNodeListId(eERType_Light);

    for (IRenderNode* pObj = m_arrObjects[eListType].m_pFirstNode; pObj; pObj = pObj->m_pNext)
    {
        if (pObj->GetRenderNodeType() == eERType_Light)
        {
            AABB box;
            pObj->FillBBox(box);
            if (Overlap::AABB_AABB(*pBBox, box))
            {
                CLightEntity* pLightEnt = (CLightEntity*)pObj;
                CDLight* pLight = &pLightEnt->m_light;

                if (pLight->m_Flags & DLF_DEFERRED_CUBEMAPS)
                {
                    Vec3 vCenterRel = vCenter - pLight->GetPosition();
                    Vec3 vCenterOBBSpace;
                    vCenterOBBSpace.x = pLightEnt->m_Matrix.GetColumn0().GetNormalized().dot(vCenterRel);
                    vCenterOBBSpace.y = pLightEnt->m_Matrix.GetColumn1().GetNormalized().dot(vCenterRel);
                    vCenterOBBSpace.z = pLightEnt->m_Matrix.GetColumn2().GetNormalized().dot(vCenterRel);

                    // Check if object center is within probe OBB
                    Vec3 vProbeExtents = pLight->m_ProbeExtents;
                    if (fabs(vCenterOBBSpace.x) < vProbeExtents.x && fabs(vCenterOBBSpace.y) < vProbeExtents.y && fabs(vCenterOBBSpace.z) < vProbeExtents.z)
                    {
                        if (pLight->m_nSortPriority > nMaxPriority 
                            && pLight->m_fProbeAttenuation > 0) // Don't return a probe that is disabled/invisible. In particular this provides better results when lighting particles.
                        {
                            pNearestLight = (CLightEntity*)pObj;
                            nMaxPriority = pLight->m_nSortPriority;
                            fMinDistance = 0;
                        }
                    }
                }
            }
        }
    }

    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            m_arrChilds[i]->GetNearestCubeProbe(fMinDistance, nMaxPriority, pNearestLight, pBBox);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
bool CObjManager::IsBoxOccluded(const AABB& objBox,
    float fDistance,
    OcclusionTestClient* const __restrict pOcclTestVars,
    bool/* bIndoorOccludersOnly*/,
    EOcclusionObjectType eOcclusionObjectType,
    const SRenderingPassInfo& passInfo)
{
    // if object was visible during last frames
    const uint32 mainFrameID = passInfo.GetMainFrameID();

    if (GetCVars()->e_OcclusionLazyHideFrames)
    {
        //This causes massive spikes in draw calls when rotating
        if (pOcclTestVars->nLastVisibleMainFrameID > mainFrameID - GetCVars()->e_OcclusionLazyHideFrames)
        {
            // prevent checking all objects in same frame
            int nId = (int)(UINT_PTR(pOcclTestVars) / 256);
            if ((nId & 7) != (mainFrameID & 7))
            {
                return false;
            }
        }
    }

    // use fast and reliable test right here
    CVisAreaManager* pVisAreaManager = GetVisAreaManager();
    if (GetCVars()->e_OcclusionVolumes && pVisAreaManager && pVisAreaManager->IsOccludedByOcclVolumes(objBox, passInfo))
    {
        pOcclTestVars->nLastOccludedMainFrameID = mainFrameID;
        return true;
    }

    if (GetCVars()->e_CoverageBuffer)
    {
        return pOcclTestVars->nLastOccludedMainFrameID == mainFrameID - 1;
    }

    pOcclTestVars->nLastVisibleMainFrameID = mainFrameID;

    return false;
}


///////////////////////////////////////////////////////////////////////////////
void CVegetation::FillBBox(AABB& aabb)
{
    FillBBoxFromExtends(aabb, m_boxExtends, m_vPos);
}

///////////////////////////////////////////////////////////////////////////////
EERType CVegetation::GetRenderNodeType()
{
    return eERType_Vegetation;
}

///////////////////////////////////////////////////////////////////////////////
float CVegetation::GetMaxViewDist()
{
    StatInstGroup& group = GetStatObjGroup();
    CStatObj* pStatObj = (CStatObj*)(IStatObj*)group.pStatObj;
    if (pStatObj)
    {
        return max(GetCVars()->e_ViewDistMin, group.fVegRadius * CVegetation::GetScale() * GetCVars()->e_ViewDistRatioVegetation * GetViewDistanceMultiplier());
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
IStatObj* CVegetation::GetEntityStatObj(unsigned int nPartId, unsigned int nSubPartId, Matrix34A* pMatrix, bool bReturnOnlyVisible)
{
    if (nPartId != 0)
    {
        return 0;
    }

    if (pMatrix)
    {
        if (m_pRNTmpData)
        {
            *pMatrix = m_pRNTmpData->userData.objMat;
        }
        else
        {
            Matrix34A tm;
            CalcMatrix(tm);
            *pMatrix = tm;
        }
    }

    return GetStatObj();
}

///////////////////////////////////////////////////////////////////////////////
Vec3 CVegetation::GetPos(bool bWorldOnly) const
{
    assert(bWorldOnly);
    return m_vPos;
}

///////////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CVegetation::GetMaterial(Vec3* pHitPos)
{
    StatInstGroup& vegetGroup = GetStatObjGroup();

    if (vegetGroup.pMaterial)
    {
        return vegetGroup.pMaterial;
    }

    if (IStatObj* pBody = vegetGroup.GetStatObj())
    {
        return pBody->GetMaterial();
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CBrush::FillBBox(AABB& aabb)
{
    aabb = CBrush::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
EERType CBrush::GetRenderNodeType()
{
    return eERType_Brush;
}

///////////////////////////////////////////////////////////////////////////////
float CBrush::GetMaxViewDist()
{
    return max(GetCVars()->e_ViewDistMin, min(GetFloatCVar(e_ViewDistCompMaxSize), CBrush::GetBBox().GetRadius()) * GetCVars()->e_ViewDistRatio * GetViewDistanceMultiplier());
}

///////////////////////////////////////////////////////////////////////////////
Vec3 CBrush::GetPos(bool bWorldOnly) const
{
    assert(bWorldOnly);
    return m_Matrix.GetTranslation();
}

///////////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CBrush::GetMaterial(Vec3* pHitPos)
{
    if (m_pMaterial)
    {
        return m_pMaterial;
    }

    if (m_pStatObj)
    {
        return m_pStatObj->GetMaterial();
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CLightEntity::FillBBox(AABB& aabb)
{
    aabb = CLightEntity::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
EERType CLightEntity::GetRenderNodeType()
{
    return eERType_Light;
}

///////////////////////////////////////////////////////////////////////////////
float CLightEntity::GetMaxViewDist()
{
    if (m_light.m_Flags & DLF_SUN)
    {
        return 10.f * DISTANCE_TO_THE_SUN;
    }
    return max(GetCVars()->e_ViewDistMin, CLightEntity::GetBBox().GetRadius() * GetCVars()->e_ViewDistRatioLights * GetViewDistanceMultiplier());
}

///////////////////////////////////////////////////////////////////////////////
Vec3 CLightEntity::GetPos(bool bWorldOnly) const
{
    assert(bWorldOnly);
    return m_light.m_Origin;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CRoadRenderNode::FillBBox(AABB& aabb)
{
    aabb = CRoadRenderNode::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
EERType CRoadRenderNode::GetRenderNodeType()
{
    return eERType_Road;
}

///////////////////////////////////////////////////////////////////////////////
float CRoadRenderNode::GetMaxViewDist()
{
    return max(GetCVars()->e_ViewDistMin, CRoadRenderNode::GetBBox().GetRadius() * GetCVars()->e_ViewDistRatio * GetViewDistanceMultiplier());
}

///////////////////////////////////////////////////////////////////////////////
Vec3 CRoadRenderNode::GetPos(bool) const
{
    return m_WSBBox.GetCenter();
}

///////////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CRoadRenderNode::GetMaterial(Vec3* pHitPos)
{
    return m_pMaterial;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CBreakableGlassRenderNode::FillBBox(AABB& aabb)
{
    aabb = CBreakableGlassRenderNode::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
const AABB CBreakableGlassRenderNode::GetBBox() const
{
    AABB bbox = m_planeBBox;
    bbox.Add(m_fragBBox);
    return bbox;
}

///////////////////////////////////////////////////////////////////////////////
EERType CBreakableGlassRenderNode::GetRenderNodeType()
{
    return eERType_BreakableGlass;
}

///////////////////////////////////////////////////////////////////////////////
float CBreakableGlassRenderNode::GetMaxViewDist()
{
    return m_maxViewDist;
}

///////////////////////////////////////////////////////////////////////////////
Vec3 CBreakableGlassRenderNode::GetPos(bool worldOnly) const
{
    return m_matrix.GetTranslation();
}

///////////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CBreakableGlassRenderNode::GetMaterial(Vec3* pHitPos)
{
    return m_glassParams.pGlassMaterial;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void COcean::FillBBox(AABB& aabb)
{
    aabb = COcean::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
EERType COcean::GetRenderNodeType()
{
    return eERType_WaterVolume;
}

///////////////////////////////////////////////////////////////////////////////
Vec3 COcean::GetPos(bool) const
{
    return Vec3(0, 0, 0);
}

///////////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> COcean::GetMaterial(Vec3* pHitPos)
{
    return m_pMaterial;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CFogVolumeRenderNode::FillBBox(AABB& aabb)
{
    aabb = CFogVolumeRenderNode::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
EERType CFogVolumeRenderNode::GetRenderNodeType()
{
    return eERType_FogVolume;
}

///////////////////////////////////////////////////////////////////////////////
float CFogVolumeRenderNode::GetMaxViewDist()
{
    return max(GetCVars()->e_ViewDistMin, CFogVolumeRenderNode::GetBBox().GetRadius() * GetCVars()->e_ViewDistRatio * GetViewDistanceMultiplier());
}

///////////////////////////////////////////////////////////////////////////////
Vec3 CFogVolumeRenderNode::GetPos(bool bWorldOnly) const
{
    return m_pos;
}

///////////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CFogVolumeRenderNode::GetMaterial(Vec3* pHitPos)
{
    return 0;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CDecalRenderNode::FillBBox(AABB& aabb)
{
    aabb = CDecalRenderNode::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
EERType CDecalRenderNode::GetRenderNodeType()
{
    return eERType_Decal;
}

///////////////////////////////////////////////////////////////////////////////
float CDecalRenderNode::GetMaxViewDist()
{
    return m_decalProperties.m_maxViewDist * GetViewDistanceMultiplier();
}

///////////////////////////////////////////////////////////////////////////////
Vec3 CDecalRenderNode::GetPos(bool bWorldOnly) const
{
    return m_pos;
}

///////////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CDecalRenderNode::GetMaterial(Vec3* pHitPos)
{
    return m_pMaterial;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CWaterVolumeRenderNode::FillBBox(AABB& aabb)
{
    aabb = CWaterVolumeRenderNode::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
EERType CWaterVolumeRenderNode::GetRenderNodeType()
{
    return eERType_WaterVolume;
}

///////////////////////////////////////////////////////////////////////////////
float CWaterVolumeRenderNode::GetMaxViewDist()
{
    return max(GetCVars()->e_ViewDistMin, CWaterVolumeRenderNode::GetBBox().GetRadius() * GetCVars()->e_ViewDistRatio * GetViewDistanceMultiplier());
}

///////////////////////////////////////////////////////////////////////////////
Vec3 CWaterVolumeRenderNode::GetPos(bool bWorldOnly) const
{
    return m_center;
}

///////////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CWaterVolumeRenderNode::GetMaterial(Vec3* pHitPos)
{
    return m_pMaterial;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CDistanceCloudRenderNode::FillBBox(AABB& aabb)
{
    aabb = CDistanceCloudRenderNode::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
EERType CDistanceCloudRenderNode::GetRenderNodeType()
{
    return eERType_DistanceCloud;
}

///////////////////////////////////////////////////////////////////////////////
float CDistanceCloudRenderNode::GetMaxViewDist()
{
    return max(GetCVars()->e_ViewDistMin, CDistanceCloudRenderNode::GetBBox().GetRadius() * GetCVars()->e_ViewDistRatio * GetViewDistanceMultiplier());
}

///////////////////////////////////////////////////////////////////////////////
Vec3 CDistanceCloudRenderNode::GetPos(bool bWorldOnly) const
{
    return m_pos;
}

///////////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CDistanceCloudRenderNode::GetMaterial(Vec3* pHitPos)
{
    return m_pMaterial;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CRopeRenderNode::FillBBox(AABB& aabb)
{
    aabb = CRopeRenderNode::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
EERType CRopeRenderNode::GetRenderNodeType()
{
    return eERType_Rope;
}

///////////////////////////////////////////////////////////////////////////////
float CRopeRenderNode::GetMaxViewDist()
{
    return(max(GetCVars()->e_ViewDistMin, CRopeRenderNode::GetBBox().GetRadius() * GetCVars()->e_ViewDistRatio * GetViewDistanceMultiplier()));
}

///////////////////////////////////////////////////////////////////////////////
Vec3 CRopeRenderNode::GetPos(bool bWorldOnly) const
{
    return m_pos;
}

///////////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CRopeRenderNode::GetMaterial(Vec3* pHitPos)
{
    return m_pMaterial;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CMergedMeshRenderNode::FillBBox(AABB& aabb)
{
    aabb = CMergedMeshRenderNode::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
EERType CMergedMeshRenderNode::GetRenderNodeType()
{
    return eERType_MergedMesh;
}

///////////////////////////////////////////////////////////////////////////////
float CMergedMeshRenderNode::GetMaxViewDist()
{
    float radius = m_internalAABB.GetRadius();
    return max(GetCVars()->e_ViewDistMin, radius * GetCVars()->e_MergedMeshesViewDistRatio);
}

///////////////////////////////////////////////////////////////////////////////
Vec3 CMergedMeshRenderNode::GetPos(bool bWorldOnly) const
{
    return m_pos;
}

///////////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CMergedMeshRenderNode::GetMaterial(Vec3* pHitPos)
{
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
EERType CTerrainNode::GetRenderNodeType()
{
    return eERType_NotRenderNode;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
#include "PrismRenderNode.h"

///////////////////////////////////////////////////////////////////////////////
void CPrismRenderNode::FillBBox(AABB& aabb)
{
    aabb = CPrismRenderNode::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
EERType CPrismRenderNode::GetRenderNodeType()
{
    return eERType_PrismObject;
}

///////////////////////////////////////////////////////////////////////////////
float CPrismRenderNode::GetMaxViewDist()
{
    return 1000.0f;
}

///////////////////////////////////////////////////////////////////////////////
Vec3 CPrismRenderNode::GetPos(bool bWorldOnly) const
{
    return m_mat.GetTranslation();
}

///////////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CPrismRenderNode::GetMaterial(Vec3* pHitPos)
{
    return m_pMaterial;
}

#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "VolumeObjectRenderNode.h"
#include "CloudRenderNode.h"

///////////////////////////////////////////////////////////////////////////////
void CVolumeObjectRenderNode::FillBBox(AABB& aabb)
{
    aabb = CVolumeObjectRenderNode::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
EERType CVolumeObjectRenderNode::GetRenderNodeType()
{
    return eERType_VolumeObject;
}

///////////////////////////////////////////////////////////////////////////////
float CVolumeObjectRenderNode::GetMaxViewDist()
{
    return max(GetCVars()->e_ViewDistMin, CVolumeObjectRenderNode::GetBBox().GetRadius() * GetCVars()->e_ViewDistRatio * GetViewDistanceMultiplier());
}

///////////////////////////////////////////////////////////////////////////////
Vec3 CVolumeObjectRenderNode::GetPos(bool bWorldOnly) const
{
    return m_pos;
}

///////////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CVolumeObjectRenderNode::GetMaterial(Vec3* pHitPos)
{
    return m_pMaterial;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CCloudRenderNode::FillBBox(AABB& aabb)
{
    aabb = CCloudRenderNode::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
EERType CCloudRenderNode::GetRenderNodeType()
{
    return eERType_Cloud;
}

//////////////////////////////////////////////////////////////////////////
float CCloudRenderNode::GetMaxViewDist()
{
    return max(GetCVars()->e_ViewDistMin, CCloudRenderNode::GetBBox().GetRadius() * GetCVars()->e_ViewDistRatio * GetViewDistanceMultiplier());
}

//////////////////////////////////////////////////////////////////////////
Vec3 CCloudRenderNode::GetPos(bool bWorldOnly) const
{
    return m_pos;
}

//////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CCloudRenderNode::GetMaterial(Vec3* pHitPos)
{
    return m_pMaterial;
}


///////////////////////////////////////////////////////////////////////////////
void CTerrainNode::FillBBox(AABB& aabb)
{
    aabb = GetBBox();
}

