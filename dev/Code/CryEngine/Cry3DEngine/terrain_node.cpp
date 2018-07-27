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

// Description : terrain node


#include "StdAfx.h"
#include "terrain_sector.h"
#include "terrain.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include "Vegetation.h"
#include <AABBSV.h>
#include "LightEntity.h"
#include "MTPseudoRandom.h"

CProcVegetPoolMan* CTerrainNode::m_pProcObjPoolMan = NULL;
SProcObjChunkPool* CTerrainNode::m_pProcObjChunkPool = NULL;

void CTerrainNode::PropagateChangesToRoot()
{
    ReleaseHeightMapGeometry();
    RemoveProcObjects();
    UpdateDetailLayersInfo(false);

    CTerrainNode* node = m_Parent;
    while (node)
    {
        node->ReleaseHeightMapGeometry();
        node->RemoveProcObjects();
        node->UpdateDetailLayersInfo(false);

        // propagate bounding boxes and error metrics to parents
        for (int i = 0; i < GetTerrain()->m_UnitToSectorBitShift; i++)
        {
            node->m_ZErrorFromBaseLOD[i] = max(max(
                        node->m_Children[0].m_ZErrorFromBaseLOD[i],
                        node->m_Children[1].m_ZErrorFromBaseLOD[i]), max(
                        node->m_Children[2].m_ZErrorFromBaseLOD[i],
                        node->m_Children[3].m_ZErrorFromBaseLOD[i]));
        }

        node->m_LocalAABB.min = SetMaxBB();
        node->m_LocalAABB.max = SetMinBB();

        for (int nChild = 0; nChild < 4; nChild++)
        {
            node->m_LocalAABB.min.CheckMin(node->m_Children[nChild].m_LocalAABB.min);
            node->m_LocalAABB.max.CheckMax(node->m_Children[nChild].m_LocalAABB.max);
        }

        node = node->m_Parent;
    }
}

float GetPointToBoxDistance(Vec3 vPos, AABB bbox)
{
    if (vPos.x >= bbox.min.x && vPos.x <= bbox.max.x)
    {
        if (vPos.y >= bbox.min.y && vPos.y <= bbox.max.y)
        {
            if (vPos.z >= bbox.min.z && vPos.z <= bbox.max.z)
            {
                return 0; // inside
            }
        }
    }
    float dy;
    if (vPos.y < bbox.min.y)
    {
        dy = bbox.min.y - vPos.y;
    }
    else if (vPos.y > bbox.max.y)
    {
        dy = vPos.y - bbox.max.y;
    }
    else
    {
        dy = 0;
    }

    float dx;
    if (vPos.x < bbox.min.x)
    {
        dx = bbox.min.x - vPos.x;
    }
    else if (vPos.x > bbox.max.x)
    {
        dx = vPos.x - bbox.max.x;
    }
    else
    {
        dx = 0;
    }

    float dz;
    if (vPos.z < bbox.min.z)
    {
        dz = bbox.min.z - vPos.z;
    }
    else if (vPos.z > bbox.max.z)
    {
        dz = vPos.z - bbox.max.z;
    }
    else
    {
        dz = 0;
    }

    return sqrt_tpl(dx * dx + dy * dy + dz * dz);
}

// Hierarchically check nodes visibility
// Add visible sectors to the list of visible terrain sectors

