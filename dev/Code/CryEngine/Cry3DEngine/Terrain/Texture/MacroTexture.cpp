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
#include "MacroTexture.h"
#include <AzCore/Math/Color.h>
#include <AzCore/IO/Streamer.h>
#include <Terrain/Bus/LegacyTerrainBus.h>

const MacroTexture::Region MacroTexture::Region::Unit(0.0f, 0.0f, 1.0f);

namespace Morton
{
    static Key Part1By1(uint32 x)
    {
        x &= 0x0000ffff;                  // x = ---- ---- ---- ---- fedc ba98 7654 3210
        x = (x ^ (x <<  8)) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
        x = (x ^ (x <<  4)) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
        x = (x ^ (x <<  2)) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
        x = (x ^ (x <<  1)) & 0x55555555; // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
        return x;
    }

    static Key EncodeU2(uint32 x, uint32 y)
    {
        return (Part1By1(y) << 1) + Part1By1(x);
    }

    static Key EncodeF2(float x, float y, uint32 RangeX, uint32 RangeY)
    {
        uint32 ux = static_cast<uint32>(x * RangeX);
        uint32 uy = static_cast<uint32>(y * RangeY);

        // To properly represent range, the maximum value is range - 1. Otherwise wrapping
        // occurs and breaks the code. Essentially, the range is [0, Range - 1]. This means
        // a value of 1.0 is technically clamped to the last element.
        ux = min(ux, RangeX - 1);
        uy = min(uy, RangeY - 1);

        return EncodeU2(ux, uy);
    }

    Key FindCommonAncestor(Key p0, Key p1, Key p2, Key p3)
    {
        while (bool bNotAllEqual = !(p0 == p1 && p1 == p2 && p2 == p3))
        {
            p0 >>= 2;
            p1 >>= 2;
            p2 >>= 2;
            p3 >>= 2;
        }
        return p0;
    }

    uint32 GetDepth(Key key)
    {
        for (uint32 d = 0; key; ++d)
        {
            if (key == 1)
            {
                return d;
            }

            key >>= 2;
        }
        AZ_Assert(false, "Bad key, missing sentinel value");
        return 0;
    }

    Key ClampToLevel(Key key, uint32 level)
    {
        uint32 depth = GetDepth(key);
        if (depth > level)
        {
            key >>= 2 * (depth - level);
        }
        return key;
    }
}

uint32 MacroTexture::InvertTreeLevel(uint32 level) const
{
    return (level > m_TreeLevelMax) ? 0 : (m_TreeLevelMax - level);
}

Morton::Key MacroTexture::MortonEncodeRegion(Region region) const
{
    uint32 range = (1 << m_TreeLevelMax);
    uint32 sentinelBit = 1 << (2 * m_TreeLevelMax);

    Morton::Key p0 = Morton::EncodeF2(region.Left,               region.Bottom,               range, range);
    Morton::Key p1 = Morton::EncodeF2(region.Left + region.Size, region.Bottom,               range, range);
    Morton::Key p2 = Morton::EncodeF2(region.Left,               region.Bottom + region.Size, range, range);
    Morton::Key p3 = Morton::EncodeF2(region.Left + region.Size, region.Bottom + region.Size, range, range);

    p0 |= sentinelBit;
    p1 |= sentinelBit;
    p2 |= sentinelBit;
    p3 |= sentinelBit;

    return Morton::FindCommonAncestor(p0, p1, p2, p3);
}

MacroTexture::MacroTexture(const LegacyTerrain::MacroTextureConfiguration& configuration)
    : m_TexturePool(configuration.maxElementCountPerPool)
    , m_TotalSectorDataSize(configuration.totalSectorDataSize)
    , m_SectorDataStartOffset(configuration.sectorStartDataOffset)
    , m_TileSizeInPixels(configuration.tileSizeInPixels)
    , m_TreeLevelMax(0)
    , m_Timestamp(0)
    , m_Endian(configuration.endian)
    , m_Filename(configuration.filePath.c_str())
{
    const Morton::Key rootKey = 0x01;

    // Build nodes from index block information
    {
        uint16 elementsLeft = configuration.indexBlocks.size();
        const int16* indices = &configuration.indexBlocks[0];
        m_Nodes.reserve(elementsLeft);
        InitNodeTree(rootKey, Region::Unit, 0, indices, elementsLeft);
        m_Nodes.shrink_to_fit();
        AZ_Assert(elementsLeft == 0, "Failed to allocate all texture blocks through the quadtree");
    }

    m_TexturePool.Create(configuration.tileSizeInPixels, configuration.texureFormat);
}

