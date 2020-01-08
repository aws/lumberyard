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

#include "CRETerrain.h"
#include <AzCore/Math/Vector4.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/mutex.h>

#include "CDLOD/CDLODQuadTree.h"
#include "TerrainCompositer.h"

#include <Terrain/Bus/TerrainRendererBus.h>

namespace Terrain
{
    // Forward declaration
    class VirtualTexture;
    class VirtualTileRequestInfo;

    class TerrainTextureCache
        : public CRETerrainContext
        , public TerrainRendererRequestBus::Handler
    {
    public:
        static const int s_PhysicalTextureSlotStart = 14;
        static const int s_IndirectionMapSlot = 16;

        // TerrainTextureCache Initialization Parameters
        struct TerrainTextureCacheDesc
        {
            struct HeightmapRenderParams
            {
                int m_lodLevelCount = 0;
                int m_tileSize = 64;
                int m_physicalTextureCacheSize = 2048;
                float m_vertexSpacing = 1.0f;
            } m_heightmapRenderParams;

            TerrainCompositer::TerrainTextureCompositingParams m_terrainTextureParams;
        };

        TerrainTextureCache();
        TerrainTextureCache(const TerrainTextureCacheDesc& terrainDesc);
        ~TerrainTextureCache();

        void Init();
        void Shutdown();

        int GetTerrainDataPhysicalCacheSize() const;
        int GetTerrainDataTileSize() const;
        int GetLodLevels() const;
        float GetLeafNodeVertexSpacing() const;

        void SetPOMDisplacement(float pomDisplacement) { m_terrainPOMDisplacement = pomDisplacement; }
        void SetPOMHeightBias(float pomHeightBias) { m_terrainPOMHeightBias = pomHeightBias; }
        void SetSelfShadowStrength(float selfShadowStrength) { m_terrainSelfShadowStrength = selfShadowStrength; }

        void RequestTerrainDataTiles(const LODSelection& selection);

        // Pump the render queue
        void RenderUpdate();

        // Begin/End Rendering setup with virtual textures
        bool BeginRendering(const SelectedNode& selectedNode, bool useCompositedTerrainTextures);
        void EndRendering();

        // Clears height and normal data virtual textures
        void ClearHeightmapCache();

        // Clear terrain material cache
        void ClearCompositedTextureCache();

        const TerrainTextureCacheDesc& GetTerrainTextureCacheDesc() const { return m_terrainTextureCacheDesc; }

        ///////////////////////////////////////////////////
        // CRETerrainContext Impl
        void SetPSConstant(ConstantNames name, float x, float y, float z, float w) override;
        void OnTractVersionUpdate() override;
        void OnHeightMapVersionUpdate() override;
        ///////////////////////////////////////////////////

        ///////////////////////////////////////////////////
        // TerrainRendererRequestBus interface implementation
        bool IsReady() override;
        ///////////////////////////////////////////////////

        // Terrain Material Compositing Requests
        void RequestTerrainTextureTiles(const LODSelection& selection);

    private:
        AZStd::unique_ptr<VirtualTexture> m_heightMapVirtualTexturePtr;
        int m_heightMapVirtualTextureSamplerStateId;

        // Initializing this i forto max size_t in case we don't update the active request count before the first call to IsReady()
        AZStd::size_t m_activeHeightMapTileRequests = std::numeric_limits<AZStd::size_t>::max();

        int m_perFrameTileUpdateCount = 1;
        int m_perFrameHeightRequestsCount = 1;
        int m_perFrameCompositerRequestsCount = 1;

        // Render callback to fill tiles with procedural heightmap data
        void RenderHeightMapTile(const AZStd::vector<CTexture*>& heightAndNormal, const Terrain::VirtualTileRequestInfo& requestInfo);

        // Callback to fill height map tile
        bool FillHeightMapTile(const AZStd::vector<CTexture*>& heightAndNormal, const Terrain::VirtualTileRequestInfo& requestInfo);

        // Cached copy of params structure
        TerrainTextureCacheDesc m_terrainTextureCacheDesc;

        ///////////////////////////////////////////////////
        // Compositer rendering
        TerrainCompositer m_Compositer;

        ///////////////////////////////////////////////////
        // Terrain rendering params
        float m_terrainPOMHeightBias = 0.0f;
        float m_terrainPOMDisplacement = 0.0f;
        float m_terrainSelfShadowStrength = 0.0f;

        bool m_useCompositedTerrainTextures = false;
    };
}

#endif
