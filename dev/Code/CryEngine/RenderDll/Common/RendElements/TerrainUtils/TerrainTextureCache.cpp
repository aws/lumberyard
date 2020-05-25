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

#include "TerrainTextureCache.h"
#include "VirtualTexture.h"
#include "TerrainCompositer.h"

#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include "Common/Shaders/Vertex.h"

namespace Terrain
{
    const int kPerFrameTileUpdates = 4;
    const int kPerFrameTileUpdatesDuringLoadScreen = 16;

    const int kPerFrameHeightRequests = 4;
    const int kPerFrameHeightRequestsDuringLoadScreen = 16;

    const int kPerFrameCompositerRequests = 2;
    const int kPerFrameCompositerRequestsDuringLoadScreen = 16;

    //////////////////////////////////////////////////////////////////////

    TerrainTextureCache::TerrainTextureCache()
    {
    }

    TerrainTextureCache::TerrainTextureCache(const TerrainTextureCacheDesc& terrainDesc)
        : m_terrainTextureCacheDesc(terrainDesc)
        , m_perFrameTileUpdateCount(kPerFrameTileUpdates)
        , m_perFrameHeightRequestsCount(kPerFrameHeightRequests)
        , m_perFrameCompositerRequestsCount(kPerFrameCompositerRequests)
    {
        const char* vtName = "HeightMap";

        const int physicalTextureCount = 1;

        ETEX_Format formats[physicalTextureCount];
        formats[0] = eTF_R8G8B8A8; // height and normal map

        ColorF clearColors[physicalTextureCount];
        clearColors[0] = ColorF(0.0f, 0.0f, 0.502f, 0.502f); // packed form of zero height and up normal

        AZ::Vector3 worldSize(0.0f);
        TerrainProviderRequestBus::BroadcastResult(worldSize, &TerrainProviderRequestBus::Events::GetWorldSize);
        int worldSizeXMeters = static_cast<int>(worldSize.GetX());
        int worldSizeYMeters = static_cast<int>(worldSize.GetY());

        AZ::Vector3 worldOrigin(0.0f);
        TerrainProviderRequestBus::BroadcastResult(worldOrigin, &TerrainProviderRequestBus::Events::GetWorldOrigin);
        int worldOriginX = static_cast<int>(worldOrigin.GetX());
        int worldOriginY = static_cast<int>(worldOrigin.GetY());

        float texelsPerMeter = 1.0f / terrainDesc.m_heightmapRenderParams.m_vertexSpacing;

        int tileSize = terrainDesc.m_heightmapRenderParams.m_tileSize;
        int tilePadding = TerrainRenderingParameters::c_TerrainDataTilePadding;
        int physicalTextureCacheSize = terrainDesc.m_heightmapRenderParams.m_physicalTextureCacheSize;

        VirtualTexture::VirtualTextureDesc vtDesc;
        vtDesc.m_name = "HeightMap";
        vtDesc.m_formats = &formats[0];
        vtDesc.m_clearColors = &clearColors[0];
        vtDesc.m_textureCount = physicalTextureCount;
        vtDesc.m_texelsPerMeter = texelsPerMeter;
        vtDesc.m_mipLevels = terrainDesc.m_heightmapRenderParams.m_lodLevelCount;
        vtDesc.m_tileSize = tileSize;
        vtDesc.m_tilePadding = tilePadding;
        vtDesc.m_physicalTextureCacheSize = physicalTextureCacheSize;
        vtDesc.m_worldSizeXMeters = worldSizeXMeters;
        vtDesc.m_worldSizeYMeters = worldSizeYMeters;
        vtDesc.m_worldOriginX = worldOriginX;
        vtDesc.m_worldOriginY = worldOriginY;
        vtDesc.m_initialIndirectionTileCacheSize = TerrainRenderingParameters::c_TerrainHeightmapIndirectionTileCacheSize;

        m_heightMapVirtualTexturePtr = AZStd::make_unique<VirtualTexture>(vtDesc);

        auto fpFillHeightMapTile = AZStd::bind(&TerrainTextureCache::FillHeightMapTile, this,
            AZStd::placeholders::_1,
            AZStd::placeholders::_2);
        m_heightMapVirtualTexturePtr->SetTileRequestCallback(fpFillHeightMapTile);

        m_heightMapVirtualTexturePtr->SetTextureSlots(s_IndirectionMapSlot, s_PhysicalTextureSlotStart);

        STexState heightMapSamplerState;
        heightMapSamplerState.SetFilterMode(FILTER_LINEAR);
        heightMapSamplerState.SetClampMode(TADDR_CLAMP, TADDR_CLAMP, TADDR_CLAMP);
        m_heightMapVirtualTextureSamplerStateId = CTexture::GetTexState(heightMapSamplerState);

        m_Compositer.Init(terrainDesc.m_terrainTextureParams);
    }

