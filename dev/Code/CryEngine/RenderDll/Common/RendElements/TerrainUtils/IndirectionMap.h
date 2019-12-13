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

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/enable_shared_from_this.h>
#include <AzCore/Jobs/JobCompletion.h>

#include "VTWrapper.h"
#include "TerrainRenderingParameters.h"

//
// The IndirectionMap class wraps management of the lookup texture for locating resident virtual
// tiles in shader.
//
// The IndirectionMapCache splits the VirtualTexture into equal sized areas, where each of these
// areas is represented by a single IndirectionMap tile of size IndirectonMapCache::c_IndirectionTileSize.
// IndirectionMapCache keeps a cache of IndirectionMap tiles, so we're not forced to allocate one large
// IndirectionMap to represent the entire world.
//
// When using the VirtaulTexture to render heightmap data or terrain base texture data, you must use
// IndirectionMapCache::GetIndirectionTileTexture(...) to locate the IndirectionMap tile required for
// looking up data in the VirtualTexture for your specified VirtualTile address
//

namespace Terrain
{
    class TileIndex
    {
    public:
        int m_x = -1;
        int m_y = -1;

        TileIndex() = default;
        TileIndex(int x, int y);

        bool operator==(const TileIndex& rhs) const;
    };
}

namespace AZStd
{
    template <>
    struct hash <Terrain::TileIndex>
    {
        inline size_t operator()(const Terrain::TileIndex& tileIndex) const
        {
            return std::hash<int>()(tileIndex.m_x) ^ (std::hash<int>()(tileIndex.m_y) << 1);
        }
    };
}

namespace Terrain
{
    class MapEntry
    {
    public:
        enum class MapEntryFlags : AZ::u8
        {
            Invalid = 0x00,
            Valid = 0xFF
        };

        AZ::u8 tileIdX = 0;
        AZ::u8 tileIdY = 0;
        AZ::u8 tileMip = 0;
        MapEntryFlags flags = MapEntryFlags::Invalid;

        inline bool IsValid() const
        {
            return (flags == MapEntryFlags::Valid);
        }
    };

    class IndirectionMap
        : public AZStd::enable_shared_from_this<IndirectionMap>
    {
    public:
        ///////////////////////////////
        // Initialization/Constructor

        // Information shared between each indirection tile
        struct IndirectionTileConfig
        {
            int m_mipLevels = 0;
            int m_bufferSize = 0;

            // Mip info
            int m_mipSizes[TerrainRenderingParameters::c_VirtualTextureMaxMipLevels] = { 0 };
            int m_mipOffsets[TerrainRenderingParameters::c_VirtualTextureMaxMipLevels] = { 0 };

            IndirectionTileConfig() = default;
            IndirectionTileConfig(int tileSize, int mipLevels);
        };

        const ETEX_Format m_textureFormat = eTF_R8G8B8A8;
        const int m_formatBpp = 4;

        IndirectionMap(const char* name, IndirectionTileConfig* sharedTileConfig);

        ///////////////////////////////
        // Setters/Getters

        TileIndex GetTileIndex()
        {
            return m_tileIndex;
        }

        void SetTileIndex(const TileIndex& tileIndex)
        {
            m_tileIndex = tileIndex;
        }

        ///////////////////////////////
        // Functionality

        void SetPixel(int x, int y, int mipLevel, const MapEntry& mapEntry);

        bool ValidateTile(int x, int y, int mipLevel);

        void Clear();

        CTexture* GetTexture();

        int GetValidEntryCount() const { return m_validEntryCount; }

        // Call before we update the indirection tile
        // Sync point for previous frame job (if it was ran)
        void Begin_MipRedirectionUpdate();

        // Call after we update the indirection map
        // Start job (if needed)
        void End_MipRedirectionUpdate();

    private:
        void UploadToGPU(int uploadSrcIndex);

        void ProcessForMipRedirection();

        int GetPixelOffset(int pixelX, int pixelY, int mipLevel) const;

        // Shared config copy
        IndirectionTileConfig m_tileConfig;

        _smart_ptr<ITexture> m_texture;

        bool m_dirty = false;
        bool m_prevFrameDirty = false;

        // Buffer with raw indirection data
        AZStd::unique_ptr<MapEntry[]> m_rawBuffer;

        // Redirection buffer
        static const int kNumDstBuffers = 2;
        AZStd::unique_ptr<MapEntry[]> m_processedBufferAlloc;
        MapEntry* m_dstBuffers[kNumDstBuffers];

        // Job-based processing of redirection data
        int m_jobDstIndex = 0;
        AZ::JobCompletion m_doneJob;

        // TileIndex for IndirectionMapCache
        TileIndex m_tileIndex;

        int m_validEntryCount = 0;
    };

    /////////////////////////////////////////////////////////////////
    // IndirectionMapCache
    class IndirectionMapCache
    {
    public:
        // NEW-TERRAIN LY-103237: handle cache and tile size more gracefully as tunable parameters
        static const int c_IndirectionTileSize = 512;

        IndirectionMapCache(const char* name, int virtualTextureSizeX, int virtualTextureSizeY, int tileSize, int mipLevels, int initialCacheSize);

        void SetPixel(int x, int y, int mipLevel, const MapEntry& mapEntry);

        void ClearPixel(int x, int y, int mipLevel);

        bool ValidateVirtualTextureTile(int x, int y, int mipLevel) const;

        // When utilizing the VirtualTexture for source data, this function is used to locate the IndirectionMap tile relevant for the specified VirtualTile address
        void GetIndirectionTileTexture(int x, int y, int mipLevel, IndirectionTileInfo& indirectionTileIndex);

        void Clear();

        ///////////////////////////////
        // Utility

        // Returns corresponding Indirection tile TileIndex
        TileIndex ResolveTileIndex(int x, int y, int mipLevel) const;

        // Returns corresponding Indirection Tile TileIndex, and virtual tile offset into the Indirection Tile as (localPixelX, localPixelY)
        TileIndex ResolveTileIndex(int x, int y, int mipLevel, int& localPixelX, int& localPixelY) const;

        // Call before we update the indirection map
        void Begin_IndirectionUpdate();

        // Call after we update the indirection map
        void End_IndirectionUpdate();

    private:

        // Util function for looking up or creating new indirection tiles as needed
        // bool forceCreate : only for initialization, forcing creation of indirection tiles in the cache
        AZStd::shared_ptr<IndirectionMap> GetOrCreateIndirectionTile(const TileIndex& tileIndex, bool forceCreate = false);

        struct CacheConfig
        {
            int m_tileCountX = 0;
            int m_tileCountY = 0;

            int m_virtualMipSizesX[TerrainRenderingParameters::c_VirtualTextureMaxMipLevels] = { 0 };
            int m_virtualMipSizesY[TerrainRenderingParameters::c_VirtualTextureMaxMipLevels] = { 0 };
        };
        CacheConfig m_cacheConfig;

        char m_name[256];

        IndirectionMap::IndirectionTileConfig m_sharedTileConfig;

        // cache
        typedef AZStd::unordered_map<TileIndex, AZStd::shared_ptr<IndirectionMap> > IndirectionTileMap;
        IndirectionTileMap m_indirectionTileMap;

        AZStd::vector<AZStd::shared_ptr<IndirectionMap> > m_indirectionTileCache;
    };
}

#endif
