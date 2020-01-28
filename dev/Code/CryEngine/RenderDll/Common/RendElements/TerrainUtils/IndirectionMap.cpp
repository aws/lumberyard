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

#include "IndirectionMap.h"

#include "TerrainRenderingParameters.h"

#include <AzCore/Jobs/JobManagerBus.h>

#ifndef NULL_RENDERER
#include <DeviceManager/DeviceManager.h>
#include <DriverD3D.h>
#endif

// NEW-TERRAIN LY-103236: Remove after thorough testing of improved code.
#define USE_IMPROVED_STAGING_SUPPORT 1

namespace Terrain
{
    TileIndex::TileIndex(int x, int y)
        : m_x(x)
        , m_y(y)
    {
    }

    bool TileIndex::operator==(const TileIndex& rhs) const
    {
        return m_x == rhs.m_x
               && m_y == rhs.m_y;
    }

    ///////////////////////////////
    // IndirectionMap
    // Initialization/Constructor

    IndirectionMap::IndirectionTileConfig::IndirectionTileConfig(int tileSize, int mipLevels)
        : m_mipLevels(mipLevels)
    {
        int textureBufferSize = 0;
        int mipSize = tileSize;
        for (int mipIndex = 0; mipIndex < m_mipLevels; ++mipIndex)
        {
            m_mipOffsets[mipIndex] = textureBufferSize;

            textureBufferSize += mipSize * mipSize;
            m_mipSizes[mipIndex] = mipSize;

            mipSize >>= 1;
        }

        // Initialize CPU-side buffer
        m_bufferSize = textureBufferSize;
    }

    IndirectionMap::IndirectionMap(const char* name, IndirectionMap::IndirectionTileConfig* sharedTileConfig)
        : m_tileConfig(*sharedTileConfig)
        , m_dirty(false)
        , m_validEntryCount(0)
    {
        const float mipEpsilon = 0.001f;
        AZ_Assert(sharedTileConfig->m_mipLevels < TerrainRenderingParameters::c_VirtualTextureMaxMipLevels, "IndirectionMap | Exceeded max supported mip levels");
        AZ_Assert((log2f(static_cast<float>(sharedTileConfig->m_mipSizes[0])) + 1.0f + mipEpsilon) >= sharedTileConfig->m_mipLevels, "IndirectionMap | Exceeded max supported mip levels for Indirection Map size");

        // Initialize CPU-side buffers
        m_rawBuffer = AZStd::make_unique<MapEntry[]>(m_tileConfig.m_bufferSize);
        m_processedBufferAlloc = AZStd::make_unique<MapEntry[]>(m_tileConfig.m_bufferSize * kNumDstBuffers);

        // Ptrs to double-buffered (or kNumDstBuffers-buffered) destination work buffers
        for (int i = 0; i < kNumDstBuffers; ++i)
        {
            m_dstBuffers[i] = &m_processedBufferAlloc[i * m_tileConfig.m_bufferSize];
        }

        // Create engine texture, use any buffer as we're going to overwrite this texture anyways
#if USE_IMPROVED_STAGING_SUPPORT
        m_texture = CTexture::CreateTextureObject(
            name, m_tileConfig.m_mipSizes[0], m_tileConfig.m_mipSizes[0], 1, eTT_2D, FT_DONT_RELEASE | FT_STAGE_UPLOAD, m_textureFormat, -1);
        CTexture* pTex = static_cast<CTexture*>(m_texture.get());
        bool success = pTex->Create2DTextureWithMips(m_tileConfig.m_mipSizes[0], m_tileConfig.m_mipSizes[0], m_tileConfig.m_mipLevels, FT_DONT_RELEASE | FT_STAGE_UPLOAD, nullptr, m_textureFormat, m_textureFormat);
        AZ_Assert(success, "Indirection map creation failed");
#else
        byte* byteBuffer = reinterpret_cast<byte*>(m_rawBuffer.get());
        m_texture = CTexture::Create2DTexture(
            name, m_tileConfig.m_mipSizes[0], m_tileConfig.m_mipSizes[0], m_tileConfig.m_mipLevels, FT_DONT_RELEASE, byteBuffer, m_textureFormat, m_textureFormat);
#endif


        // Create done job
        AZ::JobContext* jobContext = nullptr;
        AZ::JobManagerBus::BroadcastResult(jobContext, &AZ::JobManagerEvents::GetGlobalContext);
        new (&m_doneJob) AZ::JobCompletion(jobContext);
    }