    void TerrainTextureCache::Init()
    {
        TerrainRendererRequestBus::Handler::BusConnect();
        HeightmapDataNotificationBus::Handler::BusConnect();
    }

    void TerrainTextureCache::Shutdown()
    {
        HeightmapDataNotificationBus::Handler::BusDisconnect();
        TerrainRendererRequestBus::Handler::BusDisconnect();
    }


    TerrainTextureCache::~TerrainTextureCache()
    {
        Shutdown();
    }

    int TerrainTextureCache::GetTerrainDataPhysicalCacheSize() const
    {
        return m_heightMapVirtualTexturePtr->GetPhysicalTextureCacheSize();
    }

    int TerrainTextureCache::GetTerrainDataTileSize() const
    {
        return m_heightMapVirtualTexturePtr->GetTileSize();
    }

    int TerrainTextureCache::GetLodLevels() const
    {
        return m_heightMapVirtualTexturePtr->GetMipLevels();
    }

    float TerrainTextureCache::GetLeafNodeVertexSpacing() const
    {
        return m_heightMapVirtualTexturePtr->GetMetersPerTexel(0);
    }

    void TerrainTextureCache::RequestTerrainDataTiles(const LODSelection& selection)
    {
        const AZ::Vector3 sizeBias(0.001f);

        TerrainProviderNotificationBus::Broadcast(&TerrainProviderNotificationBus::Events::SynchronizeSettings, this);

        const CDLODQuadTree::MapDimensions& mapDims = selection.GetQuadTree()->GetWorldMapDims();

        for (int i = 0; i < selection.GetSelectionCount(); i++)
        {
            const SelectedNode& nodeSel = selection.GetSelection()[i];

            AZ_Assert(nodeSel.m_lodLevel < m_heightMapVirtualTexturePtr->GetMipLevels(), "TerrainTextureCache::Update | Invalid LOD level for SelectedNode");

            AZ::Vector3 nodeMin = nodeSel.GetMin();

            // We need to include the positive-edge neighboring tiles, only when our node is on the positive-edge
            // Bias the size larger so our request bounds will bleed into the neighboring tile, only when our node is on the positive-edge
            AZ::Vector3 nodeSize = nodeSel.GetSize() + sizeBias;

            m_heightMapVirtualTexturePtr->RequestTiles(nodeMin.GetX(), nodeMin.GetY(), nodeSize.GetX(), nodeSize.GetY(), nodeSel.m_lodLevel);
        }

        m_activeHeightMapTileRequests = m_heightMapVirtualTexturePtr->GetActiveRequestCount();
    }

    void TerrainTextureCache::RenderUpdate()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

        // Tunable parameters for controlling how much time per frame is spent updating terrain, determined
        // as a quantity of tiles and a quantity of height requests.  If these numbers prove too difficult to
        // tune, we could consider switching to more of a time-based system.
        const int tileUpdatesPerFrame = AZ::GetMax(m_perFrameTileUpdateCount, 1);
        const int heightDataRequestsPerFrame = AZ::GetMax(m_perFrameHeightRequestsCount, 1);

        m_heightMapVirtualTexturePtr->Update(tileUpdatesPerFrame);

