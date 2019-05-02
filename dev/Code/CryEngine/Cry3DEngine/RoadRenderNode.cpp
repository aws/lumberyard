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

#include "3dEngine.h"
#include "RoadRenderNode.h"
#include "CullBuffer.h"
#include "terrain.h"
#include "ObjMan.h"
#include "MatMan.h"
#include "MeshCompiler/MeshCompiler.h"
#include "MathConversion.h"

const float fRoadAreaZRange = 2.5f;
const float fRoadTerrainZOffset = 0.06f;

// tmp buffers used during road mesh creation
PodArray<SVF_P3F_C4B_T2S> CRoadRenderNode::m_lstVerticesMerged;
PodArray<vtx_idx> CRoadRenderNode::m_lstIndicesMerged;
PodArray<SPipTangents> CRoadRenderNode::m_lstTangMerged;
PodArray<Vec3> CRoadRenderNode::m_lstVerts;
PodArray<vtx_idx> CRoadRenderNode::m_lstIndices;
PodArray<vtx_idx> CRoadRenderNode::m_lstClippedIndices;
PodArray<SPipTangents> CRoadRenderNode::m_lstTang;
PodArray<SVF_P3F_C4B_T2S> CRoadRenderNode::m_lstVertices;
CPolygonClipContext CRoadRenderNode::s_tmpClipContext;
ILINE Vec3 max(const Vec3& v0, const Vec3& v1) { return Vec3(max(v0.x, v1.x), max(v0.y, v1.y), max(v0.z, v1.z)); }
ILINE Vec3 min(const Vec3& v0, const Vec3& v1) { return Vec3(min(v0.x, v1.x), min(v0.y, v1.y), min(v0.z, v1.z)); }

CRoadRenderNode::CRoadRenderNode()
{
    m_pRenderMesh = NULL;
    m_pMaterial = NULL;
    m_arrTexCoors[0] = m_arrTexCoorsGlobal[0] = 0;
    m_arrTexCoors[1] = m_arrTexCoorsGlobal[1] = 1;
    m_pPhysEnt = NULL;
    m_sortPrio = 0;
    m_nLayerId = 0;
    m_bIgnoreTerrainHoles = false;
    m_bPhysicalize = false;

    GetInstCount(eERType_Road)++;
}

CRoadRenderNode::~CRoadRenderNode()
{
    Dephysicalize();
    m_pRenderMesh = NULL;
    Get3DEngine()->FreeRenderNodeState(this);

    Get3DEngine()->m_lstRoadRenderNodesForUpdate.Delete(this);

    GetInstCount(eERType_Road)--;
}

void CRoadRenderNode::SetVertices(const AZStd::vector<AZ::Vector3>& pVerts, const AZ::Transform& transform, float fTexCoordBegin, float fTexCoordEnd, float fTexCoordBeginGlobal, float fTexCoordEndGlobal)
{
    PodArray<Vec3> points;
    points.reserve(pVerts.size());
    for (auto azPoint : pVerts)
    {
        points.Add(AZVec3ToLYVec3(transform * azPoint));
    }
    SetVertices(points.GetElements(), pVerts.size(), fTexCoordBegin, fTexCoordEnd, fTexCoordBeginGlobal, fTexCoordEndGlobal);
}

