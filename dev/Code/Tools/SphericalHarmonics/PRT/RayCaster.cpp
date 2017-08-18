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

#include "stdafx.h"

#if defined(OFFLINE_COMPUTATION)

#include "RayCaster.h"
#include "RasterCube.h"
#include <PRT/SimpleIndexedMesh.h>
#include <PRT/ISHMaterial.h>
#include <PRT/SHMath.h>
#include <PRT/SHRotate.h>
#include <limits>

//all implementations taken from Lightmap compiler
const NSH::EReturnSinkValue NSH::CRayCaster::CAnyHit::ReturnElement(const RasterCubeUserT& crInObject, float& rInoutfRayMaxDist)
{
    float u, v, t;

    if (!InsertAlreadyTested(&crInObject))
    {
        Vec3 outputPos;
        if (NSH::NRayTriangleIntersect::RayTriangleIntersectTest(m_RayOrigin, m_RayDir, crInObject.vertices[0], crInObject.vertices[1], crInObject.vertices[2], outputPos, t, u, v, crInObject.isSingleSided))
        {
            if (t < m_fClosest)
            {
                // do not take intersections into account which lay in the cube of the texel
                if (t >= m_Bias && t < m_RayLen)
                {
                    // Make sure our intersection is not on the plane of the triangle, which won't be fixed by
                    // the small texel cube and can lead to artifacts for lightsources which hover closer above
                    // a triangle. Just fixes 50% of all cases
                    float fDist = outputPos * m_PNormal + m_D;
                    // we need get shadow for near lying geometry.
                    const float cfThreshold = 0.00001f;
                    if (fabs(fDist) > cfThreshold)
                    {
                        // Make sure the ray is not too parallel to the plane, fixed the other 50% of the error cases
                        float fDot = crInObject.vNormal * m_RayDir;
                        const float cfBias = 0.00001f;
                        if (fabs(fDot) > cfBias)
                        {
                            m_fClosest = t;
                            m_HitData.pHitResult = &crInObject;
                            m_HitData.baryCoord.y = u;
                            m_HitData.baryCoord.z = v;
                            m_HitData.baryCoord.x = std::max(0.f, 1.f - (u + v));
                            m_HitData.dist = t;
                            rInoutfRayMaxDist = t;          // don't go further than this (optimizable)
                            return crInObject.fastProcessing ? NSH::RETURN_SINK_HIT_AND_EARLY_OUT : NSH::RETURN_SINK_HIT;
                        }
                    }
                }
            }
        }
    }
    return NSH::RETURN_SINK_FAIL;
}

const NSH::EReturnSinkValue NSH::CRayCaster::CAllHits::ReturnElement(const RasterCubeUserT& crInObject, float& rInoutfRayMaxDist)
{
    float u, v, t;
    if (!InsertAlreadyTested(&crInObject))
    {
        Vec3 outputPos;
        if (NSH::NRayTriangleIntersect::RayTriangleIntersectTest(m_RayOrigin, m_RayDir, crInObject.vertices[0], crInObject.vertices[1], crInObject.vertices[2], outputPos, t, u, v, crInObject.isSingleSided))
        {
            // do not take intersections into account which lay in the cube of the texel
            if (t >= m_Bias && t < m_RayLen)
            {
                // Make sure our intersection is not on the plane of the triangle, which won't be fixed by
                // the small texel cube and can lead to artifacts for lightsources which hover closer above
                // a triangle. Just fixes 50% of all cases
                float fDist = outputPos * m_PNormal + m_D;
                // we need get shadow for near lying geometry.
                const float cfThreshold = 0.00001f;
                if (fabs(fDist) > cfThreshold)
                {
                    // Make sure the ray is not too parallel to the plane, fixed the other 50% of the error cases
                    float fDot = crInObject.vNormal * m_RayDir;
                    const float cfBias = 0.00001f;
                    if (fabs(fDot) > cfBias)
                    {
                        m_HitData.pHitResult = &crInObject;
                        m_HitData.baryCoord.y = u;
                        m_HitData.baryCoord.z = v;
                        m_HitData.baryCoord.x = std::max(0.f, 1.f - (u + v));
                        m_HitData.dist = t;
                        m_Hits.push_back(m_HitData);
                        rInoutfRayMaxDist = t;          // don't go further than this (optimizable)
                        return crInObject.fastProcessing ? NSH::RETURN_SINK_HIT_AND_EARLY_OUT : NSH::RETURN_SINK_HIT;
                    }
                }
            }
        }
    }
    return NSH::RETURN_SINK_FAIL;
}