        // Consume a request from the extension
        ProcessHeightmapDataRequests(heightDataRequestsPerFrame);

        // Pump terrain texture compositer
        const int compositerRequestsPerFrame = AZ::GetMax(m_perFrameCompositerRequestsCount, 1);
        m_Compositer.RenderRequestedTiles(compositerRequestsPerFrame);
    }

    bool TerrainTextureCache::BeginRendering(const SelectedNode& selectedNode, bool useCompositedTerrainTextures)
    {
        const TerrainRenderingParameters* pTerrainParams = TerrainRenderingParameters::GetInstance();

        bool heightMapReady = m_heightMapVirtualTexturePtr->BeginRendering(eHWSC_Vertex,
            m_heightMapVirtualTextureSamplerStateId,
            selectedNode.m_aabb);
        bool terrainTexturesReady = true;

        if (useCompositedTerrainTextures)
        {
            m_useCompositedTerrainTextures = true;

            // Usage of the lod level from the Selected node doesn't really make sense in this case for the compositer
            // But, it's really only used to translate from world to virtual tile indices
            terrainTexturesReady = m_Compositer.BeginRendering(selectedNode.m_aabb);
        }

        bool result = heightMapReady && terrainTexturesReady;

        if (result)
        {
            static CCryNameR terrainShading1ParamsName("g_terrainShading1");
            static CCryNameR terrainMaterialParamsName("g_terrainMaterialParams");
            static CCryNameR terrainPOMParamsName("g_terrainPOMParams");

            // See TerrainSystem.cfx
            // #define TERRAIN_SYSTEM_TEXELS_PER_METER (g_terrainShading1.x)
            // #define TERRAIN_SYSTEM_MAX_ANISOTROPY (g_terrainShading1.y)
            // #define TERRAIN_SYSTEM_MAX_MIP_LEVELS (g_terrainShading1.z)
            Vec4 terrainShading1(0, 0, 0, 0);
            terrainShading1.x = static_cast<float>(m_Compositer.GetTexelsPerMeter());
            terrainShading1.y = static_cast<float>(pTerrainParams->m_cachedTerrainTextureMaxAnisotropy);
            terrainShading1.z = static_cast<float>(m_Compositer.GetMipLevels());

            AZ::Vector3 regionSize(0.0f);
            TerrainProviderRequestBus::BroadcastResult(regionSize, &TerrainProviderRequestBus::Events::GetRegionSize);

            // See TerrainSystem.cfx
            //#define TERRAIN_SYSTEM_MATERIAL_VIEW_DISTANCE (g_terrainMaterialParams.x)
            //#define TERRAIN_SYSTEM_MATERIAL_BLEND_DISTANCE (g_terrainMaterialParams.y)
            //#define TERRAIN_SYSTEM_MACRO_WORLD_WIDTH_XY (g_terrainMaterialParams.zw)
            Vec4 terrainMaterialParams(0, 0, 0, 0);
            terrainMaterialParams.x = pTerrainParams->m_terrainTextureCompositeViewDistance;
            terrainMaterialParams.y = pTerrainParams->m_terrainTextureCompositeBlendDistance;
            terrainMaterialParams.z = regionSize.GetX();
            terrainMaterialParams.w = regionSize.GetY();

            // See TerrainSystem.cfx
            // #define TERRAIN_SYSTEM_POM_HEIGHT_BIAS (g_terrainPOMParams.x)
            // #define TERRAIN_SYSTEM_POM_DISPLACEMENT (g_terrainPOMParams.y)
            // #define TERRAIN_SYSTEM_POM_SELF_SHADOW_STRENGTH (g_terrainPOMParams.z)
            Vec4 terrainPOMParams(0, 0, 0, 0);
            terrainPOMParams.x = m_terrainPOMHeightBias;
            terrainPOMParams.y = m_terrainPOMDisplacement;
            terrainPOMParams.z = m_terrainSelfShadowStrength;

            gRenDev->m_RP.m_pShader->FXSetPSFloat(terrainShading1ParamsName, &terrainShading1, 1);
            gRenDev->m_RP.m_pShader->FXSetPSFloat(terrainMaterialParamsName, &terrainMaterialParams, 1);
            gRenDev->m_RP.m_pShader->FXSetPSFloat(terrainPOMParamsName, &terrainPOMParams, 1);

            // See TerrainSystem.cfx
            // #define TERRAIN_SYSTEM_HEIGHTMAP_RANGE_MIN_MAX (g_terrainHeightmapParams.xy)
            static CCryNameR terrainHeightmapParamsName("g_terrainHeightmapParams");

            AZ::Vector2 heightRange(0.0f);
            TerrainProviderRequestBus::BroadcastResult(heightRange, &TerrainProviderRequestBus::Events::GetHeightRange);

            Vec4 terrainHeightmapParams;
            terrainHeightmapParams.x = heightRange.GetX();
            terrainHeightmapParams.y = heightRange.GetY();

            gRenDev->m_RP.m_pShader->FXSetVSFloat(terrainHeightmapParamsName, &terrainHeightmapParams, 1);

            m_Compositer.SetupMacroMaterial(selectedNode.m_aabb);
        }

        return result;
    }

    void TerrainTextureCache::EndRendering()
    {
        m_heightMapVirtualTexturePtr->EndRendering(eHWSC_Vertex);

        if (m_useCompositedTerrainTextures)
        {
            m_Compositer.EndRendering();
            m_useCompositedTerrainTextures = false;
        }
    }

    void TerrainTextureCache::ClearHeightmapCache()
    {
        m_heightMapVirtualTexturePtr->ClearCache();
    }

    void TerrainTextureCache::ClearCompositedTextureCache()
    {
        m_Compositer.ClearCompositedTextureCache();
    }

    ///////////////////////////////////////////////////
    // CRETerrainContext Impl

    void TerrainTextureCache::OnTractVersionUpdate()
    {
        // Material assets have been modified, refresh the composited material cache
        m_Compositer.ClearCompositedTextureCache();
    }

    void TerrainTextureCache::OnTerrainHeightDataChanged(const AZ::Aabb& dirtyRegion)
    {
        // If there's a valid region, just clear out the affected tiles.  Otherwise, clear the whole cache. 
        if (dirtyRegion.IsValid())
        {
            for (int lod = 0; lod < m_heightMapVirtualTexturePtr->GetMipLevels(); lod++)
            {
                m_heightMapVirtualTexturePtr->ClearTiles(dirtyRegion.GetMin().GetX(), dirtyRegion.GetMin().GetY(), dirtyRegion.GetExtents().GetX(), dirtyRegion.GetExtents().GetY(), lod);
            }
        }
        else
        {
            ClearHeightmapCache();
        }
    }

    bool TerrainTextureCache::IsReady()
    {
        AZStd::size_t activeCompositerTileRequests = m_Compositer.GetActiveTileRequestCount();

        // Soft limit to determine when ready - when request counts have fallen below a
        // reasonable threshold, the system is "ready". We could check that request counts
        // have reached zero, which would mean that everything is 100% loaded and ready,
        // but only for this specific position and view in the world. As soon as the
        // player moves their camera, we will have new requests.
        // A soft threshold better represents the readiness of the terrain renderer,
        // because it more closely matches the steady state that the system is in during
        // normal gameplay.
        bool compositerIsReady = activeCompositerTileRequests < kPerFrameCompositerRequestsDuringLoadScreen;
        bool heightMapDataReady = m_activeHeightMapTileRequests < kPerFrameTileUpdatesDuringLoadScreen;

        return compositerIsReady && heightMapDataReady;
    }

    void TerrainTextureCache::RequestTerrainTextureTiles(const LODSelection& selection)
    {
        m_Compositer.RequestTerrainTextureTiles(selection);
    }

    // Callback to fill height map tile
    bool TerrainTextureCache::FillHeightMapTile(const AZStd::vector<CTexture*>& heightAndNormal, const Terrain::VirtualTileRequestInfo& requestInfo)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

        // First, we fill the tile with procedural data
        RenderHeightMapTile(heightAndNormal, requestInfo);

        const bool includePadding = true;
        AZ::Vector4 vTileMinMax = m_heightMapVirtualTexturePtr->CalculateWorldBoundsForVirtualTile(requestInfo.m_virtualTileAddress, includePadding);

        // Next we attempt to pull the data from the terrain tile cache
        HeightmapDataRequestInfo heightMapRequest(
            requestInfo.m_physicalViewportDest.m_topLeftX,          // int viewportTopLeftX
            requestInfo.m_physicalViewportDest.m_topLeftY,          // int viewportTopLeftY
            requestInfo.m_physicalViewportDest.m_width,             // int viewportWidth
            requestInfo.m_physicalViewportDest.m_height,            // int viewportHeight
            m_heightMapVirtualTexturePtr->GetMetersPerTexel(requestInfo.m_virtualTileAddress.m_mipLevel),   // float metersPerPixel
            AZ::Vector2(vTileMinMax.GetX(), vTileMinMax.GetY()),    // AZ::Vector2 worldMin
            AZ::Vector2(vTileMinMax.GetZ(), vTileMinMax.GetW())     // AZ::Vector2 worldMax
            );

        QueueHeightmapDataRequest(heightMapRequest);

        // These can be flagged as always successful
        return true;
    }

    void TerrainTextureCache::RenderHeightMapTile(const AZStd::vector<CTexture*>& heightAndNormal, const Terrain::VirtualTileRequestInfo& requestInfo)
    {
        static CCryNameR atlasCenterAndWidthName("g_atlasCenterAndWidth");
        static CCryNameR bufferInformationName("g_bufferInformation");

        const AZ::u32 noiseTextureSlot = 0;
        const AZ::u32 heightMapTextureSlot = 1;
        const AZ::u32 tractMapTextureSlot = 2;

#if !defined(_RELEASE)

        if (heightAndNormal[0] == nullptr)
        {
            CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR, "Terrain::TerrainTextureRenderer::RenderHeightMapTile(): heightTex or normalTex is null");
            return;
        }
