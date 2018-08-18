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
#include "CullBuffer.h"
#include "RoadRenderNode.h"
#include "Brush.h"
#include "DecalRenderNode.h"
#include "RenderMeshMerger.h"
#include "DecalManager.h"
#include "VisAreas.h"
#include "ICryAnimation.h"
#include "LightEntity.h"
#include "WaterVolumeRenderNode.h"
#include "DistanceCloudRenderNode.h"
#include "MergedMeshRenderNode.h"
#include "MergedMeshGeometry.h"
#include "ShadowCache.h"

#include <AzCore/Component/TransformBus.h>

#define MAX_NODE_NUM 7

const float fNodeMinSize = 8.f;
const float fObjectToNodeSizeRatio = 1.f / 8.f;
const float fMinShadowCasterViewDist = 8.f;

PodArray<COctreeNode*> COctreeNode::m_arrEmptyNodes;


COctreeNode::~COctreeNode()
{
    for (int l = 0; l < eRNListType_ListsNum; l++)
    {
        for (IRenderNode* pObj = m_arrObjects[l].m_pFirstNode, * pNext; pObj; pObj = pNext)
        {
            pNext = pObj->m_pNext;

            if (pObj->IsAllocatedOutsideOf3DEngineDLL())
            {
                Get3DEngine()->UnRegisterEntityDirect(pObj);
            }
            else
            {
                pObj->ReleaseNode(true);
            }
        }

        assert(!m_arrObjects[l].m_pFirstNode);
    }

    for (int i = 0; i < 8; i++)
    {
        delete m_arrChilds[i];
        m_arrChilds[i] = NULL;
    }

    m_arrEmptyNodes.Delete(this);

    GetObjManager()->GetArrStreamingNodeStack().Delete(this);

    if (m_pRNTmpData)
    {
        Get3DEngine()->FreeRNTmpData(&m_pRNTmpData);
    }

    ResetStaticInstancing();
}

void COctreeNode::SetVisArea(CVisArea* pVisArea)
{
    m_pVisArea = pVisArea;
    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            m_arrChilds[i]->SetVisArea(pVisArea);
        }
    }
}


extern float arrVegetation_fSpriteSwitchState[nThreadsNum];


void COctreeNode::Render_Object_Nodes(bool bNodeCompletelyInFrustum, int nRenderMask, const SRenderingPassInfo& passInfo, SRendItemSorter& rendItemSorter)
{
    assert(nRenderMask & OCTREENODE_RENDER_FLAG_OBJECTS);

    const CCamera& rCam = passInfo.GetCamera();

    if (m_nOccludedFrameId == passInfo.GetFrameID())
    {
        return;
    }

    if (!bNodeCompletelyInFrustum && !rCam.IsAABBVisible_EH(m_objectsBox, &bNodeCompletelyInFrustum))
    {
        return;
    }

    const Vec3& vCamPos = rCam.GetPosition();

    float fNodeDistanceSq = Distance::Point_AABBSq(vCamPos, m_objectsBox) * sqr(passInfo.GetZoomFactor());

    if (fNodeDistanceSq > sqr(m_fObjectsMaxViewDist))
    {
        return;
    }

    float fNodeDistance = sqrt_tpl(fNodeDistanceSq);

    Get3DEngine()->CheckCreateRNTmpData(&m_pRNTmpData, NULL, passInfo);

    if (m_nLastVisFrameId != passInfo.GetFrameID() && m_pParent)
    {
        if (GetObjManager()->IsBoxOccluded(m_objectsBox, fNodeDistance, &m_pRNTmpData->userData.m_OcclState, m_pVisArea != NULL, eoot_OCCELL, passInfo))
        {
            m_nOccludedFrameId = passInfo.GetFrameID();
            return;
        }
    }

    m_nLastVisFrameId = passInfo.GetFrameID();

    if (!IsCompiled())
    {
        CompileObjects();
    }

    if (GetCVars()->e_ObjectsTreeBBoxes)
    {
        if (GetCVars()->e_ObjectsTreeBBoxes == 1)
        {
            const AABB& nodeBox = GetNodeBox();
            DrawBBox(nodeBox, Col_Blue);
        }
        if (GetCVars()->e_ObjectsTreeBBoxes == 2)
        {
            DrawBBox(m_objectsBox, Col_Red);
        }
    }

    m_fNodeDistance = fNodeDistance;
    m_bNodeCompletelyInFrustum = bNodeCompletelyInFrustum;

    if (HasAnyRenderableCandidates(passInfo))
    {
        // when using the occlusion culler, push the work to the jobs doing the occlusion checks, else just compute in main thread
        if (GetCVars()->e_StatObjBufferRenderTasks == 1 && passInfo.IsGeneralPass())
        {
            GetObjManager()->PushIntoCullQueue(SCheckOcclusionJobData::CreateOctreeJobData(this, nRenderMask, rendItemSorter, &passInfo.GetCamera()));
        }
        else
        {
            RenderContentJobEntry(nRenderMask, passInfo, rendItemSorter, &passInfo.GetCamera());
        }

        rendItemSorter.IncreaseOctreeCounter();
    }

    int nFirst =
        ((vCamPos.x > m_vNodeCenter.x) ? 4 : 0) |
        ((vCamPos.y > m_vNodeCenter.y) ? 2 : 0) |
        ((vCamPos.z > m_vNodeCenter.z) ? 1 : 0);

    if (m_arrChilds[nFirst  ])
    {
        m_arrChilds[nFirst  ]->Render_Object_Nodes(bNodeCompletelyInFrustum, nRenderMask, passInfo, rendItemSorter);
    }

    if (m_arrChilds[nFirst ^ 1])
    {
        m_arrChilds[nFirst ^ 1]->Render_Object_Nodes(bNodeCompletelyInFrustum, nRenderMask, passInfo, rendItemSorter);
    }

    if (m_arrChilds[nFirst ^ 2])
    {
        m_arrChilds[nFirst ^ 2]->Render_Object_Nodes(bNodeCompletelyInFrustum, nRenderMask, passInfo, rendItemSorter);
    }

    if (m_arrChilds[nFirst ^ 4])
    {
        m_arrChilds[nFirst ^ 4]->Render_Object_Nodes(bNodeCompletelyInFrustum, nRenderMask, passInfo, rendItemSorter);
    }

    if (m_arrChilds[nFirst ^ 3])
    {
        m_arrChilds[nFirst ^ 3]->Render_Object_Nodes(bNodeCompletelyInFrustum, nRenderMask, passInfo, rendItemSorter);
    }

    if (m_arrChilds[nFirst ^ 5])
    {
        m_arrChilds[nFirst ^ 5]->Render_Object_Nodes(bNodeCompletelyInFrustum, nRenderMask, passInfo, rendItemSorter);
    }

    if (m_arrChilds[nFirst ^ 6])
    {
        m_arrChilds[nFirst ^ 6]->Render_Object_Nodes(bNodeCompletelyInFrustum, nRenderMask, passInfo, rendItemSorter);
    }

    if (m_arrChilds[nFirst ^ 7])
    {
        m_arrChilds[nFirst ^ 7]->Render_Object_Nodes(bNodeCompletelyInFrustum, nRenderMask, passInfo, rendItemSorter);
    }
}