void CRoadRenderNode::SetVertices(const Vec3* pVertsAll, int nVertsNumAll,
    float fTexCoordBegin, float fTexCoordEnd,
    float fTexCoordBeginGlobal, float fTexCoordEndGlobal)
{
    if (pVertsAll != m_arrVerts.GetElements())
    {
        m_arrVerts.Clear();
        m_arrVerts.AddList((Vec3*)pVertsAll, nVertsNumAll);

        // work around for cracks between road segments
        if (m_addOverlapBetweenSectors)
        {
            if (m_arrVerts.Count() >= 4)
            {
                if (fTexCoordBegin != fTexCoordBeginGlobal)
                {
                    Vec3 m0 = (m_arrVerts[0] + m_arrVerts[1]) * .5f;
                    Vec3 m1 = (m_arrVerts[2] + m_arrVerts[3]) * .5f;
                    Vec3 vDir = (m0 - m1).GetNormalized() * 0.01f;
                    m_arrVerts[0] += vDir;
                    m_arrVerts[1] += vDir;
                }

                if (fTexCoordEnd != fTexCoordEndGlobal)
                {
                    int n = m_arrVerts.Count();
                    Vec3 m0 = (m_arrVerts[n - 1] + m_arrVerts[n - 2]) * .5f;
                    Vec3 m1 = (m_arrVerts[n - 3] + m_arrVerts[n - 4]) * .5f;
                    Vec3 vDir = (m0 - m1).GetNormalized() * 0.01f;
                    m_arrVerts[n - 1] += vDir;
                    m_arrVerts[n - 2] += vDir;
                }
            }
        }
    }

    // Adjust uv coords to smaller range to avoid f16 precision errors.
    // Need to adjust global ranges too to keep relative fades at ends of spline
    const float fNodeStart = static_cast<float>(static_cast<int>(fTexCoordBegin));
    const float fNodeOffset = fTexCoordBegin - fNodeStart;

    m_arrTexCoors[0] = fNodeOffset;
    m_arrTexCoors[1] = fNodeOffset + (fTexCoordEnd - fTexCoordBegin);

    m_arrTexCoorsGlobal[0] = fTexCoordBeginGlobal - fNodeStart;
    m_arrTexCoorsGlobal[1] = fTexCoordBeginGlobal + (fTexCoordEndGlobal - fTexCoordBeginGlobal) - fNodeStart;

    m_WSBBox.Reset();
    for (int i = 0; i < nVertsNumAll; i++)
    {
        m_WSBBox.Add(m_arrVerts[i]);
    }

    m_WSBBox.min -= Vec3(0.1f, 0.1f, fRoadAreaZRange);
    m_WSBBox.max += Vec3(0.1f, 0.1f, fRoadAreaZRange);

    ScheduleRebuild();
}

