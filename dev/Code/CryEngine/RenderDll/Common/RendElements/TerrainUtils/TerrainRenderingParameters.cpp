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

#include "StdAfx.h"

#ifdef LY_TERRAIN_RUNTIME

#include "TerrainRenderingParameters.h"
#include <AzCore/Math/MathUtils.h>

namespace Terrain
{
    namespace
    {
        void OnTerrainTextureFilterQualityCVarChanged(ICVar* pCVar)
        {
            (void)pCVar;

            TerrainRenderingParameters* pTerrainParams = TerrainRenderingParameters::GetInstance();
            if (pTerrainParams)
            {
                pTerrainParams->UpdateTerrainTextureSamplerState();
            }
        }
    }

    TerrainRenderingParameters* TerrainRenderingParameters::s_pInstance = nullptr;

    TerrainRenderingParameters* TerrainRenderingParameters::GetInstance()
    {
        if (!s_pInstance)
        {
            s_pInstance = new TerrainRenderingParameters();
        }

        return s_pInstance;
    }

    TerrainRenderingParameters::TerrainRenderingParameters()
    {
        RegisterCVars();
    }

    TerrainRenderingParameters::~TerrainRenderingParameters()
    {
        UnregisterCVars();
    }

    void TerrainRenderingParameters::RegisterCVars()
    {
        // Debug
        REGISTER_CVAR2("terrainEnabled", &m_terrainEnabled, 1, 0, "enables the drawing of terrain");
        REGISTER_CVAR2("terrainDrawAABBs", &m_terrainDrawAABBs, 0, 0, "enable/disable AABB debug rendering for terrain patches");
        REGISTER_CVAR2("terrainDebugFreezeNodes", &m_terrainDebugFreezeNodes, 0, 0, "freeze the selection of nodes for debugging purposes");
        REGISTER_CVAR2("terrainTextureCacheClear", &m_terrainTextureCacheClear, 0, 0, "clears the terrain texture cache when set to non-zero value");
        REGISTER_CVAR2("terrainWireframe", &m_terrainWireframe, 0, 0, "renders terrain wireframe (1=wireframe over terrain, 2=wireframe-only)");

        // Terrain texture compositing debug
        REGISTER_CVAR2("terrainTextureRequestDebug", &m_terrainTextureRequestDebug, 0, 0, "enable terrain texture request debug logs");

        // Terrain data/rendering
        REGISTER_CVAR2("terrainVertexSpacing", &m_terrainLeafNodeVertexSpacing, 1, 0, "vertex spacing in meters for the highest detailed terrain patches");
        REGISTER_CVAR2("terrainLeafNodeSize", &m_terrainLeafNodeSize, 32, 0, "world unit width of terrain leaf node (highest detailed LOD)");
        REGISTER_CVAR2("terrainLodLevelCount", &m_terrainLodLevelCount, 6, 0, "number of mesh LODs for terrain rendering");
        REGISTER_CVAR2("terrainViewDistance", &m_terrainViewDistance, 3000.0f, 0, "maximum view distance for terrain rendering");
        REGISTER_CVAR2("terrainLodLevelDistanceRatio", &m_terrainLodLevelDistanceRatio, 2.0f, 0, "LOD level distance ratio factor, where "
            "LodDistance(N) = terrainLodLevelDistanceRatio * LodDistance(N-1)");
        REGISTER_CVAR2("terrainLod0Distance", &m_terrainLod0Distance, 64.0f, 0, "Terrain mesh LOD0 max render distance\n");

        // Terrain Data Virtual Texture
        REGISTER_CVAR2("terrainDataTileSize", &m_terrainDataTileSize, 32, 0, "Heightmap tile size in the physical heightmap texture cache");
        REGISTER_CVAR2("terrainDataPhysicalCacheSize", &m_terrainDataPhysicalCacheSize, 1020, 0, "Physical heightmap texture cache size");
        REGISTER_CVAR2("terrainMaxScreenHeight", &m_terrainMaxScreenHeight, 1080, 0, "Clamp the terrain system's representation of screen height.\n"
            "Allows for indirect control of terrain texture tile cache occupancy.");

        // Terrain texture compositing virtual texture
        REGISTER_CVAR2("terrainTextureCompositeViewDistance", &m_terrainTextureCompositeViewDistance, 300.0f, 0, "Max rendering distance (meters) for composited terrain materials");
        REGISTER_CVAR2("terrainTextureCompositeBlendDistance", &m_terrainTextureCompositeBlendDistance, 30.0f, 0, "Blend distance (meters) from composited terrain materials to macro maps");
        REGISTER_CVAR2("terrainTextureTexelsPerMeter", &m_terrainTextureTexelsPerMeter, 128, 0, "Max texels per meter for composited terrain materials");
        REGISTER_CVAR2("terrainTextureMipLevels", &m_terrainTextureMipLevels, 10, 0, "Mip level count for composited terrain materials");
        REGISTER_CVAR2("terrainTextureTileSize", &m_terrainTextureTileSize, 256, 0, "Composited terrain material texture tile size");
        REGISTER_CVAR2("terrainTexturePhysicalCacheSize", &m_terrainTexturePhysicalCacheSize, 8192, 0, "Composited terrain material physical texture cache size");

        REGISTER_CVAR2_CB("terrainTextureFilterQuality", &m_terrainTextureFilterQuality, 0, 0, "Terrain Texture Filtering Quality\n"
            "  0 - Trilinear\n"
            "  1 - Aniso 2x\n"
            "  2 - Aniso 4x\n",
            OnTerrainTextureFilterQualityCVarChanged);

        UpdateTerrainTextureSamplerState();
    }