void COctreeNode::CompileObjects()
{
    FUNCTION_PROFILER_3DENGINE;

    m_lstCasters.Clear();

    m_bStaticInstancingIsDirty = true;
    CheckUpdateStaticInstancing();
    m_lstVegetationsForRendering.Clear();

    float fObjMaxViewDistance = 0;

    size_t numCasters = 0;
    const unsigned int nSkipShadowCastersRndFlags = ERF_HIDDEN | ERF_COLLISION_PROXY | ERF_RAYCAST_PROXY | ERF_STATIC_INSTANCING; // shadow casters with these render flags are ignored

    for (int l = 0; l < eRNListType_ListsNum; l++)
    {
        for (IRenderNode* pObj = m_arrObjects[l].m_pFirstNode; pObj; pObj = pObj->m_pNext)
        {
            int nFlags = pObj->GetRndFlags();

            IF (nFlags & nSkipShadowCastersRndFlags, 0)
            {
                continue;
            }

            IF (GetCVars()->e_ShadowsPerObject && gEnv->p3DEngine->GetPerObjectShadow(pObj), 0)
            {
                continue;
            }

            EERType eRType = pObj->GetRenderNodeType();
            float WSMaxViewDist = pObj->GetMaxViewDist();

            if (nFlags & ERF_CASTSHADOWMAPS && WSMaxViewDist > fMinShadowCasterViewDist && eRType != eERType_Light)
            {
                ++numCasters;
            }
        }
    }

    m_lstCasters.reserve(numCasters);

    IObjManager* pObjManager = GetObjManager();

    // update node
    for (int l = 0; l < eRNListType_ListsNum; l++)
    {
        for (IRenderNode* pObj = m_arrObjects[l].m_pFirstNode, * pNext; pObj; pObj = pNext)
        {
            pNext = pObj->m_pNext;

            IF (pObj->m_dwRndFlags & ERF_HIDDEN, 0)
            {
                continue;
            }

            bool bVegetHasAlphaTrans = false;

            // update vegetation instances data
            EERType eRType = pObj->GetRenderNodeType();
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

            // update REQUIRES_FORWARD_RENDERING flag
            //IF(GetCVars()->e_ShadowsOnAlphaBlend,0)
            {
                pObj->m_nInternalFlags &= ~(IRenderNode::REQUIRES_FORWARD_RENDERING | IRenderNode::REQUIRES_NEAREST_CUBEMAP);
                if (eRType != eERType_Light &&
                    eRType != eERType_Cloud &&
                    eRType != eERType_FogVolume &&
                    eRType != eERType_Decal &&
                    eRType != eERType_Road &&
                    eRType != eERType_DistanceCloud)
                {
                    if (eRType == eERType_ParticleEmitter)
                    {
                        pObj->m_nInternalFlags |= IRenderNode::REQUIRES_FORWARD_RENDERING | IRenderNode::REQUIRES_NEAREST_CUBEMAP;
                    }

                    if (CMatInfo* pMatInfo = (CMatInfo*)pObj->GetMaterial().get())
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

                    if (eRType == eERType_RenderComponent ||eRType == eERType_StaticMeshRenderComponent || eRType == eERType_DynamicMeshRenderComponent || eRType == eERType_SkinnedMeshRenderComponent)
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
                    }

                    if (!(pObj->m_nInternalFlags & (IRenderNode::REQUIRES_FORWARD_RENDERING | IRenderNode::REQUIRES_NEAREST_CUBEMAP)))
                    {
                        CompileCharacter(pObj->GetEntityCharacter(0), pObj->m_nInternalFlags);
                    }
                }
            }

            int nFlags = pObj->GetRndFlags();

            // fill shadow casters list
            const bool bHasPerObjectShadow = GetCVars()->e_ShadowsPerObject && gEnv->p3DEngine->GetPerObjectShadow(pObj);
            if (!(nFlags & nSkipShadowCastersRndFlags) && nFlags & ERF_CASTSHADOWMAPS && fNewMaxViewDist > fMinShadowCasterViewDist &&
                eRType != eERType_Light && !bHasPerObjectShadow)
            {
                COctreeNode* pNode = this;
                while (pNode && !(pNode->m_renderFlags & ERF_CASTSHADOWMAPS))
                {
                    pNode->m_renderFlags |= ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS;
                    pNode = pNode->m_pParent;
                }

                float fMaxCastDist = fNewMaxViewDist * GetCVars()->e_ShadowsCastViewDistRatio;
                m_lstCasters.Add(SCasterInfo(pObj, fMaxCastDist, eRType));


                //
                // (bethelz) Static instancing system only works on vegetation at the moment. It batches multiple instances into
                // the first CVegetation node in the chain. This means that that shadow casters are also batched into that single
                // instance. This compensates for that by expanding the bounding box of the casters to match all instances.
                //
                if (pObj->GetRenderNodeType() == eERType_Vegetation && ((CVegetation*)pObj)->m_pInstancingInfo)
                {
                    AABB objBox = ((CVegetation*)pObj)->m_pInstancingInfo->m_aabbBox;
                    m_lstCasters.Last().objSphere.center = objBox.GetCenter();
                    m_lstCasters.Last().objSphere.radius = objBox.GetRadius();
                }
            }

            fObjMaxViewDistance = max(fObjMaxViewDistance, fNewMaxViewDist);
        }
    }

    if (fObjMaxViewDistance > m_fObjectsMaxViewDist)
    {
        COctreeNode* pNode = this;
        while (pNode)
        {
            pNode->m_fObjectsMaxViewDist = max(pNode->m_fObjectsMaxViewDist, fObjMaxViewDistance);
            pNode = pNode->m_pParent;
        }
    }

    SetCompiled(true);

    const Vec3& sunDir = Get3DEngine()->GetSunDirNormalized();
    m_fpSunDirX = (uint32) (sunDir.x * 63.5f + 63.5f);
    m_fpSunDirZ = (uint32) (sunDir.z * 63.5f + 63.5f);
    m_fpSunDirYs = sunDir.y < 0.0f ? 1 : 0;
}


void COctreeNode::UpdateStaticInstancing()
{
    FUNCTION_PROFILER_3DENGINE;

    m_lstVegetationsForRendering.Clear();

    // clear
    if (m_pStaticInstancingInfo)
    {
        for (auto it = m_pStaticInstancingInfo->begin(); it != m_pStaticInstancingInfo->end(); ++it)
        {
            PodArray<SNodeInstancingInfo>*& pInfo = it->second;

            pInfo->Clear();
        }
    }

    // group objects by CStatObj *
    for (IRenderNode* pObj = m_arrObjects[eRNListType_Vegetation].m_pFirstNode, * pNext; pObj; pObj = pNext)
    {
        pNext = pObj->m_pNext;

        CVegetation* pInst = (CVegetation*)pObj;

        pObj->m_dwRndFlags &= ~ERF_STATIC_INSTANCING;

        if (pInst->m_pInstancingInfo)
        {
            SAFE_DELETE(pInst->m_pInstancingInfo);

            if (pInst->m_pRNTmpData)
            {
                Get3DEngine()->FreeRNTmpData(&pInst->m_pRNTmpData);
            }
        }

        IF (pObj->m_dwRndFlags & ERF_HIDDEN, 0)
        {
            continue;
        }

        Matrix34A objMatrix;
        CStatObj* pStatObj = (CStatObj*)pInst->GetEntityStatObj(0, 0, &objMatrix);

        int nLodA = -1;
        {
            const Vec3 vCamPos = GetSystem()->GetViewCamera().GetPosition();

            const float fEntDistance = sqrt_tpl(Distance::Point_AABBSq(vCamPos, pInst->GetBBox()));
            int wantedLod = GetObjManager()->GetObjectLOD(pInst, fEntDistance);

            int minUsableLod = pStatObj->GetMinUsableLod();
            int maxUsableLod = (int)pStatObj->m_nMaxUsableLod;
            nLodA = CLAMP(wantedLod, minUsableLod, maxUsableLod);
            if (!(pStatObj->m_nFlags & STATIC_OBJECT_COMPOUND))
            {
                nLodA = pStatObj->FindNearesLoadedLOD(nLodA);
            }
        }

        if (nLodA < 0)
        {
            continue;
        }

        pStatObj = (CStatObj*)pStatObj->GetLodObject(nLodA);

        if (!m_pStaticInstancingInfo)
        {
            m_pStaticInstancingInfo = new std::map<std::pair<IStatObj*, _smart_ptr<IMaterial> >, PodArray<SNodeInstancingInfo>*>;
        }

        std::pair<IStatObj*, _smart_ptr<IMaterial> > pairKey(pStatObj, pInst->GetMaterial());

        PodArray<SNodeInstancingInfo>*& pInfo = (*m_pStaticInstancingInfo)[pairKey];

        if (!pInfo)
        {
            pInfo = new PodArray<SNodeInstancingInfo>;
        }

        SNodeInstancingInfo ii;
        ii.pRNode = pInst;
        ii.nodeMatrix = objMatrix;

        pInfo->Add(ii);
    }

    // mark
    if (m_pStaticInstancingInfo)
    {
        for (auto it = m_pStaticInstancingInfo->begin(); it != m_pStaticInstancingInfo->end(); ++it)
        {
            PodArray<SNodeInstancingInfo>*& pInfo = it->second;

            if (pInfo->Count() > GetCVars()->e_StaticInstancingMinInstNum)
            {
                CVegetation* pFirstNode = pInfo->GetAt(0).pRNode;

                // put instancing into one of existing vegetations
                PodArrayAABB<CRenderObject::SInstanceData>* pInsts = pFirstNode->m_pInstancingInfo = new PodArrayAABB<CRenderObject::SInstanceData>;
                pInsts->PreAllocate(pInfo->Count(), pInfo->Count());
                pInsts->m_aabbBox.Reset();

                if (pFirstNode->m_pRNTmpData)
                {
                    Get3DEngine()->FreeRNTmpData(&pFirstNode->m_pRNTmpData);
                }

                for (int i = 0; i < pInfo->Count(); i++)
                {
                    SNodeInstancingInfo& ii = pInfo->GetAt(i);

                    if (i)
                    {
                        ii.pRNode->SetRndFlags(ERF_STATIC_INSTANCING, true);

                        for (int s = 0; s < m_lstCasters.Count(); s++)
                        {
                            if (m_lstCasters[s].pNode == ii.pRNode)
                            {
                                m_lstCasters.Delete(s);
                                break;
                            }
                        }
                    }

                    (*pInsts)[i].m_MatInst = ii.nodeMatrix;
                    (*pInsts)[i].m_vDissolveInfo.zero();
                    (*pInsts)[i].m_vBendInfo.zero();

                    pInsts->m_aabbBox.Add(ii.pRNode->GetBBox());
                }
            }
            else
            {
                pInfo->Clear();
            }
        }
    }

    m_bStaticInstancingIsDirty = false;
}

void COctreeNode::ResetStaticInstancing()
{
    FUNCTION_PROFILER_3DENGINE;

    m_lstVegetationsForRendering.Clear();

    for (IRenderNode* pObj = m_arrObjects[eRNListType_Vegetation].m_pFirstNode, * pNext; pObj; pObj = pNext)
    {
        pNext = pObj->m_pNext;

        CVegetation* pInst = (CVegetation*)pObj;

        pObj->m_dwRndFlags &= ~ERF_STATIC_INSTANCING;

        if (pInst->m_pInstancingInfo)
        {
            SAFE_DELETE(pInst->m_pInstancingInfo)

            if (pInst->m_pRNTmpData)
            {
                Get3DEngine()->FreeRNTmpData(&pInst->m_pRNTmpData);
            }
        }
    }

    if (m_pStaticInstancingInfo)
    {
        for (auto it = m_pStaticInstancingInfo->begin(); it != m_pStaticInstancingInfo->end(); )
        {
            PodArray<SNodeInstancingInfo>*& pInfo = it->second;

            pInfo->Reset();

            it = m_pStaticInstancingInfo->erase(it);
        }

        SAFE_DELETE(m_pStaticInstancingInfo);
    }

    m_bStaticInstancingIsDirty = true;
}

