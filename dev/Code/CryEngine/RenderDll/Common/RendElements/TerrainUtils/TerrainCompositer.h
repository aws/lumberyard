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

#include <AzCore/Math/Aabb.h>
#include <AzCore/std/containers/vector.h>

#include "Terrain/Bus/WorldMaterialRequestsBus.h"
#include "CDLOD/CDLODQuadTree.h"
#include "VirtualTexture.h"

namespace Terrain
{
    class VirtualTileRequestInfo;
    struct RegionMaterials;

    class TerrainCompositer
    {
    public:
        static const int s_PhysicalTextureSlotStart = 14;
        static const int s_IndirectionMapSlot = 16;

        static CShader* GetTerrainTileCompositingShader();

        struct TerrainTextureCompositingParams
        {
            int m_texelsPerMeter = 64;
            int m_mipLevels = 8;
            int m_tileSize = 512;
            int m_physicalTextureCacheSize = 2048;
        } m_terrainTextureParams;

        TerrainCompositer();

        void Init(const TerrainTextureCompositingParams& compositingParams);

        void RequestTiles(float worldX, float worldY, float worldXWidth, float worldYWidth, int mipLevel);
        void RequestTerrainTextureTiles(const LODSelection& selection);

        void RenderRequestedTiles(int maxRequests);

        void ClearCompositedTextureCache();

        void SetupMacroMaterial(const AZ::Aabb& nodeAABB);

        bool BeginRendering(const AZ::Aabb& nodeAABB);
        void EndRendering();

        float GetTexelsPerMeter() const { return m_VirtualTexture->GetTexelsPerMeter(0); }
        int GetMipLevels() const { return m_VirtualTexture->GetMipLevels(); }
        int GetTileSize() const { return m_VirtualTexture->GetTileSize(); }
        int GetPhysicalTextureCacheSize() const { return m_VirtualTexture->GetPhysicalTextureCacheSize(); }

        AZStd::size_t GetActiveTileRequestCount() const { return m_activeRequestCount; }

    private:
        AZStd::unique_ptr<VirtualTexture> m_VirtualTexture;

        // Initializing this to max size_t in case we don't update the active request count before the first call to IsReady()
        AZStd::atomic_size_t m_activeRequestCount = {std::numeric_limits<AZStd::size_t>::max()};

        // Temp buffers for compositing pingpong
        _smart_ptr<CTexture> m_compositeTempTextures[4];

        // Cached copy of the current region material
        RegionMaterialVector m_currentRegionMaterials;

        // Callback assigned for handling terrain material texture composite requests
        bool FillMapTile(const AZStd::vector<CTexture*>& colorTextures, const Terrain::VirtualTileRequestInfo& requestInfo);

        // Render callback to fill tiles with procedural color data
        bool RenderTile(const VirtualTileRequestInfo& requestInfo);

        // Utility for rendering a material layer into the currently set render targets
        void RenderMaterial(IMaterial* material, const MacroMaterial& regionMacroMaterial, Vec4 quadVertices, Vec4 regionUVoffsetScale, Vec4 tileUVoffsetScale);
    };
}

#endif