void CRoadRenderNode::Compile() PREFAST_SUPPRESS_WARNING(6262) //function uses > 32k stack space
{
    LOADING_TIME_PROFILE_SECTION;

    int nVertsNumAll = m_arrVerts.Count();

    assert(!(nVertsNumAll & 1));

    if (nVertsNumAll < 4)
    {
        return;
    }

    // free old object and mesh
    m_pRenderMesh = NULL;

    Plane arrPlanes[6];
    float arrTexCoors[2];

    IGeomManager* pGeoman = GetPhysicalWorld()->GetGeomManager();
    primitives::box abox;
    pe_geomparams gp;
    int matid = 0;
    abox.center.zero();
    abox.Basis.SetIdentity();
    gp.flags = geom_mat_substitutor;
    Dephysicalize();

    if (m_bPhysicalize)
    {
        pe_params_flags pf;
        pf.flagsAND = ~pef_traceable;
        pf.flagsOR = pef_parts_traceable;
        m_pPhysEnt = GetPhysicalWorld()->CreatePhysicalEntity(PE_STATIC, &pf);

        if (m_pMaterial)
        {
            ISurfaceType* psf;
            if ((psf = m_pMaterial->GetSurfaceType()) || m_pMaterial->GetSubMtl(0) && (psf = m_pMaterial->GetSubMtl(0)->GetSurfaceType()))
            {
                matid = psf->GetId();
            }
        }
    }

    // update object bbox
    {
        m_WSBBox.Reset();
        for (int i = 0; i < nVertsNumAll; i++)
        {
            Vec3 vTmp(m_arrVerts[i].x, m_arrVerts[i].y, Get3DEngine()->GetTerrainElevation(m_arrVerts[i].x, m_arrVerts[i].y, m_nSID) + fRoadTerrainZOffset);
            m_WSBBox.Add(vTmp);
        }

        // prepare arrays for final mesh
        const int nMaxVerticesToMerge = 1024 * 32; // limit memory usage
        m_lstVerticesMerged.PreAllocate(nMaxVerticesToMerge, 0);
        m_lstIndicesMerged.PreAllocate(nMaxVerticesToMerge * 6, 0);
        m_lstTangMerged.PreAllocate(nMaxVerticesToMerge, 0);

        float fChunksNum = (float)((nVertsNumAll - 2) / 2);
        float fTexStep = (m_arrTexCoors[1] - m_arrTexCoors[0]) / fChunksNum;

        // for every trapezoid
        for (int nVertId = 0; nVertId <= nVertsNumAll - 4; nVertId += 2)
        {
            const Vec3* pVerts = &m_arrVerts[nVertId];

            if (pVerts[0] == pVerts[1] ||
                pVerts[1] == pVerts[2] ||
                pVerts[2] == pVerts[3] ||
                pVerts[3] == pVerts[0])
            {
                continue;
            }

            // get texture coordinates range
            arrTexCoors[0] = m_arrTexCoors[0] + fTexStep * (nVertId / 2);
            arrTexCoors[1] = m_arrTexCoors[0] + fTexStep * (nVertId / 2 + 1);

            GetClipPlanes(&arrPlanes[0], 4, nVertId);

            // make trapezoid 2d bbox
            AABB WSBBox;
            WSBBox.Reset();
            for (int i = 0; i < 4; i++)
            {
                Vec3 vTmp(pVerts[i].x, pVerts[i].y, pVerts[i].z);
                WSBBox.Add(vTmp);
            }

            // Ignore any trapezoids that are outside the terrain boundary.  Roads rely on terrain height to work.
            float terrainSize = (float)CTerrain::GetTerrainSize();
            if ((WSBBox.min.x > terrainSize) || (WSBBox.min.y > terrainSize) ||
                (WSBBox.max.x < 0.0f) || (WSBBox.max.y < 0.0f))
            {
                continue;
            }

            // The trapezoid overlaps the terrain, so clamp the remaining bounding box to the terrain size.
            WSBBox.min.x = AZStd::max(WSBBox.min.x, 0.0f);
            WSBBox.min.y = AZStd::max(WSBBox.min.y, 0.0f);
            WSBBox.max.x = AZStd::min(WSBBox.max.x, terrainSize);
            WSBBox.max.y = AZStd::min(WSBBox.max.y, terrainSize);

            // make vert array
            int nUnitSize = GetTerrain()->GetHeightMapUnitSize();
            int x1 = int(WSBBox.min.x) / nUnitSize * nUnitSize;
            int x2 = int(WSBBox.max.x) / nUnitSize * nUnitSize + nUnitSize;
            int y1 = int(WSBBox.min.y) / nUnitSize * nUnitSize;
            int y2 = int(WSBBox.max.y) / nUnitSize * nUnitSize + nUnitSize;

            // make arrays of verts and indices used in trapezoid area
            m_lstVerts.Clear();
            m_lstIndices.Clear();
            m_lstClippedIndices.Clear();

            // Reserve room for the vertices that we're about to add.
            m_lstVerts.PreAllocate(((x2 - x1 + nUnitSize) / nUnitSize) * ((y2 - y1 + nUnitSize) / nUnitSize));

            for (int x = x1; x <= x2; x += nUnitSize)
            {
                for (int y = y1; y <= y2; y += nUnitSize)
                {
                    Vec3 vTmp((float)x, (float)y, GetTerrain()->GetZ(x, y));
                    m_lstVerts.Add(vTmp);
                }
            }
            // make indices
            int dx = (x2 - x1) / nUnitSize;
            int dy = (y2 - y1) / nUnitSize;

            // Reserve room for the indices we're about to add.
            m_lstIndices.PreAllocate(dx * dy * 6);

            for (int x = 0; x < dx; x++)
            {
                for (int y = 0; y < dy; y++)
                {
                    int nIdx0 = (x * (dy + 1) + y);
                    int nIdx1 = (x * (dy + 1) + y + (dy + 1));
                    int nIdx2 = (x * (dy + 1) + y + 1);
                    int nIdx3 = (x * (dy + 1) + y + 1 + (dy + 1));

                    assert(nIdx3 < m_lstVerts.Count());

                    int X_in_meters = x1 + x * nUnitSize;
                    int Y_in_meters = y1 + y * nUnitSize;

                    CTerrain* pTerrain = GetTerrain();

                    if (m_bIgnoreTerrainHoles || (pTerrain && !pTerrain->IsHole(X_in_meters, Y_in_meters)))
                    {
                        if (pTerrain && pTerrain->IsMeshQuadFlipped(X_in_meters, Y_in_meters, nUnitSize))
                        {
                            m_lstIndices.Add(nIdx0);
                            m_lstIndices.Add(nIdx1);
                            m_lstIndices.Add(nIdx3);

                            m_lstIndices.Add(nIdx0);
                            m_lstIndices.Add(nIdx3);
                            m_lstIndices.Add(nIdx2);
                        }
                        else
                        {
                            m_lstIndices.Add(nIdx0);
                            m_lstIndices.Add(nIdx1);
                            m_lstIndices.Add(nIdx2);

                            m_lstIndices.Add(nIdx1);
                            m_lstIndices.Add(nIdx3);
                            m_lstIndices.Add(nIdx2);
                        }
                    }
                }
            }

            // Reserve room for the clipped indices we're about to add.
            m_lstClippedIndices.PreAllocate(m_lstIndices.Count());

            // clip triangles, saving off the indices of the new clipped triangles in a separate array for speed, at the cost of memory.
            int indexCount = m_lstIndices.Count();
            for (int i = 0; i < indexCount; i += 3)
            {
                ClipTriangle(m_lstVerts, m_lstIndices, m_lstClippedIndices, i, arrPlanes);
            }

            if (m_lstClippedIndices.Count() < 3 || m_lstVerts.Count() < 3)
            {
                continue;
            }

            if (m_pPhysEnt)
            {
                Vec3 axis = (pVerts[2] + pVerts[3] - pVerts[0] - pVerts[1]).normalized(), n = (pVerts[1] - pVerts[0] ^ pVerts[2] - pVerts[0]) + (pVerts[2] - pVerts[3] ^ pVerts[2] - pVerts[3]);
                (n -= axis * (n * axis)).normalize();
                gp.q = Quat(Matrix33::CreateFromVectors(axis, n ^ axis, n));
                Vec3 BBox[] = { Vec3(VMAX), Vec3(VMIN) };
                for (int j = 0; j < m_lstClippedIndices.Count(); j++)
                {
                    Vec3 ptloc = m_lstVerts[m_lstClippedIndices[j]] * gp.q;
                    BBox[0] = min(BBox[0], ptloc);
                    BBox[1] = max(BBox[1], ptloc);
                }
                gp.pos = gp.q * (BBox[1] + BBox[0]) * 0.5f;
                (abox.size = (BBox[1] - BBox[0]) * 0.5f).z += fRoadTerrainZOffset * 2;
                phys_geometry* physGeom = pGeoman->RegisterGeometry(pGeoman->CreatePrimitive(primitives::box::type, &abox), matid);
                physGeom->pGeom->Release();
                m_pPhysEnt->AddGeometry(physGeom, &gp);
                pGeoman->UnregisterGeometry(physGeom);
            }

            // allocate tangent array
            m_lstTang.Clear();
            m_lstTang.PreAllocate(m_lstVerts.Count(), m_lstVerts.Count());

            int nStep = CTerrain::GetHeightMapUnitSize();

            Vec3 vWSBoxCenter = m_WSBBox.GetCenter(); //vWSBoxCenter.z=0;

            // make real vertex data
            m_lstVertices.Clear();
            m_lstVertices.PreAllocate(m_lstVerts.Count());
            for (int i = 0; i < m_lstVerts.Count(); i++)
            {
                SVF_P3F_C4B_T2S tmp;

                Vec3 vWSPos = m_lstVerts[i];

                tmp.xyz = (m_lstVerts[i] - vWSBoxCenter);

                // do texgen
                float d0 = arrPlanes[0].DistFromPlane(vWSPos);
                float d1 = arrPlanes[1].DistFromPlane(vWSPos);
                float d2 = arrPlanes[2].DistFromPlane(vWSPos);
                float d3 = arrPlanes[3].DistFromPlane(vWSPos);

                float t = fabsf(d0 + d1) < FLT_EPSILON ? 0.0f : d0 / (d0 + d1);
                tmp.st = Vec2f16((1 - t) * fabs(arrTexCoors[0]) + t * fabs(arrTexCoors[1]), fabsf(d2 + d3) < FLT_EPSILON ? 0.0f : d2 / (d2 + d3));

                // calculate alpha value
                float fAlpha = 1.f;
                if (m_bAlphaBlendRoadEnds)
                {
                    if (fabs(arrTexCoors[0] - m_arrTexCoorsGlobal[0]) < 0.01f)
                    {
                        fAlpha = CLAMP(t, 0, 1.f);
                    }
                    else if (fabs(arrTexCoors[1] - m_arrTexCoorsGlobal[1]) < 0.01f)
                    {
                        fAlpha = CLAMP(1.f - t, 0, 1.f);
                    }
                }

                tmp.color.bcolor[0] = 255;
                tmp.color.bcolor[1] = 255;
                tmp.color.bcolor[2] = 255;
                tmp.color.bcolor[3] = uint8(255.f * fAlpha);
                SwapEndian(tmp.color.dcolor, eLittleEndian);

                m_lstVertices.Add(tmp);

                Vec3 vTang   = pVerts[2] - pVerts[0];
                Vec3 vBitang = pVerts[1] - pVerts[0];
                Vec3 vNormal = GetTerrain()->GetTerrainSurfaceNormal(vWSPos, 0.25f);

                // Orthogonalize Tangent Frame
                vBitang = -vNormal.Cross(vTang);
                vBitang.Normalize();
                vTang = vNormal.Cross(vBitang);
                vTang.Normalize();

                m_lstTang[i] = SPipTangents(vTang, vBitang, -1);
            }

            // shift indices
            for (int i = 0; i < m_lstClippedIndices.Count(); i++)
            {
                m_lstClippedIndices[i] += m_lstVerticesMerged.Count();
            }

            if (m_lstVerticesMerged.Count() + m_lstVertices.Count() > nMaxVerticesToMerge)
            {
                Warning("Road object is too big, has to be split into several smaller parts (pos=%d,%d,%d)", (int)m_WSBBox.GetCenter().x, (int)m_WSBBox.GetCenter().y, (int)m_WSBBox.GetCenter().z);
                Get3DEngine()->UnRegisterEntityAsJob(this);
                return;
            }

            m_lstIndicesMerged.AddList(m_lstClippedIndices);
            m_lstVerticesMerged.AddList(m_lstVertices);
            m_lstTangMerged.AddList(m_lstTang);
        }

        PodArray<SPipNormal> listNormalsDummy;

        mesh_compiler::CMeshCompiler meshCompiler;
        meshCompiler.WeldPos_VF_P3X(m_lstVerticesMerged, m_lstTangMerged, listNormalsDummy, m_lstIndicesMerged, VEC_EPSILON, GetBBox());

        // make render mesh
        if (m_lstIndicesMerged.Count())
        {
            m_pRenderMesh = GetRenderer()->CreateRenderMeshInitialized(
                    m_lstVerticesMerged.GetElements(), m_lstVerticesMerged.Count(), eVF_P3F_C4B_T2S,
                    m_lstIndicesMerged.GetElements(), m_lstIndicesMerged.Count(), prtTriangleList,
                    "RoadRenderNode", GetName(), eRMT_Static, 1, 0, NULL, NULL, false, true, m_lstTangMerged.GetElements());

            float texelAreaDensity = 1.0f;
            {
                const size_t indexCount = m_lstIndicesMerged.size();
                const size_t vertexCount = m_lstVerticesMerged.size();

                if ((indexCount > 0) && (vertexCount > 0))
                {
                    float posArea;
                    float texArea;
                    const char* errorText = "";

                    const bool ok = CMeshHelpers::ComputeTexMappingAreas(
                            indexCount, &m_lstIndicesMerged[0],
                            vertexCount,
                            &m_lstVerticesMerged[0].xyz, sizeof(m_lstVerticesMerged[0]),
                            &m_lstVerticesMerged[0].st, sizeof(m_lstVerticesMerged[0]),
                            posArea, texArea, errorText);

                    if (ok)
                    {
                        texelAreaDensity = texArea / posArea;
                    }
                    else
                    {
                        gEnv->pLog->LogError("Failed to compute texture mapping density for mesh '%s': %s", GetName(), errorText);
                    }
                }
            }

            m_pRenderMesh->SetChunk((m_pMaterial != NULL) ? m_pMaterial : GetMatMan()->GetDefaultMaterial(),
                0, m_lstVerticesMerged.Count(), 0, m_lstIndicesMerged.Count(), texelAreaDensity, eVF_P3F_C4B_T2S);
            Vec3 vWSBoxCenter = m_WSBBox.GetCenter(); //vWSBoxCenter.z=0;
            AABB OSBBox(m_WSBBox.min - vWSBoxCenter, m_WSBBox.max - vWSBoxCenter);
            m_pRenderMesh->SetBBox(OSBBox.min, OSBBox.max);
        }
    }

    // activate rendering
    Get3DEngine()->RegisterEntity(this);
}