void COctreeNode::CheckUpdateStaticInstancing()
{
    if (GetCVars()->e_StaticInstancing)
    {
        if (m_bStaticInstancingIsDirty)
        {
            UpdateStaticInstancing();
        }
    }
    else if (m_pStaticInstancingInfo)
    {
        ResetStaticInstancing();
    }
}

bool IsAABBInsideHull(const SPlaneObject* pHullPlanes, int nPlanesNum, const AABB& aabbBox);
bool IsSphereInsideHull(const SPlaneObject* pHullPlanes, int nPlanesNum, const Sphere& objSphere);

void COctreeNode::FillShadowCastersList(bool bNodeCompletellyInFrustum, CDLight* pLight, ShadowMapFrustum* pFr, PodArray<SPlaneObject>* pShadowHull, uint32 nRenderNodeFlags, const SRenderingPassInfo& passInfo)
{
    if (GetCVars()->e_Objects)
    {
        if (m_renderFlags & ERF_CASTSHADOWMAPS)
        {
            FRAME_PROFILER("COctreeNode::FillShadowMapCastersList", GetSystem(), PROFILE_3DENGINE);

            _MS_ALIGN(64) ShadowMapFrustumParams params;
            params.pLight = pLight;
            params.pFr = pFr;
            params.pShadowHull = pShadowHull;
            params.passInfo = &passInfo;
            params.vCamPos = passInfo.GetCamera().GetPosition();
            params.bSun = (pLight->m_Flags & DLF_SUN) != 0;
            params.nRenderNodeFlags = nRenderNodeFlags;

            FillShadowMapCastersList(params, bNodeCompletellyInFrustum);
        }
    }
}

void COctreeNode::FillDepthCubemapRenderList(const AABB& cubemapAABB, const SRenderingPassInfo& passInfo, PodArray<struct IShadowCaster*>* objectsList)
{
    if (GetCVars()->e_Objects)
    {
        //get objects from this node
        for (IRenderNode* obj = m_arrObjects[eRNListType_Unknown].m_pFirstNode; obj; obj = obj->m_pNext)
        {
            if (cubemapAABB.IsIntersectBox(obj->GetBBox()))
            {
                objectsList->Add(obj);
            }
        }

        //check child nodes
        for (int i = 0; i <= MAX_NODE_NUM; i++)
        {
            const bool bPrefetch = i < MAX_NODE_NUM && !!m_arrChilds[i + 1];

            if (m_arrChilds[i])
            {
                m_arrChilds[i]->FillDepthCubemapRenderList(cubemapAABB, passInfo, objectsList);
            }
        }
    }
}

void COctreeNode::FillShadowMapCastersList(const ShadowMapFrustumParams& params, bool bNodeCompletellyInFrustum)
{
    if (!bNodeCompletellyInFrustum && !params.pFr->IntersectAABB(m_objectsBox, &bNodeCompletellyInFrustum))
    {
        return;
    }

    const int frameID = params.passInfo->GetFrameID();
    if (params.bSun && bNodeCompletellyInFrustum)
    {
        nFillShadowCastersSkipFrameId = frameID;
    }

    if (params.pShadowHull && !IsAABBInsideHull(params.pShadowHull->GetElements(), params.pShadowHull->Count(), m_objectsBox))
    {
        nFillShadowCastersSkipFrameId = frameID;
        return;
    }

    if (!IsCompiled())
    {
        CompileObjects();
    }

    PrefetchLine(&m_lstCasters, 0);

    const float fShadowsCastViewDistRatio = GetCVars()->e_ShadowsCastViewDistRatio;
    if (fShadowsCastViewDistRatio != 0.0f)
    {
        float fNodeDistanceSqr = Distance::Point_AABBSq(params.vCamPos, m_objectsBox);
        if (fNodeDistanceSqr > sqr(m_fObjectsMaxViewDist * fShadowsCastViewDistRatio))
        {
            nFillShadowCastersSkipFrameId = frameID;
            return;
        }
    }

    PrefetchLine(m_lstCasters.begin(), 0);
    PrefetchLine(m_lstCasters.begin(), 128);

    IRenderNode* pNotCaster = ((CLightEntity*)params.pLight->m_pOwner)->m_pNotCaster;
    bool bParticleShadows = params.bSun && params.pFr->nShadowMapLod < GetCVars()->e_ParticleShadowsNumGSMs;

    SCasterInfo* pCastersEnd = m_lstCasters.end();
    for (SCasterInfo* pCaster = m_lstCasters.begin(); pCaster < pCastersEnd; pCaster++)
    {
        if (params.bSun && pCaster->nGSMFrameId == frameID && params.pShadowHull)
        {
            continue;
        }
        if (!IsRenderNodeTypeEnabled(pCaster->nRType))
        {
            continue;
        }
        if (pCaster->pNode == NULL || pCaster->pNode == pNotCaster)
        {
            continue;
        }
        if (!bParticleShadows && pCaster->nRType == eERType_ParticleEmitter)
        {
            continue;
        }
        if ((pCaster->nRenderNodeFlags & params.nRenderNodeFlags) == 0)
        {
            continue;
        }

        float fDistanceSq = Distance::Point_PointSq(params.vCamPos, pCaster->objSphere.center);
        if (fDistanceSq > sqr(pCaster->fMaxCastingDist + pCaster->objSphere.radius))
        {
            pCaster->nGSMFrameId = frameID;
            continue;
        }

        bool bObjCompletellyInFrustum = bNodeCompletellyInFrustum;
        if (!bObjCompletellyInFrustum && !params.pFr->IntersectAABB(pCaster->objBox, &bObjCompletellyInFrustum))
        {
            continue;
        }
        if (params.bSun && bObjCompletellyInFrustum)
        {
            pCaster->nGSMFrameId = frameID;
        }

        if (params.bSun && bObjCompletellyInFrustum)
        {
            pCaster->nGSMFrameId = frameID;
        }
        if (params.pShadowHull && !IsSphereInsideHull(params.pShadowHull->GetElements(), params.pShadowHull->Count(), pCaster->objSphere))
        {
            pCaster->nGSMFrameId = frameID;
            continue;
        }

        if (pCaster->bCanExecuteAsRenderJob)
        {
            Get3DEngine()->CheckCreateRNTmpData(&pCaster->pNode->m_pRNTmpData, pCaster->pNode, *params.passInfo);
            params.pFr->pJobExecutedCastersList->Add(pCaster->pNode);
        }
        else
        {
            params.pFr->pCastersList->Add(pCaster->pNode);
        }
    }

    for (int i = 0; i <= MAX_NODE_NUM; i++)
    {
        const bool bPrefetch = i < MAX_NODE_NUM && !!m_arrChilds[i + 1];

        if (m_arrChilds[i] && (m_arrChilds[i]->m_renderFlags & ERF_CASTSHADOWMAPS) && (!params.bSun || !params.pShadowHull || m_arrChilds[i]->nFillShadowCastersSkipFrameId != frameID))
        {
            m_arrChilds[i]->FillShadowMapCastersList(params, bNodeCompletellyInFrustum);
        }
    }
}

void COctreeNode::MarkAsUncompiled(const IRenderNode* pRenderNode)
{
    if (pRenderNode)
    {
        for (int l = 0; l < eRNListType_ListsNum; l++)
        {
            for (IRenderNode* pObj = m_arrObjects[l].m_pFirstNode; pObj; pObj = pObj->m_pNext)
            {
                if (pObj == pRenderNode)
                {
                    SetCompiled(false);
                    break;
                }
            }
        }
    }
    else
    {
        SetCompiled(false);
    }

    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            m_arrChilds[i]->MarkAsUncompiled(pRenderNode);
        }
    }
}

AABB COctreeNode::GetShadowCastersBox(const AABB* pBBox, const Matrix34* pShadowSpaceTransform)
{
    if (!IsCompiled())
    {
        CompileObjects();
    }

    AABB result(AABB::RESET);
    if (!pBBox || Overlap::AABB_AABB(*pBBox, GetObjectsBBox()))
    {
        for (size_t i = 0; i < m_lstCasters.size(); ++i)
        {
            AABB casterBox = m_lstCasters[i].objBox;
            if (!pBBox || Overlap::AABB_AABB(*pBBox, casterBox))
            {
                if (pShadowSpaceTransform)
                {
                    casterBox = AABB::CreateTransformedAABB(*pShadowSpaceTransform, casterBox);
                }

                result.Add(casterBox);
            }
        }

        for (int i = 0; i < 8; i++)
        {
            if (m_arrChilds[i])
            {
                result.Add(m_arrChilds[i]->GetShadowCastersBox(pBBox, pShadowSpaceTransform));
            }
        }
    }

    return result;
}

COctreeNode* COctreeNode::FindNodeContainingBox(const AABB& objBox)
{
    {
        const AABB& nodeBox = GetNodeBox();
        if (!nodeBox.IsContainSphere(objBox.min, -0.01f) || !nodeBox.IsContainSphere(objBox.max, -0.01f))
        {
            return NULL;
        }
    }

    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            if (COctreeNode* pFoundNode = m_arrChilds[i]->FindNodeContainingBox(objBox))
            {
                return pFoundNode;
            }
        }
    }

    return this;
}

