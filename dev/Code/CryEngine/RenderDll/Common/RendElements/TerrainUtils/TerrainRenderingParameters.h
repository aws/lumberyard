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

#ifdef LY_TERRAIN_RUNTIME

#pragma once

namespace Terrain
{
    class TerrainRenderingParameters
    {
    private:
        static TerrainRenderingParameters* s_pInstance;

    public:
        static TerrainRenderingParameters* GetInstance();

        static const int c_TerrainMaxLODLevels = 15;
        static const int c_VirtualTextureMaxMipLevels = 15;
        static const int c_TerrainDataTilePadding = 1;

        // With the default terrain mesh rendering settings, we only ever need a single indirection tile
        static const int c_TerrainHeightmapIndirectionTileCacheSize = 1;

        // With the default terrain material settings, worst case is that we need 4 tiles simultaneously
        static const int c_TerrainTextureIndirectionTileCacheSize = 4;

        TerrainRenderingParameters();
        ~TerrainRenderingParameters();

        void RegisterCVars();
        void UnregisterCVars();
        void UpdateTerrainTextureSamplerState();

        float GetTerrainTextureCompositeViewDistanceSqr() const;

        // Debug
        int m_terrainEnabled = 1;
        int m_terrainDrawAABBs = 0;
        int m_terrainDebugFreezeNodes = 0;
        int m_terrainTextureCacheClear = 0;
        int m_terrainWireframe = 0;

        // Terrain texture compositing debug
        int m_terrainTextureRequestDebug = 0;

        // Terrain data/rendering
        float m_terrainLeafNodeVertexSpacing = 1;
        int m_terrainLeafNodeSize = 32;
        int m_terrainLodLevelCount = 6;
        float m_terrainViewDistance = 3000.0f;
        float m_terrainLodLevelDistanceRatio = 2.0f;
        float m_terrainLod0Distance = 64.0f;

        // Terrain Data Virtual Texture
        int m_terrainDataTileSize = 32;
        int m_terrainDataPhysicalCacheSize = 1020;
        int m_terrainMaxScreenHeight = 1080;

        // Terrain Texture Compositing Virtual Texture
        float m_terrainTextureCompositeViewDistance = 300.0f;
        float m_terrainTextureCompositeBlendDistance = 30.0f;
        int m_terrainTextureTexelsPerMeter = 128;
        int m_terrainTextureMipLevels = 10;
        int m_terrainTextureTileSize = 256;
        int m_terrainTexturePhysicalCacheSize = 8192;
        int m_terrainTextureFilterQuality = 0;

        int m_cachedTerrainTextureSamplerStateId = 0;
        int m_cachedTerrainTextureMaxAnisotropy = 1;
    };
}

#endif
