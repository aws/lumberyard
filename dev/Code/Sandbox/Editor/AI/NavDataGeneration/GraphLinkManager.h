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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_GRAPHLINKMANAGER_H
#define CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_GRAPHLINKMANAGER_H
#pragma once


#include "GraphStructures.h"

class CGraphLinkManager
{
public:
    enum
    {
        BUCKET_SIZE_SHIFT = 7
    };
    enum
    {
        BUCKET_SIZE = 128
    };

    CGraphLinkManager();
    ~CGraphLinkManager();

    void Clear();

    unsigned CreateLink();
    void DestroyLink(unsigned linkIndex);

    unsigned GetNextNode(unsigned linkId) const {return GetBidirectionalData(linkId)->directionalData[GetReversed(linkId)].nextNodeIndex; }
    void SetNextNode(unsigned linkId, unsigned nodeIndex) {GetBidirectionalData(linkId)->directionalData[GetReversed(linkId)].nextNodeIndex = nodeIndex; }

    unsigned GetPrevNode(unsigned linkId) const {return GetBidirectionalData(linkId)->directionalData[GetReversed(linkId ^ 1)].nextNodeIndex; }

    unsigned GetNextLink(unsigned linkId) const {return GetBidirectionalData(linkId)->directionalData[GetReversed(linkId)].nextLinkIndex; }
    void SetNextLink(unsigned linkId, unsigned nextLink) {GetBidirectionalData(linkId)->directionalData[GetReversed(linkId)].nextLinkIndex = nextLink; }

    void SetMaxWaterDepth(unsigned linkId, float depth) {depth *= 100.0f; Limit<float>(depth, -std::numeric_limits<short>::max(), std::numeric_limits<short>::max()); GetBidirectionalData(linkId)->directionalData[1].waterDepthInCmLow = ((int)depth) & 0xFF; GetBidirectionalData(linkId)->directionalData[1].waterDepthInCmHigh = ((int)depth) >> 8; }
    void SetMinWaterDepth(unsigned linkId, float depth) {depth *= 100.0f; Limit<float>(depth, -std::numeric_limits<short>::max(), std::numeric_limits<short>::max()); GetBidirectionalData(linkId)->directionalData[0].waterDepthInCmLow = ((int)depth) & 0xFF; GetBidirectionalData(linkId)->directionalData[0].waterDepthInCmHigh = ((int)depth) >> 8; }
    float GetMaxWaterDepth(unsigned linkId) const {return (signed((GetBidirectionalData(linkId)->directionalData[1].waterDepthInCmHigh << 8) | GetBidirectionalData(linkId)->directionalData[1].waterDepthInCmLow)) * 0.01f; }
    float GetMinWaterDepth(unsigned linkId) const {return (signed((GetBidirectionalData(linkId)->directionalData[0].waterDepthInCmHigh << 8) | GetBidirectionalData(linkId)->directionalData[0].waterDepthInCmLow)) * 0.01f; }

    /// Use this when setting up the radius during creation
    void SetRadius(unsigned linkId, float radius);

    float GetRadius(unsigned linkId) const {return GetBidirectionalData(linkId)->directionalData[GetReversed(linkId)].maxRadiusInCm * 0.01f; }
    float GetOrigRadius(unsigned linkId) const {return GetBidirectionalData(linkId)->directionalData[GetReversed(linkId)].origMaxRadiusInCm * 0.01f; }

    /// Use this when modifying the radius during game
    void ModifyRadius(unsigned linkId, float radius) {int radiusInCm = (int)(radius * 100.0f); Limit<int>(radiusInCm, -std::numeric_limits<short>::max(), std::numeric_limits<short>::max()); GetBidirectionalData(linkId)->directionalData[GetReversed(linkId)].maxRadiusInCm = radiusInCm; }

    /// Indicates if a link has been changed from its original value
    bool IsLinkModified(unsigned linkId) const {return fabsf(GetRadius(linkId) - GetOrigRadius(linkId)) > 0.01f; }

    /// Restores the link radius to its original value
    void RestoreLink(unsigned linkId)
    {
        GetBidirectionalData(linkId)->directionalData[GetReversed(linkId)].maxRadiusInCm = GetBidirectionalData(linkId)->directionalData[GetReversed(linkId)].origMaxRadiusInCm;
    }

    bool IsNewLink(unsigned linkId) const
    {
        if (fabsf(GetRadius(linkId) + 2.5f) < 2.49f)
        {
            return true;
        }
        return false;
    }

    unsigned int GetStartIndex(unsigned linkId) const {return GetBidirectionalData(linkId)->directionalData[GetReversed(linkId)].startIndex; }
    void SetStartIndex(unsigned linkId, unsigned int index);
    unsigned int GetEndIndex(unsigned linkId) const {return GetBidirectionalData(linkId)->directionalData[GetReversed(linkId)].endIndex; }
    void SetEndIndex(unsigned linkId, unsigned int index);

    void SetEdgeCenter(unsigned linkId, const Vec3& center) {GetBidirectionalData(linkId)->vEdgeCenter = center; }
    Vec3& GetEdgeCenter(unsigned linkId) {return GetBidirectionalData(linkId)->vEdgeCenter; }
    const Vec3& GetEdgeCenter(unsigned linkId) const {return GetBidirectionalData(linkId)->vEdgeCenter; }