void COctreeNode::MoveObjectsIntoList(PodArray<SRNInfo>* plstResultEntities, const AABB* pAreaBox,
    bool bRemoveObjects, bool bSkipDecals, bool bSkip_ERF_NO_DECALNODE_DECALS, bool bSkipDynamicObjects,
    EERType eRNType)
{
    FUNCTION_PROFILER_3DENGINE;

    if (pAreaBox && !Overlap::AABB_AABB(m_objectsBox, *pAreaBox))
    {
        return;
    }

    for (int l = 0; l < eRNListType_ListsNum; l++)
    {
        for (IRenderNode* pObj = m_arrObjects[l].m_pFirstNode, * pNext; pObj; pObj = pNext)
        {
            pNext = pObj->m_pNext;

            if (eRNType < eERType_TypesNum && pObj->GetRenderNodeType() != eRNType)
            {
                continue;
            }

            if (bSkipDecals && pObj->GetRenderNodeType() == eERType_Decal)
            {
                continue;
            }

            if (bSkip_ERF_NO_DECALNODE_DECALS && pObj->GetRndFlags() & ERF_NO_DECALNODE_DECALS)
            {
                continue;
            }

            if (bSkipDynamicObjects)
            {
                EERType eRType = pObj->GetRenderNodeType();

                if (eRType == eERType_RenderComponent || eRType == eERType_DynamicMeshRenderComponent || eRType == eERType_SkinnedMeshRenderComponent)
                {
                    if (pObj->IsMovableByGame())
                    {
                        continue;
                    }
                }
                else if (
                    eRType != eERType_Brush &&
                    eRType != eERType_Vegetation &&
                    eRType != eERType_StaticMeshRenderComponent)
                {
                    continue;
                }
            }

            if (pAreaBox && !Overlap::AABB_AABB(pObj->GetBBox(), *pAreaBox))
            {
                continue;
            }

            if (bRemoveObjects)
            {
                UnlinkObject(pObj);

                SetCompiled(false);
            }

            plstResultEntities->Add(pObj);
        }
    }

    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            m_arrChilds[i]->MoveObjectsIntoList(plstResultEntities, pAreaBox, bRemoveObjects, bSkipDecals, bSkip_ERF_NO_DECALNODE_DECALS, bSkipDynamicObjects, eRNType);
        }
    }
}

void COctreeNode::DeleteObjectsByFlag(int nRndFlag)
{
    FUNCTION_PROFILER_3DENGINE;
    for (int l = 0; l < eRNListType_ListsNum; l++)
    {
        for (IRenderNode* pObj = m_arrObjects[l].m_pFirstNode, * pNext; pObj; pObj = pNext)
        {
            pNext = pObj->m_pNext;

            if (pObj->GetRndFlags() & nRndFlag)
            {
                DeleteObject(pObj);
            }
        }
    }

    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            m_arrChilds[i]->DeleteObjectsByFlag(nRndFlag);
        }
    }
}

void COctreeNode::UnregisterEngineObjectsInArea(const SHotUpdateInfo* pExportInfo, PodArray<IRenderNode*>& arrUnregisteredObjects, bool bOnlyEngineObjects)
{
    FUNCTION_PROFILER_3DENGINE;

    const AABB* pAreaBox = (pExportInfo && !pExportInfo->areaBox.IsReset()) ? &pExportInfo->areaBox : NULL;

    {
        const AABB& nodeBox = GetNodeBox();
        if (pAreaBox && !Overlap::AABB_AABB(nodeBox, *pAreaBox))
        {
            return;
        }
    }

    uint32 nObjTypeMask = pExportInfo ? pExportInfo->nObjTypeMask : (uint32) ~0;

    for (int l = 0; l < eRNListType_ListsNum; l++)
    {
        for (IRenderNode* pObj = m_arrObjects[l].m_pFirstNode, * pNext; pObj; pObj = pNext)
        {
            pNext = pObj->m_pNext;

            EERType eType = pObj->GetRenderNodeType();

            if (bOnlyEngineObjects)
            {
                if (!(nObjTypeMask & (1 << eType)))
                {
                    continue;
                }

                if ((eType == eERType_Vegetation && !(pObj->GetRndFlags() & ERF_PROCEDURAL)) ||
                    eType == eERType_Brush ||
                    eType == eERType_Decal ||
                    eType == eERType_WaterVolume ||
                    eType == eERType_Road ||
                    eType == eERType_DistanceCloud )
                {
                    Get3DEngine()->UnRegisterEntityAsJob(pObj);
                    arrUnregisteredObjects.Add(pObj);
                    SetCompiled(false);
                }
            }
            else
            {
                Get3DEngine()->UnRegisterEntityAsJob(pObj);
                arrUnregisteredObjects.Add(pObj);
                SetCompiled(false);
            }
        }
    }

    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            m_arrChilds[i]->UnregisterEngineObjectsInArea(pExportInfo, arrUnregisteredObjects, bOnlyEngineObjects);
        }
    }
}

int COctreeNode::GetObjectsCount(EOcTeeNodeListType eListType)
{
    int nCount = 0;

    switch (eListType)
    {
    case eMain:
        for (int l = 0; l < eRNListType_ListsNum; l++)
        {
            for (IRenderNode* pObj = m_arrObjects[l].m_pFirstNode; pObj; pObj = pObj->m_pNext)
            {
                nCount++;
            }
        }
        break;
    case eCasters:
        for (int l = 0; l < eRNListType_ListsNum; l++)
        {
            for (IRenderNode* pObj = m_arrObjects[l].m_pFirstNode; pObj; pObj = pObj->m_pNext)
            {
                if (pObj->GetRndFlags() & ERF_CASTSHADOWMAPS)
                {
                    nCount++;
                }
            }
        }
        break;
    }

    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            nCount += m_arrChilds[i]->GetObjectsCount(eListType);
        }
    }

    return nCount;
}


void COctreeNode::GetMemoryUsage(ICrySizer* pSizer) const
{
    for (int l = 0; l < eRNListType_ListsNum; l++)
    {
        for (IRenderNode* pObj = m_arrObjects[l].m_pFirstNode; pObj; pObj = pObj->m_pNext)
        {
            EERType eType = pObj->GetRenderNodeType();
            if (!(eType == eERType_Vegetation && pObj->GetRndFlags() & ERF_PROCEDURAL))
            {
                pObj->GetMemoryUsage(pSizer);
            }
        }
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "ObjLists");

        pSizer->AddObject(m_lstCasters);
    }

    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            m_arrChilds[i]->GetMemoryUsage(pSizer);
        }
    }

    if (pSizer)
    {
        pSizer->AddObject(this, sizeof(*this));
    }
}

void COctreeNode::UpdateTerrainNodes(CTerrainNode* pParentNode)
{
    if (pParentNode != 0)
    {
        SetTerrainNode(pParentNode->FindMinNodeContainingBox(GetNodeBox()));
    }
    else if (m_nSID >= 0 && GetTerrain() != 0)
    {
        SetTerrainNode(GetTerrain()->FindMinNodeContainingBox(GetNodeBox()));
    }
    else
    {
        SetTerrainNode(NULL);
    }

    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            m_arrChilds[i]->UpdateTerrainNodes();
        }
    }
}

void C3DEngine::GetObjectsByTypeGlobal(PodArray<IRenderNode*>& lstObjects, EERType objType, const AABB* pBBox)
{
    if (Get3DEngine()->IsObjectTreeReady())
    {
        Get3DEngine()->GetObjectTree()->GetObjectsByType(lstObjects, objType, pBBox);
    }
}

void C3DEngine::MoveObjectsIntoListGlobal(PodArray<SRNInfo>* plstResultEntities, const AABB* pAreaBox,
    bool bRemoveObjects, bool bSkipDecals, bool bSkip_ERF_NO_DECALNODE_DECALS, bool bSkipDynamicObjects,
    EERType eRNType)
{
    if (Get3DEngine()->IsObjectTreeReady())
    {
        Get3DEngine()->GetObjectTree()->MoveObjectsIntoList(plstResultEntities, pAreaBox, bRemoveObjects, bSkipDecals, bSkip_ERF_NO_DECALNODE_DECALS, bSkipDynamicObjects, eRNType);
    }
}

