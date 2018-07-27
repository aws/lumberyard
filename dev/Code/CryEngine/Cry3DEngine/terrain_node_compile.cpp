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
#include "Environment/OceanEnvironmentBus.h"

#define TERRAIN_NODE_CHUNK_VERSION 8

int CTerrainNode::Load(uint8*& f, int& nDataSize, EEndian eEndian, bool bSectorPalettes, SHotUpdateInfo* pExportInfo)
{
    return Load_T(f, nDataSize, eEndian, bSectorPalettes, pExportInfo);
}
int CTerrainNode::Load(AZ::IO::HandleType& fileHandle, int& nDataSize, EEndian eEndian, bool bSectorPalettes, SHotUpdateInfo* pExportInfo)
{
    return Load_T(fileHandle, nDataSize, eEndian, bSectorPalettes, pExportInfo);
}

int CTerrainNode::FTell(uint8*& f)
{
    return -1;
}
int CTerrainNode::FTell(AZ::IO::HandleType& fileHandle)
{
    return GetPak()->FTell(fileHandle);
}

template<class T>
int CTerrainNode::Load_T(T& f, int& nDataSize, EEndian eEndian, bool bSectorPalettes, SHotUpdateInfo* pExportInfo)
{
    const AABB* pAreaBox = (pExportInfo && !pExportInfo->areaBox.IsReset()) ? &pExportInfo->areaBox : NULL;

    if (pAreaBox && !Overlap::AABB_AABB(GetBBox(), *pAreaBox))
    {
        return 0;
    }

    // set node data
    STerrainNodeChunk chunk;
    if (!CTerrain::LoadDataFromFile(&chunk, 1, f, nDataSize, eEndian))
    {
        return 0;
    }

    assert(chunk.nChunkVersion == TERRAIN_NODE_CHUNK_VERSION);
    if (chunk.nChunkVersion != TERRAIN_NODE_CHUNK_VERSION)
    {
        return 0;
    }

    // set error levels, bounding boxes and some flags
    m_LocalAABB = chunk.boxHeightmap;
    // TODO: Figure out why the bounding box includes the ocean height when the ocean is above the terrain.
    m_LocalAABB.max.z = max(m_LocalAABB.max.z + TerrainConstants::coloredVegetationMaxSafeHeight, OceanToggle::IsActive() ? OceanRequest::GetOceanLevel() : GetTerrain()->GetWaterLevel());
    m_bHasHoles = chunk.bHasHoles;

    assert(m_SurfaceTile.GetHeightmap() == nullptr);
    if (chunk.nSize)
    {
        assert(chunk.nSize == GetTerrain()->SectorSize_Units());

        const int ChunkSizeSqr = chunk.nSize * chunk.nSize;
        uint16* heightmap = new uint16[(ChunkSizeSqr + 7) & (~0x7)];
        if (!CTerrain::LoadDataFromFile(heightmap, ChunkSizeSqr, f, nDataSize, eEndian))
        {
            delete[] heightmap;
            return 0;
        }

        CTerrain::LoadDataFromFile_FixAlignment(f, nDataSize);

        ITerrain::SurfaceWeight* weights = new ITerrain::SurfaceWeight[(ChunkSizeSqr + 7) & (~0x7)];
        if (!CTerrain::LoadDataFromFile(weights, ChunkSizeSqr, f, nDataSize, eEndian))
        {
            delete[] heightmap;
            delete[] weights;
            return 0;
        }

        CTerrain::LoadDataFromFile_FixAlignment(f, nDataSize);

        m_SurfaceTile.AssignMaps(chunk.nSize, heightmap, weights);
        m_SurfaceTile.SetRange(chunk.fOffset, chunk.fRange);
    }

    // load LOD errors
    delete[] m_ZErrorFromBaseLOD;
    m_ZErrorFromBaseLOD = new float[GetTerrain()->m_UnitToSectorBitShift];
    if (!CTerrain::LoadDataFromFile(m_ZErrorFromBaseLOD, GetTerrain()->m_UnitToSectorBitShift, f, nDataSize, eEndian))
    {
        return 0;
    }
    assert(m_ZErrorFromBaseLOD[0] == 0);

    {
        for (int i = 0; i < m_DetailLayers.Count(); i++)
        {
            m_DetailLayers[i].DeleteRenderMeshes(GetRenderer());
        }
        m_DetailLayers.Clear();
        m_DetailLayers.PreAllocate(chunk.nSurfaceTypesNum, chunk.nSurfaceTypesNum);

        if (chunk.nSurfaceTypesNum)
        {
            uint8* pTypes = new uint8[chunk.nSurfaceTypesNum];

            if (!CTerrain::LoadDataFromFile(pTypes, chunk.nSurfaceTypesNum, f, nDataSize, eEndian))
            {
                delete[] pTypes;
                return 0;
            }

            for (int i = 0; i < m_DetailLayers.Count() && i < ITerrain::SurfaceWeight::Hole; i++)
            {
                m_DetailLayers[i].surfaceType = &GetTerrain()->m_SurfaceTypes[min((int)pTypes[i], (int)ITerrain::SurfaceWeight::Undefined)];
            }

            delete[] pTypes;

            CTerrain::LoadDataFromFile_FixAlignment(f, nDataSize);
        }
    }

    // count number of nodes saved
    int nNodesNum = 1;

    // process childs
    for (int i = 0; m_Children && i < 4; i++)
    {
        nNodesNum += m_Children[i].Load_T(f, nDataSize, eEndian, bSectorPalettes, pExportInfo);
    }

    return nNodesNum;
}