void CRoadRenderNode::SetSortPriority(uint8 sortPrio)
{
    m_sortPrio = sortPrio;
}

void CRoadRenderNode::SetIgnoreTerrainHoles(bool bVal)
{
    if (bVal != m_bIgnoreTerrainHoles)
    {
        m_bIgnoreTerrainHoles = bVal;
        ScheduleRebuild();
    }
}

void CRoadRenderNode::Render(const SRendParams& _RendParams, const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE;

    if (!passInfo.RenderRoads())
    {
        return;
    }

    if (GetCVars()->e_Roads == 2 && _RendParams.fDistance < 2.f)
    {
        ScheduleRebuild();
    }

    SRendParams RendParams(_RendParams);

    CRenderObject* pObj = GetIdentityCRenderObject(passInfo.ThreadID());
    if (!pObj)
    {
        return;
    }

    pObj->m_ObjFlags |= RendParams.dwFObjFlags;
    pObj->m_II.m_AmbColor = RendParams.AmbientColor;
    Vec3 vWSBoxCenter = m_WSBBox.GetCenter();
    vWSBoxCenter.z += 0.01f;
    pObj->m_II.m_Matrix.SetTranslation(vWSBoxCenter);

    RendParams.nRenderList = EFSLIST_DECAL;

    pObj->m_nSort = m_sortPrio;

    pObj->m_ObjFlags |= (FOB_DECAL | FOB_PARTICLE_SHADOWS | FOB_NO_FOG);

    if (m_pRenderMesh)
    {
        m_pRenderMesh->Render(RendParams, pObj,
            (m_pMaterial != NULL) ? m_pMaterial : GetMatMan()->GetDefaultMaterial(), passInfo);
    }
}