void COctreeNode::ActivateObjectsLayer(uint16 nLayerId, bool bActivate, bool bPhys, IGeneralMemoryHeap* pHeap)
{
    for (IRenderNode* pObj = m_arrObjects[eRNListType_Brush].m_pFirstNode; pObj; pObj = pObj->m_pNext)
    {
        if (pObj->GetRenderNodeType() == eERType_Brush)
        {
            CBrush* pBrush = (CBrush*)pObj;
            if (pBrush->m_nLayerId == nLayerId || nLayerId == uint16(~0))
            {
                if ((bActivate && pBrush->m_dwRndFlags & ERF_HIDDEN) || (!bActivate && !(pBrush->m_dwRndFlags & ERF_HIDDEN)))
                {
                    SetCompiled(false);
                }

                pBrush->SetRndFlags(ERF_HIDDEN, !bActivate);
                pBrush->SetRndFlags(ERF_ACTIVE_LAYER, bActivate);

                if (GetCVars()->e_ObjectLayersActivationPhysics == 1)
                {
                    if (bActivate && bPhys)
                    {
                        pBrush->PhysicalizeOnHeap(pHeap, false);
                    }
                    else
                    {
                        pBrush->Dephysicalize();
                    }
                }
                else if (!bPhys)
                {
                    pBrush->Dephysicalize();
                }
            }
        }
    }

    for (IRenderNode* pObj = m_arrObjects[eRNListType_DecalsAndRoads].m_pFirstNode; pObj; pObj = pObj->m_pNext)
    {
        EERType eType = pObj->GetRenderNodeType();

        if (eType == eERType_Decal)
        {
            CDecalRenderNode* pDecal = (CDecalRenderNode*)pObj;
            if (pDecal->GetLayerId() == nLayerId || nLayerId == uint16(~0))
            {
                pDecal->SetRndFlags(ERF_HIDDEN, !bActivate);

                if (bActivate)
                {
                    pDecal->RequestUpdate();
                }
                else
                {
                    pDecal->DeleteDecal();
                }
            }
        }

        if (eType == eERType_Road)
        {
            CRoadRenderNode* pDecal = (CRoadRenderNode*)pObj;
            if (pDecal->GetLayerId() == nLayerId || nLayerId == uint16(~0))
            {
                pDecal->SetRndFlags(ERF_HIDDEN, !bActivate);
            }
        }
    }

    for (IRenderNode* pObj = m_arrObjects[eRNListType_Unknown].m_pFirstNode; pObj; pObj = pObj->m_pNext)
    {
        if (pObj->GetRenderNodeType() == eERType_WaterVolume)
        {
            CWaterVolumeRenderNode* pWatVol = (CWaterVolumeRenderNode*)pObj;
            if (pWatVol->GetLayerId() == nLayerId || nLayerId == uint16(~0))
            {
                pWatVol->SetRndFlags(ERF_HIDDEN, !bActivate);

                if (GetCVars()->e_ObjectLayersActivationPhysics)
                {
                    if (bActivate && bPhys)
                    {
                        pWatVol->Physicalize();
                    }
                    else
                    {
                        pWatVol->Dephysicalize();
                    }
                }
                else if (!bPhys)
                {
                    pWatVol->Dephysicalize();
                }
            }
        }

        if (pObj->GetRenderNodeType() == eERType_DistanceCloud)
        {
            CDistanceCloudRenderNode* pCloud = (CDistanceCloudRenderNode*)pObj;
            if (pCloud->GetLayerId() == nLayerId || nLayerId == uint16(~0))
            {
                pCloud->SetRndFlags(ERF_HIDDEN, !bActivate);
            }
        }
    }

    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            m_arrChilds[i]->ActivateObjectsLayer(nLayerId, bActivate, bPhys, pHeap);
        }
    }
}

void COctreeNode::GetLayerMemoryUsage(uint16 nLayerId, ICrySizer* pSizer, int* pNumBrushes, int* pNumDecals)
{
    for (IRenderNode* pObj = m_arrObjects[eRNListType_Brush].m_pFirstNode; pObj; pObj = pObj->m_pNext)
    {
        if (pObj->GetRenderNodeType() == eERType_Brush)
        {
            CBrush* pBrush = (CBrush*)pObj;
            if (pBrush->m_nLayerId == nLayerId || nLayerId == uint16(~0))
            {
                pBrush->GetMemoryUsage(pSizer);
                if (pNumBrushes)
                {
                    (*pNumBrushes)++;
                }
            }
        }
    }

    for (IRenderNode* pObj = m_arrObjects[eRNListType_DecalsAndRoads].m_pFirstNode; pObj; pObj = pObj->m_pNext)
    {
        EERType eType = pObj->GetRenderNodeType();

        if (eType == eERType_Decal)
        {
            CDecalRenderNode* pDecal = (CDecalRenderNode*)pObj;
            if (pDecal->GetLayerId() == nLayerId || nLayerId == uint16(~0))
            {
                pDecal->GetMemoryUsage(pSizer);
                if (pNumDecals)
                {
                    (*pNumDecals)++;
                }
            }
        }

        if (eType == eERType_Road)
        {
            CRoadRenderNode* pDecal = (CRoadRenderNode*)pObj;
            if (pDecal->GetLayerId() == nLayerId || nLayerId == uint16(~0))
            {
                pDecal->GetMemoryUsage(pSizer);
                if (pNumDecals)
                {
                    (*pNumDecals)++;
                }
            }
        }
    }


    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            m_arrChilds[i]->GetLayerMemoryUsage(nLayerId, pSizer, pNumBrushes, pNumDecals);
        }
    }
}


void COctreeNode::GetObjects(PodArray<IRenderNode*>& lstObjects, const AABB* pBBox)
{
    if (pBBox && !Overlap::AABB_AABB(*pBBox, GetObjectsBBox()))
    {
        return;
    }

    unsigned int nCurrentObject(eRNListType_First);
    for (nCurrentObject = eRNListType_First; nCurrentObject < eRNListType_ListsNum; ++nCurrentObject)
    {
        for (IRenderNode* pObj = m_arrObjects[nCurrentObject].m_pFirstNode; pObj; pObj = pObj->m_pNext)
        {
            if (!pBBox || Overlap::AABB_AABB(*pBBox, pObj->GetBBox()))
            {
                lstObjects.Add(pObj);
            }
        }
    }

    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            m_arrChilds[i]->GetObjects(lstObjects, pBBox);
        }
    }
}

bool COctreeNode::GetShadowCastersTimeSliced(IRenderNode* pIgnoreNode, ShadowMapFrustum* pFrustum, int renderNodeExcludeFlags, int& totalRemainingNodes, int nCurLevel, const SRenderingPassInfo& passInfo)
{
    assert(pFrustum->pShadowCacheData);

    if (totalRemainingNodes <= 0)
    {
        return false;
    }

    if (!pFrustum->pShadowCacheData->mOctreePathNodeProcessed[nCurLevel])
    {
        if (pFrustum->aabbCasters.IsReset() || Overlap::AABB_AABB(pFrustum->aabbCasters, GetObjectsBBox()))
        {
            for (int l = 0; l < eRNListType_ListsNum; l++)
            {
                for (IRenderNode* pNode = m_arrObjects[l].m_pFirstNode; pNode; pNode = pNode->m_pNext)
                {
                    if (!IsRenderNodeTypeEnabled(pNode->GetRenderNodeType()))
                    {
                        continue;
                    }

                    if (pNode == pIgnoreNode)
                    {
                        continue;
                    }

                    const int nFlags = pNode->GetRndFlags();
                    if (nFlags & (ERF_HIDDEN | ERF_COLLISION_PROXY | ERF_RAYCAST_PROXY | renderNodeExcludeFlags))
                    {
                        continue;
                    }

                    // Ignore ERF_CASTSHADOWMAPS for ambient occlusion casters
                    if (pFrustum->m_eFrustumType != ShadowMapFrustum::e_HeightMapAO && (pNode->GetRndFlags() & ERF_CASTSHADOWMAPS) == 0)
                    {
                        continue;
                    }

                    if (pFrustum->pShadowCacheData->mProcessedCasters.find(pNode) != pFrustum->pShadowCacheData->mProcessedCasters.end())
                    {
                        continue;
                    }

                    AABB objBox = pNode->GetBBox();
                    const float fDistanceSq = Distance::Point_PointSq(passInfo.GetCamera().GetPosition(), objBox.GetCenter());
                    const float fMaxDist = pNode->GetMaxViewDist() * GetCVars()->e_ShadowsCastViewDistRatio + objBox.GetRadius();

                    if (fDistanceSq > sqr(fMaxDist))
                    {
                        continue;
                    }

                    // find closest loaded lod
                    for (int nSlot = 0; nSlot < pNode->GetSlotCount(); ++nSlot)
                    {
                        bool bCanRender = false;

                        if (IStatObj* pStatObj = pNode->GetEntityStatObj(nSlot))
                        {
                            for (int i = 0; i < MAX_STATOBJ_LODS_NUM; ++i)
                            {
                                IStatObj* pLod = pStatObj->GetLodObject(i);

                                if (pLod && pLod->m_eStreamingStatus == ecss_Ready)
                                {
                                    bCanRender = true;
                                    break;
                                }
                            }
                        }
                        else if (ICharacterInstance* pCharacter = pNode->GetEntityCharacter(nSlot))
                        {
                            bCanRender = GetCVars()->e_ShadowsCacheRenderCharacters != 0;
                        }

                        if (bCanRender)
                        {
                            if (pNode->CanExecuteRenderAsJob())
                            {
                                Get3DEngine()->CheckCreateRNTmpData(&pNode->m_pRNTmpData, pNode, passInfo);
                                pFrustum->pJobExecutedCastersList->Add(pNode);
                            }
                            else
                            {
                                pFrustum->pCastersList->Add(pNode);
                            }
                        }
                    }
                }
            }
        }

        pFrustum->pShadowCacheData->mOctreePathNodeProcessed[nCurLevel] = true;
        if (!pFrustum->pCastersList->empty() || !pFrustum->pJobExecutedCastersList->empty())
        {
            --totalRemainingNodes;
        }
    }


    for (int i = pFrustum->pShadowCacheData->mOctreePath[nCurLevel]; i < 8; ++i)
    {
        if (m_arrChilds[i] && (m_arrChilds[i]->m_renderFlags & ERF_CASTSHADOWMAPS))
        {
            bool bDone = m_arrChilds[i]->GetShadowCastersTimeSliced(pIgnoreNode, pFrustum, renderNodeExcludeFlags, totalRemainingNodes, nCurLevel + 1, passInfo);

            if (!bDone)
            {
                return false;
            }
        }

        pFrustum->pShadowCacheData->mOctreePath[nCurLevel] = i;
    }

    // this subtree is fully processed: reset traversal state
    pFrustum->pShadowCacheData->mOctreePath[nCurLevel] = 0;
    pFrustum->pShadowCacheData->mOctreePathNodeProcessed[nCurLevel] = 0;
    return true;
}

