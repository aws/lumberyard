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

#include "CryLegacy_precompiled.h"
#include "OffGridLinks.h"

#include "../NavigationSystem/NavigationSystem.h"
#include "Tile.h"

// This needs to be static because OffMeshNavigationManager expects all link ids to be
// unique, and there is one of these per navigation mesh.
MNM::OffMeshLinkID MNM::OffMeshNavigation::m_linkIDGenerator = MNM::Constants::eOffMeshLinks_InvalidOffMeshLinkID;

void MNM::OffMeshNavigation::TileLinks::CopyLinks(TriangleLink* links, uint16 linkCount)
{
    if (triangleLinkCount != linkCount)
    {
        SAFE_DELETE_ARRAY(triangleLinks);

        triangleLinkCount = linkCount;

        if (linkCount)
        {
            triangleLinks = new TriangleLink[linkCount];
        }
    }

    if (linkCount)
    {
        memcpy(triangleLinks, links, sizeof(TriangleLink) * linkCount);
    }
}

MNM::OffMeshNavigation::OffMeshNavigation()
{
}

MNM::OffMeshLinkPtr MNM::OffMeshNavigation::AddLink(NavigationMesh& navigationMesh, const TriangleID startTriangleID, const TriangleID endTriangleID, OffMeshLink& linkData, OffMeshLinkID& linkID, bool refData)
{
    // We only support 1024 links per tile
    const int kMaxTileLinks = 1024;

    //////////////////////////////////////////////////////////////////////////
    /// Figure out the tile to operate on.
    MNM::TileID tileID = MNM::ComputeTileID(startTriangleID);

    //////////////////////////////////////////////////////////////////////////
    // Attempt to find the current link data for this tile or create one if not already cached
    TileLinks* pTileLinks = NULL;
    {
        TTilesLinks::iterator tileLinksIt = m_tilesLinks.find(tileID);
        if (tileLinksIt != m_tilesLinks.end())
        {
            pTileLinks = &(tileLinksIt->second.links);
        }
        else
        {
            AZStd::pair<TTilesLinks::iterator, bool> newTileIt = m_tilesLinks.insert(TTilesLinks::value_type(tileID, STileLinks()));
            assert(newTileIt.second);

            pTileLinks = &(newTileIt.first->second.links);
        }

        assert(pTileLinks && (pTileLinks->triangleLinkCount + 1) < kMaxTileLinks);
    }

    //////////////////////////////////////////////////////////////////////////
    // Find if we have entries for this triangle, if not insert at the end
    uint16 triangleIdx = pTileLinks->triangleLinkCount;  // Default to end
    {
        for (uint16 idx = 0; idx < pTileLinks->triangleLinkCount; ++idx)
        {
            if (pTileLinks->triangleLinks[idx].startTriangleID == startTriangleID)
            {
                triangleIdx = idx;
                break;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // If a valid link ID has been passed in, use it instead of generating a new one
    // This can occur when re-adding existing links to a modified mesh.
    if (linkID == MNM::Constants::eOffMeshLinks_InvalidOffMeshLinkID)
    {
        // Generate new link id
        // NOTE: Zero is an invalid link ID
        while (++m_linkIDGenerator == MNM::Constants::eOffMeshLinks_InvalidOffMeshLinkID)
        {
            ;
        }
        linkID = m_linkIDGenerator;
    }
    // A link ID should never be larger than the latest generated
    assert(linkID <= m_linkIDGenerator);

    //////////////////////////////////////////////////////////////////////////
    // Begin insert/copy process
    TriangleLink tempTriangleLinks[kMaxTileLinks];

    // If not the first index, copy the current links up to the target index
    // into the temporary buffer
    if (triangleIdx > 0)
    {
        memcpy(tempTriangleLinks, pTileLinks->triangleLinks, sizeof(TriangleLink) * triangleIdx);
    }

    // Insert the new link
    tempTriangleLinks[triangleIdx].startTriangleID = startTriangleID;
    tempTriangleLinks[triangleIdx].endTriangleID = endTriangleID;
    tempTriangleLinks[triangleIdx].linkID = linkID;

    // Now copy the remaining links after the insert, including the one just overwritten
    const int diffCount = pTileLinks->triangleLinkCount - triangleIdx;
    if (diffCount > 0)
    {
        memcpy(&tempTriangleLinks[triangleIdx + 1], &pTileLinks->triangleLinks[triangleIdx], sizeof(TriangleLink) * diffCount);
    }

    // Now copy the update list back into the source
    pTileLinks->CopyLinks(tempTriangleLinks, pTileLinks->triangleLinkCount + 1);

    //////////////////////////////////////////////////////////////////////////
    // Now add/update the link on the tile
    // NOTE: triangleLinkCount will have been updated due to the previous copy
    if (triangleIdx < (pTileLinks->triangleLinkCount - 1))
    {
        // Update index value for all link entries for this triangle after this insert
        // since we shifted the indices with the memcpy
        TriangleID currentTriangleID = startTriangleID;
        for (uint16 idx = triangleIdx + 1; idx < pTileLinks->triangleLinkCount; ++idx)
        {
            if (pTileLinks->triangleLinks[idx].startTriangleID != currentTriangleID)
            {
                currentTriangleID = pTileLinks->triangleLinks[idx].startTriangleID;
                navigationMesh.grid.UpdateOffMeshLinkForTile(tileID, currentTriangleID, idx);
            }
        }
    }
    else
    {
        // Add the new link to the off-mesh link to the tile itself
        navigationMesh.grid.AddOffMeshLinkToTile(tileID, startTriangleID, triangleIdx);
    }

    //////////////////////////////////////////////////////////////////////////
    // Create a reverse look up
    AddLookupLink(endTriangleID, startTriangleID, linkID);

    //////////////////////////////////////////////////////////////////////////
    // Resolve and store the link data
    OffMeshLinkPtr pOffMeshLink;
    {
        if (refData)
        {
            // Reference data
            pOffMeshLink.reset(&linkData);
        }
        else
        {
            // Clone data
            pOffMeshLink.reset(linkData.Clone());
            pOffMeshLink->SetLinkID(linkID);
        }

        m_offmeshLinks.insert(TOffMeshObjectLinks::value_type(linkID, pOffMeshLink));
    }

    // Return the potentially cloned data.
    return pOffMeshLink;
}

void MNM::OffMeshNavigation::RemoveLink(NavigationMesh& navigationMesh, const TriangleID boundTriangleID, const OffMeshLinkID linkID)
{
    MNM::TileID tileID = ComputeTileID(boundTriangleID);

    TTilesLinks::iterator tileLinksIt = m_tilesLinks.find(tileID);

    if (tileLinksIt != m_tilesLinks.end())
    {
        const int maxTileLinks = 1024;
        TriangleLink tempTriangleLinks[maxTileLinks];

        TileLinks& tileLinks = tileLinksIt->second.links;

        //Note: Should be ok to copy this way, we are not going to have many links per tile...
        uint16 copyCount = 0;
        for (uint16 triIdx = 0; triIdx < tileLinks.triangleLinkCount; ++triIdx)
        {
            TriangleLink& triangleLink = tileLinks.triangleLinks[triIdx];
            if (triangleLink.linkID != linkID)
            {
                // Retain link
                tempTriangleLinks[copyCount] = triangleLink;
                copyCount++;
            }
            else
            {
                // Remove reverse look-up link
                RemoveLookupLink(triangleLink.endTriangleID, linkID);

                // Remove link data
                RemoveLinkData(linkID);
            }
        }

        // Copy the retained links to the cached link list
        tileLinks.CopyLinks(tempTriangleLinks, copyCount);

        //Update triangle off-mesh indices
        uint16 boundTriangleLeftLinks = 0;
        TriangleID currentTriangleID(0);
        for (uint16 triIdx = 0; triIdx < tileLinks.triangleLinkCount; ++triIdx)
        {
            TriangleLink& triangleLink = tileLinks.triangleLinks[triIdx];

            if (currentTriangleID != triangleLink.startTriangleID)
            {
                currentTriangleID = triangleLink.startTriangleID;
                navigationMesh.grid.UpdateOffMeshLinkForTile(tileID, currentTriangleID, triIdx);
            }
            boundTriangleLeftLinks += (currentTriangleID == boundTriangleID) ? 1 : 0;
        }

        if (!boundTriangleLeftLinks)
        {
            navigationMesh.grid.RemoveOffMeshLinkFromTile(tileID, boundTriangleID);
        }
    }
}

void MNM::OffMeshNavigation::AddLookupLink(const TriangleID startTriangleID, const TriangleID endTriangleID, const OffMeshLinkID linkID)
{
    // We only support 1024 links per tile
    const int kMaxTileLinks = 1024;

    //////////////////////////////////////////////////////////////////////////
    /// Figure out the tile to operate on.
    MNM::TileID tileID = MNM::ComputeTileID(startTriangleID);

    //////////////////////////////////////////////////////////////////////////
    // Attempt to find the current link data for this tile or create one if not already cached
    TileLinks* pTileLinks = NULL;
    {
        TTilesLinks::iterator tileLinksIt = m_tilesLinks.find(tileID);
        if (tileLinksIt != m_tilesLinks.end())
        {
            pTileLinks = &(tileLinksIt->second.lookups);
        }
        else
        {
            AZStd::pair<TTilesLinks::iterator, bool> newTileIt = m_tilesLinks.insert(TTilesLinks::value_type(tileID, STileLinks()));
            assert(newTileIt.second);

            pTileLinks = &(newTileIt.first->second.lookups);
        }

        assert(pTileLinks && (pTileLinks->triangleLinkCount + 1) < kMaxTileLinks);
    }

    //////////////////////////////////////////////////////////////////////////
    // Find if we have entries for this triangle, if not insert at the end
    uint16 triangleIdx = pTileLinks->triangleLinkCount;  // Default to end
    {
        for (uint16 idx = 0; idx < pTileLinks->triangleLinkCount; ++idx)
        {
            if (pTileLinks->triangleLinks[idx].startTriangleID == startTriangleID)
            {
                triangleIdx = idx;
                break;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Begin insert/copy process
    TriangleLink tempTriangleLinks[kMaxTileLinks];

    // If not the first index, copy the current links up to the target index
    // into the temporary buffer
    if (triangleIdx > 0)
    {
        memcpy(tempTriangleLinks, pTileLinks->triangleLinks, sizeof(TriangleLink) * triangleIdx);
    }

    // Insert the new link
    tempTriangleLinks[triangleIdx].startTriangleID = startTriangleID;
    tempTriangleLinks[triangleIdx].endTriangleID = endTriangleID;
    tempTriangleLinks[triangleIdx].linkID = linkID;

    // Now copy the remaining links after the insert, including the one just overwritten
    const int diffCount = pTileLinks->triangleLinkCount - triangleIdx;
    if (diffCount > 0)
    {
        memcpy(&tempTriangleLinks[triangleIdx + 1], &pTileLinks->triangleLinks[triangleIdx], sizeof(TriangleLink) * diffCount);
    }

    // Now copy the update list back into the source
    pTileLinks->CopyLinks(tempTriangleLinks, pTileLinks->triangleLinkCount + 1);
}

void MNM::OffMeshNavigation::RemoveLookupLink(const TriangleID boundTriangleID, const OffMeshLinkID linkID)
{
    MNM::TileID tileID = ComputeTileID(boundTriangleID);

    TTilesLinks::iterator tileLinksIt = m_tilesLinks.find(tileID);

    if (tileLinksIt != m_tilesLinks.end())
    {
        const int maxTileLinks = 1024;
        TriangleLink tempTriangleLinks[maxTileLinks];

        TileLinks& tileLinks = tileLinksIt->second.lookups;

        //Note: Should be ok to copy this way, we are not going to have many links per tile...
        uint16 copyCount = 0;
        for (uint16 triIdx = 0; triIdx < tileLinks.triangleLinkCount; ++triIdx)
        {
            TriangleLink& triangleLink = tileLinks.triangleLinks[triIdx];
            if (triangleLink.linkID != linkID)
            {
                // Retain link
                tempTriangleLinks[copyCount] = triangleLink;
                copyCount++;
            }
        }

        // Copy the retained links to the cached link list
        tileLinks.CopyLinks(tempTriangleLinks, copyCount);
    }
}

bool MNM::OffMeshNavigation::RemoveLinkData(const OffMeshLinkID linkID)
{
    TOffMeshObjectLinks::iterator it = m_offmeshLinks.find(linkID);
    if (it != m_offmeshLinks.end())
    {
        m_offmeshLinks.erase(it);
        return true;
    }

    return false;
}

void MNM::OffMeshNavigation::InvalidateLinks(const TileID tileID)
{
    // Remove all links associated with the provided tile ID

    TTilesLinks::iterator tileIt = m_tilesLinks.find(tileID);
    if (tileIt != m_tilesLinks.end())
    {
        const TileLinks& tileLinks = tileIt->second.links;
        for (uint16 triangleIdx = 0; triangleIdx < tileLinks.triangleLinkCount; ++triangleIdx)
        {
            TriangleLink& link = tileLinks.triangleLinks[triangleIdx];

            RemoveLookupLink(link.endTriangleID, link.linkID);
            RemoveLinkData(link.linkID);
        }

        m_tilesLinks.erase(tileIt);
    }
}

MNM::OffMeshNavigation::QueryLinksResult MNM::OffMeshNavigation::GetLinksForTriangle(const TriangleID triangleID, const uint16 index) const
{
    const MNM::TileID& tileID = ComputeTileID(triangleID);

    TTilesLinks::const_iterator tileCit = m_tilesLinks.find(tileID);

    if (tileCit != m_tilesLinks.end())
    {
        const TileLinks& tileLinks = tileCit->second.links;
        if (index < tileLinks.triangleLinkCount)
        {
            TriangleLink* pFirstTriangle = &tileLinks.triangleLinks[index];
            uint16 linkCount = 1;

            for (uint16 triIdx = index + 1; triIdx < tileLinks.triangleLinkCount; ++triIdx)
            {
                if (tileLinks.triangleLinks[triIdx].startTriangleID == triangleID)
                {
                    linkCount++;
                }
                else
                {
                    break;
                }
            }

            return QueryLinksResult(pFirstTriangle, linkCount);
        }
    }

    return QueryLinksResult(NULL, 0);
}

MNM::OffMeshNavigation::QueryLinksResult MNM::OffMeshNavigation::GetLookupsForTriangle(const TriangleID triangleID) const
{
    const MNM::TileID& tileID = ComputeTileID(triangleID);

    TTilesLinks::const_iterator tileCit = m_tilesLinks.find(tileID);

    if (tileCit != m_tilesLinks.end())
    {
        const TileLinks& tileLinks = tileCit->second.lookups;
        TriangleLink* pFirstTriangle = NULL;
        uint16 linkCount = 0;

        for (uint16 idx = 0; idx < tileLinks.triangleLinkCount; ++idx)
        {
            if (tileLinks.triangleLinks[idx].startTriangleID == triangleID)
            {
                if (pFirstTriangle == NULL)
                {
                    pFirstTriangle = &tileLinks.triangleLinks[idx];
                }

                ++linkCount;
            }
            else if (pFirstTriangle)
            {
                break;
            }
        }

        return QueryLinksResult(pFirstTriangle, linkCount);
    }

    return QueryLinksResult(NULL, 0);
}

const MNM::OffMeshLink* MNM::OffMeshNavigation::GetObjectLinkInfo(const OffMeshLinkID linkID) const
{
    TOffMeshObjectLinks::const_iterator linkIt = m_offmeshLinks.find(linkID);

    return (linkIt != m_offmeshLinks.end()) ? linkIt->second.get() : NULL;
}

MNM::OffMeshLink* MNM::OffMeshNavigation::GetObjectLinkInfo(const OffMeshLinkID linkID)
{
    TOffMeshObjectLinks::iterator linkIt = m_offmeshLinks.find(linkID);

    return (linkIt != m_offmeshLinks.end()) ? linkIt->second.get() : NULL;
}


bool MNM::OffMeshNavigation::CanUseLink(IEntity* pRequester, const OffMeshLinkID linkID, float* costMultiplier, float pathSharingPenalty) const
{
    // Look up the link object and allow it to test if it can be used
    // by the provided requester.
    const OffMeshLink* pOffMeshLink = GetObjectLinkInfo(linkID);
    if (pOffMeshLink && pRequester)
    {
        return pOffMeshLink->CanUse(pRequester, costMultiplier);
    }

    return false;
}

#if DEBUG_MNM_ENABLED

MNM::OffMeshNavigation::ProfileMemoryStats MNM::OffMeshNavigation::GetMemoryStats(ICrySizer* pSizer) const
{
    ProfileMemoryStats memoryStats;

    // Tile links memory
    size_t previousSize = pSizer->GetTotalSize();
    for (TTilesLinks::const_iterator tileIt = m_tilesLinks.begin(); tileIt != m_tilesLinks.end(); ++tileIt)
    {
        pSizer->AddObject(&(tileIt->second.links), tileIt->second.links.triangleLinkCount * sizeof(TriangleLink));
        pSizer->AddObject(&(tileIt->second.lookups), tileIt->second.lookups.triangleLinkCount * sizeof(TriangleLink));
    }
    pSizer->AddHashMap(m_tilesLinks);

    memoryStats.offMeshTileLinksMemory += (pSizer->GetTotalSize() - previousSize);

    //Smart object info
    previousSize = pSizer->GetTotalSize();
    pSizer->AddHashMap(m_offmeshLinks);

    memoryStats.smartObjectInfoMemory += (pSizer->GetTotalSize() - previousSize);

    //Object size
    previousSize = pSizer->GetTotalSize();
    pSizer->AddObjectSize(this);

    memoryStats.totalSize = pSizer->GetTotalSize() - previousSize;

    return memoryStats;
}

#endif
