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

// Description : terrain texture management


#include "StdAfx.h"
#include "terrain_sector.h"
#include "terrain.h"
#include "ObjMan.h"
#include <AzCore/std/functional.h>
#include "Terrain/Texture/MacroTextureImporter.h"

uint8 CTerrainNode::GetTextureLOD(float fDistance, const SRenderingPassInfo& passInfo)
{
    const int rootTreeLevel = GetTerrain()->GetRootNode()->m_nTreeLevel;
    const int maxLOD = m_bForceHighDetail ? 0 : rootTreeLevel;

    if (!bMacroTextureExists())
    {
        return maxLOD;
    }

    // The texture LOD is relative to the tree level of the node. Let N = Root Node Level.
    //
    //  0 = Leaf node, highest detail.
    //  N = Root node, lowest detail.
    //
    // Based on the tile resolution of the texture, a texture tile may not exist at the leaf level. In this case, the
    // node will walk up the tree and find the next available texture LOD.

    const int sectorSizeInPixels = GetMacroTexture()->GetTileSizeInPixels();
    if (!sectorSizeInPixels)
    {
        return maxLOD;
    }
    float pixelsPerMeter = float(sectorSizeInPixels) / float(GetTerrain()->GetSectorSize());

    //
    // (bethelz) I honestly don't think these magic numbers mean anything. It seems completely ad hoc.
    //
    // It looks like it converts the pixels per meter of the macro map into some ad-hoc distance function, which it then
    // compares against another ad-hoc LOD switch distance.
    //

    int magicDistance = int(pixelsPerMeter * 0.05f * (fDistance * passInfo.GetZoomFactor()) * GetFloatCVar(e_TerrainTextureLodRatio));
    for (int lod = 0; lod < maxLOD; lod++)
    {
        const int step = 48;
        int switchDistance = step << lod;
        if (switchDistance > magicDistance)
        {
            return lod;
        }
    }
    return maxLOD;
}

MacroTexture::Region CTerrainNode::GetTextureRegion() const
{
    float terrainSize = (float)CTerrain::GetTerrainSize();
    float left        = (float)m_nOriginX / terrainSize;
    float bottom      = (float)m_nOriginY / terrainSize;
    float size        = (float)((CTerrain::GetSectorSize() << m_nTreeLevel) - 1) / terrainSize;
    return MacroTexture::Region(bottom, left, size);
}

bool CTerrainNode::bMacroTextureExists() const
{
    return GetMacroTexture() != nullptr;
}

MacroTexture* CTerrainNode::GetMacroTexture()
{
    return GetTerrain()->m_MacroTexture.get();
}

const MacroTexture* CTerrainNode::GetMacroTexture() const
{
    return GetTerrain()->m_MacroTexture.get();
}

static void InitSectorTextureSet(
    float tileSizeInPixels,
    float nodeSizeInUnits,
    float nodeOriginXinUnits,
    float nodeOriginYinUnits,
    SSectorTextureSet& outTextureSet)
{
    float sectorSizeScale = (tileSizeInPixels - 1) / (float)(tileSizeInPixels);
    float inverseNodeSize = sectorSizeScale / nodeSizeInUnits;

    //
    // X and Y are intentionally flipped. This is because the X and Y world space coordinates are
    // transposed from the heightmap coordinates. TODO (bethelz): Fix that coordinate issue.
    //
    outTextureSet.fTexOffsetX = -inverseNodeSize * nodeOriginYinUnits + (0.5f / tileSizeInPixels);
    outTextureSet.fTexOffsetY = -inverseNodeSize * nodeOriginXinUnits + (0.5f / tileSizeInPixels);
    outTextureSet.fTexScale = inverseNodeSize;
}

void CTerrainNode::RequestTextures(const SRenderingPassInfo& passInfo)
{
    if (bMacroTextureExists() && !passInfo.IsRecursivePass() && !m_nEditorDiffuseTex)
    {
        MacroTexture::Region region = GetTextureRegion();
        GetMacroTexture()->RequestStreamTiles(region, m_TextureLOD);
    }
}

void CTerrainNode::SetSectorTexture(unsigned int nEditorDiffuseTex)
{
    FUNCTION_PROFILER_3DENGINE;

    // assign new diffuse texture
    m_TextureSet.nTex0 = nEditorDiffuseTex;

    // disable texture streaming
    m_nEditorDiffuseTex = nEditorDiffuseTex;
}