    ///////////////////////////////
    // IndirectionMap
    // Functionality

    void IndirectionMap::SetPixel(int x, int y, int mipLevel, const MapEntry& mapEntry)
    {
        if (ValidateTile(x, y, mipLevel))
        {
            const int pixelIndex = GetPixelOffset(x, y, mipLevel);
            AZ_Assert(pixelIndex < m_tileConfig.m_bufferSize, "IndirectionMap | buffer overrun!");

            const bool isOriginalValueValid = m_rawBuffer[pixelIndex].IsValid();
            const bool isNewValueValid = mapEntry.IsValid();

            if (isOriginalValueValid
                && !isNewValueValid)
            {
                // Valid entry is now invalid
                m_validEntryCount--;
            }
            else if (!isOriginalValueValid
                     && isNewValueValid)
            {
                // Invalid entry is now valid
                m_validEntryCount++;
            }
            // else: All other cases result in no net change in valid entry count

            AZ_Assert(m_validEntryCount >= 0, "IndirectionMap::m_validEntryCount below zero!");

            m_rawBuffer[pixelIndex] = mapEntry;

            // mark dirty
            m_dirty = true;
        }
#if defined(VT_VERBOSE_LOGGING)
        else
        {
            // For this case:
            // * We may have had a request for a tile outside the representable area, because the indirection map is not large enough
            // * From initialized physical tile entries that map to the virtual tile[-1,-1]
            AZ_Warning("IndirectionMap", false, "Updating IndirectionMap with invalid pixel loc or mip level - (X, Y, MipLevel) = (%d, %d, %d)", x, y, mipLevel);
        }
#endif
    }

    bool IndirectionMap::ValidateTile(int x, int y, int mipLevel)
    {
        bool isValid = (mipLevel < m_tileConfig.m_mipLevels)
            && (x >= 0)
            && (x < m_tileConfig.m_mipSizes[mipLevel])
            && (y >= 0)
            && (y < m_tileConfig.m_mipSizes[mipLevel]);

        return isValid;
    }

    void IndirectionMap::Clear()
    {
        memset(m_rawBuffer.get(), 0, sizeof(MapEntry) * m_tileConfig.m_bufferSize);
        m_dirty = true;
        m_validEntryCount = 0;
    }

    CTexture* IndirectionMap::GetTexture()
    {
        CTexture* pCTex = static_cast<CTexture*>(m_texture.get());
        return pCTex;
    }

    void IndirectionMap::Begin_MipRedirectionUpdate()
    {
        if (m_prevFrameDirty)
        {
            // Wait for the previous frame's job to finish processing
            // (Should be already done, the work on average is around 0.3 to 0.5 ms)
            m_doneJob.StartAndWaitForCompletion();
            m_doneJob.Reset(true);

            // Send the processed results to the GPU, GPU-side copy lags up to 1 frame behind
            UploadToGPU(m_jobDstIndex);

            // Swap buffers for this frame's job
            m_jobDstIndex = (m_jobDstIndex + 1) % kNumDstBuffers;

            m_prevFrameDirty = false;
        }
    }

    void IndirectionMap::End_MipRedirectionUpdate()
    {
        if (m_dirty)
        {
            // Start work only when the map is dirty

            AZ::JobContext* jobContext = nullptr;
            AZ::JobManagerBus::BroadcastResult(jobContext, &AZ::JobManagerEvents::GetGlobalContext);

            AZStd::weak_ptr<IndirectionMap> indirectionMapTileWeakPtr = shared_from_this();

            auto processMipJobs = [indirectionMapTileWeakPtr]()
                {
                    AZStd::shared_ptr<IndirectionMap> indirectionMapTileSharedPtr = indirectionMapTileWeakPtr.lock();
                    if (indirectionMapTileSharedPtr)
                    {
                        indirectionMapTileSharedPtr->ProcessForMipRedirection();
                    }
                };

            AZ::Job* pJob = AZ::CreateJobFunction(processMipJobs, true, jobContext);
            pJob->SetDependent(&m_doneJob);
            pJob->Start();

            // Flag for the frame, to upload results on the next frame
            m_prevFrameDirty = true;

            m_dirty = false;
        }
    }