    void SetExposure(unsigned linkId, float fExposure) {GetBidirectionalData(linkId)->fExposure = fExposure; }
    float GetExposure(unsigned linkId) const {return GetBidirectionalData(linkId)->fExposure; }

    // NOTE Apr 26, 2007: <pvl> "simplicity" of a link makes sense only with
    // human waypoint links.  For criterion of simplicity see
    // CWaypointHumanNavRegion::CheckWaypointLinkWalkability().
    unsigned IsSimple(unsigned linkId) const {return GetBidirectionalData(linkId)->directionalData[0].simplePassabilityCheck; }
    void SetSimple(unsigned linkId, bool linear)
    {
        GraphLinkBidirectionalData* data = GetBidirectionalData(linkId);
        data->directionalData[0].simplePassabilityCheck = linear;
        data->directionalData[1].simplePassabilityCheck = true;
    }

    bool SimplicityComputed (unsigned linkId) const { return GetBidirectionalData(linkId)->directionalData[1].simplePassabilityCheck; }

private:
    // The low bit of the pointer is a flag - it must be masked out.
    GraphLinkBidirectionalData* GetBidirectionalData(unsigned linkId);
    const GraphLinkBidirectionalData* GetBidirectionalData(unsigned linkId) const;

    bool GetReversed(unsigned linkId) const {return (linkId & 1) != 0; }

    std::vector<GraphLinkBidirectionalData*> m_buckets;
    unsigned m_freeList;
};

inline CGraphLinkManager::CGraphLinkManager()
    :   m_freeList(0)
{
}

inline CGraphLinkManager::~CGraphLinkManager()
{
    Clear();
}

inline void CGraphLinkManager::Clear()
{
    for (int i = 0, count = int(m_buckets.size()); i < count; ++i)
    {
        delete [] m_buckets[i];
    }
    m_buckets.clear();

    m_freeList = 0;
}

inline void CGraphLinkManager::SetRadius(unsigned linkId, float radius)
{
    int radiusInCm = (int)(radius * 100.0f);
    Limit<int>(radiusInCm, -std::numeric_limits<short>::max(), std::numeric_limits<short>::max());
    // NOTE Jul 27, 2007: <pvl> radius being 0.0 seems to have a special interpretation,
    // don't let it go to zero (i.e. completely impassable) just because of
    // quantization - set it to 1cm instead.  This prevents graph consistency
    // checking from getting confused and flagging false errors.
    if (radiusInCm == 0 && radius != 0)
    {
        radiusInCm = (radius > 0);
    }
    GetBidirectionalData(linkId)->directionalData[GetReversed(linkId)].maxRadiusInCm = radiusInCm;
    GetBidirectionalData(linkId)->directionalData[GetReversed(linkId)].origMaxRadiusInCm = radiusInCm;
}


inline void CGraphLinkManager::SetStartIndex(unsigned linkId, unsigned int index)
{
    assert(index < 3 && index >= 0);
    GetBidirectionalData(linkId)->directionalData[GetReversed(linkId)].startIndex = index;
}

inline void CGraphLinkManager::SetEndIndex(unsigned linkId, unsigned int index)
{
    assert(index < 3 && index >= 0);
    GetBidirectionalData(linkId)->directionalData[GetReversed(linkId)].endIndex = index;
}

inline GraphLinkBidirectionalData* CGraphLinkManager::GetBidirectionalData(unsigned linkId)
{
    unsigned bidirectionalIndex = (linkId >> 1) - 1;
    return &m_buckets[bidirectionalIndex >> BUCKET_SIZE_SHIFT][bidirectionalIndex & (BUCKET_SIZE - 1)];
}

inline const GraphLinkBidirectionalData* CGraphLinkManager::GetBidirectionalData(unsigned linkId) const
{
    unsigned bidirectionalIndex = (linkId >> 1) - 1;
    return &m_buckets[bidirectionalIndex >> BUCKET_SIZE_SHIFT][bidirectionalIndex & (BUCKET_SIZE - 1)];
}

inline unsigned CGraphLinkManager::CreateLink()
{
    if (!m_freeList)
    {
        m_buckets.push_back(new GraphLinkBidirectionalData[BUCKET_SIZE]);
        GraphLinkBidirectionalData* pLinks = m_buckets.back();
        pLinks[0].directionalData[0].nextLinkIndex = m_freeList;
        unsigned nextLink = m_freeList;
        for (int i = 1; i < BUCKET_SIZE; ++i)
        {
            pLinks[i].directionalData[0].nextLinkIndex = nextLink;
            nextLink = ((((m_buckets.size() - 1) << BUCKET_SIZE_SHIFT) + i) + 1) << 1;
        }
        m_freeList = nextLink;
    }

    unsigned link = m_freeList;
    GraphLinkBidirectionalData* bidirectionalData = GetBidirectionalData(link);
    m_freeList = bidirectionalData->directionalData[0].nextLinkIndex;
    bidirectionalData->directionalData[0].nextLinkIndex = 0;
    return link;
}

inline void CGraphLinkManager::DestroyLink(unsigned linkIndex)
{
    linkIndex = linkIndex & ~1;
    if (GraphLinkBidirectionalData* pData = GetBidirectionalData(linkIndex))
    {
        *pData = GraphLinkBidirectionalData();
        pData->directionalData[0].nextLinkIndex = m_freeList;
        m_freeList = linkIndex;
    }
}

#endif // CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_GRAPHLINKMANAGER_H