NSH::CRayCaster::CRayCaster()
    : m_IsClone(0)
    , m_CloneRefCount(0)
    , m_UseFullVisCache(false)
    , m_OnlyUpperHemisphere(true)
    , m_FullVisQueries(0)
    , m_FullVisSuccesses(0)
{
    m_pRasterCube = new TRasterCubeImpl;
    m_AllHits.m_Hits.reserve(50);//reserve for 50 hits, it is unique, so don't save it here
    m_HitData.reserve(50);//reserve for 50 hits, it is unique, so don't save it here
    assert(m_pRasterCube);
}

NSH::CRayCaster::~CRayCaster()
{
    if (m_IsClone)
    {
        m_pOriginal->DecrementCloneRefCount();
    }
    else  //is the original
    if (m_CloneRefCount == 0)
    {
        delete m_pRasterCube;
        m_pRasterCube = NULL;
    }
}

void NSH::CRayCaster::LogMeshStats()
{
    GetSHLog().Log("	average full visibility: %.2f percent\n", (m_FullVisQueries > 0) ? ((float)m_FullVisSuccesses / (float)m_FullVisQueries) * 100.f : 0);
}

const uint32 NSH::CRayCaster::GetFullVisQueries() const
{
    return m_FullVisQueries;
}

const uint32 NSH::CRayCaster::GetFullVisSuccesses() const
{
    return m_FullVisSuccesses;
}
void NSH::CRayCaster::ResetMeshStats()
{
    m_FullVisQueries = m_FullVisSuccesses = 0;
}

NSH::CSmartPtr<NSH::CRayCaster, CSHAllocator<> > NSH::CRayCaster::CreateClone() const
{
    NSH::CSmartPtr<CRayCaster, CSHAllocator<> > pClone(new CRayCaster);
    assert(pClone);

    pClone->m_Parameters = m_Parameters;
    pClone->m_pRasterCube = m_pRasterCube;
    pClone->m_pOriginal = (CRayCaster*)this;

    IncrementCloneRefCount();
    return pClone;
}

void NSH::CRayCaster::DecrementCloneRefCount() const
{
    assert(m_CloneRefCount > 0);
    if (!m_IsClone)
    {
        m_CloneRefCount--;
    }
}

void NSH::CRayCaster::IncrementCloneRefCount() const
{
    if (!m_IsClone)
    {
        m_CloneRefCount++;
    }
}

void NSH::CRayCaster::ResetCache
(
    const TVec& crFrom,
    const float cRayLen,
    const TVec& cNormal,
    const float cRayTracingBias,
    const bool cOnlyUpperHemisphere,
    const bool cUseFullVisCache
)
{
    m_RayCache.Reset();
    m_UseFullVisCache = cUseFullVisCache;
    m_OnlyUpperHemisphere = cOnlyUpperHemisphere;
    m_Source = crFrom;
    m_Normal = cNormal;
    m_RayLen = cRayLen;
    m_RayTracingBias = cRayTracingBias;
    if (cUseFullVisCache)
    {
        m_FullVisCache.GenerateFullVisInfo(*m_pRasterCube, crFrom, cRayLen, cNormal, cRayTracingBias, cOnlyUpperHemisphere);
    }
}