bool CTerrainNode::CheckVis(bool bAllInside, bool bAllowRenderIntoCBuffer, const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE;

    m_QueuedLOD = MML_NOT_SET;

    const AABB& worldBox = GetBBox();
    const CCamera& camera = passInfo.GetCamera();

    if (!bAllInside && !camera.IsAABBVisible_EHM(worldBox, &bAllInside))
    {
        return false;
    }

    const float distanceToCamera = GetPointToBoxDistance(camera.GetPosition(), worldBox);
    m_DistanceToCamera[passInfo.GetRecursiveLevel()] = distanceToCamera;

    if (distanceToCamera > camera.GetFarPlane())
    {
        return false; // too far
    }
    if (m_bHasHoles == 2)
    {
        return false; // has no visible mesh
    }
    Get3DEngine()->CheckCreateRNTmpData(&m_pRNTmpData, NULL, passInfo);

    // occlusion test (affects only static objects)
    if (m_Parent && GetObjManager()->IsBoxOccluded(worldBox, distanceToCamera, &m_pRNTmpData->userData.m_OcclState, false, eoot_TERRAIN_NODE, passInfo))
    {
        return false;
    }

    // find LOD of this sector
    SetLOD(passInfo);

    m_nSetLodFrameId = passInfo.GetMainFrameID();

    int nSectorSize = CTerrain::GetSectorSize() << m_nTreeLevel;
    bool bCameraInsideSector = distanceToCamera < nSectorSize;

    // The Geometry / Texture LODs have nothing to do with the tree level. But this is roughly saying that
    // we should traverse deeper into the tree if we have a certain threshold of geometry or texture detail,
    // these exact heuristics are just empirical.
    //
    // There are two things to consider when mucking with these heuristics:
    //   1) This doesn't actually control the geometry LOD, it only controls the render mesh draw call granularity.
    //      This means the farther up the tree you stop, the bigger the mesh you're going to build, which is
    //      a tradeoff between draw call count and dynamic mesh size. As an example, if we are at LOD 2, that equates to
    //      a step size of 4 units. No matter where we stop the recursion, the geometry is still built at 1 vertex per 4 units.
    //      This heuristic controls how big the sector is, so if, theoretically, we stopped at the root node, we would build single giant
    //      render mesh at 1 vertex per 4 units.
    //
    //   2) The texture tiles are setup such that the maximum resolution at a node is 256x256 pixels. This means if you don't refine the
    //      quadtree enough will a super high-res texture, you can artificially limit the texture LOD.
    //
    bool bHasMoreGeometryDetail = (m_QueuedLOD + GetTerrain()->m_MeterToUnitBitShift) <= m_nTreeLevel;

    // (TODO bethelz): I will probably remove this in the next pass. It places a strange dependency on the texture
    // LOD but doesn't use any metric to judge how much detail we're preserving. It mostly just forces render mesh updates for no
    // good reason.
    // bool bHasMoreTextureDetail = (m_TextureLOD + GetTerrain()->m_nBitShift) < m_nTreeLevel;

    bool bNeedsMoreDetail = (bCameraInsideSector || bHasMoreGeometryDetail || /*bHasMoreTextureDetail ||*/ m_bForceHighDetail);
    bool bContinueRecursion = m_Children && bNeedsMoreDetail;

    if (bContinueRecursion)
    {
        Vec3 boxCenter = worldBox.GetCenter();
        Vec3 cameraPos = camera.GetPosition();

        bool bGreaterThanCenterX = (cameraPos.x > boxCenter.x);
        bool bGreaterThanCenterY = (cameraPos.y > boxCenter.y);

        uint32 firstIndex = (bGreaterThanCenterX ? 2 : 0) | (bGreaterThanCenterY ? 1 : 0);
        m_Children[firstIndex].CheckVis(bAllInside, bAllowRenderIntoCBuffer, passInfo);
        m_Children[firstIndex ^ 1].CheckVis(bAllInside, bAllowRenderIntoCBuffer, passInfo);
        m_Children[firstIndex ^ 2].CheckVis(bAllInside, bAllowRenderIntoCBuffer, passInfo);
        m_Children[firstIndex ^ 3].CheckVis(bAllInside, bAllowRenderIntoCBuffer, passInfo);
    }
    else
    {
        if (GetCVars()->e_StatObjBufferRenderTasks == 1 && passInfo.IsGeneralPass())
        {
            GetObjManager()->PushIntoCullQueue(SCheckOcclusionJobData::CreateTerrainJobData(this, GetBBox(), distanceToCamera));
        }
        else
        {
            GetTerrain()->AddVisSector(this);
        }

        if (passInfo.IsGeneralPass())
        {
            if (distanceToCamera < GetTerrain()->m_fDistanceToSectorWithWater)
            {
                GetTerrain()->m_fDistanceToSectorWithWater = distanceToCamera;
            }
        }

        if (m_Children)
        {
            for (int i = 0; i < 4; i++)
            {
                m_Children[i].SetChildsLod(m_QueuedLOD, passInfo);
            }
        }

        RequestTextures(passInfo);
    }

    // update procedural vegetation
    if (GetCVars()->e_ProcVegetation)
    {
        if (passInfo.IsGeneralPass())
        {
            if (distanceToCamera < GetCVars()->e_ProcVegetationMaxViewDistance)
            {
                GetTerrain()->ActivateNodeProcObj(this);
            }
        }
    }

    return true;
}

void CTerrainNode::Init(int x1, int y1, int nNodeSize, CTerrainNode* pParent, bool bBuildErrorsTable)
{
    m_Children = NULL;
    m_pRNTmpData = NULL;
    const int lodCount = GetTerrain()->m_UnitToSectorBitShift;
    m_ZErrorFromBaseLOD = new float[lodCount]; // TODO: fix duplicated reallocation
    memset(m_ZErrorFromBaseLOD, 0, sizeof(float) * GetTerrain()->m_UnitToSectorBitShift);

    m_CurrentLOD = 0;

    m_pLeafData = 0;
    m_nTreeLevel = 0;

    ZeroStruct(m_TextureSet);

    m_LocalAABB.Reset();
    m_QueuedLOD = 100;
    m_TextureLOD = 100;
    m_nLastTimeUsed = (int)GetCurTimeSec() + 20;
    m_Parent = pParent;
    m_nOriginX = x1;
    m_nOriginY = y1;

    memset(&m_SurfaceTile, 0, sizeof(SurfaceTile));

    for (int iStackLevel = 0; iStackLevel < MAX_RECURSION_LEVELS; iStackLevel++)
    {
        m_DistanceToCamera[iStackLevel] = 0.f;
    }

    if (nNodeSize == CTerrain::GetSectorSize())
    {
        m_nTreeLevel = 0;
    }
    else
    {
        int nSize = nNodeSize / 2;
        m_Children = new CTerrainNode[4];
        m_Children[0].Init(x1, y1, nSize, this, bBuildErrorsTable);
        m_Children[1].Init(x1 + nSize, y1, nSize, this, bBuildErrorsTable);
        m_Children[2].Init(x1, y1 + nSize, nSize, this, bBuildErrorsTable);
        m_Children[3].Init(x1 + nSize, y1 + nSize, nSize, this, bBuildErrorsTable);
        m_nTreeLevel = m_Children[0].m_nTreeLevel + 1;

        for (int i = 0; i < lodCount; i++)
        {
            m_ZErrorFromBaseLOD[i] = max(max(
                        m_Children[0].m_ZErrorFromBaseLOD[i],
                        m_Children[1].m_ZErrorFromBaseLOD[i]), max(
                        m_Children[2].m_ZErrorFromBaseLOD[i],
                        m_Children[3].m_ZErrorFromBaseLOD[i]));
        }

        m_LocalAABB.min = SetMaxBB();
        m_LocalAABB.max = SetMinBB();

        for (int nChild = 0; nChild < 4; nChild++)
        {
            m_LocalAABB.min.CheckMin(m_Children[nChild].m_LocalAABB.min);
            m_LocalAABB.max.CheckMax(m_Children[nChild].m_LocalAABB.max);
        }

        m_DistanceToCamera[0] = 2.f * CTerrain::GetTerrainSize();
        if (m_DistanceToCamera[0] < 0)
        {
            m_DistanceToCamera[0] = 0;
        }
    }

    int nSectorSize = CTerrain::GetSectorSize() << m_nTreeLevel;
    assert(x1 >= 0 && y1 >= 0 && x1 < CTerrain::GetTerrainSize() && y1 < CTerrain::GetTerrainSize());
    GetTerrain()->m_NodePyramid[m_nTreeLevel][x1 / nSectorSize][y1 / nSectorSize] = this;
}