#endif

        // calculate world space width based on the lod level
        const bool includePadding = true;
        float metersPerTexel = m_heightMapVirtualTexturePtr->GetMetersPerTexel(requestInfo.m_virtualTileAddress.m_mipLevel);

        // returns x: x-minimum, y: y-minimum, z: x-maximum, w: y-maximum
        AZ::Vector4 virtualTileWorldBounds = m_heightMapVirtualTexturePtr->CalculateWorldBoundsForVirtualTile(requestInfo.m_virtualTileAddress, includePadding);

#if defined(VT_VERBOSE_LOGGING)
        CryLog("TerrainTextureCache::RenderHeightMapTile() | Filling virtual texture | Viewport [%d, %d, %d, %d] VTile [%d, %d, %d]",
            requestInfo.m_physicalViewportDest.m_topLeftX,
            requestInfo.m_physicalViewportDest.m_topLeftY,
            requestInfo.m_physicalViewportDest.m_width,
            requestInfo.m_physicalViewportDest.m_height,
            requestInfo.m_virtualTileAddress.m_x,
            requestInfo.m_virtualTileAddress.m_y,
            requestInfo.m_virtualTileAddress.m_mipLevel);
        CryLog("TerrainTextureCache::RenderHeightMapTile() | Filling virtual texture | World Bounds [%f, %f, %f, %f]",
            static_cast<float>(virtualTileWorldBounds.GetX()),
            static_cast<float>(virtualTileWorldBounds.GetY()),
            static_cast<float>(virtualTileWorldBounds.GetZ()),
            static_cast<float>(virtualTileWorldBounds.GetW()));
