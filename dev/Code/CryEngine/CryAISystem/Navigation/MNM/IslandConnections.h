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

#ifndef CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_ISLANDCONNECTIONS_H
#define CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_ISLANDCONNECTIONS_H
#pragma once

#include "MNM.h"
#include "OpenList.h"

namespace MNM
{
    struct OffMeshNavigation;

    class IslandConnections
    {
    public:
        typedef stl::STLPoolAllocator<MNM::GlobalIslandID, stl::PoolAllocatorSynchronizationSinglethreaded> TIslandsWayAllocator;
        typedef std::list<MNM::GlobalIslandID, TIslandsWayAllocator> TIslandsWay;

        struct Link
        {
            Link(const MNM::TriangleID _toTriangleID, const MNM::OffMeshLinkID _offMeshLinkID, const MNM::GlobalIslandID _toIsland, uint32 _objectIDThatCreatesTheConnection)
                : toTriangleID(_toTriangleID)
                , offMeshLinkID(_offMeshLinkID)
                , toIsland(_toIsland)
                , objectIDThatCreatesTheConnection(_objectIDThatCreatesTheConnection)
            {
            }

            inline bool operator==(const Link& rhs) const
            {
                return toIsland == rhs.toIsland && toTriangleID == rhs.toTriangleID && offMeshLinkID == rhs.offMeshLinkID &&
                       objectIDThatCreatesTheConnection == rhs.objectIDThatCreatesTheConnection;
            }

            MNM::TriangleID     toTriangleID;
            MNM::OffMeshLinkID  offMeshLinkID;
            MNM::GlobalIslandID toIsland;
            uint32              objectIDThatCreatesTheConnection;
        };

        IslandConnections() {}

        void Reset();

        void SetOneWayConnectionBetweenIsland(const MNM::GlobalIslandID fromIsland, const Link link);
        void RemoveOneWayConnectionBetweenIsland(const MNM::GlobalIslandID fromIsland, const Link link);
        void RemoveAllIslandConnectionsForObject(const NavigationMeshID& meshID, const uint32 objectId);

        bool CanNavigateBetweenIslands(const IEntity* pEntityToTestOffGridLinks, const MNM::GlobalIslandID fromIsland, const MNM::GlobalIslandID toIsland, TIslandsWay& way) const;

#ifdef CRYAISYSTEM_DEBUG
        void DebugDraw() const;
#endif

    private:

        struct IslandNode
        {
            IslandNode()
                : id (MNM::Constants::eGlobalIsland_InvalidIslandID)
                , cost(.0f)
            {}

            IslandNode(const MNM::GlobalIslandID _id, const float _cost)
                : id(_id)
                , cost(_cost)
            {}

            IslandNode(const IslandNode& other)
                : id(other.id)
                , cost(other.cost)
            {}

            ILINE void operator=(const IslandNode& other)
            {
                id = other.id;
                cost = other.cost;
            }

            ILINE bool operator<(const IslandNode& other) const
            {
                return id < other.id;
            }

            ILINE bool operator==(const IslandNode& other) const
            {
                return id == other.id;
            }

            MNM::GlobalIslandID id;
            float cost;
        };

        struct IsNodeLessCostlyPredicate
        {
            bool operator() (const IslandNode& firstNode, const IslandNode& secondNode) { return firstNode.cost < secondNode.cost; }
        };

        typedef OpenList<IslandNode, IsNodeLessCostlyPredicate> IslandOpenList;

        typedef stl::STLPoolAllocator<std::pair<const IslandNode, IslandNode>, stl::PoolAllocatorSynchronizationSinglethreaded> TCameFromMapAllocator;
        typedef std::map<IslandNode, IslandNode, std::less<IslandNode>, TCameFromMapAllocator> TCameFromMap;
        typedef std::vector<MNM::GlobalIslandID> TIslandClosedSet;

        void ReconstructWay(const TCameFromMap& cameFromMap, const MNM::GlobalIslandID fromIsland, const MNM::GlobalIslandID toIsland, TIslandsWay& way) const;

        typedef std::vector<Link> TLinksVector;
        typedef std::map<MNM::GlobalIslandID, TLinksVector> TIslandConnectionsMap;
        TIslandConnectionsMap m_islandConnections;
    };
}

#endif // CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_ISLANDCONNECTIONS_H