CTerrainNode::~CTerrainNode()
{
    if (GetTerrain()->m_pTerrainUpdateDispatcher)
    {
        GetTerrain()->m_pTerrainUpdateDispatcher->RemoveJob(this);
    }

    Get3DEngine()->OnCasterDeleted(this);

    ReleaseHeightMapGeometry();

    SAFE_DELETE_ARRAY(m_Children);

    RemoveProcObjects(false);

    m_bHasHoles = 0;

    delete m_pLeafData;
    m_pLeafData = NULL;

    delete[] m_ZErrorFromBaseLOD;
    m_ZErrorFromBaseLOD = NULL;

    int nSectorSize = CTerrain::GetSectorSize() << m_nTreeLevel;
    assert(m_nOriginX < CTerrain::GetTerrainSize() && m_nOriginY < CTerrain::GetTerrainSize());
    GetTerrain()->m_NodePyramid[m_nTreeLevel][m_nOriginX / nSectorSize][m_nOriginY / nSectorSize] = NULL;

    if (m_pRNTmpData)
    {
        Get3DEngine()->FreeRNTmpData(&m_pRNTmpData);
    }
}

void CTerrainNode::CheckLeafData()
{
    if (!m_pLeafData)
    {
        m_pLeafData = new STerrainNodeLeafData;
    }
}

void CTerrainNode::CheckNodeGeomUnload(const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE;

    float fDistanse = GetPointToBoxDistance(passInfo.GetCamera().GetPosition(), GetBBox()) * passInfo.GetZoomFactor();

    int nTime = fastftol_positive(GetCurTimeSec());

    // support timer reset
    m_nLastTimeUsed = min(m_nLastTimeUsed, nTime);

    if (m_nLastTimeUsed < (nTime - 16) && fDistanse > 512)
    { // try to release vert buffer if not in use int time
        ReleaseHeightMapGeometry();
    }
}

void CTerrainNode::RemoveProcObjects(bool bRecursive)
{
    FUNCTION_PROFILER_3DENGINE;

    // remove procedurally placed objects
    if (m_bProcObjectsReady)
    {
        if (m_pProcObjPoolPtr)
        {
            m_pProcObjPoolPtr->ReleaseAllObjects();
            m_pProcObjPoolMan->ReleaseObject(m_pProcObjPoolPtr);
            m_pProcObjPoolPtr = NULL;
        }

        m_bProcObjectsReady = false;

        if (GetCVars()->e_TerrainLog == 3)
        {
            PrintMessage("ProcObjects removed %d", GetSecIndex());
        }
    }

    if (bRecursive && m_Children)
    {
        for (int i = 0; i < 4; i++)
        {
            m_Children[i].RemoveProcObjects(bRecursive);
        }
    }
}

void CTerrainNode::SetChildsLod(int nNewGeomLOD, const SRenderingPassInfo& passInfo)
{
    m_QueuedLOD = nNewGeomLOD;
    m_nSetLodFrameId = passInfo.GetMainFrameID();

    // Since LOD requests (GetAreaLOD()) are often made to leaf nodes, make sure the new LOD value propagates to all ancestors.
    if (m_Children)
    {
        m_Children[0].SetChildsLod(nNewGeomLOD, passInfo);
        m_Children[1].SetChildsLod(nNewGeomLOD, passInfo);
        m_Children[2].SetChildsLod(nNewGeomLOD, passInfo);
        m_Children[3].SetChildsLod(nNewGeomLOD, passInfo);
    }
}

int CTerrainNode::GetAreaLOD(const SRenderingPassInfo& passInfo)
{
    if (m_nSetLodFrameId == passInfo.GetMainFrameID())
    {
        return m_QueuedLOD;
    }
    return MML_NOT_SET;
}

