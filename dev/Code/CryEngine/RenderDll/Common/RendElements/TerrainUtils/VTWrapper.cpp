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

#include "VTWrapper.h"
#include "TerrainRenderingParameters.h"
#include "IndirectionMap.h"

namespace Terrain
{
    /////////////////////////////////////////////////////////////////
    // VirtualTileComparator

    bool VirtualTileComparator::operator() (VirtualTile a, VirtualTile b)
    {
        // Prioritizing lower detailed mips because we redirect to lower detailed textures

        if (b.m_mipLevel > a.m_mipLevel)
        {
            return false;
        }
        else if (b.m_mipLevel < a.m_mipLevel)
        {
            return true;
        }
        // else if mip levels are equal

        // TODO add priority for tiles near the camera
        // I could technically use the hash function I've defined below, but this is going to change anyways
        AZ::u32 aVal = (a.m_x & 0xFFFF) | ((a.m_y << 16) & 0xFFFF0000);
        AZ::u32 bVal = (b.m_x & 0xFFFF) | ((b.m_y << 16) & 0xFFFF0000);
        return bVal < aVal;
    }

    /////////////////////////////////////////////////////////////////
    // Virtual Tile

    bool VirtualTile::operator==(const VirtualTile& rhs) const
    {
        return m_x == rhs.m_x
               && m_y == rhs.m_y
               && m_mipLevel == rhs.m_mipLevel;
    }

    /////////////////////////////////////////////////////////////////
    // Physical Tile

    PhysicalTile::PhysicalTile(int x, int y)
        : m_x(x)
        , m_y(y)
    {
        m_virtualAddr.m_x = -1;
        m_virtualAddr.m_y = -1;
        m_virtualAddr.m_mipLevel = -1;
    }

    bool PhysicalTile::operator==(const PhysicalTile& rhs) const
    {
        // Ignores virtual addr
        return m_x == rhs.m_x
               && m_y == rhs.m_y;
    }

    /////////////////////////////////////////////////////////////////
    // VTWrapperDesc

    void VTWrapper::VTWrapperDesc::CalculateVirtualTextureSize(int worldSizeXMeters, int worldSizeYMeters)
    {
        double vtSizeX = ceil(m_texelsPerMeter * worldSizeXMeters);
        double vtSizeY = ceil(m_texelsPerMeter * worldSizeYMeters);

        double indirectionMapWorldTileSize = m_tileSize * m_texelsPerMeter * IndirectionMapCache::c_IndirectionTileSize;

        m_virtualTextureSizeX = static_cast<int>(ceil(vtSizeX / indirectionMapWorldTileSize) * indirectionMapWorldTileSize);
        m_virtualTextureSizeY = static_cast<int>(ceil(vtSizeY / indirectionMapWorldTileSize) * indirectionMapWorldTileSize);

        AZ_TracePrintf("VTWrapperDesc", "CalculateVirtualTextureSize() | tileSize=[%d] texelsPerMeter=[%.3f] worldSize=[%d, %d] | VtSize=[%d, %d]", m_tileSize, m_texelsPerMeter, worldSizeXMeters, worldSizeYMeters, m_virtualTextureSizeX, m_virtualTextureSizeY);
    }

    /////////////////////////////////////////////////////////////////
    // VTWrapper Constructor/Destructor

    VTWrapper::VTWrapper(const VTWrapperDesc& desc)
        : m_physicalTextureCount(desc.m_textureCount)
        , m_virtualTextureSizeX(desc.m_virtualTextureSizeX)
        , m_virtualTextureSizeY(desc.m_virtualTextureSizeY)
        , m_physicalTextureCacheSize(desc.m_physicalTextureCacheSize)
        , m_tileSize(desc.m_tileSize)
        , m_tilePadding(desc.m_tilePadding)
        , m_mipLevels(desc.m_mipLevels)
    {
        AZ_Assert((desc.m_virtualTextureSizeX % desc.m_tileSize == 0)
            && (desc.m_virtualTextureSizeY % desc.m_tileSize == 0),
            "VTWrapper | Virtual Texture Size must be divisible by Tile Size");

        AZ_Assert((desc.m_virtualTextureSizeX % (IndirectionMapCache::c_IndirectionTileSize* desc.m_tileSize) == 0)
            && (desc.m_virtualTextureSizeY % (IndirectionMapCache::c_IndirectionTileSize* desc.m_tileSize) == 0),
            "VTWrapper | Virtual Texture Size must be divisible by (IndirectionMapCache::c_IndirectionTileSize * Tile Size)");

        char texNameBuf[256];

        const int initialIndirectionTileCacheSize = desc.m_initialIndirectionTileCacheSize;

        // Create indirection map
        m_indirectionMapCachePtr = AZStd::make_unique<IndirectionMapCache>(desc.m_name, m_virtualTextureSizeX, m_virtualTextureSizeY, m_tileSize, m_mipLevels, initialIndirectionTileCacheSize);

        // Allocate physical texture cache
        for (int texIndex = 0; texIndex < m_physicalTextureCount; ++texIndex)
        {
            azsnprintf(texNameBuf, 256, "$VT_%s_%d", desc.m_name, texIndex);

            _smart_ptr<ITexture> pTex = CTexture::CreateRenderTarget(texNameBuf,
                m_physicalTextureCacheSize, m_physicalTextureCacheSize, desc.m_clearColors[texIndex], eTT_2D,
                FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_DONT_RELEASE | FT_NOMIPS,
                desc.m_formats[texIndex]);

            m_physicalTextureCache.push_back(pTex);
        }

        // Fill LRU queue data structures
        const int tileCountX = m_physicalTextureCacheSize / (m_tileSize + 2 * m_tilePadding);
        const int tileCountY = tileCountX;

        AZ_Assert(tileCountX <= 256, "VTWrapper | Tile count too large for physical texture size and tile size");

        m_physicalTileCountX = tileCountX;

        ResetLRUCache();
    }