#endif

        float nodeWidth = virtualTileWorldBounds.GetZ() - virtualTileWorldBounds.GetX();
        float nodeCenterX = virtualTileWorldBounds.GetX() + nodeWidth * 0.5f;
        float nodeCenterY = virtualTileWorldBounds.GetY() + nodeWidth * 0.5f;

        // Set up these generation parameters based on the tmp buffers, then the final copy will ignore the one pixel border
        Vec4 atlasCenterAndWidth(nodeCenterX, nodeCenterY, 0.0f, nodeWidth); // used for everything except copy
        Vec4 bufferInformation(0, 0, 0, 0); // used for normals
        {
            bufferInformation.x = 1.0f / requestInfo.m_physicalViewportDest.m_width;
            bufferInformation.y = static_cast<float>(requestInfo.m_physicalViewportDest.m_width);
            bufferInformation.z = metersPerTexel; // should be equal to metersPerPixel
            bufferInformation.w = requestInfo.m_physicalViewportDest.m_width / 2.0f;
        }

        CShader* terrainHeightGeneratorShader = nullptr;
        Terrain::TerrainDataRequestBus::BroadcastResult(terrainHeightGeneratorShader, &Terrain::TerrainDataRequestBus::Events::GetTerrainHeightGeneratorShader);

        if (terrainHeightGeneratorShader && terrainHeightGeneratorShader->IsValid())
        {
            gRenDev->FX_SetState(GS_NODEPTHTEST);
            gRenDev->SetCullMode(R_CULL_NONE);
            gRenDev->FX_Commit();

            // If we're using heightmap data for the terrain, this just performs an empty fill on areas
            // so chunks with no data (yet) won't have garbage - height = 0, normal = UP.
            {
                gRenDev->FX_PushRenderTarget(0, heightAndNormal[0], nullptr, -1, false, 1);
                gRenDev->SetViewport(requestInfo.m_physicalViewportDest.m_topLeftX, requestInfo.m_physicalViewportDest.m_topLeftY,
                    requestInfo.m_physicalViewportDest.m_width, requestInfo.m_physicalViewportDest.m_height);
                gRenDev->FX_SetActiveRenderTargets();

                terrainHeightGeneratorShader->FXSetTechnique("Terrain_EmptyFill");
                uint32 numberOfPasses = 0;
                terrainHeightGeneratorShader->FXBegin(&numberOfPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
                terrainHeightGeneratorShader->FXBeginPass(0);

                gRenDev->DrawQuad(0, 0, 1, 1, ColorF(1, 1, 1, 1));

                terrainHeightGeneratorShader->FXEndPass();
                terrainHeightGeneratorShader->FXEnd();

                gRenDev->FX_PopRenderTarget(0);
            }

            gRenDev->FX_SetActiveRenderTargets();
        }
    }

    AZ::u32 TerrainTextureCache::PackHeightAndNormalAsUint(float height, AZ::Vector3 normal, float maxHeight)
    {
        struct Components
        {
            AZ::u16 height;
            AZ::u8 normalX;
            AZ::u8 normalY;
        };
        static_assert(sizeof(struct Components) == 4, "components must be 4 bytes");

        union
        {
            AZ::u32 result;
            Components components;
        } packedResult;

        // NEW-TERRAIN LY-103283: Figure out a better way to handle the float->u16 height conversion.

        // can just pack straight as an integer type - it's little endian so byte 1 is the
        // low significance bits and byte 2 is the high significant bits.
        packedResult.components.height = static_cast<AZ::u16>(height * (65536.0f / maxHeight));

        // Terrain normals are trivially packed by just dropping the Z-component - it will be
        // reconstructed in the shader. We can consider changing this if the error-level is
        // unnacceptable but with simple terrain, it's probably fine.
        packedResult.components.normalX = static_cast<uint8>(255.0f * (normal.GetX() * 0.5f + 0.5f));
        packedResult.components.normalY = static_cast<uint8>(255.0f * (normal.GetY() * 0.5f + 0.5f));

        return packedResult.result;
    }

    void TerrainTextureCache::QueueHeightmapDataRequest(const Terrain::HeightmapDataRequestInfo& heightmapDataRequest)
    {
        // request all of the tiles necessary for this height map
        // once we have all of the tiles (or none, if none are found, then call the callback and send the data to the renderer

        AZStd::lock_guard<AZStd::mutex> readyHeightmapRequestsGuard(m_readyHeightmapRequestsMutex);

        // queue request
        m_readyHeightmapRequestsQueue.emplace(heightmapDataRequest);
    }

    void TerrainTextureCache::ProcessHeightmapDataRequests(int numRequestsToHandle)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

        int requestsProcessed = 0;
        int metersPerPixelsWarningCount = 0;
        while (requestsProcessed < numRequestsToHandle)
        {
            ++requestsProcessed;

            HeightmapDataRequestInfo heightmapDataRequest;
            {
                AZStd::lock_guard<AZStd::mutex> readyHeightmapRequestsGuard(m_readyHeightmapRequestsMutex);

                if (!m_readyHeightmapRequestsQueue.empty())
                {
                    heightmapDataRequest = m_readyHeightmapRequestsQueue.front();
                    m_readyHeightmapRequestsQueue.pop();
                }
                else
                {
                    // No requests are ready
                    continue;
                }
            }

            AZ::Vector2 heightRange(0.0f);
            TerrainProviderRequestBus::BroadcastResult(heightRange, &TerrainProviderRequestBus::Events::GetHeightRange);
            float maxHeight = heightRange.GetY();

            // Make sure our working data buffers are large enough
            // * Basing the buffer size off of full viewport instead of the modified viewport
            // * This will result in less reallocation of the working buffers, as data tile size won't change during normal execution
            const AZ::Vector2 requestMinBounds = heightmapDataRequest.GetWorldMin();
            const AZ::Vector2 requestWidth = heightmapDataRequest.GetWorldWidth();

            const Terrain::Viewport2D requestViewport = heightmapDataRequest.GetViewport();
            AZ::u32 maxBufferSizeRequired = requestViewport.m_width * requestViewport.m_height;
            if (m_workBufferSize < maxBufferSizeRequired)
            {
                m_workBuffer = AZStd::make_unique<AZ::u32[]>(maxBufferSizeRequired);
                m_workBufferSize = maxBufferSizeRequired;
            }

            // Perform data copy
            for (int y = 0; y < requestViewport.m_height; y++)
            {
                for (int x = 0; x < requestViewport.m_width; x++)
                {
                    float fx = (float)(requestMinBounds.GetX() + (x * heightmapDataRequest.GetMetersPerPixel()));
                    float fy = (float)(requestMinBounds.GetY() + (y * heightmapDataRequest.GetMetersPerPixel()));
                    float heightSample = 0.0f;
                    AZ::Vector3 normalSample = AZ::Vector3::CreateAxisZ();
                    TerrainDataRequestBus::BroadcastResult(heightSample, &TerrainDataRequestBus::Events::GetHeightSynchronous, fx, fy);
                    TerrainDataRequestBus::BroadcastResult(normalSample, &TerrainDataRequestBus::Events::GetNormalSynchronous, fx, fy);

                    m_workBuffer[(y * requestViewport.m_width) + x] = PackHeightAndNormalAsUint(heightSample, normalSample, maxHeight);
                }
            }

            // Apply modified viewport to the requested viewport
            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Renderer, "TerrainTextureCache.updatePhysicalCacheTextureRegion");
                CTexture* heightAndNormalTexture = m_heightMapVirtualTexturePtr->GetPhysicalTexture(0);

                // Update texture regions
                heightAndNormalTexture->UpdateTextureRegion(reinterpret_cast<byte*>(m_workBuffer.get()),
                    requestViewport.m_topLeftX, requestViewport.m_topLeftY, 0,
                    requestViewport.m_width, requestViewport.m_height, 1,
                    eTF_R8G8B8A8);
            }
        }

        // Output error message for existence of tiles that have a meters per pixel greater than
        // the requested metersPerPixel.
        // Threshold higher than 1 can be used to reduce per frame prints.
        if (metersPerPixelsWarningCount > 1)
        {
            AZ_Error("TerrainTextureCache", false,
                "ProcessHeightmapDataRequests | Source tile meters per pixel is lower than the requested - %d occurences",
                metersPerPixelsWarningCount);
        }
    }


}

#endif