bool CTerrainNode::RenderNodeHeightmap(const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE;
    if (m_DistanceToCamera[passInfo.GetRecursiveLevel()] < 8) // make sure near sectors are always potentially visible
    {
        m_nLastTimeUsed = fastftol_positive(GetCurTimeSec());
    }

    m_nLastTimeUsed = fastftol_positive(GetCurTimeSec());

    if (!GetVisAreaManager()->IsOutdoorAreasVisible())
    {
        return true; // all fine, no update needed
    }
    const CCamera& rCamera = passInfo.GetCamera();
    if (GetCVars()->e_TerrainDrawThisSectorOnly)
    {
        if (
            rCamera.GetPosition().x > GetBBox().max.x ||
            rCamera.GetPosition().x < GetBBox().min.x ||
            rCamera.GetPosition().y > GetBBox().max.y ||
            rCamera.GetPosition().y < GetBBox().min.y)
        {
            return true;
        }
    }

    if (GetCVars()->e_TerrainBBoxes)
    {
        ColorB colour = ColorB(255 * ((m_nTreeLevel & 1) > 0), 255 * ((m_nTreeLevel & 2) > 0), 255, 255);
        GetRenderer()->GetIRenderAuxGeom()->DrawAABB(GetBBox(), false, colour, eBBD_Faceted);
        if (GetCVars()->e_TerrainBBoxes == 3 && m_SurfaceTile.GetSize())
        {
            GetRenderer()->DrawLabel(GetBBox().GetCenter(), 2, "%dx%d, LOD: %d, %d", m_SurfaceTile.GetSize(), m_SurfaceTile.GetSize(), m_CurrentLOD, m_TextureLOD);
        }
        else if (GetCVars()->e_TerrainBBoxes == 4 && m_ZErrorFromBaseLOD)
        {
            GetRenderer()->DrawLabel(GetBBox().GetCenter(), 2, "%dx%d, LOD: %d, %d", m_SurfaceTile.GetSize(), m_SurfaceTile.GetSize(), m_CurrentLOD, m_TextureLOD);
        }
    }

    if (GetCVars()->e_TerrainDetailMaterialsDebug)
    {
        AZStd::string outputStr;
        int nLayersNum = 0;
        for (int i = 0; i < m_DetailLayers.Count(); i++)
        {
            if (m_DetailLayers[i].HasRM() && m_DetailLayers[i].surfaceType->HasMaterial())
            {
                if (m_DetailLayers[i].GetIndexCount())
                {
                    outputStr += AZStd::string(m_DetailLayers[i].surfaceType->szName) + "\n";
                    nLayersNum++;
                }
            }
        }

        if (nLayersNum >= GetCVars()->e_TerrainDetailMaterialsDebug)
        {
            GetRenderer()->GetIRenderAuxGeom()->DrawAABB(GetBBox(), false, ColorB(255 * ((nLayersNum & 1) > 0), 255 * ((nLayersNum & 2) > 0), 255, 255), eBBD_Faceted);
            GetRenderer()->DrawLabel(GetBBox().GetCenter(), 2, "Total Layers: %d\n%s", nLayersNum, outputStr.c_str());
        }
    }

    // pre-cache surface types
    for (int s = 0; s < ITerrain::SurfaceWeight::Hole; s++)
    {
        SSurfaceType* pSurf = &Cry3DEngineBase::GetTerrain()->m_SurfaceTypes[s];

        if (pSurf->HasMaterial())
        {
            uint8 szProj[] = "XYZ";

            for (int p = 0; p < 3; p++)
            {
                if (CMatInfo* pMatInfo = (CMatInfo*)pSurf->GetMaterialOfProjection(szProj[p]).get())
                {
                    pMatInfo->PrecacheMaterial(GetDistance(passInfo), NULL, GetDistance(passInfo) < 32.f);
                }
            }
        }
    }

    return RenderSector(passInfo);
}

float CTerrainNode::GetSurfaceTypeAmount(Vec3 vPos, int nSurfType)
{
    float fUnitSize = (float)GetTerrain()->GetHeightMapUnitSize();
    vPos *= 1.f / fUnitSize;

    int x1 = int(vPos.x);
    int y1 = int(vPos.y);
    int x2 = int(vPos.x + 1);
    int y2 = int(vPos.y + 1);

    float dx = vPos.x - x1;
    float dy = vPos.y - y1;

    // (bethelz) TODO: Change this to use weights instead of assume first surface.
    float s00 = GetTerrain()->GetSurfaceWeight((int)(x1 * fUnitSize), (int)(y1 * fUnitSize)).PrimaryId() == nSurfType;
    float s01 = GetTerrain()->GetSurfaceWeight((int)(x1 * fUnitSize), (int)(y2 * fUnitSize)).PrimaryId() == nSurfType;
    float s10 = GetTerrain()->GetSurfaceWeight((int)(x2 * fUnitSize), (int)(y1 * fUnitSize)).PrimaryId() == nSurfType;
    float s11 = GetTerrain()->GetSurfaceWeight((int)(x2 * fUnitSize), (int)(y2 * fUnitSize)).PrimaryId() == nSurfType;

    if (s00 || s01 || s10 || s11)
    {
        float s0 = s00 * (1.f - dy) + s01 * dy;
        float s1 = s10 * (1.f - dy) + s11 * dy;
        float res = s0 * (1.f - dx) + s1 * dx;
        return res;
    }

    return 0;
}