void CRoadRenderNode::ClipTriangle(PodArray<Vec3>& lstVerts, PodArray<vtx_idx>& lstInds, PodArray<vtx_idx>& lstClippedInds, int nStartIdxId, Plane* pPlanes)
{
    const PodArray<Vec3>& clipped = s_tmpClipContext.Clip(
            lstVerts[lstInds[nStartIdxId + 0]],
            lstVerts[lstInds[nStartIdxId + 1]],
            lstVerts[lstInds[nStartIdxId + 2]],
            pPlanes, 4);

    if (clipped.Count() < 3)
    {
        return; // entire triangle is clipped away
    }

    if (clipped.Count() == 3)
    {
        if (clipped[0].IsEquivalent(lstVerts[lstInds[nStartIdxId + 0]]))
        {
            if (clipped[1].IsEquivalent(lstVerts[lstInds[nStartIdxId + 1]]))
            {
                if (clipped[2].IsEquivalent(lstVerts[lstInds[nStartIdxId + 2]]))
                {
                    // The original triangle remains as-is, so add it to the clipped index list.
                    lstClippedInds.Add(nStartIdxId + 0);
                    lstClippedInds.Add(nStartIdxId + 1);
                    lstClippedInds.Add(nStartIdxId + 2);
                    return;
                }
            }
        }
    }
    // replace old triangle with several new triangles
    int nStartId = lstVerts.Count();
    lstVerts.AddList(clipped);

    // Add all the new triangle indices to our clipped index list
    for (int i = 0; i < clipped.Count() - 2; i++)
    {
        lstClippedInds.Add(nStartId + 0);
        lstClippedInds.Add(nStartId + i + 1);
        lstClippedInds.Add(nStartId + i + 2);
    }

    return;
}