void CTerrainNode::SetupTexturing(const SRenderingPassInfo& passInfo)
{
    if (passInfo.IsShadowPass())
    {
        return;
    }

    assert(GetLeafData());

    FUNCTION_PROFILER_3DENGINE_LEGACYONLY;

    SSectorTextureSet renderSet = m_TextureSet;

    if (bMacroTextureExists())
    {
        float tileSizeInPixels = (float)GetMacroTexture()->GetTileSizeInPixels();
        float nodeSizeInUnits = (float)(CTerrain::GetSectorSize() << m_nTreeLevel);
        InitSectorTextureSet(tileSizeInPixels, nodeSizeInUnits, m_nOriginX, m_nOriginY, m_TextureSet);

        if (!m_nEditorDiffuseTex)
        {
            MacroTexture::TextureTile tile;
            if (GetMacroTexture()->TryGetTextureTile(GetTextureRegion(), m_TextureLOD, tile))
            {
                float terrainSize = (float)CTerrain::GetTerrainSize();
                float tileSize = tile.region.Size   * terrainSize;
                float tileX    = tile.region.Left   * terrainSize;
                float tileY    = tile.region.Bottom * terrainSize;
                InitSectorTextureSet(tileSizeInPixels, tileSize, tileX, tileY, renderSet);

                renderSet.nTex0 = m_TextureSet.nTex0 = tile.textureId;
            }
            else // Engine tile lookup failed, revert to white
            {
                renderSet = m_TextureSet = SSectorTextureSet(GetTerrain()->m_nWhiteTexId);
            }
        }
        else // Using editor override texture instead.
        {
            m_TextureSet.nTex0 = m_nEditorDiffuseTex;

            renderSet = m_TextureSet;
        }
    }

    STerrainNodeLeafData& leafData = *GetLeafData();
    _smart_ptr<IRenderMesh>& pRenderMesh = leafData.m_pRenderMesh;

    const int DEBUG_MATERIAL_WHITE = 1;
    const int DEBUG_MATERIAL_DEFAULT = 2;

    switch (GetCVars()->e_TerrainTextureDebug)
    {
    case DEBUG_MATERIAL_WHITE:
        pRenderMesh->SetCustomTexID(m_pTerrain->m_nWhiteTexId);
        break;

    case DEBUG_MATERIAL_DEFAULT:
        pRenderMesh->SetCustomTexID(0);
        break;

    default:
        uint32 textureId = renderSet.nTex0;
        pRenderMesh->SetCustomTexID(textureId);
    }

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    // Use m_TextureParams for double buffering to prevent flickering of terrain texture due to threading issues
    const int fillThreadId = passInfo.ThreadID();
    leafData.m_TextureParams[fillThreadId].Set(renderSet);
#else
    leafData.m_TextureParams[0].Set(renderSet);
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

    pRenderMesh->GetChunks()[0].pRE->m_CustomData = leafData.m_TextureParams;
}

void CTerrain::SetTerrainSectorTexture(int nTexSectorX, int nTexSectorY, unsigned int textureId, bool bMergeNotAllowed)
{
    int nDiffTexTreeLevelOffset = 0;

    if (nTexSectorX < 0 ||
        nTexSectorY < 0 ||
        nTexSectorX >= CTerrain::GetSectorsTableSize() >> nDiffTexTreeLevelOffset ||
        nTexSectorY >= CTerrain::GetSectorsTableSize() >> nDiffTexTreeLevelOffset)
    {
        Warning("CTerrain::LockSectorTexture: (nTexSectorX, nTexSectorY) values out of range");
        return;
    }

    CTerrainNode* pNode = m_NodePyramid[nDiffTexTreeLevelOffset][nTexSectorX][nTexSectorY];
    pNode->SetSectorTexture(textureId);

    while (pNode)
    {
        pNode->m_bForceHighDetail = bMergeNotAllowed;
        pNode = pNode->m_Parent;
    }
}

void CTerrain::ProcessTextureStreamingRequests(const SRenderingPassInfo& passInfo)
{
    MacroTexture::LoadPolicy loadPolicy =
        Get3DEngine()->IsTerrainSyncLoad() ?
        MacroTexture::LoadPolicy::Immediate :
        MacroTexture::LoadPolicy::Stream;

    float inverseTerrainSize = CTerrain::GetInvUnitSize() / (float)GetTerrainSize();
    const Vec3 cameraPosition = passInfo.GetCamera().GetPosition();
    const Vec2 focusPoint(cameraPosition.x * inverseTerrainSize, cameraPosition.y * inverseTerrainSize);

    const uint32 NewStreamRequestCountMax = 6;
    const uint32 streamRequestCap = (loadPolicy == MacroTexture::LoadPolicy::Stream) ? NewStreamRequestCountMax : ~0;

    if (m_MacroTexture)
    {
        m_MacroTexture->Update(streamRequestCap, loadPolicy, &focusPoint);
    }
}

void CTerrain::CloseTerrainTextureFile()
{
    for (int i = m_lstActiveProcObjNodes.size() - 1; i >= 0; --i)
    {
        m_lstActiveProcObjNodes[i]->RemoveProcObjects(false);
        m_lstActiveProcObjNodes.Delete(i);
    }

    m_MacroTexture.reset();

    TraverseTree([this](CTerrainNode* node)
        {
            node->m_TextureSet = SSectorTextureSet(m_nWhiteTexId);
        });
}

bool CTerrain::OpenTerrainTextureFile(const char* szFileName)
{
    FUNCTION_PROFILER_3DENGINE;

    AZ_Assert(!m_MacroTexture, "Attempting to open already open terrain texture");

    // unlock all nodes
    TraverseTree([this](CTerrainNode* node)
        {
            node->m_nEditorDiffuseTex = 0;
            node->m_bForceHighDetail = false;
            node->m_TextureSet = SSectorTextureSet(m_nWhiteTexId);
        });

    const uint32 MaxElementCountPerPool = max(GetCVars()->e_TerrainTextureStreamingPoolItemsNum, 1);

    m_MacroTexture = MacroTextureImporter::Import(Get3DEngine()->GetLevelFilePath(szFileName), MaxElementCountPerPool);
    if (!m_MacroTexture)
    {
        return false;
    }

    if (!m_bEditor)
    {
        FRAME_PROFILER("CTerrain::OpenTerrainTextureFile: ReleaseHoleNodes & UpdateTerrainNodes", GetSystem(), PROFILE_3DENGINE);
        if (Get3DEngine()->IsObjectTreeReady())
        {
            Get3DEngine()->GetObjectTree()->UpdateTerrainNodes();
        }
    }

    return true;
}

bool CTerrain::IsTextureStreamingInProgress() const
{
    MacroTexture::TileStatistics statistics;
    if (TryGetTextureStatistics(statistics))
    {
        return statistics.streamingCount > 0;
    }
    return false;
}

bool CTerrain::TryGetTextureStatistics(MacroTexture::TileStatistics& statistics) const
{
    if (m_MacroTexture)
    {
        m_MacroTexture->GetTileStatistics(statistics);
        return true;
    }
    return false;
}