const bool NSH::CRayCaster::CastRay(NSH::SRayResult& rResult, const TVec& crFrom, const TVec& crDir, const double cRayLen, const TVec& crSourcePlaneNormal, const float cBias) const
{
    if (m_UseFullVisCache && m_FullVisCache.IsFullyVisible(crDir))
    {
        rResult.faceID = -1;
        rResult.pMesh = NULL;
        rResult.baryCoord.x = rResult.baryCoord.y = rResult.baryCoord.z = 0.f;
        m_FullVisQueries++;
        m_FullVisSuccesses++;
        return false;
    }
    if (m_UseFullVisCache)
    {
        m_FullVisQueries++;
    }
    assert(fabs(crDir.len2() - 1.f) < 0.05f);
    m_AnyHits.SetupRay(crDir, crFrom, (float)cRayLen, (float)-(crSourcePlaneNormal * crFrom), crSourcePlaneNormal, cBias);
    bool bUsedCache = true;
    if (!m_RayCache.CheckCache<CAnyHit>(m_AnyHits))//if cache was useless
    {
        m_pRasterCube->GatherRayHitsDirection<CAnyHit, false>(TVector3D(crFrom.x, crFrom.y, crFrom.z), TVector3D(crDir.x, crDir.y, crDir.z), m_AnyHits);
        bUsedCache = false;
        m_RayCache.Reset();
    }
    if (!m_AnyHits.IsIntersecting())
    {
        rResult.faceID = -1;
        rResult.pMesh = NULL;
        rResult.baryCoord.x = rResult.baryCoord.y = rResult.baryCoord.z = 0.f;
        return false;
    }
    else
    {
        const SHitData& crHitData = m_AnyHits.GetHitData();
        //record hits
        if (!bUsedCache)
        {
            m_RayCache.RecordHit(crHitData.pHitResult);
        }
        rResult.faceID      = crHitData.pHitResult->faceID;
        rResult.pMesh           = crHitData.pHitResult->pMesh;
        rResult.baryCoord = crHitData.baryCoord;
        return true;
    }
}

const bool NSH::CRayCaster::CastRay(TRayResultVec& rResults, const TVec& crFrom, const TVec& crDir, const double cRayLen, const TVec& crSourcePlaneNormal, const float cBias) const
{
    if (m_UseFullVisCache && m_FullVisCache.IsFullyVisible(crDir))
    {
        m_FullVisQueries++;
        m_FullVisSuccesses++;
        rResults.resize(0);
        return false;
    }
    if (m_UseFullVisCache)
    {
        m_FullVisQueries++;
    }
    assert(IsNormalized(crDir));
    m_AllHits.SetupRay(crDir, crFrom, (float)cRayLen, (float)-(crSourcePlaneNormal * crFrom), crSourcePlaneNormal, cBias);
    bool bUsedCache = true;
    if (!m_RayCache.CheckCache<CAllHits>(m_AllHits))//if cache was useless
    {
        m_pRasterCube->GatherRayHitsDirection<CAllHits, false>(TVector3D(crFrom.x, crFrom.y, crFrom.z), TVector3D(crDir.x, crDir.y, crDir.z), m_AllHits);
        bUsedCache = false;
        m_RayCache.Reset();
    }
    m_AllHits.GetHitData(m_HitData);
    rResults.resize(m_HitData.size());
    int i = 0;
    const THitVec::const_iterator cEnd = m_HitData.end();
    for (THitVec::const_iterator iter = m_HitData.begin(); iter != cEnd; ++iter)
    {
        const SHitData& crHitData = *iter;
        NSH::SRayResult result;
        if (!bUsedCache)
        {
            m_RayCache.RecordHit(crHitData.pHitResult);
        }
        result.faceID           = crHitData.pHitResult->faceID;
        result.pMesh            = crHitData.pHitResult->pMesh;
        result.baryCoord    = crHitData.baryCoord;
        rResults[i++] = result;
    }
    return !rResults.empty();
}