void CRoadRenderNode::SetMaterial(_smart_ptr<IMaterial> pMat)
{
    m_pMaterial = pMat;
}

void CRoadRenderNode::Dephysicalize(bool bKeepIfReferenced)
{
    if (m_pPhysEnt)
    {
        GetPhysicalWorld()->DestroyPhysicalEntity(m_pPhysEnt);
    }
    m_pPhysEnt = NULL;
}

void CRoadRenderNode::GetMemoryUsage(ICrySizer* pSizer) const
{
    SIZER_COMPONENT_NAME(pSizer, "Road");
    pSizer->AddObject(this, sizeof(*this));
    pSizer->AddObject(m_arrVerts);
}

void CRoadRenderNode::ScheduleRebuild()
{
    if (Get3DEngine()->m_lstRoadRenderNodesForUpdate.Find(this) < 0)
    {
        Get3DEngine()->m_lstRoadRenderNodesForUpdate.Add(this);
    }
}

void CRoadRenderNode::OnTerrainChanged()
{
    if (!m_pRenderMesh)
    {
        return;
    }

    int nPosStride = 0;

    IRenderMesh::ThreadAccessLock lock(m_pRenderMesh);

    byte* pPos = m_pRenderMesh->GetPosPtr(nPosStride, FSL_SYSTEM_UPDATE);

    Vec3 vWSBoxCenter = m_WSBBox.GetCenter(); //vWSBoxCenter.z=0;

    for (int i = 0, nVertsNum = m_pRenderMesh->GetVerticesCount(); i < nVertsNum; i++)
    {
        Vec3& vPos = *(Vec3*)&pPos[i * nPosStride];
        vPos.z = GetTerrain()->GetBilinearZ(vWSBoxCenter.x + vPos.x, vWSBoxCenter.y + vPos.y) + 0.01f - vWSBoxCenter.z;
    }
    m_pRenderMesh->UnlockStream(VSF_GENERAL);
    ScheduleRebuild();
}