    ///////////////////////////////
    // IndirectionMap
    // Private/Utility

    void IndirectionMap::UploadToGPU(int uploadSrcIndex)
    {
#ifndef NULL_RENDERER
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

#if USE_IMPROVED_STAGING_SUPPORT
        byte* byteBuffer = reinterpret_cast<byte*>(m_dstBuffers[uploadSrcIndex]);

        CTexture* pTex = static_cast<CTexture*>(m_texture.get());
        for (int i = 0; i < m_tileConfig.m_mipLevels; i++)
        {
            int subResourceSize = CTexture::TextureDataSize(m_tileConfig.m_mipSizes[i], m_tileConfig.m_mipSizes[i], 1, 1, 1, m_textureFormat);

            auto transferFunc = [=](void* pData, uint32 rowPitch, uint32 slicePitch)
                {
                    byte* srcBase = byteBuffer + m_tileConfig.m_mipOffsets[i] * m_formatBpp;

                    if (rowPitch == m_tileConfig.m_mipSizes[i] * m_formatBpp)
                    {
                        memcpy(pData, srcBase, subResourceSize);
                    }
                    else
                    {
                        // pitch doesn't match source, copy row by row
                        int rowSizeBytes = m_tileConfig.m_mipSizes[i] * m_formatBpp;
                        for (int j = 0; j < m_tileConfig.m_mipSizes[i]; j++)
                        {
                            memcpy((byte*)pData + rowPitch * j, srcBase + rowSizeBytes * j, rowSizeBytes);
                        }
                    }
                    return true;
                };

            pTex->GetDevTexture()->AccessCurrStagingResource(i, true, transferFunc);
        }

        pTex->GetDevTexture()->UploadFromStagingResource();
#else
        SAFE_RELEASE(m_texture);
        byte* byteBuffer = reinterpret_cast<byte*>(m_dstBuffers[uploadSrcIndex]);

        CTexture* pTex = static_cast<CTexture*>(m_texture.get());
        pTex->Create2DTextureWithMips(m_tileConfig.m_mipSizes[0], m_tileConfig.m_mipSizes[0], m_tileConfig.m_mipLevels, FT_DONT_RELEASE, byteBuffer, m_textureFormat, m_textureFormat);
#endif
#endif
    }

    // LOD Fallback processing
    // We propagate physical addresses to the highest detailed mip available, starting from the lowest detailed mip.
    // There is a ptr to the current mip and a ptr to the next higher detailed mip. As we iterate over the current mip,
    // we check the corresponding 4 pixels in the next higher detailed mip. If any of the 4 pixel entries is invalid,
    // we redirect them to their analogous lower detailed entry, otherwise we copy the valid physical address info.
    void IndirectionMap::ProcessForMipRedirection()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

        // Double-buffering
        MapEntry* dstBuffer = m_dstBuffers[m_jobDstIndex];

        // First seed the redirection buffer with all pixels in the lowest detailed mip
        const int lastMipLevel = m_tileConfig.m_mipLevels - 1;
        for (int pixelY = 0; pixelY < m_tileConfig.m_mipSizes[lastMipLevel]; ++pixelY)
        {
            for (int pixelX = 0; pixelX < m_tileConfig.m_mipSizes[lastMipLevel]; ++pixelX)
            {
                int pixelIndex = GetPixelOffset(pixelX, pixelY, lastMipLevel);
                dstBuffer[pixelIndex] = m_rawBuffer[pixelIndex];
            }
        }