MacroTexture::~MacroTexture()
{
    FlushTiles();
}

void MacroTexture::InitNodeTree(Morton::Key key, Region region, uint32 depth, const int16*& indices, uint16& elementsLeft)
{
    m_TreeLevelMax = max(m_TreeLevelMax, depth);

    m_NodeMortonLookup[key] = m_Nodes.size();
    m_Nodes.emplace_back(region, key, depth, *indices);

    ++indices;
    --elementsLeft;

    region.Size /= 2.0f;

    for (uint32 i = 0; i < 4; ++i)
    {
        if (*indices >= 0)
        {
            Region childRegion = region;
            childRegion.Left   += (i & 1) ? region.Size : 0.0f;
            childRegion.Bottom += (i & 2) ? region.Size : 0.0f;

            Morton::Key childKey = key;
            childKey <<= 2;
            childKey |= i;

            InitNodeTree(childKey, childRegion, depth + 1, indices, elementsLeft);
        }
        else
        {
            ++indices;
            --elementsLeft;
        }
    }
}

void MacroTexture::GetMemoryUsage(ICrySizer* sizer) const
{
    sizer->AddObject(this, sizeof(*this));
    sizer->AddObject(&m_TexturePool, sizeof(m_TexturePool));
    sizer->AddObject(m_Nodes.data(), sizeof(Node) * m_Nodes.capacity());
    sizer->AddObject(m_ActiveNodes.data(), sizeof(Node*) * m_ActiveNodes.capacity());
    sizer->AddContainer(m_NodeMortonLookup);
}

MacroTexture::Node* MacroTexture::HashTableLookup(Morton::Key key)
{
    auto it = m_NodeMortonLookup.find(key);
    if (it != m_NodeMortonLookup.end())
    {
        return &m_Nodes[it->second];
    }
    return nullptr;
}

void MacroTexture::FlushTiles()
{
    //For efficiency sake the AZ::IO::Streamer API keeps file handles open.
    //We should close the file handle of the terrain macro texture whenever this
    //class is shutting down. This is important for example when the Terrain Editor
    //needs to modify the content of the macro texture file.
    AZ::IO::Streamer::Instance().FlushCache(m_Filename.c_str());

    for (Node* node : m_ActiveNodes)
    {
        DeactivateNode(*node);
    }
    m_ActiveNodes.clear();
    AZ_Assert(m_TexturePool.GetUsedTextureCount() == 0, "Some textures got leaked");
}

void MacroTexture::RequestStreamTiles(const Region& region, uint32 maxMipLevel)
{
    Morton::Key key = MortonEncodeRegion(region);

    key = Morton::ClampToLevel(key, InvertTreeLevel(maxMipLevel));

    while (key)
    {
        if (Node* node = HashTableLookup(key))
        {
            ActivateNode(*node);
        }

        key >>= 2;
    }
}

void MacroTexture::ActivateNode(Node& node)
{
    node.timestamp = m_Timestamp;

    auto it = AZStd::find(m_ActiveNodes.begin(), m_ActiveNodes.end(), &node);
    if (it == m_ActiveNodes.end())
    {
        m_ActiveNodes.push_back(&node);
    }
}

void MacroTexture::DeactivateNode(Node& node)
{
    if (node.textureId)
    {
        m_TexturePool.Release(node.textureId);
        node.textureId = 0;
    }

    if (node.readStream)
    {
        node.readStream->Abort();
        node.readStream = nullptr;
    }

    node.distanceToFocus = 0.0f;
}

bool MacroTexture::TryGetTextureTile(const Region& region, uint32 maxMipLevel, TextureTile& outResults)
{
    Morton::Key key = MortonEncodeRegion(region);

    key = Morton::ClampToLevel(key, InvertTreeLevel(maxMipLevel));

    while (key)
    {
        Node* node = HashTableLookup(key);

        if (node && node->textureId)
        {
            outResults.mipLevel = InvertTreeLevel(node->treeLevel);
            outResults.region = node->region;
            outResults.textureId = node->textureId;
            return true;
        }

        key >>= 2;
    }

    return false;
}