bool COctreeNode::IsObjectTypeInTheBox(EERType objType, const AABB& WSBBox)
{
    if (!Overlap::AABB_AABB(WSBBox, GetObjectsBBox()))
    {
        return false;
    }

    if (objType == eERType_Road && !m_bHasRoads)
    {
        return false;
    }

    ERNListType eListType = IRenderNode::GetRenderNodeListId(objType);

    for (IRenderNode* pObj = m_arrObjects[eListType].m_pFirstNode; pObj; pObj = pObj->m_pNext)
    {
        if (pObj->GetRenderNodeType() == objType)
        {
            if (Overlap::AABB_AABB(WSBBox, pObj->GetBBox()))
            {
                return true;
            }
        }
    }

    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            if (m_arrChilds[i]->IsObjectTypeInTheBox(objType, WSBBox))
            {
                return true;
            }
        }
    }

    return false;
}

#ifdef SUPPORT_TERRAIN_AO_PRE_COMPUTATIONS

bool COctreeNode::RayObjectsIntersection2D(Vec3 vStart, Vec3 vEnd, Vec3& vClosestHitPoint, float& fClosestHitDistance, EERType eERType)
{
    FUNCTION_PROFILER_3DENGINE;

    //  Vec3 vBoxHitPoint;
    //  if(!Intersect::Ray_AABB(Ray(vStart, vEnd-vStart), m_objectsBox, vBoxHitPoint))
    //  return false;

    if (vStart.x > m_objectsBox.max.x || vStart.y > m_objectsBox.max.y ||
        vStart.x < m_objectsBox.min.x || vStart.y < m_objectsBox.min.y)
    {
        return false;
    }

    if (!IsCompiled())
    {
        CompileObjects();
    }

    const bool oceanEnabled = OceanToggle::IsActive() ? OceanRequest::OceanIsEnabled() : true;
    const float fOceanLevel = OceanToggle::IsActive() ? OceanRequest::GetOceanLevel() : GetTerrain()->GetWaterLevel();

    ERNListType eListType = IRenderNode::GetRenderNodeListId(eERType);

    for (IRenderNode* pObj = m_arrObjects[eListType].m_pFirstNode; pObj; pObj = pObj->m_pNext)
    {
        uint32 dwFlags = pObj->GetRndFlags();

        if (dwFlags & ERF_HIDDEN || !(dwFlags & ERF_CASTSHADOWMAPS) || (eERType == eERType_Vegetation && dwFlags & ERF_PROCEDURAL) || dwFlags & ERF_COLLISION_PROXY)
        {
            continue;
        }

        if (pObj->GetRenderNodeType() != eERType)
        {
            continue;
        }

        //      if(!Intersect::Ray_AABB(Ray(vStart, vEnd-vStart), pObj->GetBBox(), vBoxHitPoint))
        //      continue;

        const AABB& objBox = pObj->GetBBox();

        if ((objBox.max.z - objBox.min.z) < 2.f)
        {
            continue;
        }

        if (oceanEnabled && objBox.max.z < fOceanLevel)
        {
            continue;
        }

        if (vStart.x > objBox.max.x || vStart.y > objBox.max.y || vStart.x < objBox.min.x || vStart.y < objBox.min.y)
        {
            continue;
        }

        Matrix34A objMatrix;
        CStatObj* pStatObj = (CStatObj*)pObj->GetEntityStatObj(0, 0, &objMatrix);

        if (pStatObj->GetOcclusionAmount() < 0.32f)
        {
            continue;
        }

        {
            if (pStatObj->m_nFlags & STATIC_OBJECT_HIDDEN)
            {
                continue;
            }

            Matrix34 matInv = objMatrix.GetInverted();
            Vec3 vOSStart = matInv.TransformPoint(vStart);
            Vec3 vOSEnd = matInv.TransformPoint(vEnd);
            Vec3 vOSHitPoint(0, 0, 0), vOSHitNorm(0, 0, 0);

            Vec3 vBoxHitPoint;
            if (!Intersect::Ray_AABB(Ray(vOSStart, vOSEnd - vOSStart), pStatObj->GetAABB(), vBoxHitPoint))
            {
                continue;
            }

            vOSHitPoint = vOSStart;
            vOSHitPoint.z = pStatObj->GetObjectHeight(vOSStart.x, vOSStart.y);

            if (vOSHitPoint.z != 0)
            {
                Vec3 vHitPoint = objMatrix.TransformPoint(vOSHitPoint);
                float fDist = vHitPoint.GetDistance(vStart);
                if (fDist < fClosestHitDistance)
                {
                    fClosestHitDistance = fDist;
                    vClosestHitPoint = vHitPoint;
                }
            }
        }
    }

    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            m_arrChilds[i]->RayObjectsIntersection2D(vStart, vEnd, vClosestHitPoint, fClosestHitDistance, eERType);
        }
    }

    return false;
}

#endif

void COctreeNode::GenerateStatObjAndMatTables(std::vector<IStatObj*>* pStatObjTable, std::vector<_smart_ptr<IMaterial> >* pMatTable, std::vector<IStatInstGroup*>* pStatInstGroupTable, SHotUpdateInfo* pExportInfo)
{
    COMPILE_TIME_ASSERT(eERType_TypesNum == 28);//if eERType number is changed, have to check this code.
    AABB* pBox = (pExportInfo && !pExportInfo->areaBox.IsReset()) ? &pExportInfo->areaBox : NULL;

    if (pBox && !Overlap::AABB_AABB(GetNodeBox(), *pBox))
    {
        return;
    }

    uint32 nObjTypeMask = pExportInfo ? pExportInfo->nObjTypeMask : (uint32) ~0;

    for (int l = 0; l < eRNListType_ListsNum; l++)
    {
        for (IRenderNode* pObj = m_arrObjects[l].m_pFirstNode; pObj; pObj = pObj->m_pNext)
        {
            EERType eType = pObj->GetRenderNodeType();

            if (!(nObjTypeMask & (1 << eType)))
            {
                continue;
            }

            if (eType == eERType_Brush)
            {
                CBrush* pBrush = (CBrush*)pObj;
                if (CObjManager::GetItemId<IStatObj>(pStatObjTable, pBrush->GetEntityStatObj(), false) < 0)
                {
                    pStatObjTable->push_back(pBrush->m_pStatObj);
                }
            }

            //Add static meshes that have static transforms to the
            //static object table.
            if (eType == eERType_StaticMeshRenderComponent)
            {
                if (CObjManager::GetItemId<IStatObj>(pStatObjTable, pObj->GetEntityStatObj(), false) < 0)
                {
                    pStatObjTable->push_back(pObj->GetEntityStatObj());
                }
            }

            if (eType == eERType_Brush ||
                eType == eERType_Road ||
                eType == eERType_Decal ||
                eType == eERType_WaterVolume ||
                eType == eERType_DistanceCloud ||
                eType == eERType_StaticMeshRenderComponent)
            {
                if ((eType != eERType_Brush || ((CBrush*)pObj)->m_pMaterial) && CObjManager::GetItemId(pMatTable, pObj->GetMaterial(), false) < 0)
                {
                    pMatTable->push_back(pObj->GetMaterial());
                }
            }

            if (eType == eERType_Vegetation)
            {
                CVegetation* pVegetation = (CVegetation*)pObj;
                IStatInstGroup& pStatInstGroup = pVegetation->GetStatObjGroup();
                stl::push_back_unique(*pStatInstGroupTable, &pStatInstGroup);
            }
            if (eType == eERType_MergedMesh)
            {
                CMergedMeshRenderNode* pRN = (CMergedMeshRenderNode*)pObj;
                for (uint32 i = 0; i < pRN->NumGroups(); ++i)
                {
                    int grpid = pRN->Group(i)->instGroupId;
                    IStatInstGroup& pStatInstGroup = pRN->GetStatObjGroup(grpid);
                    stl::push_back_unique(*pStatInstGroupTable, &pStatInstGroup);
                }
            }
        }
    }

    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            m_arrChilds[i]->GenerateStatObjAndMatTables(pStatObjTable, pMatTable, pStatInstGroupTable, pExportInfo);
        }
    }
}

int COctreeNode::Cmp_OctreeNodeSize(const void* v1, const void* v2)
{
    COctreeNode* pNode1 = *((COctreeNode**)v1);
    COctreeNode* pNode2 = *((COctreeNode**)v2);

    if (pNode1->GetNodeRadius2() > pNode2->GetNodeRadius2())
    {
        return +1;
    }
    if (pNode1->GetNodeRadius2() < pNode2->GetNodeRadius2())
    {
        return -1;
    }

    return 0;
}

COctreeNode* COctreeNode::FindChildFor(IRenderNode* pObj, const AABB& objBox, const float fObjRadius, const Vec3& vObjCenter)
{
    int nChildId =
        ((vObjCenter.x > m_vNodeCenter.x) ? 4 : 0) |
        ((vObjCenter.y > m_vNodeCenter.y) ? 2 : 0) |
        ((vObjCenter.z > m_vNodeCenter.z) ? 1 : 0);

    if (!m_arrChilds[nChildId])
    {
        m_arrChilds[nChildId] = COctreeNode::Create(m_nSID, GetChildBBox(nChildId), m_pVisArea, this);
    }

    return m_arrChilds[nChildId];
}


bool COctreeNode::HasChildNodes()
{
    if (!m_arrChilds[0] && !m_arrChilds[1] && !m_arrChilds[2] && !m_arrChilds[3])
    {
        if (!m_arrChilds[4] && !m_arrChilds[5] && !m_arrChilds[6] && !m_arrChilds[7])
        {
            return false;
        }
    }

    return true;
}

