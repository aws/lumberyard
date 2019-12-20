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

#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>
#include <Terrain/Bus/TerrainProviderBus.h>

#include "LRUCacheQueue.h"

//#define VT_VERBOSE_LOGGING

//
// VTWrapper.h defines a set of utility classes that handle texture management for Terrain::VirtualTexture (see VirtualTexture.h)
//
// Terrain::VTWrapper
// Wraps texture management for a VirtualTexture cache, where tiles are addressed by tile index.
// The tiles are indexed like addressing individual pixel indices within each mipmap.
//
// Terrain::VTWrapper manages a virtual tile page table, mapping each virtual tile resident in
// cache to their respective physical tiles.
//

namespace Terrain
{
    // VirtualTile: a virtual tile address
    class VirtualTile
    {
    public:
        int m_x = -1;
        int m_y = -1;
        int m_mipLevel = -1;

        VirtualTile() = default;

        bool operator==(const VirtualTile& rhs) const;
    };

    struct VirtualTileComparator
    {
        bool operator() (VirtualTile a, VirtualTile b);
    };

    class PhysicalTile
    {
    public:
        int m_x = -1;
        int m_y = -1;

        VirtualTile m_virtualAddr;

        PhysicalTile() = default;
        PhysicalTile(int x, int y);

        bool operator==(const PhysicalTile& rhs) const;
    };

    struct IndirectionTileInfo
    {
        CTexture* m_indirectionTilePtr = nullptr;
        int m_vTileOffsetX = -1;
        int m_vTileOffsetY = -1;
    };
}

// TileIndex, VirtualTile, and PhysicalTile hashing
namespace AZStd
{
    template <>
    struct hash <Terrain::VirtualTile>
    {
        inline size_t operator()(const Terrain::VirtualTile& vTileAddr) const
        {
            AZ::u64 mip = (static_cast<AZ::u64>(vTileAddr.m_mipLevel) << 60) & 0xF000000000000000;    // mip level top 4 bits
            AZ::u64 tileX = static_cast<AZ::u64>(vTileAddr.m_x) & 0x000000003FFFFFFF;                 // x lower bits 29 to 0
            AZ::u64 tileY = (static_cast<AZ::u64>(vTileAddr.m_y) << 14) & 0x0FFFFFFFC0000000;         // y higher bits 59 to 30

            return std::hash<AZ::u64>{} (mip | tileX | tileY);
        }
    };

    template <>
    struct hash <Terrain::PhysicalTile>
    {
        inline size_t operator()(const Terrain::PhysicalTile& pTileAddr) const
        {
            AZ::u32 tileX = static_cast<AZ::u32>(pTileAddr.m_x) & 0x0000FFFF;         // x lower bits 15 to 0
            AZ::u32 tileY = (static_cast<AZ::u32>(pTileAddr.m_y) << 16) & 0xFFFF0000;   // y higher bits 32 to 16

            return std::hash<AZ::u32>{} (tileX | tileY);
        }
    };
}

namespace Terrain
{
    // Forward declaration
    class IndirectionMapCache;

    class VirtualTileRequestInfo
    {
    public:
        Viewport2D m_physicalViewportDest;
        VirtualTile m_virtualTileAddress;

        VirtualTileRequestInfo() = default;
    };

    typedef AZStd::function<bool(const AZStd::vector<CTexture*>& physicalTextures, const VirtualTileRequestInfo& virtualTileRequestInfo)> TileRequestCallback;

    class VTWrapper
    {
    public:
        struct VTWrapperDesc
        {
            const char* m_name = nullptr;
            ETEX_Format* m_formats = nullptr;
            ColorF* m_clearColors = nullptr;
            int m_textureCount = 1;
            float m_texelsPerMeter = 1.0f;
            int m_virtualTextureSizeX = 2048;
            int m_virtualTextureSizeY = 2048;
            int m_mipLevels = 1;
            int m_tileSize = 64;
            int m_tilePadding = 1;
            int m_physicalTextureCacheSize = 2048;
            int m_initialIndirectionTileCacheSize = 1;

            void CalculateVirtualTextureSize(int worldSizeXMeters, int worldSizeYMeters);
        };

        VTWrapper(const VTWrapperDesc& desc);

        ~VTWrapper();

        ///////////////////////////////
        // Setters/Getters

        CTexture* GetPhysicalTexture(int index)
        {
            return static_cast<CTexture*>(m_physicalTextureCache[index].get());
        }

        void GetIndirectionTile(int vTileX, int vTileY, int mipLevel, IndirectionTileInfo& indirectionTileInfo);

        int GetMipLevels() const
        {
            return m_mipLevels;
        }


        int GetTileSize() const
        {
            return m_tileSize;
        }

        int GetTilePadding() const
        {
            return m_tilePadding;
        }

        int GetVirtualTextureSizeX() const
        {
            return m_virtualTextureSizeX;
        }

        int GetVirtualTextureSizeY() const
        {
            return m_virtualTextureSizeY;
        }

        int GetPhysicalTextureCacheSize() const
        {
            return m_physicalTextureCacheSize;
        }

        int GetPhysicalTextureCount() const
        {
            return m_physicalTextureCount;
        }

        // Returns the size of the virtual texture represented by a single indirection tile
        int GetVirtualTextureChunkSize() const;

        void SetTileRequestCallback(TileRequestCallback funcPtr)
        {
            m_tileRequestCallback = funcPtr;
        }

        AZStd::size_t GetActiveRequestCount() const
        {
            return m_tileRequestQueue.size();
        }

        ///////////////////////////////
        // Functionality

        void ClearCache();

        // Perform a request lookup based on the virtual addr
        // If the virtual tile exists in the cache, then the LRU cache queue is updated
        // If the virtual tile does not exist in the cache, then we queue a request
        bool RequestTile(int x, int y, int mipLevel);

        ///////////////////////////////
        // Update pump

        // Process 'maxRequestsToProcess' virtual tile requests
        void Update(int maxRequestsToProcess);

        ///////////////////////////////
        // Utility

        // Convert physical tile coordinates to a viewport that takes into account tile padding
        Viewport2D GetPhysicalTileViewport(const PhysicalTile& physicalTile);

    protected:
        void ResetLRUCache();

        // Virtualized texture size
        int m_virtualTextureSizeX;
        int m_virtualTextureSizeY;
        int m_mipLevels;

        // Physical cache
        int m_physicalTextureCacheSize;
        int m_physicalTileCountX;
        int m_physicalTextureCount;
        AZStd::vector<_smart_ptr<ITexture> > m_physicalTextureCache;

        // Tile
        int m_tileSize;
        int m_tilePadding;

        // Indirection map
        AZStd::unique_ptr<IndirectionMapCache> m_indirectionMapCachePtr;

        // Page table: mapping virtual tiles to their respective physical tile address
        typedef AZStd::unordered_map<VirtualTile, PhysicalTile> VirtualTileMap;
        VirtualTileMap m_virtualTileMap;

        LRUCacheQueue<PhysicalTile> m_lruCacheQueue;

        // VirtualTile request queue
        AZStd::set<VirtualTile, VirtualTileComparator> m_tileRequestQueue;

        // Request callback
        TileRequestCallback m_tileRequestCallback;
    };
}

#endif