    VTWrapper::~VTWrapper()
    {
    }

    ///////////////////////////////
    // Setters/Getters

    void VTWrapper::GetIndirectionTile(int vTileX, int vTileY, int mipLevel, IndirectionTileInfo& indirectionTileInfo)
    {
        m_indirectionMapCachePtr->GetIndirectionTileTexture(vTileX, vTileY, mipLevel, indirectionTileInfo);
    }

    int VTWrapper::GetVirtualTextureChunkSize() const
    {
        return m_tileSize * IndirectionMapCache::c_IndirectionTileSize;
    }

    ///////////////////////////////
    // VTWrapper Functionality

    void VTWrapper::ClearCache()
    {
        m_virtualTileMap.clear();

        for (int texIndex = 0; texIndex < m_physicalTextureCount; ++texIndex)
        {
            CTexture* pTex = static_cast<CTexture*>(m_physicalTextureCache[texIndex].get());
            pTex->Clear();
        }

        m_indirectionMapCachePtr->Clear();

        ResetLRUCache();
    }

    // Perform a request lookup based on the virtual addr
    // If the virtual tile exists in the cache, then the LRU cache queue is updated
    // If the virtual tile does not exist in the cache, then we queue a request
    bool VTWrapper::RequestTile(int x, int y, int mipLevel)
    {
        if (!m_indirectionMapCachePtr->ValidateVirtualTextureTile(x, y, mipLevel))
        {
#if defined(VT_VERBOSE_LOGGING)
            AZ_Warning("VTWrapper", false, "VTWrapper::RequestTile() - Invalid Virtual Tile Request - (X, Y, MipLevel) = (%d, %d, %d)", x, y, mipLevel);
#endif
            return false;
        }

        VirtualTile virtualTile;
        virtualTile.m_x = x;
        virtualTile.m_y = y;
        virtualTile.m_mipLevel = mipLevel;

        VirtualTileMap::iterator iter = m_virtualTileMap.find(virtualTile);
        if (iter != m_virtualTileMap.end())
        {
            // VirtualTile is resident in physical cache

            // We need to update the LRU Cache Queue
            // Any tiles that are accessed most recently get pushed to the back of the queue
            // * To do that, we need to find the physical tile's list iterator, then move it to the back

            // Virtual tile maps to a physical tile
            PhysicalTile& physicalTileAddr = iter->second;

            // Verify the physical tile is pointing to the virtual tile that was requested
            AZ_Assert(physicalTileAddr.m_virtualAddr == virtualTile, "VTWrapper | Cache Data structures in invalid state!");

            m_lruCacheQueue.ReferenceCacheEntry(physicalTileAddr);

            return true;
        }

        // VirtualTile is not resident

        // submit tile request (queue backed with a set, so duplicates will be disregarded)
        auto insertionResult = m_tileRequestQueue.insert(virtualTile);

#if defined(VT_VERBOSE_LOGGING)
        // if the insertion occurred, we have a unique request
        bool uniqueTileRequest = insertionResult.second;
        if (uniqueTileRequest)
        {
            AZ_TracePrintf("VTWrapper", "RequestTile() | Unique Request | [%d, %d, %d]", virtualTile.m_x, virtualTile.m_y, virtualTile.m_mipLevel);
        }
        else
        {
            AZ_TracePrintf("VTWrapper", "RequestTile() | Duplicate Request | [%d, %d, %d]", virtualTile.m_x, virtualTile.m_y, virtualTile.m_mipLevel);
        }
#else
        (void)insertionResult;
#endif

        return false;
    }

    ///////////////////////////////
    // VTWrapper Update pump