void CRoadRenderNode::OnRenderNodeBecomeVisible(const SRenderingPassInfo& passInfo)
{
    assert(m_pRNTmpData);
    m_pRNTmpData->userData.objMat.SetIdentity();
}

void CRoadRenderNode::GetTexCoordInfo(float* pTexCoordInfo)
{
    pTexCoordInfo[0] = m_arrTexCoors[0];
    pTexCoordInfo[1] = m_arrTexCoors[1];
    pTexCoordInfo[2] = m_arrTexCoorsGlobal[0];
    pTexCoordInfo[3] = m_arrTexCoorsGlobal[1];
}

void CRoadRenderNode::GetClipPlanes(Plane* pPlanes, int nPlanesNum, int nVertId)
{
    const Vec3* pVerts = &m_arrVerts[nVertId];

    if (
        pVerts[0] == pVerts[1] ||
        pVerts[1] == pVerts[2] ||
        pVerts[2] == pVerts[3] ||
        pVerts[3] == pVerts[0])
    {
        return;
    }

    assert(nPlanesNum == 4 || nPlanesNum == 6);

    // define 6 clip planes
    pPlanes[0].SetPlane(pVerts[0], pVerts[1], pVerts[1] + Vec3(0, 0, 1));
    pPlanes[1].SetPlane(pVerts[2], pVerts[3], pVerts[3] + Vec3(0, 0, -1));
    pPlanes[2].SetPlane(pVerts[0], pVerts[2], pVerts[2] + Vec3(0, 0, -1));
    pPlanes[3].SetPlane(pVerts[1], pVerts[3], pVerts[3] + Vec3(0, 0, 1));

    if (nPlanesNum == 6)
    {
        Vec3 vHeight(0, 0, fRoadAreaZRange);
        pPlanes[4].SetPlane(pVerts[0] - vHeight, pVerts[1] - vHeight, pVerts[2] - vHeight);
        pPlanes[5].SetPlane(pVerts[1] + vHeight, pVerts[0] + vHeight, pVerts[2] + vHeight);
    }
}

void CRoadRenderNode::OffsetPosition(const Vec3& delta)
{
    if (m_pRNTmpData)
    {
        m_pRNTmpData->OffsetPosition(delta);
    }
    m_WSBBox.Move(delta);
}