    void TerrainRenderingParameters::UnregisterCVars()
    {
        // Debug
        UNREGISTER_CVAR("terrainEnabled");
        UNREGISTER_CVAR("terrainDrawAABBs");
        UNREGISTER_CVAR("terrainDebugFreezeNodes");
        UNREGISTER_CVAR("terrainTextureCacheClear");
        UNREGISTER_CVAR("terrainWireframe");

        // Terrain texture compositing debug
        UNREGISTER_CVAR("terrainTextureRequestDebug");

        // Terrain data/rendering
        UNREGISTER_CVAR("terrainVertexSpacing");
        UNREGISTER_CVAR("terrainLeafNodeSize");
        UNREGISTER_CVAR("terrainLodLevelCount");
        UNREGISTER_CVAR("terrainViewDistance");
        UNREGISTER_CVAR("terrainLodLevelDistanceRatio");
        UNREGISTER_CVAR("terrainLod0Distance");

        // Terrain Data Virtual Texture
        UNREGISTER_CVAR("terrainDataTileSize");
        UNREGISTER_CVAR("terrainDataPhysicalCacheSize");
        UNREGISTER_CVAR("terrainMaxScreenHeight");

        // Terrain Texture Compositing Virtual Texture
        UNREGISTER_CVAR("terrainTextureCompositeViewDistance");
        UNREGISTER_CVAR("terrainTextureCompositeBlendDistance");
        UNREGISTER_CVAR("terrainTextureTexelsPerMeter");
        UNREGISTER_CVAR("terrainTextureMipLevels");
        UNREGISTER_CVAR("terrainTextureTileSize");
        UNREGISTER_CVAR("terrainTexturePhysicalCacheSize");
        UNREGISTER_CVAR("terrainTextureFilterQuality");
    }

    void TerrainRenderingParameters::UpdateTerrainTextureSamplerState()
    {
        int clampedTerrainQualityCVar = AZ::GetClamp(m_terrainTextureFilterQuality, 0, 2);

        // Resolve sampler state
        int filterMode = FILTER_LINEAR;
        switch (clampedTerrainQualityCVar)
        {
        default:
        case 0:
            filterMode = FILTER_LINEAR;
            m_cachedTerrainTextureMaxAnisotropy = 1;
            break;
        case 1:
            filterMode = FILTER_ANISO2X;
            m_cachedTerrainTextureMaxAnisotropy = 2;
            break;
        case 2:
            filterMode = FILTER_ANISO4X;
            m_cachedTerrainTextureMaxAnisotropy = 4;
            break;
        }

        STexState samplerState;
        samplerState.SetFilterMode(filterMode);
        samplerState.SetClampMode(TADDR_CLAMP, TADDR_CLAMP, TADDR_CLAMP);
        m_cachedTerrainTextureSamplerStateId = CTexture::GetTexState(samplerState);
    }

    float TerrainRenderingParameters::GetTerrainTextureCompositeViewDistanceSqr() const
    {
        return m_terrainTextureCompositeViewDistance * m_terrainTextureCompositeViewDistance;
    }
}

#endif