int COctreeNode::CountChildNodes()
{
    return
        (m_arrChilds[0] != 0) +
        (m_arrChilds[1] != 0) +
        (m_arrChilds[2] != 0) +
        (m_arrChilds[3] != 0) +
        (m_arrChilds[4] != 0) +
        (m_arrChilds[5] != 0) +
        (m_arrChilds[6] != 0) +
        (m_arrChilds[7] != 0);
}

void COctreeNode::ReleaseEmptyNodes()
{
    FUNCTION_PROFILER_3DENGINE;

    if (!m_arrEmptyNodes.Count())
    {
        return;
    }

    // sort childs first
    qsort(m_arrEmptyNodes.GetElements(), m_arrEmptyNodes.Count(), sizeof(m_arrEmptyNodes[0]), Cmp_OctreeNodeSize);

    int nInitCount = m_arrEmptyNodes.Count();

    for (int i = 0; i < nInitCount && m_arrEmptyNodes.Count(); i++)
    {
        COctreeNode* pNode = m_arrEmptyNodes[0];

        if (pNode && pNode->IsEmpty())
        {
            COctreeNode* pParent = pNode->m_pParent;

            // unregister in parent
            for (int n = 0; n < 8; n++)
            {
                if (pParent->m_arrChilds[n] == pNode)
                {
                    pParent->m_arrChilds[n] = NULL;
                }
            }

            delete pNode;

            // request parent validation
            if (pParent && pParent->IsEmpty() && m_arrEmptyNodes.Find(pParent) < 0)
            {
                m_arrEmptyNodes.Add(pParent);
            }
        }

        // remove from list
        m_arrEmptyNodes.Delete(pNode);
    }
}

void COctreeNode::StaticReset()
{
    ReleaseEmptyNodes();
    stl::free_container(m_arrEmptyNodes);
}

static float Distance_PrecacheCam_AABBSq(const SObjManPrecacheCamera& a, const AABB& b)
{
    float d2 = 0.0f;

    if (a.bbox.max.x < b.min.x)
    {
        d2 += sqr(b.min.x - a.bbox.max.x);
    }
    if (b.max.x < a.bbox.min.x)
    {
        d2 += sqr(a.bbox.min.x - b.max.x);
    }
    if (a.bbox.max.y < b.min.y)
    {
        d2 += sqr(b.min.y - a.bbox.max.y);
    }
    if (b.max.y < a.bbox.min.y)
    {
        d2 += sqr(a.bbox.min.y - b.max.y);
    }
    if (a.bbox.max.z < b.min.z)
    {
        d2 += sqr(b.min.z - a.bbox.max.z);
    }
    if (b.max.z < a.bbox.min.z)
    {
        d2 += sqr(a.bbox.min.z - b.max.z);
    }

    return d2;
}

bool COctreeNode::UpdateStreamingPriority(PodArray<COctreeNode*>& arrRecursion, float fMinDist, float fMaxDist, bool bFullUpdate, const SObjManPrecacheCamera* pPrecacheCams, size_t nPrecacheCams, const SRenderingPassInfo& passInfo)
{
    //  FUNCTION_PROFILER_3DENGINE;

    // Select the minimum distance to the node
    float fNodeDistanceSq = Distance_PrecacheCam_AABBSq(pPrecacheCams[0], m_objectsBox);
    for (size_t iPrecacheCam = 1; iPrecacheCam < nPrecacheCams; ++iPrecacheCam)
    {
        float fPcNodeDistanceSq = Distance_PrecacheCam_AABBSq(pPrecacheCams[iPrecacheCam], m_objectsBox);
        fNodeDistanceSq = min(fNodeDistanceSq, fPcNodeDistanceSq);
    }
    float fNodeDistance = sqrt_tpl(fNodeDistanceSq);

    if (passInfo.GetCamera().IsAABBVisible_E(GetNodeBox()))
    {
        fNodeDistance *= passInfo.GetZoomFactor();
    }

    if (!IsCompiled())
    {
        CompileObjects();
    }

    const float fPredictionDistanceFar = GetFloatCVar(e_StreamPredictionDistanceFar);

    if (fNodeDistance > min(m_fObjectsMaxViewDist, fMaxDist) + fPredictionDistanceFar)
    {
        return true;
    }

    AABB objBox;

    const bool bEnablePerNodeDistance = GetCVars()->e_StreamCgfUpdatePerNodeDistance > 0;
    CVisArea* pRoot0 = GetVisAreaManager() ? GetVisAreaManager()->GetCurVisArea() : NULL;

    float fMinDistSq = fMinDist * fMinDist;

    PREFAST_SUPPRESS_WARNING(6255)
    float* pfMinVisAreaDistSq = (float*)alloca(sizeof(float) * nPrecacheCams);

    for (size_t iPrecacheCam = 0; iPrecacheCam < nPrecacheCams; ++iPrecacheCam)
    {
        float fMinVisAreaDist = 0.0f;

        if (pRoot0)
        {
            // search from camera to entity visarea or outdoor
            AABB aabbCam = pPrecacheCams[iPrecacheCam].bbox;
            float fResDist = 10000.0f;
            if (pRoot0->GetDistanceThruVisAreas(aabbCam, m_pVisArea, m_objectsBox, bFullUpdate ? 2 : GetCVars()->e_StreamPredictionMaxVisAreaRecursion, fResDist))
            {
                fMinVisAreaDist = fResDist;
            }
        }
        else if (m_pVisArea)
        {
            // search from entity to outdoor
            AABB aabbCam = pPrecacheCams[iPrecacheCam].bbox;
            float fResDist = 10000.0f;
            if (m_pVisArea->GetDistanceThruVisAreas(m_objectsBox, NULL, aabbCam, bFullUpdate ? 2 : GetCVars()->e_StreamPredictionMaxVisAreaRecursion, fResDist))
            {
                fMinVisAreaDist = fResDist;
            }
        }

        pfMinVisAreaDistSq[iPrecacheCam] = fMinVisAreaDist * fMinVisAreaDist;
    }

    for (int l = 0; l < eRNListType_ListsNum; l++)
    {
        for (IRenderNode* pObj = m_arrObjects[l].m_pFirstNode; pObj; pObj = pObj->m_pNext)
        {
            if (pObj->m_pNext)
            {
                cryPrefetchT0SSE(pObj->m_pNext);
            }


            IF (pObj->m_dwRndFlags & ERF_HIDDEN, 0)
            {
                continue;
            }

#ifdef _DEBUG
            const char* szName = pObj->GetName();
            const char* szClassName = pObj->GetEntityClassName();

            if (pObj->GetRndFlags() & ERF_SELECTED)
            {
                int selected = 1;
            }
#endif // _DEBUG

            pObj->FillBBox(objBox);

            // stream more in zoom mode if in frustum
            float fZoomFactorSq = passInfo.GetCamera().IsAABBVisible_E(objBox)
                ? passInfo.GetZoomFactor() * passInfo.GetZoomFactor()
                : 1.0f;

            for (size_t iPrecacheCam = 0; iPrecacheCam < nPrecacheCams; ++iPrecacheCam)
            {
                const Vec3& pcPosition = pPrecacheCams[iPrecacheCam].vPosition;

                float fEntDistanceSq = Distance_PrecacheCam_AABBSq(pPrecacheCams[iPrecacheCam], objBox);
                fEntDistanceSq = max(fEntDistanceSq, fMinDistSq);
                fEntDistanceSq *= fZoomFactorSq;
                fEntDistanceSq = max(fEntDistanceSq, pfMinVisAreaDistSq[iPrecacheCam]);

                float fMaxDistComb = min(pObj->m_fWSMaxViewDist, fMaxDist) + fPredictionDistanceFar;
                float fMaxDistCombSq = fMaxDistComb * fMaxDistComb;

                if (/*fMinDistSq <= fEntDistanceSq &&*/ fEntDistanceSq < fMaxDistCombSq)
                {
                    float fEntDistance = sqrt_tpl(fEntDistanceSq);
                    assert(fEntDistance >= 0 && _finite(fEntDistance));

                    float fDist = fEntDistance;
                    if (!bFullUpdate && fEntDistance < fNodeDistance && bEnablePerNodeDistance)
                    {
                        fDist = fNodeDistance;
                    }

                    // If we're inside the object, very close, or facing the object, set importance scale to 1.0f. Otherwise, 0.8f.
                    float fImportanceScale = (float)fsel(
                            4.0f - fEntDistance,
                            1.0f,
                            (float)fsel(
                                (objBox.GetCenter() - pcPosition).Dot(pPrecacheCams[iPrecacheCam].vDirection),
                                1.0f,
                                0.8f));

                    // I replaced fEntDistance with fNoideDistance here because of Timur request! It's suppose to be unified to-node-distance
                    GetObjManager()->UpdateRenderNodeStreamingPriority(pObj, fDist, fImportanceScale, bFullUpdate, passInfo);
                }
            }
        }
    }

    // Prioritise the first camera (probably the real camera)
    int nFirst =
        ((pPrecacheCams[0].vPosition.x > m_vNodeCenter.x) ? 4 : 0) |
        ((pPrecacheCams[0].vPosition.y > m_vNodeCenter.y) ? 2 : 0) |
        ((pPrecacheCams[0].vPosition.z > m_vNodeCenter.z) ? 1 : 0);

    if (m_arrChilds[nFirst  ])
    {
        arrRecursion.Add(m_arrChilds[nFirst  ]);
    }

    if (m_arrChilds[nFirst ^ 1])
    {
        arrRecursion.Add(m_arrChilds[nFirst ^ 1]);
    }

    if (m_arrChilds[nFirst ^ 2])
    {
        arrRecursion.Add(m_arrChilds[nFirst ^ 2]);
    }

    if (m_arrChilds[nFirst ^ 4])
    {
        arrRecursion.Add(m_arrChilds[nFirst ^ 4]);
    }

    if (m_arrChilds[nFirst ^ 3])
    {
        arrRecursion.Add(m_arrChilds[nFirst ^ 3]);
    }

    if (m_arrChilds[nFirst ^ 5])
    {
        arrRecursion.Add(m_arrChilds[nFirst ^ 5]);
    }

    if (m_arrChilds[nFirst ^ 6])
    {
        arrRecursion.Add(m_arrChilds[nFirst ^ 6]);
    }

    if (m_arrChilds[nFirst ^ 7])
    {
        arrRecursion.Add(m_arrChilds[nFirst ^ 7]);
    }

    return true;
}