bool CTerrainNode::CheckUpdateProcObjects(const SRenderingPassInfo& passInfo)
{
    if (GetCVars()->e_TerrainDrawThisSectorOnly)
    {
        const CCamera& rCamera = passInfo.GetCamera();
        if (
            rCamera.GetPosition().x > GetBBox().max.x ||
            rCamera.GetPosition().x < GetBBox().min.x ||
            rCamera.GetPosition().y > GetBBox().max.y ||
            rCamera.GetPosition().y < GetBBox().min.y)
        {
            return false;
        }
    }

    if (m_bProcObjectsReady)
    {
        return false;
    }

    FUNCTION_PROFILER_3DENGINE;

    int nInstancesCounter = 0;

    CMTRand_int32 rndGen(gEnv->bNoRandomSeed ? 0 : m_nOriginX + m_nOriginY);

    float nSectorSize = (float)(CTerrain::GetSectorSize() << m_nTreeLevel);
    for (int nLayer = 0; nLayer < m_DetailLayers.Count(); nLayer++)
    {
        for (int g = 0; g < m_DetailLayers[nLayer].surfaceType->lstnVegetationGroups.Count(); g++)
        {
            if (m_DetailLayers[nLayer].surfaceType->lstnVegetationGroups[g] >= 0)
            {
                int nGroupId = m_DetailLayers[nLayer].surfaceType->lstnVegetationGroups[g];
                assert(DEFAULT_SID < GetObjManager()->GetListStaticTypes().Count() && nGroupId >= 0 && nGroupId < GetObjManager()->GetListStaticTypes()[DEFAULT_SID].Count());
                if (DEFAULT_SID >= GetObjManager()->GetListStaticTypes().Count() || nGroupId < 0 || nGroupId >= GetObjManager()->GetListStaticTypes()[DEFAULT_SID].Count())
                {
                    continue;
                }
                StatInstGroup* pGroup = &GetObjManager()->GetListStaticTypes()[DEFAULT_SID][nGroupId];
                if (!pGroup || !pGroup->GetStatObj() || pGroup->fSize <= 0 || !pGroup->GetStatObj())
                {
                    continue;
                }

                if (!CheckMinSpec(pGroup->minConfigSpec)) // Check min spec of this group.
                {
                    continue;
                }

                if (pGroup->fDensity < 0.2f)
                {
                    pGroup->fDensity = 0.2f;
                }

                float fMinX = (float)m_nOriginX;
                float fMinY = (float)m_nOriginY;
                float fMaxX = (float)(m_nOriginX + nSectorSize);
                float fMaxY = (float)(m_nOriginY + nSectorSize);

                for (float fX = fMinX; fX < fMaxX; fX += pGroup->fDensity)
                {
                    for (float fY = fMinY; fY < fMaxY; fY += pGroup->fDensity)
                    {
                        Vec3 vPos(fX + (rndGen.GenerateFloat() - 0.5f) * pGroup->fDensity, fY + (rndGen.GenerateFloat() - 0.5f) * pGroup->fDensity, 0);
                        vPos.x = CLAMP(vPos.x, fMinX, fMaxX);
                        vPos.y = CLAMP(vPos.y, fMinY, fMaxY);

                        {
                            // filtered surface type lockup
                            float fSurfaceTypeAmount = GetSurfaceTypeAmount(vPos, m_DetailLayers[nLayer].surfaceType->ucThisSurfaceTypeId);
                            if (fSurfaceTypeAmount <= 0.5)
                            {
                                continue;
                            }

                            vPos.z = GetTerrain()->GetBilinearZ(vPos.x, vPos.y);
                        }

                        Vec3 vWPos = vPos;
                        if (vWPos.x < 0 || vWPos.x >= CTerrain::GetTerrainSize() || vWPos.y < 0 || vWPos.y >= CTerrain::GetTerrainSize())
                        {
                            continue;
                        }

                        float fScale = pGroup->fSize + (rndGen.GenerateFloat() - 0.5f) * pGroup->fSizeVar;
                        if (fScale <= 0)
                        {
                            continue;
                        }

                        if (vWPos.z < pGroup->fElevationMin || vWPos.z > pGroup->fElevationMax)
                        {
                            continue;
                        }

                        // check slope range
                        if (pGroup->fSlopeMin != 0 || pGroup->fSlopeMax != 255)
                        {
                            int nStep = CTerrain::GetHeightMapUnitSize();
                            int x = (int)fX;
                            int y = (int)fY;

                            // calculate surface normal
                            float sx;
                            if ((x + nStep) < CTerrain::GetTerrainSize() && x >= nStep)
                            {
                                sx = GetTerrain()->GetZ(x + nStep, y) - GetTerrain()->GetZ(x - nStep, y);
                            }
                            else
                            {
                                sx = 0;
                            }

                            float sy;
                            if ((y + nStep) < CTerrain::GetTerrainSize() && y >= nStep)
                            {
                                sy = GetTerrain()->GetZ(x, y + nStep) - GetTerrain()->GetZ(x, y - nStep);
                            }
                            else
                            {
                                sy = 0;
                            }

                            Vec3 vNormal = Vec3(-sx, -sy, nStep * 2.0f);
                            vNormal.NormalizeFast();

                            float fSlope = (1 - vNormal.z) * 255;
                            if (fSlope < pGroup->fSlopeMin || fSlope > pGroup->fSlopeMax)
                            {
                                continue;
                            }
                        }

                        if (pGroup->fVegRadius * fScale < GetCVars()->e_VegetationMinSize)
                        {
                            continue; // skip creation of very small objects
                        }
                        if (!m_pProcObjPoolPtr)
                        {
                            m_pProcObjPoolPtr = m_pProcObjPoolMan->GetObject();
                        }

                        CVegetation* pEnt = m_pProcObjPoolPtr->AllocateProcObject();
                        assert(pEnt);

                        pEnt->SetScale(fScale);
                        pEnt->m_vPos = vWPos;

                        pEnt->SetStatObjGroupIndex(nGroupId);

                        const uint32 nRnd = cry_random_uint32(); // keep fixed amount of cry_random() calls
                        const bool bRandomRotation = pGroup->bRandomRotation;
                        const int32 nRotationRange = pGroup->nRotationRangeToTerrainNormal;

                        if (bRandomRotation || nRotationRange >= 360)
                        {
                            pEnt->m_ucAngle = static_cast<byte>(nRnd);
                        }
                        else if (nRotationRange > 0)
                        {
                            const Vec3 vTerrainNormal = Get3DEngine()->GetTerrainSurfaceNormal(vPos);

                            if (abs(vTerrainNormal.x) == 0.f && abs(vTerrainNormal.y) == 0.f)
                            {
                                pEnt->m_ucAngle = static_cast<byte>(nRnd);
                            }
                            else
                            {
                                const float rndDegree = (float)-nRotationRange * 0.5f + (float)(nRnd % (uint32)nRotationRange);
                                const float finaleDegree = RAD2DEG(atan2f(vTerrainNormal.y, vTerrainNormal.x)) + rndDegree;
                                pEnt->m_ucAngle = (byte)((finaleDegree / 360.0f) * 255.0f);
                            }
                        }

                        AABB aabb = pEnt->CalcBBox();

                        pEnt->SetRndFlags(ERF_PROCEDURAL, true);

                        pEnt->m_fWSMaxViewDist = pEnt->GetMaxViewDist(); // note: duplicated

                        pEnt->UpdateRndFlags();

                        pEnt->Physicalize(true);

                        float fObjRadius = aabb.GetRadius();
                        if (fObjRadius > MAX_VALID_OBJECT_VOLUME || !_finite(fObjRadius) || fObjRadius <= 0)
                        {
                            Warning("CTerrainNode::CheckUpdateProcObjects: Object has invalid bbox: %s,%s, GetRadius() = %.2f",
                                pEnt->GetName(), pEnt->GetEntityClassName(), fObjRadius);
                        }
                        if (Get3DEngine()->IsObjectTreeReady())
                        {
                            Get3DEngine()->GetObjectTree()->InsertObject(pEnt, aabb, fObjRadius, aabb.GetCenter());
                        }

                        nInstancesCounter++;
                        if (nInstancesCounter >= (MAX_PROC_OBJ_CHUNKS_NUM / GetCVars()->e_ProcVegetationMaxSectorsInCache) * GetCVars()->e_ProcVegetationMaxObjectsInChunk)
                        {
                            m_bProcObjectsReady = true;
                            AZ_Warning("ProcVegetation", GetCVars()->e_ProcVegetation < 2, "Exceeded maximum procedural vegetation count for terrain node: %d.", nInstancesCounter);
                            return true;
                        }
                    }
                }
            }
        }
    }

    m_bProcObjectsReady = true;
    return true;
}