bool MacroTexture::OrderByImportance(const Node* lhs, const Node* rhs)
{
    // Returns true if lhs should sort to the left of rhs (i.e. if we should swap).

    bool bInProgressL = lhs->streamingStatus == ecss_InProgress;
    bool bInProgressR = rhs->streamingStatus == ecss_InProgress;

    if (bInProgressL != bInProgressR)
    {
        bool bLeftInProgress = bInProgressL && !bInProgressR;
        return bLeftInProgress;
    }

    if (lhs->timestamp != rhs->timestamp)
    {
        return lhs->timestamp > rhs->timestamp;
    }

    if (lhs->treeLevel != rhs->treeLevel)
    {
        return lhs->treeLevel < rhs->treeLevel;
    }

    return lhs->distanceToFocus < rhs->distanceToFocus;
}

void MacroTexture::Update(uint32 maxNewStreamRequestCount, LoadPolicy loadPolicy, const Vec2* streamingFocusPoint)
{
    if (m_ActiveNodes.empty())
    {
        return;
    }

    m_TexturePool.Update();

    if (streamingFocusPoint)
    {
        for (Node* node : m_ActiveNodes)
        {
            float halfSize = node->region.Size * 0.5f;

            Vec2 nodePoint(node->region.Left + halfSize, node->region.Bottom + halfSize);

            node->distanceToFocus = nodePoint.GetDistance(*streamingFocusPoint);
        }
    }

    std::sort(m_ActiveNodes.begin(), m_ActiveNodes.end(), OrderByImportance);

    // while exceeded pool limits, cull items off the end of the list
    AZ_Assert(m_TexturePool.GetCapacity(), "Empty pool allocated");
    while (m_ActiveNodes.size() > m_TexturePool.GetCapacity())
    {
        DeactivateNode(*m_ActiveNodes.back());
        m_ActiveNodes.pop_back();
    }

    // create stream request for N more active nodes that aren't paged yet.
    uint32 streamRequestCount = 0;
    for (Node* node : m_ActiveNodes)
    {
        if (TryQueueStreamRequest(*node, loadPolicy))
        {
            ++streamRequestCount;

            if (streamRequestCount == maxNewStreamRequestCount)
            {
                break;
            }
        }
    }

    ++m_Timestamp;
}

bool MacroTexture::TryQueueStreamRequest(Node& node, LoadPolicy loadPolicy)
{
    if (node.streamingStatus != ecss_NotLoaded)
    {
        return false;
    }

    AZ_Assert(!node.readStream, "Read stream not properly cleaned up");
    AZ_Assert(m_SectorDataStartOffset && m_TotalSectorDataSize, "Invalid texture file offset.")

    StreamReadParams params;
    params.nOffset = m_SectorDataStartOffset + node.textureFileOffset * m_TotalSectorDataSize;
    params.nSize = m_TotalSectorDataSize;
    params.nLoadTime = 1000;
    params.nMaxLoadTime = 0;

    bool bFinishNow = loadPolicy == LoadPolicy::Immediate;
    if (bFinishNow)
    {
        params.ePriority = estpUrgent;
    }

    node.readStream = GetSystem()->GetStreamEngine()->StartRead(eStreamTaskTypeTerrain, m_Filename, this, &params);
    node.readStream->SetUserData(reinterpret_cast<DWORD_PTR>(&node));
    node.streamingStatus = ecss_InProgress;

    if (bFinishNow)
    {
        node.readStream->Wait();
    }
    return true;
}

void MacroTexture::StreamOnComplete(IReadStream* stream, unsigned error)
{
    Node& node = *reinterpret_cast<Node*>(stream->GetUserData());

    if (stream->IsError())
    {
        // The only acceptable error is ERROR_USER_ABORT. Any other error should assert.
        AZ_Assert(error == ERROR_USER_ABORT, "Failed to stream MacroTexture tile");
        node.streamingStatus = ecss_NotLoaded;
        node.readStream = nullptr;
        return;
    }

    node.textureId = m_TexturePool.Allocate(static_cast<const byte*>(stream->GetBuffer()), m_Endian);
    node.streamingStatus = ecss_Ready;
    node.readStream = nullptr;
}

void MacroTexture::GetTileStatistics(LegacyTerrain::MacroTexture::TileStatistics& statistics) const
{
    uint32 resident = 0;
    uint32 streaming = 0;
    uint32 waiting = 0;

    for (Node* node : m_ActiveNodes)
    {
        switch (node->streamingStatus)
        {
        case ecss_Ready:
            ++resident;
            break;

        case ecss_InProgress:
            ++streaming;
            break;

        case ecss_NotLoaded:
            ++waiting;
            break;
        }
    }

    statistics.poolCapacity = m_TexturePool.GetCapacity();
    statistics.activeTotalCount = m_ActiveNodes.size();
    statistics.residentCount = resident;
    statistics.streamingCount = streaming;
    statistics.waitingCount = waiting;
}
