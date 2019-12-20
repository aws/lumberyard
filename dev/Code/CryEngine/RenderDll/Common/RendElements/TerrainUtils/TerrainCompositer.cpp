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

#include "TerrainCompositer.h"
#include "VirtualTexture.h"
#include "CDLOD/CDLODQuadTree.h"

#include "Textures/TextureManager.h"

// Change this from "#undef" to "#define" to enable verbose reporting from the composite system for debugging.
#undef REPORT_REQUESTED_REGIONS

namespace Terrain
{
    CShader* TerrainCompositer::GetTerrainTileCompositingShader()
    {
#if defined(NULL_RENDERER)
        return nullptr;
#else
        constexpr const char* terrainTileCompositingShaderName = "TerrainTileCompositeSystem";

        CShader* compositingShader = nullptr;
        Terrain::TerrainDataRequestBus::BroadcastResult(compositingShader, &Terrain::TerrainDataRequestBus::Events::GetTerrainMaterialCompositingShader);

        gRenDev->m_cEF.mfRefreshSystemShader(terrainTileCompositingShaderName, compositingShader);

        bool terrainTileCompositeShaderIsReady = (compositingShader && compositingShader->IsValid());
        return terrainTileCompositeShaderIsReady ? compositingShader : nullptr;
#endif
    }

    TerrainCompositer::TerrainCompositer()
    {
    }

