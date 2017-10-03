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

#pragma once

#include "TexturePool.h"
#include "../../Cry3DEngineBase.h"
#include <IStreamEngine.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/string.h>

namespace Morton
{
    using Key = uint32;
}

struct MacroTextureConfiguration
{
    AZStd::vector<int16> indexBlocks;
    AZStd::string filePath = "";
    uint32 maxElementCountPerPool = 0x1000;
    uint32 totalSectorDataSize = 0;
    uint32 tileSizeInPixels = 0;
    uint32 sectorStartDataOffset = 0;
    float colorMultiplier_deprecated = 1.0f;
    ETEX_Format texureFormat = eTF_Unknown;
    bool endian = eLittleEndian;
};

/*!
 * A runtime class for a tiled, quadtree-based megatexture format. This implementation
 * loads a texture file and pages in and out tiles based on requested regions in UV space.
 * Texture tiles are pooled and can be resolved based on regions to a concrete, resident texture tile.
 * Higher mip levels are returned on a cache miss.
 */
class MacroTexture
    : private Cry3DEngineBase
    , public IStreamCallback
{
public:

    MacroTexture(const MacroTextureConfiguration& configuration);
    ~MacroTexture();

    using UniquePtr = AZStd::unique_ptr<MacroTexture>;

    //! Creates a new instance from a file on disk.
    static MacroTexture::UniquePtr Create(const char* filepath, uint32 maxElementCountPerPool);

    //! Region defines a square in UV space for tile resolve / streaming operations. All
    //! units are in [0, 1] parametric UV space.
    struct Region
    {
        static const Region Unit;

        Region() {}
        Region(float bottom, float left, float size)
            : Bottom(bottom)
            , Left(left)
            , Size(size)
        {}

        float Bottom;
        float Left;
        float Size;
    };

    enum class LoadPolicy
    {
        Stream,
        Immediate
    };

    //! A resolved, resident texture tile returned by the runtime.
    struct TextureTile
    {
        //! The region mapped to this texture tile.
        Region region;

        //! The renderer texture id of this texture tile.
        uint32 textureId;

        //! The mip level [0, N] (0 = Lowest Res, N = Highest Res) of this texture tile within the macro texture.
        uint32 mipLevel;
    };

    //! Current statistics of the internal streaming engine and texture pools.
    struct TileStatistics
    {
        //! Total number of textures available.
        uint32 poolCapacity;

        //! Total texture tiles requested for paging.
        uint32 activeTotalCount;

        //! Total texture tiles resident in memory.
        uint32 residentCount;

        //! Total texture tiles currently streaming from disk.
        uint32 streamingCount;

        //! Total texture tiles waiting to be streamed (requires eviction).
        uint32 waitingCount;
    };

    //! Flush all tiles from memory and reset the texture pool.
    void FlushTiles();

    //! Make resident all texture tiles touching the given region, including the entire mip chain clamped
    //! to the minimum mip level supplied.
    void RequestStreamTiles(const Region& region, uint32 minMipLevel);

    //! Request a single texture tile that fully contains the requested region, clamped to the minimum
    //! mip level supplied. If higher mip levels are not resident in memory, this method searches the mip
    //! chain to find a lower resolution tile. If none of the tiles are resident, this method will return false.
    bool TryGetTextureTile(const Region& region, uint32 minMipLevel, TextureTile& outResults);

    //! Updates internal caches and stream requests.
    //!     @param[in] maxNewStreamRequestCount     Clamps to a fixed number of new stream requests this cycle.
    //!     @param[in] loadPolicy                   Dictates how newly requested tiles should be loaded.
    //!     @param[in] streamingFocusPoint          An optional [0, 1]x[0, 1] UV position which is used to
    //!                                             prioritize new stream requests based on distance.
    void Update(uint32 maxNewStreamRequestCount, LoadPolicy loadPolicy, const Vec2* streamingFocusPoint);

    void GetTileStatistics(TileStatistics& statistics) const;

    void GetMemoryUsage(ICrySizer* sizer) const;

    inline uint32 GetTileSizeInPixels() const
    {
        return m_TileSizeInPixels;
    }

private:

    MacroTexture(const MacroTexture& other) AZ_DELETE_METHOD;
    MacroTexture& operator=(const MacroTexture& other) AZ_DELETE_METHOD;
	
    void InitNodeTree(Morton::Key key, Region region, uint32 depth, const int16*& indices, uint16& elementsLeft);

    struct Node
    {
        Node(const Region& region_, uint32 mortonKey_, uint32 treeLevel_, uint32 textureFileOffset_)
            : region(region_)
            , mortonKey(mortonKey_)
            , treeLevel(treeLevel_)
            , textureFileOffset(textureFileOffset_)
            , textureId(0)
            , timestamp(0)
            , distanceToFocus(0.0f)
            , streamingStatus(ecss_NotLoaded)
            , readStream(nullptr)
        {}

        const Region region;
        const uint32 mortonKey;
        const uint32 treeLevel;
        const uint32 textureFileOffset;

        uint32 textureId;
        uint32 timestamp;
        float distanceToFocus;
        EFileStreamingStatus streamingStatus;
        IReadStreamPtr readStream;
    };

    Node* HashTableLookup(Morton::Key key);

    uint32 InvertTreeLevel(uint32 level) const;

    Morton::Key MortonEncodeRegion(Region region) const;

    void ActivateNode(Node& node);
    void DeactivateNode(Node& node);

    bool TryQueueStreamRequest(Node& node, LoadPolicy loadPolicy);
    virtual void StreamOnComplete(IReadStream* stream, unsigned error) override;

    uint32 m_TotalSectorDataSize;
    uint32 m_SectorDataStartOffset;
    uint32 m_TileSizeInPixels;
    uint32 m_TreeLevelMax;
    uint32 m_Timestamp;
    EEndian m_Endian;
    string m_Filename;

    AZStd::vector<Node> m_Nodes;
    AZStd::unordered_map<Morton::Key, uint32> m_NodeMortonLookup;
    AZStd::vector<Node*> m_ActiveNodes;

    TexturePool m_TexturePool;

    static bool OrderByImportance(const Node* lhs, const Node* rhs);
};