CVegetation* CProcObjSector::AllocateProcObject()
{
    FUNCTION_PROFILER_3DENGINE;

    // find pool id
    int nLastPoolId = m_nProcVegetNum / GetCVars()->e_ProcVegetationMaxObjectsInChunk;
    if (nLastPoolId >= m_ProcVegetChunks.Count())
    {
        m_ProcVegetChunks.PreAllocate(nLastPoolId + 1, nLastPoolId + 1);
        SProcObjChunk* pChunk = m_ProcVegetChunks[nLastPoolId] = CTerrainNode::GetProcObjChunkPool()->GetObject();

        // init objects
        for (int o = 0; o < GetCVars()->e_ProcVegetationMaxObjectsInChunk; o++)
        {
            pChunk->m_pInstances[o].Init();
        }
    }

    // find empty slot id and return pointer to it
    int nNextSlotInPool = m_nProcVegetNum - nLastPoolId * GetCVars()->e_ProcVegetationMaxObjectsInChunk;
    CVegetation* pObj = &(m_ProcVegetChunks[nLastPoolId]->m_pInstances)[nNextSlotInPool];
    m_nProcVegetNum++;
    return pObj;
}

void CProcObjSector::ReleaseAllObjects()
{
    for (int i = 0; i < m_ProcVegetChunks.Count(); i++)
    {
        SProcObjChunk* pChunk = m_ProcVegetChunks[i];
        for (int o = 0; o < GetCVars()->e_ProcVegetationMaxObjectsInChunk; o++)
        {
            pChunk->m_pInstances[o].ShutDown();
        }
        CTerrainNode::GetProcObjChunkPool()->ReleaseObject(m_ProcVegetChunks[i]);
    }
    m_ProcVegetChunks.Clear();
    m_nProcVegetNum = 0;
}

void CProcObjSector::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this));

    pSizer->AddObject(m_ProcVegetChunks);

    for (int i = 0; i < m_ProcVegetChunks.Count(); i++)
    {
        pSizer->AddObject(m_ProcVegetChunks[i], sizeof(CVegetation) * GetCVars()->e_ProcVegetationMaxObjectsInChunk);
    }
}

CProcObjSector::~CProcObjSector()
{
    FUNCTION_PROFILER_3DENGINE;

    ReleaseAllObjects();
}

void SProcObjChunk::GetMemoryUsage(class ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this));

    if (m_pInstances)
    {
        pSizer->AddObject(m_pInstances, sizeof(CVegetation) * GetCVars()->e_ProcVegetationMaxObjectsInChunk);
    }
}

void CTerrainNode::IntersectTerrainAABB(const AABB& aabbBox, PodArray<CTerrainNode*>& lstResult)
{
    if (GetBBox().IsIntersectBox(aabbBox))
    {
        lstResult.Add(this);
        if (m_Children)
        {
            for (int i = 0; i < 4; i++)
            {
                m_Children[i].IntersectTerrainAABB(aabbBox, lstResult);
            }
        }
    }
}