void CTerrainNode::ReleaseHoleNodes()
{
    if (!m_Children)
    {
        return;
    }

    if (m_bHasHoles == 2)
    {
        SAFE_DELETE_ARRAY(m_Children);
    }
    else
    {
        for (int i = 0; i < 4; i++)
        {
            m_Children[i].ReleaseHoleNodes();
        }
    }
}

int CTerrainNode::GetData(byte*& pData, int& nDataSize, EEndian eEndian, SHotUpdateInfo* pExportInfo)
{
    const AABB* pAreaBox = (pExportInfo && !pExportInfo->areaBox.IsReset()) ? &pExportInfo->areaBox : NULL;

    AABB boxWS = GetBBox();

    if (pAreaBox && !Overlap::AABB_AABB(boxWS, *pAreaBox))
    {
        return 0;
    }

    if (pData)
    {
        // get node data
        STerrainNodeChunk* pCunk = (STerrainNodeChunk*)pData;
        pCunk->nChunkVersion = TERRAIN_NODE_CHUNK_VERSION;
        pCunk->boxHeightmap = boxWS;
        pCunk->bHasHoles = m_bHasHoles;
        pCunk->fOffset = m_SurfaceTile.GetOffset();
        pCunk->fRange = m_SurfaceTile.GetRange();
        pCunk->nSize = m_SurfaceTile.GetSize();
        pCunk->nSurfaceTypesNum = m_DetailLayers.Count();

        SwapEndian(*pCunk, eEndian);
        UPDATE_PTR_AND_SIZE(pData, nDataSize, sizeof(STerrainNodeChunk));

        const int SizeSqr = m_SurfaceTile.GetSize() * m_SurfaceTile.GetSize();

        AddToPtr(pData, nDataSize, m_SurfaceTile.GetHeightmap(), SizeSqr, eEndian, true);
        AddToPtr(pData, nDataSize, m_SurfaceTile.GetWeightmap(), SizeSqr, eEndian, true);
        AddToPtr(pData, nDataSize, m_ZErrorFromBaseLOD, GetTerrain()->m_UnitToSectorBitShift, eEndian);

        // get used surf types
        CRY_ASSERT(m_DetailLayers.Count() <= SurfaceTile::MaxSurfaceCount);

        uint8 surfaceIds[SurfaceTile::MaxSurfaceCount] = { ITerrain::SurfaceWeight::Undefined };
        for (int i = 0; i < SurfaceTile::MaxSurfaceCount && i < m_DetailLayers.Count(); i++)
        {
            surfaceIds[i] = m_DetailLayers[i].surfaceType->ucThisSurfaceTypeId;
        }

        AddToPtr(pData, nDataSize, surfaceIds, m_DetailLayers.Count(), eEndian, true);
    }
    else // just count size
    {
        nDataSize += sizeof(STerrainNodeChunk);
        nDataSize += m_SurfaceTile.GetSize() * m_SurfaceTile.GetSize() * sizeof(m_SurfaceTile.GetHeightmap()[0]);
        while (nDataSize & 3)
        {
            nDataSize++;
        }
        nDataSize += m_SurfaceTile.GetSize() * m_SurfaceTile.GetSize() * sizeof(m_SurfaceTile.GetWeightmap()[0]);
        while (nDataSize & 3)
        {
            nDataSize++;
        }
        nDataSize += GetTerrain()->m_UnitToSectorBitShift * sizeof(m_ZErrorFromBaseLOD[0]);
        nDataSize += m_DetailLayers.Count() * sizeof(uint8);
        while (nDataSize & 3)
        {
            nDataSize++;
        }
    }

    // count number of nodes saved
    int nNodesNum = 1;

    // process childs
    for (int i = 0; m_Children && i < 4; i++)
    {
        nNodesNum += m_Children[i].GetData(pData, nDataSize, eEndian, pExportInfo);
    }

    return nNodesNum;
}