    // Process 'maxRequestsToProcess' virtual tile requests
    void VTWrapper::Update(int maxRequestsToProcess)
    {
        if (!m_tileRequestCallback)
        {
            AZ_WarningOnce("VTWrapper", false, "Attempting to update VirtualTexture with no tile request callback set!");
            return;
        }

        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

        // Indirection Map sync point
        // Wait for any processing jobs, because we need to modify the raw data buffer that the jobs are reading from
        m_indirectionMapCachePtr->Begin_IndirectionUpdate();

        int processedRequestCount = 0;

        AZStd::vector<CTexture*> physicalTextures;
        for (int indexTexture = 0; indexTexture < m_physicalTextureCount; ++indexTexture)
        {
            physicalTextures.push_back(static_cast<CTexture*>(m_physicalTextureCache[indexTexture].get()));
        }

        while (processedRequestCount < maxRequestsToProcess
               && !m_tileRequestQueue.empty())
        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Renderer, "VTWrapper.ProcessRequest");

            const auto tileRequestIter = m_tileRequestQueue.begin();
            const VirtualTile virtualTileAddrRequest = *tileRequestIter;
            m_tileRequestQueue.erase(tileRequestIter);

            // Get the next free tile and move it to the back of the LRU queue
            PhysicalTile& freeTile = m_lruCacheQueue.PeekNextLRUCacheEntry();

            const VirtualTile prevVirtualAddr = freeTile.m_virtualAddr;

            // Fill tile request
            VirtualTileRequestInfo virtualTileRequestInfo;
            virtualTileRequestInfo.m_physicalViewportDest = GetPhysicalTileViewport(freeTile);
            virtualTileRequestInfo.m_virtualTileAddress = virtualTileAddrRequest;

            bool tileRequestResult = m_tileRequestCallback(physicalTextures, virtualTileRequestInfo);
            if (tileRequestResult)
            {
                // Update entry from cache
                freeTile.m_virtualAddr = virtualTileAddrRequest;
                m_virtualTileMap[freeTile.m_virtualAddr] = freeTile;

                // Indirection map updating
                // Map virtual tile to physical tile
                MapEntry physicalTileData;
                physicalTileData.tileIdX = freeTile.m_x;
                physicalTileData.tileIdY = freeTile.m_y;
                physicalTileData.tileMip = freeTile.m_virtualAddr.m_mipLevel;
                physicalTileData.flags = MapEntry::MapEntryFlags::Valid;
                m_indirectionMapCachePtr->SetPixel(freeTile.m_virtualAddr.m_x, freeTile.m_virtualAddr.m_y, freeTile.m_virtualAddr.m_mipLevel, physicalTileData);

                // Remove old entry from the virtual tile map
                // If the entry doesn't exist, it should not matter (e.g. free physical tile)
                m_virtualTileMap.erase(prevVirtualAddr);

                // Update indirection map, old virtual entry needs to point to nothing now
                m_indirectionMapCachePtr->ClearPixel(prevVirtualAddr.m_x, prevVirtualAddr.m_y, prevVirtualAddr.m_mipLevel);

                // Reference the LRU entry so it gets moved to the back of the queue
                m_lruCacheQueue.ReferenceCacheEntry(freeTile);
            }

            ++processedRequestCount;
        }

        // Each frame, all required data is requested, whether it is resident in the cache or not. We require these requests
        // for updating LRU of data resident in the cache. So to get rid of state data request, we can clear the queue each time
        // we've processed it for the frame, and stale requests just won't be added in subsequent frames.
        m_tileRequestQueue.clear();

        // Update indirection map
        // Kicks off processing jobs for any dirty indirection tiles
        m_indirectionMapCachePtr->End_IndirectionUpdate();
    }

    ///////////////////////////////
    // VTWrapper Utility

    // Convert physical tile coordinates to a viewport that takes into account tile padding
    Viewport2D VTWrapper::GetPhysicalTileViewport(const PhysicalTile& physicalTile)
    {
        int tileWidthWithPadding = m_tileSize + 2 * m_tilePadding;
        int topLeftX = physicalTile.m_x * (tileWidthWithPadding);
        int topLeftY = physicalTile.m_y * (tileWidthWithPadding);

        int width = tileWidthWithPadding;
        int height = tileWidthWithPadding;

        AZ_Assert((topLeftX + width) <= m_physicalTextureCacheSize && (topLeftY + height) <= m_physicalTextureCacheSize,
            "VTWrapper | Calculated invalid viewport!");

        return Viewport2D(topLeftX, topLeftY, width, height);
    }

    void VTWrapper::ResetLRUCache()
    {
        m_lruCacheQueue.Clear();

        // Fill LRU queue data structures
        for (int tileIndexY = 0; tileIndexY < m_physicalTileCountX; ++tileIndexY)
        {
            for (int tileIndexX = 0; tileIndexX < m_physicalTileCountX; ++tileIndexX)
            {
                PhysicalTile physicalTile(tileIndexX, tileIndexY);
                m_lruCacheQueue.AddCacheEntry(physicalTile);
            }
        }
    }
}

#endif