void CTerrainNode::UpdateDetailLayersInfo(bool bRecursive)
{
    FUNCTION_PROFILER_3DENGINE;

    if (m_Children)
    {
        if (bRecursive)
        {
            for (int child = 0; child < 4; child++)
            {
                m_Children[child].UpdateDetailLayersInfo(bRecursive);
            }
        }

        m_bHasHoles = 0;

        static PodArray<SSurfaceType*> childLayers;
        childLayers.Clear();

        for (int nChild = 0; nChild < 4; nChild++)
        {
            if (m_Children[nChild].m_bHasHoles)
            {
                m_bHasHoles = 1;
            }

            for (int i = 0; i < m_Children[nChild].m_DetailLayers.Count(); i++)
            {
                SSurfaceType* type = m_Children[nChild].m_DetailLayers[i].surfaceType;
                assert(type);
                if (childLayers.Find(type) < 0 && type->HasMaterial())
                {
                    childLayers.Add(type);
                }
            }
        }

        if (m_bHasHoles)
        {
            if (m_Children[0].m_bHasHoles == 2 &&
                m_Children[1].m_bHasHoles == 2 &&
                m_Children[2].m_bHasHoles == 2 &&
                m_Children[3].m_bHasHoles == 2)
            {
                m_bHasHoles = 2;
            }
        }

        for (int i = 0; i < m_DetailLayers.Count(); i++)
        {
            m_DetailLayers[i].DeleteRenderMeshes(GetRenderer());
        }
        m_DetailLayers.Clear();
        m_DetailLayers.PreAllocate(childLayers.Count());

        assert(childLayers.Count() <= SurfaceTile::MaxSurfaceCount);

        for (int i = 0; i < childLayers.Count(); i++)
        {
            DetailLayerMesh layer;
            layer.surfaceType = childLayers[i];
            m_DetailLayers.Add(layer);
        }
    }
    else
    {
        int surfaceTypesInSector[SurfaceTile::MaxSurfaceCount] = { 0 };

        m_bHasHoles = 0;

        for (int X = m_nOriginX; X <= m_nOriginX + CTerrain::GetSectorSize(); X += CTerrain::GetHeightMapUnitSize())
        {
            for (int Y = m_nOriginY; Y <= m_nOriginY + CTerrain::GetSectorSize(); Y += CTerrain::GetHeightMapUnitSize())
            {
                ITerrain::SurfaceWeight weight = GetTerrain()->GetSurfaceWeight(X, Y);
                if (ITerrain::SurfaceWeight::Hole == weight.PrimaryId())
                {
                    m_bHasHoles = 1;
                }

                for (int i = 0; i < ITerrain::SurfaceWeight::WeightCount; ++i)
                {
                    CRY_ASSERT(weight.Ids[i] < SurfaceTile::MaxSurfaceCount);
                    surfaceTypesInSector[weight.Ids[i]]++;
                }
            }
        }

        if (surfaceTypesInSector[ITerrain::SurfaceWeight::Hole] == (CTerrain::GetSectorSize() / CTerrain::GetHeightMapUnitSize() + 1) * (CTerrain::GetSectorSize() / CTerrain::GetHeightMapUnitSize() + 1))
        {
            m_bHasHoles = 2; // only holes
        }

        for (int i = 0; i < m_DetailLayers.Count(); i++)
        {
            m_DetailLayers[i].DeleteRenderMeshes(GetRenderer());
        }
        m_DetailLayers.Clear();

        int nSurfCount = 0;
        for (int i = 0; i < SurfaceTile::MaxSurfaceCount; i++)
        {
            if (surfaceTypesInSector[i])
            {
                DetailLayerMesh si;
                si.surfaceType = &GetTerrain()->m_SurfaceTypes[i];
                if (si.surfaceType->HasMaterial())
                {
                    nSurfCount++;
                }
            }
        }

        m_DetailLayers.PreAllocate(nSurfCount);

        for (int i = 0; i < SurfaceTile::MaxSurfaceCount; i++)
        {
            if (surfaceTypesInSector[i])
            {
                DetailLayerMesh si;
                si.surfaceType = &GetTerrain()->m_SurfaceTypes[i];
                if (si.surfaceType->HasMaterial())
                {
                    m_DetailLayers.Add(si);
                }
            }
        }
    }
}

void CTerrainNode::IntersectWithShadowFrustum(bool bAllIn, PodArray<CTerrainNode*>* plstResult, ShadowMapFrustum* pFrustum, const float fHalfGSMBoxSize, const SRenderingPassInfo& passInfo)
{
    if (bAllIn || (pFrustum && pFrustum->IntersectAABB(GetBBox(), &bAllIn)))
    {
        float fSectorSize = GetBBox().max.x - GetBBox().min.x;
        if (m_Children && (fSectorSize > fHalfGSMBoxSize || fSectorSize > 128))
        {
            for (int i = 0; i < 4; i++)
            {
                m_Children[i].IntersectWithShadowFrustum(bAllIn, plstResult, pFrustum, fHalfGSMBoxSize, passInfo);
            }
        }
        else
        {
            if (!GetLeafData() || !GetLeafData()->m_pRenderMesh || (m_QueuedLOD == MML_NOT_SET && GetLeafData() && GetLeafData()->m_pRenderMesh))
            {
                m_DistanceToCamera[passInfo.GetRecursiveLevel()] = GetPointToBoxDistance(passInfo.GetCamera().GetPosition(), GetBBox());
                Get3DEngine()->CheckCreateRNTmpData(&m_pRNTmpData, NULL, passInfo);
                SetLOD(passInfo);
                int nSectorSize = CTerrain::GetSectorSize() << m_nTreeLevel;
                while ((1 << m_QueuedLOD) * CTerrain::GetHeightMapUnitSize() < nSectorSize / 64)
                {
                    m_QueuedLOD++;
                }
            }

            plstResult->Add(this);
        }
    }
}

void CTerrainNode::IntersectWithBox(const AABB& aabbBox, PodArray<CTerrainNode*>* plstResult)
{
    if (aabbBox.IsIntersectBox(GetBBox()))
    {
        for (int i = 0; m_Children && i < 4; i++)
        {
            m_Children[i].IntersectWithBox(aabbBox, plstResult);
        }

        plstResult->Add(this);
    }
}

CTerrainNode* CTerrainNode::FindMinNodeContainingBox(const AABB& aabbBox)
{
    AABB boxWS = GetBBox();

    if (aabbBox.min.x < boxWS.min.x - 0.01f || aabbBox.min.y < boxWS.min.y - 0.01f ||
        aabbBox.max.x > boxWS.max.x + 0.01f || aabbBox.max.y > boxWS.max.y + 0.01f)
    {
        return NULL;
    }

    if (m_Children)
    {
        for (int i = 0; i < 4; i++)
        {
            if (CTerrainNode* pRes = m_Children[i].FindMinNodeContainingBox(aabbBox))
            {
                return pRes;
            }
        }
    }

    return this;
}

void CTerrainNode::UpdateDistance(const SRenderingPassInfo& passInfo)
{
    m_DistanceToCamera[passInfo.GetRecursiveLevel()] = GetPointToBoxDistance(passInfo.GetCamera().GetPosition(), GetBBox());
}

const float CTerrainNode::GetDistance(const SRenderingPassInfo& passInfo)
{
    return m_DistanceToCamera[passInfo.GetRecursiveLevel()];
}