int COctreeNode::Load(AZ::IO::HandleType& fileHandle, int& nDataSize, std::vector<IStatObj*>* pStatObjTable, std::vector<_smart_ptr<IMaterial> >* pMatTable, EEndian eEndian, AABB* pBox, const SLayerVisibility* pLayerVisibilityMask)
{
    return Load_T(fileHandle, nDataSize, pStatObjTable, pMatTable, eEndian, pBox, pLayerVisibilityMask);
}
int COctreeNode::Load(uint8*& f, int& nDataSize, std::vector<IStatObj*>* pStatObjTable, std::vector<_smart_ptr<IMaterial> >* pMatTable, EEndian eEndian, AABB* pBox, const SLayerVisibility* pLayerVisibilityMask)
{
    return Load_T(f, nDataSize, pStatObjTable, pMatTable, eEndian, pBox, pLayerVisibilityMask);
}

template <class T>
int COctreeNode::Load_T(T& f, int& nDataSize, std::vector<IStatObj*>* pStatObjTable, std::vector<_smart_ptr<IMaterial> >* pMatTable, EEndian eEndian, AABB* pBox, const SLayerVisibility* pLayerVisibilityMask)
{
    if (pBox && !Overlap::AABB_AABB(GetNodeBox(), *pBox))
    {
        return 0;
    }

    SOcTreeNodeChunk chunk;
    if (!CTerrain::LoadDataFromFile(&chunk, 1, f, nDataSize, eEndian))
    {
        return 0;
    }

    assert(chunk.nChunkVersion == OCTREENODE_CHUNK_VERSION || chunk.nChunkVersion == OCTREENODE_CHUNK_VERSION_OLD);
    if (chunk.nChunkVersion != OCTREENODE_CHUNK_VERSION && chunk.nChunkVersion != OCTREENODE_CHUNK_VERSION_OLD)
    {
        return 0;
    }

    if (chunk.nObjectsBlockSize)
    {
        // load objects data into memory buffer, make sure buffer is aligned

        _smart_ptr<IMemoryBlock> pMemBlock = gEnv->pCryPak->PoolAllocMemoryBlock(chunk.nObjectsBlockSize + 8, "LoadObjectInstances");
        byte* pPtr = (byte*)pMemBlock->GetData();

        while (UINT_PTR(pPtr) & 3)
        {
            pPtr++;
        }

        if (!CTerrain::LoadDataFromFile(pPtr, chunk.nObjectsBlockSize, f, nDataSize, eEndian))
        {
            return 0;
        }

        if (!m_bEditor)
        {
            LoadObjects(pPtr, pPtr + chunk.nObjectsBlockSize, pStatObjTable, pMatTable, eEndian, chunk.nChunkVersion, pLayerVisibilityMask);
        }
    }

    // count number of nodes loaded
    int nNodesNum = 1;

    // process childs
    for (int nChildId = 0; nChildId < 8; nChildId++)
    {
        if (chunk.ucChildsMask & (1 << nChildId))
        {
            if (!m_arrChilds[nChildId])
            {
                m_arrChilds[nChildId] = COctreeNode::Create(m_nSID, GetChildBBox(nChildId), m_pVisArea, this);
            }

            int nNewNodesNum = m_arrChilds[nChildId]->Load_T(f, nDataSize, pStatObjTable, pMatTable, eEndian, pBox, pLayerVisibilityMask);

            if (!nNewNodesNum && !pBox)
            {
                return 0; // data error
            }
            nNodesNum += nNewNodesNum;
        }
    }

    return nNodesNum;
}





#if ENGINE_ENABLE_COMPILATION
int COctreeNode::GetData(byte*& pData, int& nDataSize, std::vector<IStatObj*>* pStatObjTable, std::vector<_smart_ptr<IMaterial> >* pMatTable, std::vector<IStatInstGroup*>* pStatInstGroupTable, EEndian eEndian, SHotUpdateInfo* pExportInfo)
{
    AABB* pBox = (pExportInfo && !pExportInfo->areaBox.IsReset()) ? &pExportInfo->areaBox : NULL;

    const AABB& nodeBox = GetNodeBox();
    if (pBox && !Overlap::AABB_AABB(nodeBox, *pBox))
    {
        return 0;
    }

    if (pData)
    {
        // get node data
        SOcTreeNodeChunk chunk;
        chunk.nChunkVersion = OCTREENODE_CHUNK_VERSION;
        chunk.nodeBox = nodeBox;

        // fill ChildsMask
        chunk.ucChildsMask = 0;
        for (int i = 0; i < 8; i++)
        {
            if (m_arrChilds[i])
            {
                chunk.ucChildsMask |= (1 << i);
            }
        }

        CMemoryBlock memblock;
        SaveObjects(&memblock, pStatObjTable, pMatTable, pStatInstGroupTable, eEndian, pExportInfo);

        chunk.nObjectsBlockSize = memblock.GetSize();

        AddToPtr(pData, nDataSize, chunk, eEndian);

        AddToPtr(pData, nDataSize, (byte*)memblock.GetData(), memblock.GetSize(), eEndian);
    }
    else // just count size
    {
        nDataSize += sizeof(SOcTreeNodeChunk);
        nDataSize += SaveObjects(NULL, NULL, NULL, NULL, eEndian, pExportInfo);
    }

    // count number of nodes loaded
    int nNodesNum = 1;

    // process childs
    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            nNodesNum += m_arrChilds[i]->GetData(pData, nDataSize, pStatObjTable, pMatTable, pStatInstGroupTable, eEndian, pExportInfo);
        }
    }

    return nNodesNum;
}
#endif

bool COctreeNode::CleanUpTree()
{
    //  FreeAreaBrushes();

    bool bChildObjectsFound = false;
    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            if (!m_arrChilds[i]->CleanUpTree())
            {
                delete m_arrChilds[i];
                m_arrChilds[i] = NULL;
            }
            else
            {
                bChildObjectsFound = true;
            }
        }
    }

    // update max view distances

    m_fObjectsMaxViewDist = 0.f;
    m_objectsBox = GetNodeBox();

    for (int l = 0; l < eRNListType_ListsNum; l++)
    {
        for (IRenderNode* pObj = m_arrObjects[l].m_pFirstNode; pObj; pObj = pObj->m_pNext)
        {
            pObj->m_fWSMaxViewDist = pObj->GetMaxViewDist();
            m_fObjectsMaxViewDist = max(m_fObjectsMaxViewDist, pObj->m_fWSMaxViewDist);
            m_objectsBox.Add(pObj->GetBBox());
        }
    }

    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            m_fObjectsMaxViewDist = max(m_fObjectsMaxViewDist, m_arrChilds[i]->m_fObjectsMaxViewDist);
            m_objectsBox.Add(m_arrChilds[i]->m_objectsBox);
        }
    }

    return (bChildObjectsFound || HasObjects());
}

//////////////////////////////////////////////////////////////////////////
bool COctreeNode::CheckRenderFlagsMinSpec(uint32 dwRndFlags)
{
    int nRenderNodeMinSpec = (dwRndFlags & ERF_SPEC_BITS_MASK) >> ERF_SPEC_BITS_SHIFT;
    return CheckMinSpec(nRenderNodeMinSpec);
}

void COctreeNode::OffsetObjects(const Vec3& offset)
{
    SetCompiled(false);
    m_objectsBox.Move(offset);
    m_vNodeCenter += offset;

    for (int l = 0; l < eRNListType_ListsNum; l++)
    {
        for (IRenderNode* pObj = m_arrObjects[l].m_pFirstNode; pObj; pObj = pObj->m_pNext)
        {
            pObj->OffsetPosition(offset);
        }
    }
    for (int i = 0; i < 8; ++i)
    {
        if (m_arrChilds[i])
        {
            m_arrChilds[i]->OffsetObjects(offset);
        }
    }
}

bool COctreeNode::HasAnyRenderableCandidates(const SRenderingPassInfo& passInfo) const
{
    // This checks if anything will be rendered, assuming we pass occlusion checks
    // This is based on COctreeNode::RenderContentJobEntry's implementation,
    // if that would do nothing, we can skip the running of occlusion and rendering jobs for this node
    const bool bVegetation = passInfo.RenderVegetation() && m_arrObjects[eRNListType_Vegetation].m_pFirstNode != NULL;
    const bool bBrushes = passInfo.RenderBrushes() && m_arrObjects[eRNListType_Brush].m_pFirstNode != NULL;
    const bool bDecalsAndRoads = (passInfo.RenderDecals() || passInfo.RenderRoads()) && m_arrObjects[eRNListType_DecalsAndRoads].m_pFirstNode != NULL;
    const bool bUnknown = m_arrObjects[eRNListType_Unknown].m_pFirstNode != NULL;
    return bVegetation || bBrushes || bDecalsAndRoads || bUnknown;
}