const bool NSH::CRayCaster::SetupGeometry(const TGeomVec& crGeometries, const NTransfer::STransferParameters& crParameters)
{
    assert(!m_IsClone);
    m_Parameters = crParameters;
    typedef TGeomVec::const_iterator TGeomIter;
    //first count triangles and compute boundaries
    uint32 triCount = 0;
    Vec3 minEx(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec3 maxEx(std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min());

    TRasterCubeImpl&    rasterCube = *m_pRasterCube;
    const TGeomIter cEnd = crGeometries.end();
    for (TGeomIter iter = crGeometries.begin(); iter != cEnd; ++iter)
    {
        const CSimpleIndexedMesh* pMesh = *iter;
        //update triangle count
        triCount += pMesh->GetFaceCount();
        //update extensions
        if (minEx.x > pMesh->GetMinExt().x)
        {
            minEx.x = pMesh->GetMinExt().x;
        }
        if (minEx.y > pMesh->GetMinExt().y)
        {
            minEx.y = pMesh->GetMinExt().y;
        }
        if (minEx.z > pMesh->GetMinExt().z)
        {
            minEx.z = pMesh->GetMinExt().z;
        }
        if (maxEx.x < pMesh->GetMaxExt().x)
        {
            maxEx.x = pMesh->GetMaxExt().x;
        }
        if (maxEx.y < pMesh->GetMaxExt().y)
        {
            maxEx.y = pMesh->GetMaxExt().y;
        }
        if (maxEx.z < pMesh->GetMaxExt().z)
        {
            maxEx.z = pMesh->GetMaxExt().z;
        }
        //make sure it has some extension in all dimensions
    }
    const float s_cMinExt = 0.001f;
    maxEx.x = std::max(s_cMinExt, maxEx.x);
    maxEx.y = std::max(s_cMinExt, maxEx.y);
    maxEx.z = std::max(s_cMinExt, maxEx.z);
    //now deinit and init raster cube
    rasterCube.DeInit();
    //now insert all triangles
    RasterCubeUserT* pCache = new RasterCubeUserT[triCount];
    assert(pCache);
    size_t cacheCounter = 0;
    //compute all triangle information
    for (TGeomIter iter = crGeometries.begin(); iter != cEnd; ++iter)
    {
        const CSimpleIndexedMesh* cpMesh = *iter;
        for (uint32 i = 0; i < cpMesh->GetFaceCount(); ++i)
        {
            if (!cpMesh->ConsiderForRayCastingByFaceID(i))
            {
                continue;
            }
            const NSH::NMaterial::ISHMaterial& crMat = cpMesh->GetMaterialByFaceID(i);
            RasterCubeUserT& rElem  = pCache[cacheCounter++];   //reference to element to store
            rElem.pMesh                         = cpMesh;   //set mesh pointer
            rElem.faceID                        = i;            //set face id within mesh
            rElem.fastProcessing        = !(crMat.HasTransparencyTransfer());
            rElem.isSingleSided         = !cpMesh->Has2SidedMatByFaceID(i);
            const CObjFace& f               = cpMesh->GetObjFace(i);
            //set vertices
            rElem.vertices[0].x = cpMesh->GetVertex(f.v[0]).x;
            rElem.vertices[0].y = cpMesh->GetVertex(f.v[0]).y;
            rElem.vertices[0].z = cpMesh->GetVertex(f.v[0]).z;
            rElem.vertices[1].x = cpMesh->GetVertex(f.v[1]).x;
            rElem.vertices[1].y = cpMesh->GetVertex(f.v[1]).y;
            rElem.vertices[1].z = cpMesh->GetVertex(f.v[1]).z;
            rElem.vertices[2].x = cpMesh->GetVertex(f.v[2]).x;
            rElem.vertices[2].y = cpMesh->GetVertex(f.v[2]).y;
            rElem.vertices[2].z = cpMesh->GetVertex(f.v[2]).z;
            //compute face normal
            rElem.vNormal = (rElem.vertices[0] - rElem.vertices[1]) ^ (rElem.vertices[0] - rElem.vertices[2]);
            if (rElem.vNormal.len() < 0.0001f)
            {
                rElem.vNormal = Vec3(0.f, 0.f, 1.f);
            }
            else
            {
                rElem.vNormal.Normalize();
            }
        }
    }
    assert(cacheCounter <= triCount);

    if (!rasterCube.Init(minEx, maxEx, (uint32)cacheCounter))
    {
        GetSHLog().LogError("Initialization of RasterCube failed\n");
        return false;
    }
    for (uint32 i = 0; i < cacheCounter; ++i)
    {
        rasterCube.PutInTriangle(pCache[i].vertices, pCache[i]);
    }
    if (!rasterCube.PreProcess(false))
    {
        GetSHLog().LogError("Preprocess for RasterCube failed\n");
        return false;
    }
    //second pass
    for (uint32 i = 0; i < cacheCounter; ++i)
    {
        rasterCube.PutInTriangle(pCache[i].vertices, pCache[i]);
    }
    //free cache
    delete [] pCache;
    return true;
}

#endif