void CTerrainNode::GetMemoryUsage(ICrySizer* pSizer) const
{
    {
        SIZER_COMPONENT_NAME(pSizer, "TerrainNodeSelf");
        pSizer->AddObject(this, sizeof(*this));
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "SurfaceTypeInfo");
        pSizer->AddObject(m_DetailLayers);
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "Heightmap");
        pSizer->AddObject(m_SurfaceTile.GetHeightmap(), m_SurfaceTile.GetSize() * m_SurfaceTile.GetSize() * sizeof(m_SurfaceTile.GetHeightmap()[0]));
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "Weightmap");
        pSizer->AddObject(m_SurfaceTile.GetWeightmap(), m_SurfaceTile.GetSize() * m_SurfaceTile.GetSize() * sizeof(m_SurfaceTile.GetWeightmap()[0]));
    }

    {
        SIZER_COMPONENT_NAME(pSizer, "GeomErrors");
        pSizer->AddObject(m_ZErrorFromBaseLOD, GetTerrain()->m_UnitToSectorBitShift * sizeof(m_ZErrorFromBaseLOD[0]));
    }

    if (m_pLeafData)
    {
        SIZER_COMPONENT_NAME(pSizer, "LeafData");
        pSizer->AddObject(m_pLeafData, sizeof(*m_pLeafData));
    }

    // childs
    if (m_Children)
    {
        for (int i = 0; i < 4; i++)
        {
            m_Children[i].GetMemoryUsage(pSizer);
        }
    }
}

void CTerrainNode::GetResourceMemoryUsage(ICrySizer* pSizer, const AABB& cstAABB)
{
    size_t  nCurrentChild(0);
    size_t  nTotalChildren(4);

    SIZER_COMPONENT_NAME(pSizer, "CTerrainNode");

    if (m_Children)
    {
        for (nCurrentChild = 0; nCurrentChild < nTotalChildren; ++nCurrentChild)
        {
            m_Children[nCurrentChild].GetResourceMemoryUsage(pSizer, cstAABB);
        }
    }

    size_t  nCurrentSurfaceTypeInfo(0);
    size_t  nNumberOfSurfaceTypeInfo(0);


    nNumberOfSurfaceTypeInfo = m_DetailLayers.size();
    for (nCurrentSurfaceTypeInfo = 0; nCurrentSurfaceTypeInfo < nNumberOfSurfaceTypeInfo; ++nCurrentSurfaceTypeInfo)
    {
        DetailLayerMesh&    rSurfaceType = m_DetailLayers[nCurrentSurfaceTypeInfo];
        if (rSurfaceType.surfaceType)
        {
            if (rSurfaceType.surfaceType->pLayerMat)
            {
                rSurfaceType.surfaceType->pLayerMat->GetResourceMemoryUsage(pSizer);
            }
        }
    }
}

//
// This is used by IShadowCaster to render terrain into the shadow map.
//
void CTerrainNode::Render(const SRendParams& RendParams, const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE;

    // render only prepared nodes for now
    if (!GetLeafData() || !GetLeafData()->m_pRenderMesh)
    {
        // get distances
        m_DistanceToCamera[passInfo.GetRecursiveLevel()] = GetPointToBoxDistance(passInfo.GetCamera().GetPosition(), GetBBox());
        SetLOD(passInfo);

        int nSectorSize = CTerrain::GetSectorSize() << m_nTreeLevel;
        while ((1 << m_QueuedLOD) * CTerrain::GetHeightMapUnitSize() < nSectorSize / 64)
        {
            m_QueuedLOD++;
        }
    }

    if (GetCVars()->e_TerrainDrawThisSectorOnly)
    {
        if (
            passInfo.GetCamera().GetPosition().x > GetBBox().max.x ||
            passInfo.GetCamera().GetPosition().x < GetBBox().min.x ||
            passInfo.GetCamera().GetPosition().y > GetBBox().max.y ||
            passInfo.GetCamera().GetPosition().y < GetBBox().min.y)
        {
            return;
        }
    }

    CheckLeafData();

    if (!RenderSector(passInfo))
    {
        m_pTerrain->m_pTerrainUpdateDispatcher->QueueJob(this, passInfo);
    }

    m_nLastTimeUsed = fastftol_positive(GetCurTimeSec());
}

int DetailLayerMesh::GetIndexCount()
{
    int nCount = 0;
    for (int i = 0; i < 3; i++)
    {
        if (triplanarMeshes[i])
        {
            nCount += triplanarMeshes[i]->GetIndicesCount();
        }
    }

    return nCount;
}

void DetailLayerMesh::DeleteRenderMeshes(IRenderer* pRend)
{
    for (int i = 0; i < 3; i++)
    {
        triplanarMeshes[i] = nullptr;
    }
}

int CTerrainNode::GetSectorSizeInHeightmapUnits() const
{
    int nSectorSize = CTerrain::GetSectorSize() << m_nTreeLevel;
    return nSectorSize / CTerrain::GetHeightMapUnitSize();
}

SProcObjChunk::SProcObjChunk()
{
    m_pInstances = new CVegetation[GetCVars()->e_ProcVegetationMaxObjectsInChunk];
}

SProcObjChunk::~SProcObjChunk()
{
    delete[] m_pInstances;
}


CTerrainNode* CTerrain::FindMinNodeContainingBox(const AABB& someBox)
{
    FUNCTION_PROFILER_3DENGINE;
    if (m_RootNode)
    {
        return m_RootNode->FindMinNodeContainingBox(someBox);
    }
    return nullptr;
}

int CTerrain::FindMinNodesContainingBox(const AABB& someBox, PodArray<CTerrainNode*>& arrNodes)
{
    AABB aabbSeg;
    if (!someBox.IsIntersectBox(aabbSeg))
    {
        return 0;
    }
    AABB aabb = someBox;
    aabb.ClipToBox(aabbSeg);
    CTerrainNode* pn = FindMinNodeContainingBox(aabb);
    assert(pn);
    if (!pn)
    {
        return 0;
    }
    arrNodes.Add(pn);
    return 1;
}