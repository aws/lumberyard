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

#ifndef CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_OFFGRIDLINKS_H
#define CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_OFFGRIDLINKS_H
#pragma once

#include "../MNM/MNM.h"

struct NavigationMesh;
struct IAIPathAgent;

namespace MNM
{
    //////////////////////////////////////////////////////////////////////////
    /// One of this objects is bound to every Navigation Mesh
    /// Keeps track of off-mesh links per Tile
    /// Each tile can have up to 1024 Triangle links (limited by the Tile link structure within the mesh)
    ///
    /// Some triangles in the NavigationMesh will have a special link with an index
    /// which allows to access this off-mesh data
    struct OffMeshNavigation
    {
    private:

        //////////////////////////////////////////////////////////////////////////
        //Note: This structure could hold any data for the link
        //      For the time being it will store the necessary SO info to interface with the current SO system
        struct TriangleLink
        {
            TriangleID  startTriangleID;
            TriangleID  endTriangleID;
            OffMeshLinkID   linkID;
        };
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////

        struct TileLinks
        {
            TileLinks()
                : triangleLinks(NULL)
                , triangleLinkCount(0)
            {
            }

            ~TileLinks()
            {
                SAFE_DELETE_ARRAY(triangleLinks);
            }

            void CopyLinks(TriangleLink* links, uint16 linkCount);

            TriangleLink*   triangleLinks;
            uint16          triangleLinkCount;
        };

    public:

        struct QueryLinksResult
        {
            QueryLinksResult(const TriangleLink* _firstLink, uint16 _linkCount)
                : pFirstLink(_firstLink)
                , currentLink(0)
                , linkCount(_linkCount)
            {
            }

            WayTriangleData GetNextTriangle() const
            {
                if (currentLink < linkCount)
                {
                    currentLink++;
                    return WayTriangleData(pFirstLink[currentLink - 1].endTriangleID, pFirstLink[currentLink - 1].linkID);
                }

                return WayTriangleData(0, 0);
            }

        private:
            const TriangleLink* pFirstLink;
            mutable uint16              currentLink;
            uint16              linkCount;
        };

#if DEBUG_MNM_ENABLED
        struct ProfileMemoryStats
        {
            ProfileMemoryStats()
                : offMeshTileLinksMemory(0)
                , smartObjectInfoMemory(0)
                , totalSize(0)
            {
            }

            size_t  offMeshTileLinksMemory;
            size_t  smartObjectInfoMemory;
            size_t  totalSize;
        };
#endif

        OffMeshNavigation();

        OffMeshLinkPtr AddLink(NavigationMesh& navigationMesh, const TriangleID startTriangleID, const TriangleID endTriangleID, OffMeshLink& linkData, OffMeshLinkID& linkID, bool refData);
        void RemoveLink(NavigationMesh& navigationMesh, const TriangleID boundTriangleID, const OffMeshLinkID linkID);
        void InvalidateLinks(const TileID tileID);

        QueryLinksResult GetLinksForTriangle(const TriangleID triangleID, const uint16 index) const;
        const OffMeshLink* GetObjectLinkInfo(const OffMeshLinkID linkID) const;
        OffMeshLink* GetObjectLinkInfo(const OffMeshLinkID linkID);
        QueryLinksResult GetLookupsForTriangle(const TriangleID triangleID) const;
        bool CanUseLink(IEntity* pRequester, const OffMeshLinkID linkID, float* costMultiplier, float pathSharingPenalty = 0) const;

#if DEBUG_MNM_ENABLED
        ProfileMemoryStats GetMemoryStats(ICrySizer* pSizer) const;
#endif

    private:
        void AddLookupLink(const TriangleID startTriangleID, const TriangleID endTriangleID, const OffMeshLinkID linkID);
        void RemoveLookupLink(const TriangleID boundTriangleID, const OffMeshLinkID linkID);
        bool RemoveLinkData(const OffMeshLinkID linkID);


        struct STileLinks
        {
            TileLinks links;
            TileLinks lookups;
        };

        typedef std__hash_map<TileID, STileLinks> TTilesLinks;
        typedef std__hash_map<OffMeshLinkID, OffMeshLinkPtr> TOffMeshObjectLinks;

        TTilesLinks m_tilesLinks;

        static OffMeshLinkID m_linkIDGenerator;
        TOffMeshObjectLinks m_offmeshLinks;
    };
}

#endif // CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_OFFGRIDLINKS_H