        // Process each mip level, propagating valid entries to higher detailed mip levels
        int currentMipLevel = m_tileConfig.m_mipLevels - 1;
        while (currentMipLevel > 0)
        {
            // next higher mip level (destination data for this mip iteration)
            const int nextMipLevel = currentMipLevel - 1;
            const int nextMipSize = m_tileConfig.m_mipSizes[nextMipLevel];

            // Must be evenly sized
            AZ_Assert((nextMipSize % 2) == 0, "Mipsize is odd!");

            // Iterating over each 2x2 pixel area in next higher detailed mip level
            for (int nextMipPixelY = 0; nextMipPixelY < nextMipSize; nextMipPixelY += 2)
            {
                for (int nextMipPixelX = 0; nextMipPixelX < nextMipSize; nextMipPixelX += 2)
                {
                    // Corresponding pixel from the lower detailed mip for this 2x2 pixel area
                    const int currentMipPixelX = (nextMipPixelX >> 1);
                    const int currentMipPixelY = (nextMipPixelY >> 1);
                    const int currentMipPixelIndex = GetPixelOffset(currentMipPixelX, currentMipPixelY, currentMipLevel);

                    // Get the entry from the processed buffer, that way we re-use results from
                    // each previously processed mip
                    const MapEntry currentMipEntry = dstBuffer[currentMipPixelIndex];

                    {
                        int nextMipPixelIndices[4];
                        nextMipPixelIndices[0] = GetPixelOffset(nextMipPixelX, nextMipPixelY, nextMipLevel);
                        nextMipPixelIndices[1] = GetPixelOffset(nextMipPixelX + 1, nextMipPixelY, nextMipLevel);
                        nextMipPixelIndices[2] = GetPixelOffset(nextMipPixelX, nextMipPixelY + 1, nextMipLevel);
                        nextMipPixelIndices[3] = GetPixelOffset(nextMipPixelX + 1, nextMipPixelY + 1, nextMipLevel);

                        for (int idx = 0; idx < 4; ++idx)
                        {
                            const MapEntry& nextMipEntry = m_rawBuffer[nextMipPixelIndices[idx]];
                            if (!nextMipEntry.IsValid())
                            {
                                // redirect to fallback mip
                                // no guarantee this is valid, but the entry is invalid anyways
                                dstBuffer[nextMipPixelIndices[idx]] = currentMipEntry;
                            }
                            else
                            {
                                // just copy
                                dstBuffer[nextMipPixelIndices[idx]] = nextMipEntry;
                            }
                        }
                    }
                }
            }

            --currentMipLevel;
        }
    }

    int IndirectionMap::GetPixelOffset(int pixelX, int pixelY, int mipLevel) const
    {
        return m_tileConfig.m_mipOffsets[mipLevel] + pixelY * m_tileConfig.m_mipSizes[mipLevel] + pixelX;
    }

    ///////////////////////////////
    // IndirectionMapCache
    // Initialization/Constructor

    IndirectionMapCache::IndirectionMapCache(const char* name, int virtualTextureSizeX, int virtualTextureSizeY, int tileSize, int mipLevels, int initialCacheSize)
    {
        m_sharedTileConfig = IndirectionMap::IndirectionTileConfig(IndirectionMapCache::c_IndirectionTileSize, mipLevels);

        int indirectionTileVirtualSize = tileSize * IndirectionMapCache::c_IndirectionTileSize;

        AZ_Assert((virtualTextureSizeX % indirectionTileVirtualSize == 0)
            && (virtualTextureSizeY % indirectionTileVirtualSize == 0),
            "IndirectionMapCache | Virtual texture size must be divisible by (tileSize * IndirectionMapCache::c_IndirectionTileSize)");

        m_cacheConfig.m_tileCountX = virtualTextureSizeX / indirectionTileVirtualSize;
        m_cacheConfig.m_tileCountY = virtualTextureSizeY / indirectionTileVirtualSize;

        int virtualMipSizeX = virtualTextureSizeX;
        int virtualMipSizeY = virtualTextureSizeY;
        for (int mipIndex = 0; mipIndex < mipLevels; ++mipIndex)
        {
            m_cacheConfig.m_virtualMipSizesX[mipIndex] = virtualMipSizeX;
            m_cacheConfig.m_virtualMipSizesY[mipIndex] = virtualMipSizeY;

            virtualMipSizeX >>= 1;
            virtualMipSizeY >>= 1;
        }

        // Setup texture name
        azsnprintf(m_name, 256, "$IndirectionMapTile_%s", name);
        const bool forceCreate = true;

        // Fill cache with indirection tile entries
        for (int currentTileIndex = 0; currentTileIndex < initialCacheSize; ++currentTileIndex)
        {
            // The tile index at this point is irrelevant
            // As the IndirectionMapCache is utilized, it will pick up the next available IndirectionTile (so any tile that has no valid references)
            // Otherwise, it will create more entries as needed

            TileIndex tileIndex(currentTileIndex, 0);
            GetOrCreateIndirectionTile(tileIndex, forceCreate);
        }
    }

    void IndirectionMapCache::SetPixel(int x, int y, int mipLevel, const MapEntry& mapEntry)
    {
        if (ValidateVirtualTextureTile(x, y, mipLevel))
        {
            // incoming coordinates are virtual tile addresses
            int localPixelX, localPixelY;
            TileIndex indirectionTileIndex = ResolveTileIndex(x, y, mipLevel, localPixelX, localPixelY);

            AZStd::shared_ptr<IndirectionMap> pIndirectionTile = GetOrCreateIndirectionTile(indirectionTileIndex);
            pIndirectionTile->SetPixel(localPixelX, localPixelY, mipLevel, mapEntry);
        }
    }

    void IndirectionMapCache::ClearPixel(int x, int y, int mipLevel)
    {
        if (ValidateVirtualTextureTile(x, y, mipLevel))
        {
            // incoming coordinates are virtual tile addresses
            int localPixelX, localPixelY;
            TileIndex indirectionTileIndex = ResolveTileIndex(x, y, mipLevel, localPixelX, localPixelY);

            // Locate indirection map and clear the entry
            IndirectionTileMap::iterator iter = m_indirectionTileMap.find(indirectionTileIndex);
            if (iter != m_indirectionTileMap.end())
            {
                AZStd::shared_ptr<IndirectionMap> indirectionTilePtr = iter->second;

                MapEntry deadEntry;
                indirectionTilePtr->SetPixel(localPixelX, localPixelY, mipLevel, deadEntry);
            }

            // When clearing, we don't care if the tile doesn't exist anymore
            // So, don't attempt to grab a new entry, and don't hint the lru cache queue
        }
    }

    bool IndirectionMapCache::ValidateVirtualTextureTile(int x, int y, int mipLevel) const
    {
        bool isValid = (mipLevel < m_sharedTileConfig.m_mipLevels)
            && (x >= 0)
            && (x < m_cacheConfig.m_virtualMipSizesX[mipLevel])
            && (y >= 0)
            && (y < m_cacheConfig.m_virtualMipSizesY[mipLevel]);

        return isValid;
    }

    void IndirectionMapCache::GetIndirectionTileTexture(int x, int y, int mipLevel, IndirectionTileInfo& indirectionTileIndex)
    {
        TileIndex tileIndex = ResolveTileIndex(x, y, mipLevel);

        indirectionTileIndex.m_vTileOffsetX = tileIndex.m_x * IndirectionMapCache::c_IndirectionTileSize;
        indirectionTileIndex.m_vTileOffsetY = tileIndex.m_y * IndirectionMapCache::c_IndirectionTileSize;

        IndirectionTileMap::iterator iter = m_indirectionTileMap.find(tileIndex);
        if (iter != m_indirectionTileMap.end())
        {
            AZStd::shared_ptr<IndirectionMap> indirectionTilePtr = iter->second;
            indirectionTileIndex.m_indirectionTilePtr = indirectionTilePtr->GetTexture();
            return;
        }

        // Indirection texture not found
        indirectionTileIndex.m_indirectionTilePtr = nullptr;
    }

    void IndirectionMapCache::Clear()
    {
        TileIndex invalidTileIndex;

        // Clear data for each of the tiles
        for (IndirectionTileMap::iterator iter = m_indirectionTileMap.begin(); iter != m_indirectionTileMap.end(); ++iter)
        {
            iter->second->Clear();

            // Invalidate the tile index
            iter->second->SetTileIndex(invalidTileIndex);
        }

        // Clear mapping table, so any requests for indirection tiles will fail
        m_indirectionTileMap.clear();
    }

    ///////////////////////////////
    // IndirectionMapCache
    // Utility

    // Returns corresponding Indirection tile TileIndex
    TileIndex IndirectionMapCache::ResolveTileIndex(int x, int y, int mipLevel) const
    {
        int tileX = x / m_sharedTileConfig.m_mipSizes[mipLevel];
        int tileY = y / m_sharedTileConfig.m_mipSizes[mipLevel];

        return TileIndex(tileX, tileY);
    }

    // Returns corresponding Indirection Tile TileIndex, and virtual tile offset into the Indirection Tile as (localPixelX, localPixelY)
    TileIndex IndirectionMapCache::ResolveTileIndex(int x, int y, int mipLevel, int& localPixelX, int& localPixelY) const
    {
        TileIndex tileIndex = ResolveTileIndex(x, y, mipLevel);

        localPixelX = x - tileIndex.m_x * m_sharedTileConfig.m_mipSizes[mipLevel];
        localPixelY = y - tileIndex.m_y * m_sharedTileConfig.m_mipSizes[mipLevel];

        return tileIndex;
    }

    void IndirectionMapCache::Begin_IndirectionUpdate()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

        for (auto currMapEntryPair : m_indirectionTileMap)
        {
            AZStd::shared_ptr<IndirectionMap> currIndirectionTile = currMapEntryPair.second;
            currIndirectionTile->Begin_MipRedirectionUpdate();
        }
    }

    void IndirectionMapCache::End_IndirectionUpdate()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

        for (auto currMapEntryPair : m_indirectionTileMap)
        {
            AZStd::shared_ptr<IndirectionMap> currIndirectionTile = currMapEntryPair.second;
            currIndirectionTile->End_MipRedirectionUpdate();
        }
    }

    AZStd::shared_ptr<IndirectionMap> IndirectionMapCache::GetOrCreateIndirectionTile(const TileIndex& tileIndex, bool forceCreate)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

        // Locate indirection map and clear the entry
        IndirectionTileMap::iterator iter = m_indirectionTileMap.find(tileIndex);
        if (iter != m_indirectionTileMap.end())
        {
            AZStd::shared_ptr<IndirectionMap> indirectionTilePtr = iter->second;
            return indirectionTilePtr;
        }
        else
        {
            // The indirection tile doesn't exist, so get a new cache entry
            AZStd::shared_ptr<IndirectionMap> freeIndirectionTile = nullptr;

            if (!forceCreate)
            {
                for (AZStd::shared_ptr<IndirectionMap> currIndirectionTile : m_indirectionTileCache)
                {
                    if (currIndirectionTile->GetValidEntryCount() == 0)
                    {
                        // Found an empty tile
                        freeIndirectionTile = currIndirectionTile;
                        break;
                    }
                }
            }

            if (!freeIndirectionTile)
            {
                // Still haven't found a tile, so create one

                size_t currentTileIndex = m_indirectionTileCache.size();

                char tileName[256];
                azsnprintf(tileName, 256, "%s_%d", m_name, static_cast<int>(currentTileIndex));
                freeIndirectionTile = AZStd::make_shared<IndirectionMap>(tileName, &m_sharedTileConfig);

                m_indirectionTileCache.push_back(freeIndirectionTile);
            }

            // Clear the old cache entry
            TileIndex oldTileIndex = freeIndirectionTile->GetTileIndex();
            m_indirectionTileMap.erase(oldTileIndex);

            // Update TileIndex -> IndirectionTile map
            m_indirectionTileMap[tileIndex] = freeIndirectionTile;

            freeIndirectionTile->Clear();
            freeIndirectionTile->SetTileIndex(tileIndex);
            return freeIndirectionTile;
        }
    }
}

#endif