    void TerrainCompositer::Init(const TerrainTextureCompositingParams& compositingParams)
    {
        ColorF clearColors[2];
        clearColors[0] = Col_Black;
        clearColors[1] = Col_Black;

        ETEX_Format textureFormats[2];
        textureFormats[0] = eTF_R8G8B8A8;
        textureFormats[1] = eTF_R8G8B8A8;

        AZ::Vector3 worldSize(0.0f);
        TerrainProviderRequestBus::BroadcastResult(worldSize, &TerrainProviderRequestBus::Events::GetWorldSize);
        int worldSizeXMeters = static_cast<int>(worldSize.GetX());
        int worldSizeYMeters = static_cast<int>(worldSize.GetY());

        AZ::Vector3 worldOrigin(0.0f);
        TerrainProviderRequestBus::BroadcastResult(worldOrigin, &TerrainProviderRequestBus::Events::GetWorldOrigin);
        int worldOriginX = static_cast<int>(worldOrigin.GetX());
        int worldOriginY = static_cast<int>(worldOrigin.GetY());

        VirtualTexture::VirtualTextureDesc vtDesc;
        vtDesc.m_name = "ColorMap";
        vtDesc.m_formats = &textureFormats[0];
        vtDesc.m_clearColors = &clearColors[0];
        vtDesc.m_textureCount = 2;
        vtDesc.m_texelsPerMeter = static_cast<float>(compositingParams.m_texelsPerMeter);
        vtDesc.m_mipLevels = compositingParams.m_mipLevels;
        vtDesc.m_tileSize = compositingParams.m_tileSize;
        vtDesc.m_tilePadding = 4;   // padding of 4 required for texture compression and anisotropic filtering
        vtDesc.m_physicalTextureCacheSize = compositingParams.m_physicalTextureCacheSize;
        vtDesc.m_worldSizeXMeters = worldSizeXMeters;
        vtDesc.m_worldSizeYMeters = worldSizeYMeters;
        vtDesc.m_worldOriginX = worldOriginX;
        vtDesc.m_worldOriginY = worldOriginY;
        vtDesc.m_initialIndirectionTileCacheSize = TerrainRenderingParameters::c_TerrainTextureIndirectionTileCacheSize;

        m_VirtualTexture = AZStd::make_unique<VirtualTexture>(vtDesc);

        auto fpFillMapTile = AZStd::bind(&TerrainCompositer::FillMapTile, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
        m_VirtualTexture->SetTileRequestCallback(fpFillMapTile);

        // Set texture slots
        m_VirtualTexture->SetTextureSlots(s_IndirectionMapSlot, s_PhysicalTextureSlotStart);
    }

    void TerrainCompositer::RequestTiles(float worldX, float worldY, float worldXWidth, float worldYWidth, int mipLevel)
    {
        bool compositingShaderIsReady = GetTerrainTileCompositingShader() != nullptr;
        if (compositingShaderIsReady)
        {
            m_VirtualTexture->RequestTiles(worldX, worldY, worldXWidth, worldYWidth, mipLevel);
            m_activeRequestCount = m_VirtualTexture->GetActiveRequestCount();
        }
    }

    void TerrainCompositer::RequestTerrainTextureTiles(const LODSelection& selection)
    {
#if defined(REPORT_REQUESTED_REGIONS)
        bool compositerRegionReport = gEnv->pRenderer->GetFrameID() % 60 == 0;
        AZStd::unordered_set<AZStd::pair<int, int> > regionSet;
#endif

        bool compositingShaderIsReady = GetTerrainTileCompositingShader() != nullptr;
        if (compositingShaderIsReady)
        {
            const int maxMipLevels = m_VirtualTexture->GetMipLevels();
            const AZ::Vector3 sizeBias(0.001f);

            for (int i = 0; i < selection.GetSelectionCount(); i++)
            {
                const SelectedNode& nodeSel = selection.GetSelection()[i];

                AZ_Verify(nodeSel.m_lodLevel < maxMipLevels, "TerrainCompositer::RequestTerrainTextureTiles | Invalid LOD level for SelectedNode");

                // Request height tiles from virtual texture
                AZ::Vector3 nodeMin = nodeSel.GetMin();

                // We don't need to include the positive-edge neighboring tiles (unlike heightmap streaming), so bias the node size smaller
                AZ::Vector3 nodeSize = nodeSel.GetSize() - sizeBias;

                m_VirtualTexture->RequestTiles(nodeMin.GetX(), nodeMin.GetY(), nodeSize.GetX(), nodeSize.GetY(), nodeSel.m_lodLevel);

#if defined(REPORT_REQUESTED_REGIONS)
                if (compositerRegionReport)
                {
                    AZ::Vector3 worldOrigin(0.0f);
                    AZ::Vector3 regionSize(0.0f);
                    TerrainProviderRequestBus::BroadcastResult(worldOrigin, &TerrainProviderRequestBus::Events::GetWorldOrigin);
                    TerrainProviderRequestBus::BroadcastResult(regionSize, &TerrainProviderRequestBus::Events::GetRegionSize);

                    int vTileX, vTileY;
                    m_VirtualTexture->ConvertWorldToVirtual(nodeMin.GetX(), nodeMin.GetY(), nodeSel.m_lodLevel, vTileX, vTileY);
                    AZ::Vector4 tileWorldMinMax = m_VirtualTexture->CalculateWorldBoundsForVirtualTile(vTileX, vTileY, nodeSel.m_lodLevel, true);

                    AZ::Vector3 nodeMax = nodeMin + nodeSize;

                    const int startRegionX = static_cast<int>(floor((tileWorldMinMax.GetX() - worldOrigin.GetX()) / regionSize.GetX()));
                    const int startRegionY = static_cast<int>(floor((tileWorldMinMax.GetY() - worldOrigin.GetY()) / regionSize.GetY()));
                    const int endRegionX = static_cast<int>(floor((tileWorldMinMax.GetZ() - worldOrigin.GetX()) / regionSize.GetX()));
                    const int endRegionY = static_cast<int>(floor((tileWorldMinMax.GetW() - worldOrigin.GetY()) / regionSize.GetY()));

                    for (int regionY = startRegionY; regionY <= endRegionY; ++regionY)
                    {
                        for (int regionX = startRegionX; regionX <= endRegionX; ++regionX)
                        {
                            regionSet.insert(AZStd::make_pair(regionX, regionY));
                        }
                    }
                }
#endif
            }

#if defined(REPORT_REQUESTED_REGIONS)
            if (compositerRegionReport)
            {
                AZ_Printf("TerrainCompositer", "==============================");
                AZ_Printf("TerrainCompositer", "Compositer Region Report");

                for (const AZStd::pair<int, int>& regionIndex : regionSet)
                {
                    AZ_Printf("TerrainCompositer", "[%d,%d]", regionIndex.first, regionIndex.second);
                }

                AZ_Printf("TerrainCompositer", "==============================");
            }
#endif

            m_activeRequestCount = m_VirtualTexture->GetActiveRequestCount();
        }
    }

    bool TerrainCompositer::FillMapTile(const AZStd::vector<CTexture*>& colorTextures, const Terrain::VirtualTileRequestInfo& requestInfo)
    {
        (void)colorTextures;
        return RenderTile(requestInfo);
    }

    void TerrainCompositer::RenderRequestedTiles(int maxRequests)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

        bool compositingShaderIsReady = GetTerrainTileCompositingShader() != nullptr;
        if (compositingShaderIsReady)
        {
            m_VirtualTexture->Update(maxRequests);
        }
    }

    void TerrainCompositer::ClearCompositedTextureCache()
    {
        m_VirtualTexture->ClearCache();
    }

    void TerrainCompositer::EndRendering()
    {
        m_VirtualTexture->EndRendering(eHWSC_Pixel);
    }

    #if defined(NULL_RENDERER)
    bool TerrainCompositer::RenderTile(const Terrain::VirtualTileRequestInfo& requestInfo) { return false; }
    void TerrainCompositer::RenderMaterial(IMaterial* material, const MacroMaterial& regionMacroMaterial, Vec4 quadVertices, Vec4 regionUVoffsetScale, Vec4 tileUVoffsetScale) {}
    bool TerrainCompositer::BeginRendering(const AZ::Aabb& nodeAABB) { return false;  }
    void TerrainCompositer::SetupMacroMaterial(const AZ::Aabb& nodeAABB) { }
    #endif
} // end namespace Terrain

#endif
