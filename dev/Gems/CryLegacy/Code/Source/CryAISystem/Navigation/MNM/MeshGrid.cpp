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
#include "MeshGrid.h"
#include "OffGridLinks.h"
#include "Tile.h"
#include "IslandConnections.h"

#if defined(min)
#undef min
#endif
#if defined(max)
#undef max
#endif

//#pragma optimize("", off)
//#pragma inline_depth(0)

namespace MNM
{
    bool MeshGrid::WayQueryRequest::CanUseOffMeshLink(const OffMeshLinkID linkID, float* costMultiplier) const
    {
        if (m_pRequester)
        {
            if (IEntity* pEntity = m_pRequester->GetPathAgentEntity())
            {
                return GetOffMeshNavigation().CanUseLink(pEntity, linkID, costMultiplier);
            }
        }

        return true;  // Always allow by default
    }

    bool MeshGrid::WayQueryRequest::IsPointValidForAgent(const Vec3& pos, uint32 flags) const
    {
        if (m_pRequester)
        {
            return m_pRequester->IsPointValidForAgent(pos, flags);
        }

        return true;  // Always allow by default
    }

    const real_t MeshGrid::kMinPullingThreshold = real_t(0.05f);
    const real_t MeshGrid::kMaxPullingThreshold = real_t(0.95f);
    const real_t MeshGrid::kAdjecencyCalculationToleranceSq = square(real_t(0.02f));

    static const int NeighbourOffset_MeshGrid[14][3] =
    {
        { 1, 0, 0},
        { 1, 0, 1},
        { 1, 0, -1},

        { 0, 1, 0},
        { 0, 1, 1},
        { 0, 1, -1},

        { 0, 0, 1},

        {-1, 0, 0},
        {-1, 0, -1},
        {-1, 0, 1},

        { 0, -1, 0},
        { 0, -1, -1},
        { 0, -1, 1},

        { 0, 0, -1},
    };

    MeshGrid::MeshGrid()
        : m_tiles(0)
        , m_tileCount(0)
        , m_tileCapacity(0)
        , m_triangleCount(0)
    {
        m_islands.reserve(32);
    }

    MeshGrid::~MeshGrid()
    {
        for (size_t i = 0; i < m_tileCapacity; ++i)
        {
            m_tiles[i].tile.Destroy();
        }

        SAFE_DELETE_ARRAY(m_tiles);
    }

    void MeshGrid::Init(const Params& params)
    {
        assert(m_tiles == NULL);
        assert(m_tileCapacity == 0);

        m_params = params;

        Grow(params.tileCount);
    }

#pragma warning(push)
#   pragma warning(disable:28285)
    size_t MeshGrid::GetTriangles(aabb_t aabb, TriangleID* triangles, size_t maxTriCount, float minIslandArea) const
    {
        const size_t minX = (std::max(aabb.min.x, real_t(0)) / real_t(m_params.tileSize.x)).as_uint();
        const size_t minY = (std::max(aabb.min.y, real_t(0)) / real_t(m_params.tileSize.y)).as_uint();
        const size_t minZ = (std::max(aabb.min.z, real_t(0)) / real_t(m_params.tileSize.z)).as_uint();

        const size_t maxX = (std::max(aabb.max.x, real_t(0)) / real_t(m_params.tileSize.x)).as_uint();
        const size_t maxY = (std::max(aabb.max.y, real_t(0)) / real_t(m_params.tileSize.y)).as_uint();
        const size_t maxZ = (std::max(aabb.max.z, real_t(0)) / real_t(m_params.tileSize.z)).as_uint();

        size_t triCount = 0;

        for (uint y = minY; y <= maxY; ++y)
        {
            for (uint x = minX; x <= maxX; ++x)
            {
                for (uint z = minZ; z <= maxZ; ++z)
                {
                    if (const TileID tileID = GetTileID(x, y, z))
                    {
                        const Tile& tile = GetTile(tileID);

                        const vector3_t tileOrigin(
                            real_t(x * m_params.tileSize.x),
                            real_t(y * m_params.tileSize.y),
                            real_t(z * m_params.tileSize.z));

                        aabb_t relative(aabb);
                        relative.min = vector3_t::maximize(relative.min - tileOrigin, vector3_t(0, 0, 0));
                        relative.max = vector3_t::minimize(relative.max - tileOrigin,
                                vector3_t(m_params.tileSize.x, m_params.tileSize.y, m_params.tileSize.z));

                        if (!tile.nodeCount)
                        {
                            for (size_t i = 0; i < tile.triangleCount; ++i)
                            {
                                const Tile::Triangle& triangle = tile.triangles[i];

                                const Tile::Vertex& v0 = tile.vertices[triangle.vertex[0]];
                                const Tile::Vertex& v1 = tile.vertices[triangle.vertex[1]];
                                const Tile::Vertex& v2 = tile.vertices[triangle.vertex[2]];

                                const aabb_t triaabb(vector3_t::minimize(v0, v1, v2), vector3_t::maximize(v0, v1, v2));

                                if (relative.overlaps(triaabb))
                                {
                                    TriangleID triangleID = ComputeTriangleID(tileID, i);

                                    if (minIslandArea <= 0.f || GetIslandAreaForTriangle(triangleID) >= minIslandArea)
                                    {
                                        triangles[triCount++] = triangleID;

                                        if (triCount == maxTriCount)
                                        {
                                            return triCount;
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            size_t nodeID = 0;
                            const size_t nodeCount = tile.nodeCount;

                            while (nodeID < nodeCount)
                            {
                                const Tile::BVNode& node = tile.nodes[nodeID];

                                if (!relative.overlaps(node.aabb))
                                {
                                    nodeID += node.leaf ? 1 : node.offset;
                                }
                                else
                                {
                                    ++nodeID;

                                    if (node.leaf)
                                    {
                                        TriangleID triangleID = ComputeTriangleID(tileID, node.offset);

                                        if (minIslandArea <= 0.f || GetIslandAreaForTriangle(triangleID) >= minIslandArea)
                                        {
                                            triangles[triCount++] = triangleID;

                                            if (triCount == maxTriCount)
                                            {
                                                return triCount;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        return triCount;
    }
#pragma warning(pop)


    TriangleID MeshGrid::GetTriangleAt(const vector3_t& location, const real_t verticalDownwardRange, const real_t verticalUpwardRange, float minIslandArea) const
    {
        const MNM::aabb_t aabb(
            MNM::vector3_t(MNM::real_t(location.x), MNM::real_t(location.y), MNM::real_t(location.z - verticalDownwardRange)),
            MNM::vector3_t(MNM::real_t(location.x), MNM::real_t(location.y), MNM::real_t(location.z + verticalUpwardRange)));

        const size_t MaxTriCandidateCount = 1024;
        TriangleID candidates[MaxTriCandidateCount];

        TriangleID closestID = 0;

        const size_t candidateCount = GetTriangles(aabb, candidates, MaxTriCandidateCount, minIslandArea);
        MNM::real_t::unsigned_overflow_type distMinSq = std::numeric_limits<MNM::real_t::unsigned_overflow_type>::max();

        if (candidateCount)
        {
            MNM::vector3_t a, b, c;

            for (size_t i = 0; i < candidateCount; ++i)
            {
                GetVertices(candidates[i], a, b, c);

                if (PointInTriangle(vector2_t(location), vector2_t(a), vector2_t(b), vector2_t(c)))
                {
                    const MNM::vector3_t ptClosest = ClosestPtPointTriangle(location, a, b, c);
                    const MNM::real_t::unsigned_overflow_type dSq = (ptClosest - location).lenSqNoOverflow();

                    if (dSq < distMinSq)
                    {
                        distMinSq = dSq;
                        closestID = candidates[i];
                    }
                }
            }
        }

        return closestID;
    }

    TriangleID MeshGrid::GetTriangleEdgeAlongLine(const vector3_t& startLocation, const vector3_t& endLocation,
        const real_t verticalDownwardRange, const real_t verticalUpwardRange, vector3_t& hit, float minIslandArea) const
    {
        // Determine bounding box
        real_t minX, minY, minZ;
        real_t maxX = (startLocation.x > endLocation.x) ? (minX = endLocation.x, startLocation.x) : (minX = startLocation.x, endLocation.x);
        real_t maxY = (startLocation.y > endLocation.y) ? (minY = endLocation.y, startLocation.y) : (minY = startLocation.y, endLocation.y);
        real_t maxZ = (startLocation.z > endLocation.z) ? (minZ = endLocation.z, startLocation.z) : (minZ = startLocation.z, endLocation.z);

        const aabb_t aabb(vector3_t(minX, minY, minZ - verticalDownwardRange), vector3_t(maxX, maxY, maxZ + verticalUpwardRange));

        // Gather intersecting triangles
        const size_t MaxTriCandidateCount = 1024;
        TriangleID candidates[MaxTriCandidateCount];
        const size_t candidateCount = GetTriangles(aabb, candidates, MaxTriCandidateCount /*, minIslandArea parameter is no longer supported*/);

        TriangleID triangleID = 0;

        if (candidateCount)
        {
            vector3_t verts[3];
            real_t s, t;
            real_t closest = real_t::max();

            for (size_t i = 0; i < candidateCount; ++i)
            {
                if (GetVertices(candidates[i], verts))
                {
                    for (uint8 v = 0; v < 3; ++v)
                    {
                        if (IntersectSegmentSegment(startLocation, endLocation, verts[v], verts[next_mod3(v)], s, t))
                        {
                            if (s < closest)
                            {
                                closest = s;
                                vector3_t d = (endLocation - startLocation);
                                hit = startLocation + (d * s);
                                triangleID = candidates[i];
                            }
                        }
                    }
                }
            }
        }

        return triangleID;
    }

    bool MeshGrid::IsTriangleAcceptableForLocation(const vector3_t& location, TriangleID triangleID) const
    {
        const MNM::real_t range = MNM::real_t(1.0f);
        if (triangleID)
        {
            const MNM::aabb_t aabb(
                MNM::vector3_t(MNM::real_t(location.x - range), MNM::real_t(location.y - range), MNM::real_t(location.z - range)),
                MNM::vector3_t(MNM::real_t(location.x + range), MNM::real_t(location.y + range), MNM::real_t(location.z + range)));

            const size_t MaxTriCandidateCount = 1024;
            TriangleID candidates[MaxTriCandidateCount];

            const size_t candidateCount = GetTriangles(aabb, candidates, MaxTriCandidateCount);
            MNM::real_t distMinSq = MNM::real_t::max();

            if (candidateCount)
            {
                MNM::vector3_t start = location;
                MNM::vector3_t a, b, c;

                for (size_t i = 0; i < candidateCount; ++i)
                {
                    GetVertices(candidates[i], a, b, c);

                    if (candidates[i] == triangleID && PointInTriangle(vector2_t(location), vector2_t(a), vector2_t(b), vector2_t(c)))
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    TriangleID MeshGrid::GetClosestTriangle(const vector3_t& location, real_t vrange, real_t hrange, real_t* distSq,
        vector3_t* closest, float minIslandArea) const
    {
        const MNM::aabb_t aabb(
            MNM::vector3_t(MNM::real_t(location.x - hrange), MNM::real_t(location.y - hrange), MNM::real_t(location.z - vrange)),
            MNM::vector3_t(MNM::real_t(location.x + hrange), MNM::real_t(location.y + hrange), MNM::real_t(location.z + vrange)));

        const size_t MaxTriCandidateCount = 1024;
        TriangleID candidates[MaxTriCandidateCount];

        TriangleID closestID = 0;

        const size_t candidateCount = GetTriangles(aabb, candidates, MaxTriCandidateCount, minIslandArea);
        MNM::real_t distMinSq = MNM::real_t::max();

        if (candidateCount)
        {
            MNM::vector3_t a, b, c;

            for (size_t i = 0; i < candidateCount; ++i)
            {
                GetVertices(candidates[i], a, b, c);

                const MNM::vector3_t ptClosest = ClosestPtPointTriangle(location, a, b, c);
                const MNM::real_t dSq = (ptClosest - location).lenNoOverflow();

                if (dSq < distMinSq)
                {
                    if (closest)
                    {
                        *closest = ptClosest;
                    }

                    distMinSq = dSq;
                    closestID = candidates[i];
                }
            }
        }

        if (distSq)
        {
            *distSq = distMinSq;
        }

        return closestID;
    }

    bool MeshGrid::GetVertices(TriangleID triangleID, vector3_t& v0, vector3_t& v1, vector3_t& v2) const
    {
        if (const TileID tileID = ComputeTileID(triangleID))
        {
            const TileContainer& container = m_tiles[tileID - 1];

            const vector3_t origin(
                real_t(container.x * m_params.tileSize.x),
                real_t(container.y * m_params.tileSize.y),
                real_t(container.z * m_params.tileSize.z));

            const Tile::Triangle& triangle = container.tile.triangles[ComputeTriangleIndex(triangleID)];
            v0 = origin + vector3_t(container.tile.vertices[triangle.vertex[0]]);
            v1 = origin + vector3_t(container.tile.vertices[triangle.vertex[1]]);
            v2 = origin + vector3_t(container.tile.vertices[triangle.vertex[2]]);

            return true;
        }

        return false;
    }

    bool MeshGrid::GetVertices(TriangleID triangleID, vector3_t* verts) const
    {
        return GetVertices(triangleID, verts[0], verts[1], verts[2]);
    }

    bool MeshGrid::GetLinkedEdges(TriangleID triangleID, size_t& linkedEdges) const
    {
        if (const TileID tileID = ComputeTileID(triangleID))
        {
            const TileContainer& container = m_tiles[tileID - 1];
            const Tile::Triangle& triangle = container.tile.triangles[ComputeTriangleIndex(triangleID)];

            linkedEdges = 0;

            for (size_t l = 0; l < triangle.linkCount; ++l)
            {
                const Tile::Link& link = container.tile.links[triangle.firstLink + l];
                const size_t edge = link.edge;
                linkedEdges |= 1ULL << edge;
            }

            return true;
        }

        return false;
    }

    bool MeshGrid::GetTriangle(TriangleID triangleID, Tile::Triangle& triangle) const
    {
        if (const TileID tileID = ComputeTileID(triangleID))
        {
            const TileContainer& container = m_tiles[tileID - 1];
            triangle = container.tile.triangles[ComputeTriangleIndex(triangleID)];

            return true;
        }

        return false;
    }

    bool MeshGrid::PushPointInsideTriangle(const TriangleID triangleID, vector3_t& location, real_t amount) const
    {
        if (amount <= 0)
        {
            return false;
        }

        vector3_t v0, v1, v2;
        if (const TileID tileID = ComputeTileID(triangleID))
        {
            const TileContainer& container = m_tiles[tileID - 1];
            const vector3_t origin(
                real_t(container.x * m_params.tileSize.x),
                real_t(container.y * m_params.tileSize.y),
                real_t(container.z * m_params.tileSize.z));
            const vector3_t locationTileOffsetted = (location - origin);

            const Tile::Triangle& triangle = container.tile.triangles[ComputeTriangleIndex(triangleID)];
            v0 = vector3_t(container.tile.vertices[triangle.vertex[0]]);
            v1 = vector3_t(container.tile.vertices[triangle.vertex[1]]);
            v2 = vector3_t(container.tile.vertices[triangle.vertex[2]]);

            const vector3_t triangleCenter = ((v0 + v1 + v2) * real_t::fraction(1, 3));
            const vector3_t locationToCenter = triangleCenter - locationTileOffsetted;
            const real_t locationToCenterLen = locationToCenter.lenNoOverflow();

            if (locationToCenterLen > amount)
            {
                location += (locationToCenter / locationToCenterLen) * amount;
            }
            else
            {
                // If locationToCenterLen is smaller than the amount I wanna push
                // the point, then it's safer use directly the center position
                // otherwise the point could end up outside the other side of the triangle
                location = triangleCenter + origin;
            }
            return true;
        }

        return false;
    }

    void MeshGrid::ResetConnectedIslandsIDs()
    {
        for (TileMap::iterator tileIt = m_tileMap.begin(); tileIt != m_tileMap.end(); ++tileIt)
        {
            Tile& tile = m_tiles[tileIt->second - 1].tile;

            for (uint16 i = 0; i < tile.triangleCount; ++i)
            {
                Tile::Triangle& triangle = tile.triangles[i];
                triangle.islandID = MNM::Constants::eStaticIsland_InvalidIslandID;
            }
        }

        m_islands.clear();
    }

    void MeshGrid::ComputeStaticIslandsAndConnections(const NavigationMeshID meshID, const MNM::OffMeshNavigation& offMeshNavigation, MNM::IslandConnections& islandConnections)
    {
        FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

        ResetConnectedIslandsIDs();
        ComputeStaticIslands();
        ResolvePendingIslandConnectionRequests(meshID, offMeshNavigation, islandConnections);
    }

    void MeshGrid::ComputeStaticIslands()
    {
        FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

        typedef std::vector<TriangleID> Triangles;
        Triangles trianglesToVisit;
        trianglesToVisit.reserve(4096);

        // The idea is to iterate through the existing tiles and then finding the triangles from which
        // start the assigning of the different island ids.
        for (TileMap::iterator tileIt = m_tileMap.begin(); tileIt != m_tileMap.end(); ++tileIt)
        {
            const TileID tileID = tileIt->second;
            Tile& tile = m_tiles[tileID - 1].tile;
            for (uint16 triangleIndex = 0; triangleIndex < tile.triangleCount; ++triangleIndex)
            {
                Tile::Triangle& sourceTriangle = tile.triangles[triangleIndex];
                if (sourceTriangle.islandID == MNM::Constants::eStaticIsland_InvalidIslandID)
                {
                    Island& newIsland = GetNewIsland();
                    sourceTriangle.islandID = newIsland.id;
                    const TriangleID triangleID = ComputeTriangleID(tileID, triangleIndex);

                    newIsland.area = tile.GetTriangleArea(triangleID).as_float();

                    trianglesToVisit.push_back(triangleID);

                    // We now have another triangle to start from to assign the new island ids to all the connected
                    // triangles
                    size_t totalTrianglesToVisit = 1;
                    for (size_t index = 0; index < totalTrianglesToVisit; ++index)
                    {
                        // Get next triangle to start the evaluation from
                        const MNM::TriangleID currentTriangleID = trianglesToVisit[index];

                        const TileID currentTileId = ComputeTileID(currentTriangleID);
                        CRY_ASSERT_MESSAGE(currentTileId > 0, "ComputeStaticIslands is trying to access a triangle ID associated with an invalid tile id.");

                        const TileContainer& container = m_tiles[currentTileId - 1];
                        const Tile& currentTile = container.tile;
                        const Tile::Triangle& currentTriangle = currentTile.triangles[ComputeTriangleIndex(currentTriangleID)];

                        // Calc area of this triangle
                        float farea = currentTile.GetTriangleArea(currentTriangleID).as_float();

                        for (uint16 l = 0; l < currentTriangle.linkCount; ++l)
                        {
                            const Tile::Link& link = currentTile.links[currentTriangle.firstLink + l];
                            if (link.side == Tile::Link::Internal)
                            {
                                Tile::Triangle& nextTriangle = currentTile.triangles[link.triangle];
                                if (nextTriangle.islandID == MNM::Constants::eGlobalIsland_InvalidIslandID)
                                {
                                    ++totalTrianglesToVisit;
                                    nextTriangle.islandID = newIsland.id;
                                    newIsland.area += farea;
                                    trianglesToVisit.push_back(ComputeTriangleID(currentTileId, link.triangle));
                                }
                            }
                            else if (link.side == Tile::Link::OffMesh)
                            {
                                QueueIslandConnectionSetup(currentTriangle.islandID, currentTriangleID, link.triangle);
                            }
                            else
                            {
                                const TileID neighbourTileID = GetNeighbourTileID(container.x, container.y, container.z, link.side);
                                CRY_ASSERT_MESSAGE(neighbourTileID > 0, "ComputeStaticIslands is trying to access an invalid neighbour tile.");

                                const Tile& neighbourTile = m_tiles[neighbourTileID - 1].tile;
                                Tile::Triangle& nextTriangle = neighbourTile.triangles[link.triangle];
                                if (nextTriangle.islandID == MNM::Constants::eGlobalIsland_InvalidIslandID)
                                {
                                    ++totalTrianglesToVisit;
                                    nextTriangle.islandID = newIsland.id;
                                    newIsland.area += farea;
                                    trianglesToVisit.push_back(ComputeTriangleID(neighbourTileID, link.triangle));
                                }
                            }
                        }
                    }
                    trianglesToVisit.clear();
                }
            }
        }
    }

    MeshGrid::Island& MeshGrid::GetNewIsland()
    {
        assert(m_islands.size() != std::numeric_limits<StaticIslandID>::max());

        // Generate new id (NOTE: Invalid is 0)
        StaticIslandID id = (m_islands.size() + 1);

        m_islands.push_back(Island(id));
        return m_islands.back();
    }

    void MeshGrid::QueueIslandConnectionSetup(const StaticIslandID islandID, const TriangleID startingTriangleID, const uint16 offMeshLinkIndex)
    {
        m_islandConnectionRequests.push_back(IslandConnectionRequest(islandID, startingTriangleID, offMeshLinkIndex));
    }

    void MeshGrid::ResolvePendingIslandConnectionRequests(const NavigationMeshID meshID, const MNM::OffMeshNavigation& offMeshNavigation,
        MNM::IslandConnections& islandConnections)
    {
        while (!m_islandConnectionRequests.empty())
        {
            IslandConnectionRequest& request = m_islandConnectionRequests.back();
            OffMeshNavigation::QueryLinksResult links = offMeshNavigation.GetLinksForTriangle(request.startingTriangleID, request.offMeshLinkIndex);
            while (WayTriangleData nextTri = links.GetNextTriangle())
            {
                Tile::Triangle endTriangle;
                if (GetTriangle(nextTri.triangleID, endTriangle))
                {
                    const OffMeshLink* pLink = offMeshNavigation.GetObjectLinkInfo(nextTri.offMeshLinkID);
                    assert(pLink);
                    MNM::IslandConnections::Link link(nextTri.triangleID, nextTri.offMeshLinkID, GlobalIslandID(meshID, endTriangle.islandID), pLink->GetEntityIdForOffMeshLink());
                    islandConnections.SetOneWayConnectionBetweenIsland(GlobalIslandID(meshID, request.startingIslandID), link);
                }
            }
            m_islandConnectionRequests.pop_back();
        }
    }

    void MeshGrid::SearchForIslandConnectionsToRefresh(const TileID tileID)
    {
        Tile& tile = m_tiles[tileID - 1].tile;
        for (uint16 triangleIndex = 0; triangleIndex < tile.triangleCount; ++triangleIndex)
        {
            Tile::Triangle& triangle = tile.triangles[triangleIndex];
            for (uint16 l = 0; l < triangle.linkCount; ++l)
            {
                const Tile::Link& link = tile.links[triangle.firstLink + l];
                if (link.side == Tile::Link::OffMesh)
                {
                    QueueIslandConnectionSetup(triangle.islandID, ComputeTriangleID(tileID, triangleIndex), link.triangle);
                }
            }
        }
    }

    float MeshGrid::GetIslandArea(StaticIslandID islandID) const
    {
        bool isValid = (islandID >= MNM::Constants::eStaticIsland_FirstValidIslandID && islandID <= m_islands.size());
        return (isValid) ? m_islands[islandID - 1].area : -1.f;
    }

    float MeshGrid::GetIslandAreaForTriangle(TriangleID triangleID) const
    {
        Tile::Triangle triangle;
        if (GetTriangle(triangleID, triangle))
        {
            return GetIslandArea(triangle.islandID);
        }

        return -1.f;
    }

    void MeshGrid::PredictNextTriangleEntryPosition(const TriangleID bestNodeTriangleID,
        const vector3_t& bestNodePosition, const TriangleID nextTriangleID, const unsigned int edgeIndex
        , const vector3_t finalLocation, vector3_t& outPosition) const
    {
        IF_UNLIKELY (edgeIndex == MNM::Constants::InvalidEdgeIndex)
        {
            // For SO links we don't set up the edgeIndex value since it's more probable that the animations
            // ending point is better approximated by the triangle center value
            vector3_t v0, v1, v2;
            GetVertices(nextTriangleID, v0, v1, v2);
            outPosition = (v0 + v1 + v2) * real_t::fraction(1, 3);
            return;
        }

        Tile::Triangle triangle;
        if (GetTriangle(bestNodeTriangleID, triangle))
        {
            const TileID bestTriangleTileID = ComputeTileID(bestNodeTriangleID);
            assert(bestTriangleTileID);
            const TileContainer& currentContainer = m_tiles[bestTriangleTileID - 1];
            const Tile& currentTile = currentContainer.tile;
            const vector3_t tileOrigin(real_t(currentContainer.x * m_params.tileSize.x), real_t(currentContainer.y * m_params.tileSize.y), real_t(currentContainer.z * m_params.tileSize.z));

            assert(edgeIndex < 3);
            const vector3_t v0 = tileOrigin + vector3_t(currentTile.vertices[triangle.vertex[edgeIndex]]);
            const vector3_t v1 = tileOrigin + vector3_t(currentTile.vertices[triangle.vertex[next_mod3(edgeIndex)]]);

            switch (gAIEnv.CVars.MNMPathfinderPositionInTrianglePredictionType)
            {
            case ePredictionType_TriangleCenter:
            {
                const vector3_t v2 = tileOrigin + vector3_t(currentTile.vertices[triangle.vertex[dec_mod3[edgeIndex]]]);
                outPosition = (v0 + v1 + v2) * real_t::fraction(1, 3);
            }
            break;
            case ePredictionType_Advanced:
            default:
            {
                const vector3_t v0v1 = v1 - v0;
                real_t s, t;
                if (IntersectSegmentSegment(v0, v1, bestNodePosition, finalLocation, s, t))
                {
                    // If the two segments intersect,
                    // let's choose the point that goes in the direction we want to go
                    s = clamp(s, kMinPullingThreshold, kMaxPullingThreshold);
                    outPosition = v0 + v0v1 * s;
                }
                else
                {
                    // Otherwise we need to understand where the segment is in relation
                    // of where we want to go.
                    // Let's choose the point on the segment that is closer towards the target
                    const real_t::unsigned_overflow_type distSqAE = (v0 - finalLocation).lenSqNoOverflow();
                    const real_t::unsigned_overflow_type distSqBE = (v1 - finalLocation).lenSqNoOverflow();
                    const real_t segmentPercentage = (distSqAE < distSqBE) ? kMinPullingThreshold : kMaxPullingThreshold;
                    outPosition = v0 + v0v1 * segmentPercentage;
                }
            }
            break;
            }
        }
        else
        {
            // At this point it's not acceptable to have an invalid triangle
            assert(0);
        }
    }

    MeshGrid::EWayQueryResult MeshGrid::FindWay(WayQueryRequest& inputRequest, WayQueryWorkingSet& workingSet, WayQueryResult& result) const
    {
        result.SetWaySize(0);
        if (result.GetWayMaxSize() < 2)
        {
            return eWQR_Done;
        }

        if (inputRequest.From() && inputRequest.To())
        {
            if (inputRequest.From() == inputRequest.To())
            {
                WayTriangleData* pOutputWay = result.GetWayData();
                pOutputWay[0] = WayTriangleData(inputRequest.From(), 0);
                pOutputWay[1] = WayTriangleData(inputRequest.To(), 0);
                result.SetWaySize(2);
                return eWQR_Done;
            }
            else
            {
                const vector3_t origin(m_params.origin);
                const vector3_t startLocation = inputRequest.GetFromLocation();
                const vector3_t endLocation = inputRequest.GetToLocation();

                WayTriangleData lastBestNodeID(inputRequest.From(), 0);

                while (workingSet.aStarOpenList.CanDoStep())
                {
                    // switch the smallest element with the last one and pop the last element
                    AStarOpenList::OpenNodeListElement element = workingSet.aStarOpenList.PopBestNode();
                    WayTriangleData bestNodeID = element.triData;

                    lastBestNodeID = bestNodeID;
                    IF_UNLIKELY (bestNodeID.triangleID == inputRequest.To())
                    {
                        workingSet.aStarOpenList.StepDone();
                        break;
                    }

                    AStarOpenList::Node* bestNode = element.pNode;
                    const vector3_t bestNodeLocation = bestNode->location - origin;

                    const TileID tileID = ComputeTileID(bestNodeID.triangleID);

                    // Clear from previous step
                    workingSet.nextLinkedTriangles.clear();

                    if (tileID)
                    {
                        const TileContainer& container = m_tiles[tileID - 1];
                        const Tile& tile = container.tile;
                        const uint16 triangleIdx = ComputeTriangleIndex(bestNodeID.triangleID);
                        const Tile::Triangle& triangle = tile.triangles[triangleIdx];

                        //Gather all accessible triangles first

                        for (size_t l = 0; l < triangle.linkCount; ++l)
                        {
                            const Tile::Link& link = tile.links[triangle.firstLink + l];

                            WayTriangleData nextTri(0, 0);

                            if (link.side == Tile::Link::Internal)
                            {
                                nextTri.triangleID = ComputeTriangleID(tileID, link.triangle);
                                nextTri.incidentEdge = link.edge;
                            }
                            else if (link.side == Tile::Link::OffMesh)
                            {
                                OffMeshNavigation::QueryLinksResult links = inputRequest.GetOffMeshNavigation().GetLinksForTriangle(bestNodeID.triangleID, link.triangle);
                                while (nextTri = links.GetNextTriangle())
                                {
                                    if (inputRequest.CanUseOffMeshLink(nextTri.offMeshLinkID, &nextTri.costMultiplier))
                                    {
                                        workingSet.nextLinkedTriangles.push_back(nextTri);
                                        nextTri.incidentEdge = (unsigned int)MNM::Constants::InvalidEdgeIndex;
                                    }
                                }
                                continue;
                            }
                            else
                            {
                                TileID neighbourTileID = GetNeighbourTileID(container.x, container.y, container.z, link.side);
                                nextTri.triangleID = ComputeTriangleID(neighbourTileID, link.triangle);
                                nextTri.incidentEdge = link.edge;
                            }

                            Vec3 edgeMidPoint;
                            if (CalculateMidEdge(bestNodeID.triangleID, nextTri.triangleID, edgeMidPoint))
                            {
                                const uint32 flags = 0;
                                if (inputRequest.IsPointValidForAgent(edgeMidPoint, flags))
                                {
                                    workingSet.nextLinkedTriangles.push_back(nextTri);
                                }
                            }
                            //////////////////////////////////////////////////////////////////////////
                            // NOTE: This is user defined only at compile time

                            BreakOnInvalidTriangle(nextTri.triangleID, m_tileCapacity);

                            //////////////////////////////////////////////////////////////////////////
                        }
                    }
                    else
                    {
                        AIError("MeshGrid::FindWay - Bad Navmesh data Tile: %d, Triangle: %d, skipping ", tileID, ComputeTriangleIndex(bestNodeID.triangleID));
                        BreakOnInvalidTriangle(bestNodeID.triangleID, m_tileCapacity);
                    }


                    const size_t triangleCount = workingSet.nextLinkedTriangles.size();
                    for (size_t t = 0; t < triangleCount; ++t)
                    {
                        WayTriangleData nextTri = workingSet.nextLinkedTriangles[t];

                        if (nextTri == bestNode->prevTriangle)
                        {
                            continue;
                        }

                        AStarOpenList::Node* nextNode = NULL;
                        const bool inserted = workingSet.aStarOpenList.InsertNode(nextTri, &nextNode);

                        assert(nextNode);

                        if (inserted)
                        {
                            IF_UNLIKELY (nextTri.triangleID == inputRequest.To())
                            {
                                nextNode->location = endLocation;
                            }
                            else
                            {
                                PredictNextTriangleEntryPosition(bestNodeID.triangleID, bestNodeLocation, nextTri.triangleID, nextTri.incidentEdge, endLocation, nextNode->location);
                            }

                            nextNode->open = false;
                        }

                        //Euclidean distance
                        const vector3_t targetDistance = endLocation - nextNode->location;
                        const vector3_t stepDistance = bestNodeLocation - nextNode->location;
                        const real_t heuristic = targetDistance.lenNoOverflow();
                        const real_t stepCost = stepDistance.lenNoOverflow();

                        //Euclidean distance approximation
                        //const real_t heuristic = targetDistance.approximatedLen();
                        //const real_t stepCost = stepDistance.approximatedLen();

                        const real_t dangersTotalCost = CalculateHeuristicCostForDangers(nextNode->location, startLocation, m_params.origin, inputRequest.GetDangersInfos());

                        real_t costMultiplier = real_t(nextTri.costMultiplier);

                        const real_t cost = bestNode->cost + (stepCost * costMultiplier) + dangersTotalCost;
                        const real_t total = cost + heuristic;

                        if (nextNode->open && nextNode->estimatedTotalCost <= total)
                        {
                            continue;
                        }

                        nextNode->cost = cost;
                        nextNode->estimatedTotalCost = total;
                        nextNode->prevTriangle = bestNodeID;

                        if (!nextNode->open)
                        {
                            nextNode->open = true;
                            nextNode->location += origin;
                            workingSet.aStarOpenList.AddToOpenList(nextTri, nextNode, total);
                        }
                    }

                    workingSet.aStarOpenList.StepDone();
                }

                if (lastBestNodeID.triangleID == inputRequest.To())
                {
                    size_t wayTriCount = 0;
                    WayTriangleData wayTriangle = lastBestNodeID;
                    WayTriangleData nextInsertion(wayTriangle.triangleID, 0);

                    WayTriangleData* outputWay = result.GetWayData();

                    while (wayTriangle.triangleID != inputRequest.From())
                    {
                        const AStarOpenList::Node* node = workingSet.aStarOpenList.FindNode(wayTriangle);
                        assert(node);
                        outputWay[wayTriCount++] = nextInsertion;

                        //Iterate the path backwards, and shift the offMeshLink to the previous triangle (start of the link)
                        nextInsertion.offMeshLinkID = wayTriangle.offMeshLinkID;
                        wayTriangle = node->prevTriangle;
                        nextInsertion.triangleID = wayTriangle.triangleID;

                        if (wayTriCount == result.GetWayMaxSize())
                        {
                            break;
                        }
                    }

                    if (wayTriCount < result.GetWayMaxSize())
                    {
                        outputWay[wayTriCount++] = WayTriangleData(inputRequest.From(), nextInsertion.offMeshLinkID);
                    }

                    result.SetWaySize(wayTriCount);
                    return eWQR_Done;
                }
                else if (!workingSet.aStarOpenList.Empty())
                {
                    //We did not finish yet...
                    return eWQR_Continuing;
                }
            }
        }

        return eWQR_Done;
    }

    struct CostAccumulator
    {
        CostAccumulator(const Vec3& locationToEval, const Vec3& startingLocation, real_t& totalCost)
            : m_totalCost(totalCost)
            , m_locationToEvaluate(locationToEval)
            , m_startingLocation(startingLocation)
        {}

        void operator()(const DangerAreaConstPtr& dangerInfo)
        {
            m_totalCost += dangerInfo->GetDangerHeuristicCost(m_locationToEvaluate, m_startingLocation);
        }
    private:
        real_t&     m_totalCost;
        const Vec3& m_locationToEvaluate;
        const Vec3& m_startingLocation;
    };

    real_t MeshGrid::CalculateHeuristicCostForDangers(const vector3_t& locationToEval, const vector3_t& startingLocation, const Vec3& meshOrigin, const DangerousAreasList& dangersInfos) const
    {
        real_t totalCost(0.0f);
        const Vec3 startingLocationInWorldSpace = startingLocation.GetVec3() + meshOrigin;
        const Vec3 locationInWorldSpace = locationToEval.GetVec3() + meshOrigin;
        std::for_each(dangersInfos.begin(), dangersInfos.end(), CostAccumulator(locationInWorldSpace, startingLocationInWorldSpace, totalCost));
        return totalCost;
    }

    void MeshGrid::PullString(const vector3_t from, const TriangleID fromTriID, const vector3_t to, const TriangleID toTriID, vector3_t& middlePoint) const
    {
        if (const TileID fromTileID = ComputeTileID(fromTriID))
        {
            const TileContainer& startContainer = m_tiles[fromTileID - 1];
            const Tile& startTile = startContainer.tile;
            uint16 fromTriangleIdx = ComputeTriangleIndex(fromTriID);
            const Tile::Triangle& fromTriangle = startTile.triangles[fromTriangleIdx];

            uint16 vi0 = 0, vi1 = 0;
            for (int l = 0; l < fromTriangle.linkCount; ++l)
            {
                const Tile::Link& link = startTile.links[fromTriangle.firstLink + l];
                if (link.side == Tile::Link::Internal)
                {
                    TriangleID newTriangleID = ComputeTriangleID(fromTileID, link.triangle);
                    if (newTriangleID == toTriID)
                    {
                        vi0 = link.edge;
                        vi1 = next_mod3(link.edge);
                        assert(vi0 < 3);
                        assert(vi1 < 3);
                        break;
                    }
                }
                else
                {
                    if (link.side != Tile::Link::OffMesh)
                    {
                        TileID neighbourTileID = GetNeighbourTileID(startContainer.x, startContainer.y, startContainer.z, link.side);
                        TriangleID newTriangleID = ComputeTriangleID(neighbourTileID, link.triangle);

                        if (newTriangleID == toTriID)
                        {
                            vi0 = link.edge;
                            vi1 = next_mod3(link.edge);
                            assert(vi0 < 3);
                            assert(vi1 < 3);
                            break;
                        }
                    }
                }
            }

            vector3_t fromVertices[3];
            GetVertices(fromTriID, fromVertices[0], fromVertices[1], fromVertices[2]);

            if (vi0 != vi1)
            {
                real_t s, t;
                PREFAST_ASSUME(vi0 < 3 && vi1 < 3);
                vector3_t dir = fromVertices[vi1] - fromVertices[vi0];
                dir.normalized();
                if (IntersectSegmentSegment(vector2_t(fromVertices[vi0]),
                        vector2_t(fromVertices[vi1]), vector2_t(from), vector2_t(to), s, t))
                {
                    const Tile::Triangle& triangle = startContainer.tile.triangles[ComputeTriangleIndex(fromTriID)];

                    s = clamp(s, kMinPullingThreshold, kMaxPullingThreshold);
                    middlePoint = fromVertices[vi0] + dir * s;
                }
                else
                {
                    if (s < 0)
                    {
                        middlePoint = fromVertices[vi0] + dir * kMinPullingThreshold;
                    }
                    else
                    {
                        middlePoint = fromVertices[vi0] + dir * kMaxPullingThreshold;
                    }
                }
            }
        }
    }

    void MeshGrid::AddOffMeshLinkToTile(const TileID tileID, const TriangleID triangleID, const uint16 offMeshIndex)
    {
        Tile& tile = GetTile(tileID);

        m_profiler.FreeMemory(LinkMemory, tile.linkCount * sizeof(Tile::Link));
        m_profiler.AddStat(LinkCount, -(int)tile.linkCount);

        tile.AddOffMeshLink(triangleID, offMeshIndex);

        m_profiler.AddMemory(LinkMemory, tile.linkCount * sizeof(Tile::Link));
        m_profiler.AddStat(LinkCount, tile.linkCount);
    }

    void MeshGrid::UpdateOffMeshLinkForTile(const TileID tileID, const TriangleID triangleID, const uint16 offMeshIndex)
    {
        Tile& tile = GetTile(tileID);

        tile.UpdateOffMeshLink(triangleID, offMeshIndex);
    }

    void MeshGrid::RemoveOffMeshLinkFromTile(const TileID tileID, const TriangleID triangleID)
    {
        Tile& tile = GetTile(tileID);

        m_profiler.FreeMemory(LinkMemory, tile.linkCount * sizeof(Tile::Link));
        m_profiler.AddStat(LinkCount, -(int)tile.linkCount);

        tile.RemoveOffMeshLink(triangleID);

        m_profiler.AddMemory(LinkMemory, tile.linkCount * sizeof(Tile::Link));
        m_profiler.AddStat(LinkCount, tile.linkCount);
    }

    inline size_t OppositeSide(size_t side)
    {
        return (side + 7) % 14;
    }

    std::tuple<bool, float, Vec3> IntersectSegmentTriangle(const Vec3& segP0, const Vec3& segP1, const Vec3& triV0, const Vec3& triV1, const Vec3& triV2)
    {
        const auto noIntersect = std::make_tuple(false, -1.0f, Vec3(IDENTITY));

        Vec3 u = triV1 - triV0;
        Vec3 v = triV2 - triV0;
        Vec3 n = u.Cross(v);

        // Check for degenerate triangle
        if (fcmp(n.GetLengthSquared(), 0.0f))
        {
            return noIntersect;
        }

        // Get the ray direction vector
        Vec3 dir = segP1 - segP0;
        Vec3 w0 = segP0 - triV0;

        float a = -n.Dot(w0);
        float b = n.Dot(dir);

        // Ray is parallel to triangle plane
        if (fcmp(b, 0.0f))
        {
            return noIntersect;
        }

        // Compute intersection point of ray with triangle plane
        float r = a / b;

        // If ray goes away from triangle, or intersects past the segment length, reject
        if (r < 0.0f || r > 1.0f)
        {
            return noIntersect;
        }

        // Check to see if the intersection point is within the triangle (Barycentric coordinate check)
        // http://blogs.msdn.com/b/rezanour/archive/2011/08/07/barycentric-coordinates-and-point-in-triangle-tests.aspx
        Vec3 I = segP0 + r * dir;
        Vec3 w = I - triV0;

        float uu = u.Dot(u);
        float uv = u.Dot(v);
        float vv = v.Dot(v);
        float wu = w.Dot(u);
        float wv = w.Dot(v);
        float D = uv * uv - uu * vv;

        float s = (uv * wv - vv * wu) / D;
        if (s < 0.0f || s > 1.0f)
        {
            return noIntersect;
        }

        float t = (uv * wu - uu * wv) / D;
        if (t < 0.0f || (s + t) > 1.0f)
        {
            return noIntersect;
        }

        return std::make_tuple(true, r, I);
    }

    std::tuple<bool, float, Vec3> MeshGrid::RayCastWorld(const Vec3& segP0, const Vec3& segP1) const
    {
        auto minResult = std::make_tuple(false, FLT_MAX, Vec3(IDENTITY));

        for (int i = 0; i < this->m_tileCount; ++i)
        {
            Tile& tile = m_tiles[i].tile;

            for (int j = 0; j < tile.triangleCount; ++j)
            {
                // Get the triangle vertices
                Vec3 v0, v1, v2;
                {
                    // Vertices are stored in packed format; unpack them
                    vector3_t vec0, vec1, vec2;
                    GetVertices(ComputeTriangleID(i + 1, j), vec0, vec1, vec2);
                    v0 = vec0.GetVec3();
                    v1 = vec1.GetVec3();
                    v2 = vec2.GetVec3();
                }

                // Intersect and record closest
                auto result = IntersectSegmentTriangle(segP0, segP1, v0, v1, v2);
                if (std::get<0>(result) && std::get<1>(result) < std::get<1>(minResult))
                {
                    minResult = result;
                }
            }
        }

        return minResult;
    }

    MeshGrid::ERayCastResult MeshGrid::RayCast(const vector3_t& from, TriangleID fromTri, const vector3_t& to, TriangleID toTri,
        RaycastRequestBase& raycastRequest) const
    {
        FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

        const bool useNewRaycast = gAIEnv.CVars.MNMRaycastImplementation != 0;
        if (useNewRaycast)
        {
            return RayCast_new(from, fromTri, to, toTri, raycastRequest);
        }
        else
        {
            return RayCast_old(from, fromTri, to, toTri, raycastRequest);
        }
    }

    struct RaycastNode
    {
        RaycastNode()
            : triangleID(MNM::Constants::InvalidTriangleID)
            , percentageOfTotalDistance(-1.0f)
            , incidentEdge((uint16) MNM::Constants::InvalidEdgeIndex)
        {}

        RaycastNode(const TriangleID _triangleID, const real_t _percentageOfTotalDistance, const uint16 _incidentEdge)
            : triangleID(_triangleID)
            , percentageOfTotalDistance(_percentageOfTotalDistance)
            , incidentEdge(_incidentEdge)
        {}

        RaycastNode(const RaycastNode& otherNode)
            : triangleID(otherNode.triangleID)
            , percentageOfTotalDistance(otherNode.percentageOfTotalDistance)
            , incidentEdge(otherNode.incidentEdge)
        {}

        ILINE bool IsValid() const { return triangleID != MNM::Constants::InvalidTriangleID; }

        ILINE bool operator<(const RaycastNode& other) const
        {
            return triangleID < other.triangleID;
        }

        ILINE bool operator==(const RaycastNode& other) const
        {
            return triangleID == other.triangleID;
        }

        TriangleID triangleID;
        real_t percentageOfTotalDistance;
        uint16 incidentEdge;
    };

    struct IsNodeCloserToEndPredicate
    {
        bool operator() (const RaycastNode& firstNode, const RaycastNode& secondNode) { return firstNode.percentageOfTotalDistance > secondNode.percentageOfTotalDistance; }
    };

    typedef OpenList<RaycastNode, IsNodeCloserToEndPredicate> RaycastOpenList;
    typedef VectorSet<TriangleID> RaycastClosedList;

    MeshGrid::ERayCastResult MeshGrid::RayCast_new(const vector3_t& from, TriangleID fromTriangleID, const vector3_t& to, TriangleID toTriangleID,
        RaycastRequestBase& raycastRequest) const
    {
        FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

        if (!IsLocationInTriangle(from, fromTriangleID))
        {
            return eRayCastResult_InvalidStart;
        }

#ifdef RAYCAST_DO_NOT_ACCEPT_INVALID_END
        if (!IsLocationInTriangle(to, toTriangleID))
        {
            return eRayCastResult_InvalidEnd;
        }
#endif

        RaycastClosedList closedList;
        closedList.reserve(raycastRequest.maxWayTriCount);

        RaycastCameFromMap cameFrom;
        cameFrom.reserve(raycastRequest.maxWayTriCount);

        const size_t expectedOpenListSize = 10;
        RaycastOpenList openList(expectedOpenListSize);

        RaycastNode furtherNodeVisited;
        RayHit rayHit;

        RaycastNode currentNode(fromTriangleID, real_t(.0f), (uint16) MNM::Constants::InvalidEdgeIndex);

        while (currentNode.IsValid())
        {
            if (currentNode.triangleID == toTriangleID)
            {
                ReconstructRaycastResult(fromTriangleID, toTriangleID, cameFrom, raycastRequest);
                return eRayCastResult_NoHit;
            }

            if (currentNode.percentageOfTotalDistance > furtherNodeVisited.percentageOfTotalDistance)
            {
                furtherNodeVisited = currentNode;
            }

            rayHit.triangleID = currentNode.triangleID;
            RaycastNode nextNode;

            closedList.insert(currentNode.triangleID);

            TileID tileID = ComputeTileID(currentNode.triangleID);
            if (tileID != MNM::Constants::InvalidTileID)
            {
                const TileContainer* container = &m_tiles[tileID - 1];
                const Tile* tile = &container->tile;

                vector3_t tileOrigin(
                    real_t(container->x * m_params.tileSize.x),
                    real_t(container->y * m_params.tileSize.y),
                    real_t(container->z * m_params.tileSize.z));

                const Tile::Triangle& triangle = tile->triangles[ComputeTriangleIndex(currentNode.triangleID)];
                for (uint16 edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
                {
                    if (currentNode.incidentEdge == edgeIndex)
                    {
                        continue;
                    }

                    const vector3_t a = tileOrigin + vector3_t(tile->vertices[triangle.vertex[edgeIndex]]);
                    const vector3_t b = tileOrigin + vector3_t(tile->vertices[triangle.vertex[ inc_mod3[edgeIndex] ]]);

                    real_t s, t;
                    if (IntersectSegmentSegment(vector2_t(from), vector2_t(to), vector2_t(a), vector2_t(b), s, t))
                    {
                        if (s < currentNode.percentageOfTotalDistance)
                        {
                            continue;
                        }

                        rayHit.distance = s;
                        rayHit.edge = edgeIndex;

                        for (size_t linkIndex = 0; linkIndex < triangle.linkCount; ++linkIndex)
                        {
                            const Tile::Link& link = tile->links[triangle.firstLink + linkIndex];
                            if (link.edge != edgeIndex)
                            {
                                continue;
                            }

                            const uint16 side = link.side;

                            if (side == Tile::Link::Internal)
                            {
                                const Tile::Triangle& opposite = tile->triangles[link.triangle];

                                for (size_t oe = 0; oe < opposite.linkCount; ++oe)
                                {
                                    const Tile::Link& reciprocal = tile->links[opposite.firstLink + oe];
                                    const TriangleID possibleNextID = ComputeTriangleID(tileID, link.triangle);

                                    if (reciprocal.triangle == ComputeTriangleIndex(currentNode.triangleID))
                                    {
                                        if (closedList.find(possibleNextID) != closedList.end())
                                        {
                                            break;
                                        }

                                        cameFrom[possibleNextID] = currentNode.triangleID;

                                        if (s > currentNode.percentageOfTotalDistance)
                                        {
                                            nextNode.incidentEdge = reciprocal.edge;
                                            nextNode.percentageOfTotalDistance = s;
                                            nextNode.triangleID = possibleNextID;
                                        }
                                        else
                                        {
                                            RaycastNode possibleNextEdge(possibleNextID, s, reciprocal.edge);
                                            openList.InsertElement(possibleNextEdge);
                                        }

                                        break;
                                    }
                                }
                            }
                            else if (side != Tile::Link::OffMesh)
                            {
                                TileID neighbourTileID = GetNeighbourTileID(container->x, container->y, container->z, link.side);
                                const TileContainer& neighbourContainer = m_tiles[neighbourTileID - 1];

                                const Tile::Triangle& opposite = neighbourContainer.tile.triangles[link.triangle];

                                const uint16 currentTriangleIndex = ComputeTriangleIndex(currentNode.triangleID);
                                const uint16 currentOppositeSide = OppositeSide(side);

                                for (size_t reciprocalLinkIndex = 0; reciprocalLinkIndex < opposite.linkCount; ++reciprocalLinkIndex)
                                {
                                    const Tile::Link& reciprocal = neighbourContainer.tile.links[opposite.firstLink + reciprocalLinkIndex];
                                    if ((reciprocal.triangle == currentTriangleIndex) && (reciprocal.side == currentOppositeSide))
                                    {
                                        const TriangleID possibleNextID = ComputeTriangleID(neighbourTileID, link.triangle);

                                        if (closedList.find(possibleNextID) != closedList.end())
                                        {
                                            break;
                                        }

                                        const vector3_t neighbourTileOrigin = vector3_t(
                                                real_t(neighbourContainer.x * m_params.tileSize.x),
                                                real_t(neighbourContainer.y * m_params.tileSize.y),
                                                real_t(neighbourContainer.z * m_params.tileSize.z));

                                        const uint16 i0 = reciprocal.edge;
                                        const uint16 i1 = inc_mod3[reciprocal.edge];

                                        assert(i0 < 3);
                                        assert(i1 < 3);

                                        const vector3_t c = neighbourTileOrigin + vector3_t(neighbourContainer.tile.vertices[opposite.vertex[i0]]);
                                        const vector3_t d = neighbourTileOrigin + vector3_t(neighbourContainer.tile.vertices[opposite.vertex[i1]]);

                                        real_t p, q;
                                        if (IntersectSegmentSegment(vector2_t(from), vector2_t(to), vector2_t(c), vector2_t(d), p, q))
                                        {
                                            cameFrom[possibleNextID] = currentNode.triangleID;

                                            if (p > currentNode.percentageOfTotalDistance)
                                            {
                                                nextNode.incidentEdge = reciprocal.edge;
                                                nextNode.percentageOfTotalDistance = p;
                                                nextNode.triangleID = possibleNextID;
                                            }
                                            else
                                            {
                                                RaycastNode possibleNextEdge(possibleNextID, p, reciprocal.edge);
                                                openList.InsertElement(possibleNextEdge);
                                            }

                                            break; // reciprocal link loop
                                        }
                                    }
                                }
                            }

                            if (nextNode.IsValid())
                            {
                                break;
                            }
                        }

                        if (nextNode.IsValid())
                        {
                            break;
                        }
                    }
                }
            }

            if (nextNode.IsValid())
            {
                openList.Reset();
                currentNode = nextNode;
            }
            else
            {
                currentNode = (!openList.IsEmpty()) ? openList.PopBestElement() : RaycastNode();
            }
        }

        ReconstructRaycastResult(fromTriangleID, furtherNodeVisited.triangleID, cameFrom, raycastRequest);

        RayHit& requestRayHit = raycastRequest.hit;
        requestRayHit.distance = rayHit.distance;
        requestRayHit.triangleID = rayHit.triangleID;
        requestRayHit.edge = rayHit.edge;

        return eRayCastResult_Hit;
    }

    MeshGrid::ERayCastResult MeshGrid::ReconstructRaycastResult(const TriangleID fromTriangleID, const TriangleID toTriangleID, const RaycastCameFromMap& cameFrom, RaycastRequestBase& raycastRequest) const
    {
        TriangleID currentTriangleID = toTriangleID;
        size_t elementIndex = 0;
        RaycastCameFromMap::const_iterator elementIterator = cameFrom.find(currentTriangleID);
        while (elementIterator != cameFrom.end())
        {
            if (elementIndex >= raycastRequest.maxWayTriCount)
            {
                return eRayCastResult_RayTooLong;
            }

            raycastRequest.way[elementIndex++] = currentTriangleID;
            currentTriangleID = elementIterator->second;
            elementIterator = cameFrom.find(currentTriangleID);
        }

        raycastRequest.way[elementIndex++] = currentTriangleID;
        raycastRequest.wayTriCount = elementIndex;

        return elementIndex < raycastRequest.maxWayTriCount ? eRayCastResult_NoHit : eRayCastResult_RayTooLong;
    }

    bool MeshGrid::IsLocationInTriangle(const vector3_t& location, const TriangleID triangleID) const
    {
        if (triangleID == MNM::Constants::InvalidTriangleID)
        {
            return false;
        }

        TileID tileID = ComputeTileID(triangleID);
        if (tileID == MNM::Constants::InvalidTileID)
        {
            return false;
        }

        const TileContainer* container = &m_tiles[tileID - 1];
        const Tile* tile = &container->tile;

        vector3_t tileOrigin(
            real_t(container->x * m_params.tileSize.x),
            real_t(container->y * m_params.tileSize.y),
            real_t(container->z * m_params.tileSize.z));

        if (triangleID)
        {
            const Tile::Triangle& triangle = tile->triangles[ComputeTriangleIndex(triangleID)];

            const vector2_t a = vector2_t(tileOrigin) + vector2_t(tile->vertices[triangle.vertex[0]]);
            const vector2_t b = vector2_t(tileOrigin) + vector2_t(tile->vertices[triangle.vertex[1]]);
            const vector2_t c = vector2_t(tileOrigin) + vector2_t(tile->vertices[triangle.vertex[2]]);

            return PointInTriangle(vector2_t(location), a, b, c);
        }

        return false;
    }

    MeshGrid::ERayCastResult MeshGrid::RayCast_old(const vector3_t& from, TriangleID fromTri, const vector3_t& to, TriangleID toTri, RaycastRequestBase& raycastRequest) const
    {
        FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

        if (TileID tileID = ComputeTileID(fromTri))
        {
            const TileContainer* container = &m_tiles[tileID - 1];
            const Tile* tile = &container->tile;

            vector3_t tileOrigin(
                real_t(container->x * m_params.tileSize.x),
                real_t(container->y * m_params.tileSize.y),
                real_t(container->z * m_params.tileSize.z));

            if (fromTri)
            {
                const Tile::Triangle& triangle = tile->triangles[ComputeTriangleIndex(fromTri)];

                const vector2_t a = vector2_t(tileOrigin) + vector2_t(tile->vertices[triangle.vertex[0]]);
                const vector2_t b = vector2_t(tileOrigin) + vector2_t(tile->vertices[triangle.vertex[1]]);
                const vector2_t c = vector2_t(tileOrigin) + vector2_t(tile->vertices[triangle.vertex[2]]);

                if (!PointInTriangle(vector2_t(from), a, b, c))
                {
                    fromTri = 0;
                }
            }

            if (!fromTri)
            {
                RayHit& hit = raycastRequest.hit;
                hit.distance = -real_t::max();
                hit.triangleID = 0;
                hit.edge = 0;

                return eRayCastResult_Hit;
            }

            real_t distance = -1;
            size_t triCount = 0;
            TriangleID currentID = fromTri;
            size_t incidentEdge = MNM::Constants::InvalidEdgeIndex;

            while (currentID)
            {
                if (triCount < raycastRequest.maxWayTriCount)
                {
                    raycastRequest.way[triCount++] = currentID;
                }
                else
                {
                    // We don't allow rays that pass through more than maxWayTriCount triangles
                    return eRayCastResult_RayTooLong;
                }

                if (toTri && currentID == toTri)
                {
                    raycastRequest.wayTriCount = triCount;
                    return eRayCastResult_NoHit;
                }

                const Tile::Triangle& triangle = tile->triangles[ComputeTriangleIndex(currentID)];
                TriangleID nextID = 0;
                real_t possibleDistance = distance;
                size_t possibleIncidentEdge = MNM::Constants::InvalidEdgeIndex;
                const TileContainer* possibleContainer = NULL;
                const Tile* possibleTile = NULL;
                vector3_t possibleTileOrigin(tileOrigin);
                TileID possibleTileID = tileID;

                for (size_t e = 0; e < 3; ++e)
                {
                    if (incidentEdge == e)
                    {
                        continue;
                    }

                    const vector3_t a = tileOrigin + vector3_t(tile->vertices[triangle.vertex[e]]);
                    const vector3_t b = tileOrigin + vector3_t(tile->vertices[triangle.vertex[next_mod3(e)]]);

                    real_t s, t;
                    if (IntersectSegmentSegment(vector2_t(from), vector2_t(to), vector2_t(a), vector2_t(b), s, t))
                    {
                        if (s < distance)
                        {
                            continue;
                        }

                        for (size_t l = 0; l < triangle.linkCount; ++l)
                        {
                            const Tile::Link& link = tile->links[triangle.firstLink + l];
                            if (link.edge != e)
                            {
                                continue;
                            }

                            const size_t side = link.side;

                            if (side == Tile::Link::Internal)
                            {
                                const Tile::Triangle& opposite = tile->triangles[link.triangle];

                                for (size_t oe = 0; oe < opposite.linkCount; ++oe)
                                {
                                    const Tile::Link& reciprocal = tile->links[opposite.firstLink + oe];
                                    const TriangleID possibleNextID = ComputeTriangleID(tileID, link.triangle);
                                    if (reciprocal.triangle == ComputeTriangleIndex(currentID))
                                    {
                                        distance = s;
                                        nextID = possibleNextID;
                                        possibleIncidentEdge = reciprocal.edge;
                                        possibleTile = tile;
                                        possibleTileOrigin = tileOrigin;
                                        possibleContainer = container;
                                        possibleTileID = tileID;

                                        break; // opposite edge loop
                                    }
                                }
                            }
                            else if (side != Tile::Link::OffMesh)
                            {
                                TileID neighbourTileID = GetNeighbourTileID(container->x, container->y, container->z, link.side);
                                const TileContainer& neighbourContainer = m_tiles[neighbourTileID - 1];

                                const Tile::Triangle& opposite = neighbourContainer.tile.triangles[link.triangle];

                                const uint16 currentTriangleIndex = ComputeTriangleIndex(currentID);
                                const uint16 currentOppositeSide = OppositeSide(side);

                                for (size_t rl = 0; rl < opposite.linkCount; ++rl)
                                {
                                    const Tile::Link& reciprocal = neighbourContainer.tile.links[opposite.firstLink + rl];
                                    if ((reciprocal.triangle == currentTriangleIndex) && (reciprocal.side == currentOppositeSide))
                                    {
                                        const vector3_t neighbourTileOrigin = vector3_t(
                                                real_t(neighbourContainer.x * m_params.tileSize.x),
                                                real_t(neighbourContainer.y * m_params.tileSize.y),
                                                real_t(neighbourContainer.z * m_params.tileSize.z));

                                        const uint16 i0 = reciprocal.edge;
                                        const uint16 i1 = next_mod3(reciprocal.edge);

                                        assert(i0 < 3);
                                        assert(i1 < 3);

                                        const vector3_t c = neighbourTileOrigin + vector3_t(
                                                neighbourContainer.tile.vertices[opposite.vertex[i0]]);
                                        const vector3_t d = neighbourTileOrigin + vector3_t(
                                                neighbourContainer.tile.vertices[opposite.vertex[i1]]);

                                        const TriangleID possibleNextID = ComputeTriangleID(neighbourTileID, link.triangle);
                                        real_t p, q;
                                        if (IntersectSegmentSegment(vector2_t(from), vector2_t(to), vector2_t(c), vector2_t(d), p, q))
                                        {
                                            distance = p;
                                            nextID = possibleNextID;
                                            possibleIncidentEdge = reciprocal.edge;

                                            possibleTileID = neighbourTileID;
                                            possibleContainer = &neighbourContainer;
                                            possibleTile = &neighbourContainer.tile;

                                            possibleTileOrigin = neighbourTileOrigin;
                                        }

                                        break; // reciprocal link loop
                                    }
                                }
                            }

                            if (nextID)
                            {
                                break; // link loop
                            }
                        }
                        if (IsTriangleAlreadyInWay(nextID, raycastRequest.way, triCount))
                        {
                            assert(0);
                            nextID = 0;
                        }

                        // If distance is equals to 0, it means that our starting position is placed
                        // on an edge or on a shared vertex. This means that we need to continue evaluating
                        // the other edges of the triangle to check if we have a better intersection.
                        // This will happen if the ray starts from one edge of the triangle but intersects also
                        // on of the other two edges. If we stop to the first intersection (the one with 0 distance)
                        // we can end up trying to raycast in the wrong direction.
                        // As soon as the distance is more than 0 we can stop the evaluation of the other edges
                        // because it means we are moving towards the direction of the ray.
                        distance = s;
                        const bool shouldStopEvaluationOfOtherEdges = distance > 0;
                        if (shouldStopEvaluationOfOtherEdges)
                        {
                            if (nextID)
                            {
                                break; // edge loop
                            }
                            else
                            {
                                RayHit& hit = raycastRequest.hit;
                                hit.distance = distance;
                                hit.triangleID = currentID;
                                hit.edge = e;

                                raycastRequest.wayTriCount = triCount;

                                return eRayCastResult_Hit;
                            }
                        }
                    }
                }

                currentID = nextID;
                incidentEdge = possibleIncidentEdge;
                tile = possibleTile ? possibleTile : tile;
                tileID = possibleTileID;
                container = possibleContainer ? possibleContainer : container;
                tileOrigin = possibleTileOrigin;
            }

            raycastRequest.wayTriCount = triCount;

            bool isEndingTriangleAcceptable = IsTriangleAcceptableForLocation(to, raycastRequest.way[triCount - 1]);
            return isEndingTriangleAcceptable ? eRayCastResult_NoHit : eRayCastResult_Unacceptable;
        }

        return eRayCastResult_InvalidStart;
    }

#pragma warning(push)
#   pragma warning(disable:28285)
    bool TestEdgeOverlap1D(const real_t& a0, const real_t& a1,
        const real_t& b0, const real_t& b1, const real_t& dx)
    {
        if ((a0 == b0) && (a1 == b1))
        {
            return true;
        }

        const real_t amin = a0 < a1 ? a0 : a1;
        const real_t amax = a0 < a1 ? a1 : a0;

        const real_t bmin = b0 < b1 ? b0 : b1;
        const real_t bmax = b0 < b1 ? b1 : b0;

        const real_t ominx = std::max(amin + dx, bmin + dx);
        const real_t omaxx = std::min(amax - dx, bmax - dx);

        if (ominx > omaxx)
        {
            return false;
        }

        return true;
    }
#pragma warning(pop)

    bool TestEdgeOverlap2D(const real_t& toleranceSq, const vector2_t& a0, const vector2_t& a1,
        const vector2_t& b0, const vector2_t& b1, const real_t& dx)
    {
        if ((a0.x == b0.x) && (a1.x == b1.x) && (a0.x == a1.x))
        {
            return TestEdgeOverlap1D(a0.y, a1.y, b0.y, b1.y, dx);
        }

        const vector2_t amin = a0.x < a1.x ? a0 : a1;
        const vector2_t amax = a0.x < a1.x ? a1 : a0;

        const vector2_t bmin = b0.x < b1.x ? b0 : b1;
        const vector2_t bmax = b0.x < b1.x ? b1 : b0;

        const real_t ominx = std::max(amin.x, bmin.x) + dx;
        const real_t omaxx = std::min(amax.x, bmax.x) - dx;

        if (ominx >= omaxx)
        {
            return false;
        }

        const real_t aslope = ((amax.x - amin.x) != 0) ? ((amax.y - amin.y) / (amax.x - amin.x)) : 0;
        const real_t a_cValue = amin.y - aslope * amin.x;

        const real_t bslope = ((bmax.x - bmin.x) != 0) ? ((bmax.y - bmin.y) / (bmax.x - bmin.x)) : 0;
        const real_t b_cValue = bmin.y - bslope * bmin.x;

        const real_t aominy = a_cValue + aslope * ominx;
        const real_t bominy = b_cValue + bslope * ominx;

        const real_t aomaxy = a_cValue + aslope * omaxx;
        const real_t bomaxy = b_cValue + bslope * omaxx;

        const real_t dminy = bominy - aominy;
        const real_t dmaxy = bomaxy - aomaxy;

        if ((square(dminy) > toleranceSq) || (square(dmaxy) > toleranceSq))
        {
            return false;
        }

        return true;
    }

    bool TestEdgeOverlap(size_t side, const real_t& toleranceSq, const vector3_t& a0, const vector3_t& a1,
        const vector3_t& b0, const vector3_t& b1)
    {
        const int ox = NeighbourOffset_MeshGrid[side][0];
        const int oy = NeighbourOffset_MeshGrid[side][1];
        const real_t dx = real_t::fraction(1, 1000);

        if (ox || oy)
        {
            const size_t dim = ox ? 0 : 1;
            const size_t odim = dim ^ 1;

            if ((a1[dim] - a0[dim] != 0) || (b1[dim] - b0[dim] != 0) || (a0[dim] != b0[dim]))
            {
                return false;
            }

            return TestEdgeOverlap2D(toleranceSq, vector2_t(a0[odim], a0.z), vector2_t(a1[odim], a1.z),
                vector2_t(b0[odim], b0.z), vector2_t(b1[odim], b1.z), dx);
        }
        else
        {
            if ((a1.z - a0.z != 0) || (b1.z - b0.z != 0) || (a0.z != b0.z))
            {
                return false;
            }

            return TestEdgeOverlap2D(toleranceSq, vector2_t(a0.x, a0.y), vector2_t(a1.x, a1.y),
                vector2_t(b0.x, b0.y), vector2_t(b1.x, b1.y), dx);
        }
    }

    TileID MeshGrid::SetTile(size_t x, size_t y, size_t z, Tile& tile)
    {
        FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

        assert((x <= max_x) && (y <= max_y) && (z <= max_z));

        const size_t tileName = ComputeTileName(x, y, z);

        std::pair<TileMap::iterator, bool> iresult = m_tileMap.insert(TileMap::value_type((uint32)tileName, 0));

        size_t tileID;

        if (iresult.second)
        {
            ++m_tileCount;

            if (m_frees.empty())
            {
                tileID = m_tileCount;

                if (m_tileCount > m_tileCapacity)
                {
                    Grow(std::max<size_t>(4, m_tileCapacity >> 1));
                }
            }
            else
            {
                tileID = m_frees.back() + 1;
                m_frees.pop_back();
            }

            iresult.first->second = tileID;

            m_profiler.AddStat(TileCount, 1);
        }
        else
        {
            tileID = iresult.first->second;

            Tile& oldTile = m_tiles[tileID - 1].tile;
            m_triangleCount -= oldTile.triangleCount;

            m_profiler.AddStat(VertexCount, -(int)oldTile.vertexCount);
            m_profiler.AddStat(TriangleCount, -(int)oldTile.triangleCount);
            m_profiler.AddStat(BVTreeNodeCount, -(int)oldTile.nodeCount);
            m_profiler.AddStat(LinkCount, -(int)oldTile.linkCount);

            m_profiler.FreeMemory(VertexMemory, oldTile.vertexCount * sizeof(Tile::Vertex));
            m_profiler.FreeMemory(TriangleMemory, oldTile.triangleCount * sizeof(Tile::Triangle));
            m_profiler.FreeMemory(BVTreeMemory, oldTile.nodeCount * sizeof(Tile::BVNode));
            m_profiler.FreeMemory(LinkMemory, oldTile.linkCount * sizeof(Tile::Link));

            oldTile.Destroy();
        }

        m_profiler.AddStat(VertexCount, tile.vertexCount);
        m_profiler.AddStat(TriangleCount, tile.triangleCount);
        m_profiler.AddStat(BVTreeNodeCount, tile.nodeCount);
        m_profiler.AddStat(LinkCount, tile.linkCount);

        m_profiler.AddMemory(VertexMemory, tile.vertexCount * sizeof(Tile::Vertex));
        m_profiler.AddMemory(TriangleMemory, tile.triangleCount * sizeof(Tile::Triangle));
        m_profiler.AddMemory(BVTreeMemory, tile.nodeCount * sizeof(Tile::BVNode));
        m_profiler.AddMemory(LinkMemory, tile.linkCount * sizeof(Tile::Link));

        m_profiler.FreeMemory(GridMemory, m_profiler[GridMemory].used);
        m_profiler.AddMemory(GridMemory, m_tileMap.size() * sizeof(TileMap::value_type));
        m_profiler.AddMemory(GridMemory, m_tileCapacity * sizeof(TileContainer));
        m_profiler.AddMemory(GridMemory, m_frees.capacity() * sizeof(Frees::value_type));

        m_triangleCount += tile.triangleCount;

        TileContainer& container = m_tiles[tileID - 1];
        container.x = x;
        container.y = y;
        container.z = z;
        container.tile.Swap(tile);
        tile.Destroy();

        return tileID;
    }

    void MeshGrid::ClearTile(TileID tileID, bool clearNetwork)
    {
        FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

        assert((tileID > 0) && (tileID <= m_tileCapacity));
        if ((tileID > 0) && (tileID <= m_tileCapacity))
        {
            TileContainer& container = m_tiles[tileID - 1];

            m_profiler.AddStat(VertexCount, -(int)container.tile.vertexCount);
            m_profiler.AddStat(TriangleCount, -(int)container.tile.triangleCount);
            m_profiler.AddStat(BVTreeNodeCount, -(int)container.tile.nodeCount);
            m_profiler.AddStat(LinkCount, -(int)container.tile.linkCount);

            m_profiler.FreeMemory(VertexMemory, container.tile.vertexCount * sizeof(Tile::Vertex));
            m_profiler.FreeMemory(TriangleMemory, container.tile.triangleCount * sizeof(Tile::Triangle));
            m_profiler.FreeMemory(BVTreeMemory, container.tile.nodeCount * sizeof(Tile::BVNode));
            m_profiler.FreeMemory(LinkMemory, container.tile.linkCount * sizeof(Tile::Link));

            m_profiler.AddStat(TileCount, -1);

            m_triangleCount -= container.tile.triangleCount;

            m_frees.push_back(tileID - 1);
            --m_tileCount;

            container.tile.Destroy();

            TileMap::iterator it = m_tileMap.find(ComputeTileName(container.x, container.y, container.z));
            assert(it != m_tileMap.end());
            m_tileMap.erase(it);

            if (clearNetwork)
            {
                for (size_t side = 0; side < SideCount; ++side)
                {
                    size_t nx = container.x + NeighbourOffset_MeshGrid[side][0];
                    size_t ny = container.y + NeighbourOffset_MeshGrid[side][1];
                    size_t nz = container.z + NeighbourOffset_MeshGrid[side][2];

                    if (TileID neighbourID = GetTileID(nx, ny, nz))
                    {
                        TileContainer& ncontainer = m_tiles[neighbourID - 1];

                        ReComputeAdjacency(ncontainer.x, ncontainer.y, ncontainer.z, kAdjecencyCalculationToleranceSq, ncontainer.tile,
                            OppositeSide(side), container.x, container.y, container.z, tileID);
                    }
                }
            }

            m_profiler.FreeMemory(GridMemory, m_profiler[GridMemory].used);
            m_profiler.AddMemory(GridMemory, m_tileMap.size() * sizeof(TileMap::value_type));
            m_profiler.AddMemory(GridMemory, m_tileCapacity * sizeof(TileContainer));
            m_profiler.AddMemory(GridMemory, m_frees.capacity() * sizeof(Frees::value_type));
        }
    }

    void MeshGrid::CreateNetwork()
    {
        FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

        TileMap::iterator it = m_tileMap.begin();
        TileMap::iterator end = m_tileMap.end();

        const real_t toleranceSq = square(real_t(std::max(m_params.voxelSize.x, m_params.voxelSize.z)));

        for (; it != end; ++it)
        {
            const TileID tileID = it->second;

            TileContainer& container = m_tiles[tileID - 1];

            ComputeAdjacency(container.x, container.y, container.z, toleranceSq, container.tile);
        }
    }

    void MeshGrid::ConnectToNetwork(TileID tileID)
    {
        FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

        assert((tileID > 0) && (tileID <= m_tileCapacity));
        if ((tileID > 0) && (tileID <= m_tileCapacity))
        {
            TileContainer& container = m_tiles[tileID - 1];

            ComputeAdjacency(container.x, container.y, container.z, kAdjecencyCalculationToleranceSq, container.tile);

            for (size_t side = 0; side < SideCount; ++side)
            {
                size_t nx = container.x + NeighbourOffset_MeshGrid[side][0];
                size_t ny = container.y + NeighbourOffset_MeshGrid[side][1];
                size_t nz = container.z + NeighbourOffset_MeshGrid[side][2];

                if (TileID neighbourID = GetTileID(nx, ny, nz))
                {
                    TileContainer& ncontainer = m_tiles[neighbourID - 1];

                    ReComputeAdjacency(ncontainer.x, ncontainer.y, ncontainer.z, kAdjecencyCalculationToleranceSq, ncontainer.tile,
                        OppositeSide(side), container.x, container.y, container.z, tileID);
                }
            }
        }
    }

    TileID MeshGrid::GetTileID(size_t x, size_t y, size_t z) const
    {
        const size_t tileName = ComputeTileName(x, y, z);

        TileMap::const_iterator it = m_tileMap.find(tileName);
        if (it != m_tileMap.end())
        {
            return it->second;
        }
        return 0;
    }

    const Tile& MeshGrid::GetTile(TileID tileID) const
    {
        assert(tileID > 0);
        assert(tileID <= m_tileCapacity);
        return m_tiles[tileID - 1].tile;
    }

    Tile& MeshGrid::GetTile(TileID tileID)
    {
        assert(tileID > 0);
        assert(tileID <= m_tileCapacity);
        return m_tiles[tileID - 1].tile;
    }

    const vector3_t MeshGrid::GetTileContainerCoordinates(TileID tileID) const
    {
        assert(tileID > 0);
        assert(tileID <= m_tileCapacity);
        const TileContainer& container = m_tiles[tileID - 1];
        return vector3_t(MNM::real_t(container.x), MNM::real_t(container.y), MNM::real_t(container.z));
    }

    void MeshGrid::Swap(MeshGrid& other)
    {
        std::swap(m_tiles, other.m_tiles);
        std::swap(m_tileCount, other.m_tileCount);
        std::swap(m_tileCapacity, other.m_tileCapacity);
        std::swap(m_triangleCount, other.m_triangleCount);

        m_frees.swap(other.m_frees);
        m_tileMap.swap(other.m_tileMap);

        std::swap(m_params, other.m_params);
        std::swap(m_profiler, other.m_profiler);
    }

    void MeshGrid::Draw(size_t drawFlags, TileID excludeID) const
    {
        TileMap::const_iterator it = m_tileMap.begin();
        TileMap::const_iterator end = m_tileMap.end();

        // collect areas
        // TODO: Clean this up!  Temporary to get up and running.
        std::vector<float> islandAreas(m_islands.size());
        for (size_t i = 0; i < m_islands.size(); ++i)
        {
            islandAreas[i] = m_islands[i].area;
        }

        for (; it != end; ++it)
        {
            if (excludeID == it->second)
            {
                continue;
            }

            const TileContainer& container = m_tiles[it->second - 1];

            container.tile.Draw(drawFlags, vector3_t(
                    real_t(m_params.origin.x + container.x * m_params.tileSize.x),
                    real_t(m_params.origin.y + container.y * m_params.tileSize.y),
                    real_t(m_params.origin.z + container.z * m_params.tileSize.z)),
                it->second,
                islandAreas);
        }
    }

    const MeshGrid::ProfilerType& MeshGrid::GetProfiler() const
    {
        return m_profiler;
    }

    struct Edge
    {
        uint16 vertex[2];
        uint16 triangle[2];
    };

    void ComputeTileTriangleAdjacency(Tile::Triangle* triangles, size_t triangleCount, size_t vertexCount,
        Edge* edges, uint16* adjacency)
    {
        FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

        {
            enum
            {
                Unused = 0xffff,
            };

            const size_t MaxLookUp = 4096;
            uint16 edgeLookUp[MaxLookUp];
            assert(MaxLookUp > vertexCount + triangleCount * 3);

            std::fill(&edgeLookUp[0], &edgeLookUp[0] + vertexCount, static_cast<uint16>(Unused));

            size_t edgeCount = 0;

            for (size_t i = 0; i < triangleCount; ++i)
            {
                const Tile::Triangle& triangle = triangles[i];

                for (size_t v = 0; v < 3; ++v)
                {
                    uint16 i1 = triangle.vertex[v];
                    uint16 i2 = triangle.vertex[next_mod3(v)];

                    if (i1 < i2)
                    {
                        size_t edgeIdx = edgeCount++;
                        adjacency[i * 3 + v] = edgeIdx;

                        Edge& edge = edges[edgeIdx];
                        edge.triangle[0] = i;
                        edge.triangle[1] = i;
                        edge.vertex[0] = i1;
                        edge.vertex[1] = i2;

                        edgeLookUp[vertexCount + edgeIdx] = edgeLookUp[i1];
                        edgeLookUp[i1] = edgeIdx;
                    }
                }
            }

            for (size_t i = 0; i < triangleCount; ++i)
            {
                const Tile::Triangle& triangle = triangles[i];

                for (size_t v = 0; v < 3; ++v)
                {
                    uint16 i1 = triangle.vertex[v];
                    uint16 i2 = triangle.vertex[next_mod3(v)];

                    if (i1 > i2)
                    {
                        size_t edgeIndex = edgeLookUp[i2];

                        for (; edgeIndex != Unused; edgeIndex = edgeLookUp[vertexCount + edgeIndex])
                        {
                            Edge& edge = edges[edgeIndex];

                            if ((edge.vertex[1] == i1) && (edge.triangle[0] == edge.triangle[1]))
                            {
                                edge.triangle[1] = i;
                                adjacency[i * 3 + v] = edgeIndex;
                                break;
                            }
                        }

                        if (edgeIndex == Unused)
                        {
                            size_t edgeIdx = edgeCount++;
                            adjacency[i * 3 + v] = edgeIdx;

                            Edge& edge = edges[edgeIdx];
                            edge.vertex[0] = i1;
                            edge.vertex[1] = i2;
                            edge.triangle[0] = i;
                            edge.triangle[1] = i;
                        }
                    }
                }
            }
        }
    }

    void MeshGrid::Grow(size_t amount)
    {
        const size_t oldCapacity = m_tileCapacity;

        m_tileCapacity = m_tileCapacity + amount;
        TileContainer* tiles = new TileContainer[m_tileCapacity];

        if (oldCapacity)
        {
            memcpy(tiles, m_tiles, oldCapacity * sizeof(TileContainer));
        }

        std::swap(m_tiles, tiles);
        delete[] tiles;
    }

    TileID MeshGrid::GetNeighbourTileID(size_t x, size_t y, size_t z, size_t side) const
    {
        size_t nx = x + NeighbourOffset_MeshGrid[side][0];
        size_t ny = y + NeighbourOffset_MeshGrid[side][1];
        size_t nz = z + NeighbourOffset_MeshGrid[side][2];

        return GetTileID(nx, ny, nz);
    }

    static const size_t MaxTriangleCount = 1024;

    struct SideTileInfo
    {
        SideTileInfo()
            : tile(0)
        {
        }

        Tile* tile;
        vector3_t offset;
    };

#pragma warning (push)
#pragma warning (disable: 6262)
    void MeshGrid::ComputeAdjacency(size_t x, size_t y, size_t z, const real_t& toleranceSq, Tile& tile)
    {
        FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

        const size_t vertexCount = tile.vertexCount;
        const Tile::Vertex* vertices = tile.vertices;

        const size_t triCount = tile.triangleCount;
        if (!triCount)
        {
            return;
        }

        m_profiler.StartTimer(NetworkConstruction);

        assert(triCount <= MaxTriangleCount);

        Edge edges[MaxTriangleCount * 3];
        uint16 adjacency[MaxTriangleCount * 3];
        ComputeTileTriangleAdjacency(tile.triangles, triCount, vertexCount, edges, adjacency);

        const size_t MaxLinkCount = MaxTriangleCount * 6;
        Tile::Link links[MaxLinkCount];
        size_t linkCount = 0;

        SideTileInfo sides[SideCount];

        for (size_t s = 0; s < SideCount; ++s)
        {
            SideTileInfo& side = sides[s];

            if (TileID id = GetTileID(x + NeighbourOffset_MeshGrid[s][0], y + NeighbourOffset_MeshGrid[s][1], z + NeighbourOffset_MeshGrid[s][2]))
            {
                side.tile = &GetTile(id);
                side.offset = vector3_t(
                        NeighbourOffset_MeshGrid[s][0] * m_params.tileSize.x,
                        NeighbourOffset_MeshGrid[s][1] * m_params.tileSize.y,
                        NeighbourOffset_MeshGrid[s][2] * m_params.tileSize.z);
            }
        }

        for (size_t i = 0; i < triCount; ++i)
        {
            size_t triLinkCount = 0;

            for (size_t e = 0; e < 3; ++e)
            {
                const size_t edgeIndex = adjacency[i * 3 + e];
                const Edge& edge = edges[edgeIndex];
                if ((edge.triangle[0] != i) && (edge.triangle[1] != i))
                {
                    continue;
                }

                if (edge.triangle[0] != edge.triangle[1])
                {
                    Tile::Link& link = links[linkCount++];
                    link.side = Tile::Link::Internal;
                    link.edge = e;
                    link.triangle = (edge.triangle[1] == i) ? edge.triangle[0] : edge.triangle[1];

                    ++triLinkCount;
                }
            }

            if (triLinkCount == 3)
            {
                Tile::Triangle& triangle = tile.triangles[i];
                triangle.linkCount = triLinkCount;
                triangle.firstLink = linkCount - triLinkCount;

                continue;
            }

            for (size_t e = 0; e < 3; ++e)
            {
                const size_t edgeIndex = adjacency[i * 3 + e];
                const Edge& edge = edges[edgeIndex];
                if ((edge.triangle[0] != i) && (edge.triangle[1] != i))
                {
                    continue;
                }

                if (edge.triangle[0] != edge.triangle[1])
                {
                    continue;
                }

                const vector3_t a0 = vector3_t(vertices[edge.vertex[0]]);
                const vector3_t a1 = vector3_t(vertices[edge.vertex[1]]);

                for (size_t s = 0; s < SideCount; ++s)
                {
                    if (const Tile* stile = sides[s].tile)
                    {
                        const vector3_t& offset = sides[s].offset;

                        const size_t striangleCount = stile->triangleCount;
                        const Tile::Triangle* striangles = stile->triangles;
                        const Tile::Vertex* svertices = stile->vertices;

                        for (size_t k = 0; k < striangleCount; ++k)
                        {
                            const Tile::Triangle& striangle = striangles[k];

                            for (size_t ne = 0; ne < 3; ++ne)
                            {
                                const vector3_t b0 = offset + vector3_t(svertices[striangle.vertex[ne]]);
                                const vector3_t b1 = offset + vector3_t(svertices[striangle.vertex[next_mod3(ne)]]);

                                if (TestEdgeOverlap(s, toleranceSq, a0, a1, b0, b1))
                                {
                                    Tile::Link& link = links[linkCount++];
                                    link.side = s;
                                    link.edge = e;
                                    link.triangle = k;

#if DEBUG_MNM_DATA_CONSISTENCY_ENABLED
                                    const TileID checkId = GetTileID(x + NeighbourOffset_MeshGrid[s][0], y + NeighbourOffset_MeshGrid[s][1], z + NeighbourOffset_MeshGrid[s][2]);
                                    BreakOnInvalidTriangle(ComputeTriangleID(checkId, k), m_tileCapacity);
#endif

                                    ++triLinkCount;
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            Tile::Triangle& triangle = tile.triangles[i];
            triangle.linkCount = triLinkCount;
            triangle.firstLink = linkCount - triLinkCount;
        }

        m_profiler.FreeMemory(LinkMemory, tile.linkCount * sizeof(Tile::Link));
        m_profiler.AddStat(LinkCount, -(int)tile.linkCount);

        tile.CopyLinks(links, linkCount);

        m_profiler.AddMemory(LinkMemory, tile.linkCount * sizeof(Tile::Link));
        m_profiler.AddStat(LinkCount, tile.linkCount);
        m_profiler.StopTimer(NetworkConstruction);
    }
#pragma warning (pop)

    void MeshGrid::ReComputeAdjacency(size_t x, size_t y, size_t z, const real_t& toleranceSq, Tile& tile,
        size_t side, size_t tx, size_t ty, size_t tz, TileID targetID)
    {
        FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

        const Tile::Vertex* vertices = tile.vertices;

        const size_t originTriangleCount = tile.triangleCount;
        if (!originTriangleCount)
        {
            return;
        }

        m_profiler.StartTimer(NetworkConstruction);

        if (!tile.linkCount)
        {
            ComputeAdjacency(x, y, z, toleranceSq, tile);
        }
        else
        {
            const vector3_t noffset = vector3_t(
                    NeighbourOffset_MeshGrid[side][0] * m_params.tileSize.x,
                    NeighbourOffset_MeshGrid[side][1] * m_params.tileSize.y,
                    NeighbourOffset_MeshGrid[side][2] * m_params.tileSize.z);

            assert(originTriangleCount <= MaxTriangleCount);

            const size_t MaxLinkCount = MaxTriangleCount * 6;
            Tile::Link links[MaxLinkCount];
            size_t linkCount = 0;

            const Tile* targetTile = targetID ? &m_tiles[targetID - 1].tile : 0;

            const Tile::Vertex* targetVertices = targetTile ? targetTile->vertices : 0;

            const size_t targetTriangleCount = targetTile ? targetTile->triangleCount : 0;
            const Tile::Triangle* targetTriangles = targetTile ? targetTile->triangles : 0;

            for (size_t i = 0; i < originTriangleCount; ++i)
            {
                size_t triLinkCount = 0;
                size_t triLinkCountI = 0;

                Tile::Triangle& originTriangle = tile.triangles[i];
                const size_t originLinkCount = originTriangle.linkCount;
                const size_t originFirstLink = originTriangle.firstLink;

                for (size_t l = 0; l < originLinkCount; ++l)
                {
                    const Tile::Link& originLink = tile.links[originFirstLink + l];

                    if ((originLink.side == Tile::Link::Internal) || (originLink.side != side))
                    {
                        links[linkCount++] = originLink;
                        ++triLinkCount;
                        triLinkCountI += (originLink.side == Tile::Link::Internal) ? 1 : 0;
                    }
                }

                if ((triLinkCountI == 3) || !targetID)
                {
                    originTriangle.linkCount = triLinkCount;
                    originTriangle.firstLink = linkCount - triLinkCount;

                    continue;
                }

                for (size_t e = 0; e < 3; ++e)
                {
                    const vector3_t a0 = vector3_t(vertices[originTriangle.vertex[e]]);
                    const vector3_t a1 = vector3_t(vertices[originTriangle.vertex[next_mod3(e)]]);

                    for (size_t k = 0; k < targetTriangleCount; ++k)
                    {
                        const Tile::Triangle& ttriangle = targetTriangles[k];

                        for (size_t ne = 0; ne < 3; ++ne)
                        {
                            const vector3_t b0 = noffset + vector3_t(targetVertices[ttriangle.vertex[ne]]);
                            const vector3_t b1 = noffset + vector3_t(targetVertices[ttriangle.vertex[next_mod3(ne)]]);

                            if (TestEdgeOverlap(side, toleranceSq, a0, a1, b0, b1))
                            {
                                Tile::Link& link = links[linkCount++];
                                link.side = side;
                                link.edge = e;
                                link.triangle = k;
                                ++triLinkCount;
#if DEBUG_MNM_DATA_CONSISTENCY_ENABLED
                                const TileID checkId = GetTileID(x + NeighbourOffset_MeshGrid[side][0], y + NeighbourOffset_MeshGrid[side][1], z + NeighbourOffset_MeshGrid[side][2]);
                                BreakOnInvalidTriangle(ComputeTriangleID(checkId, k), m_tileCapacity);
                                /*
                                Tile::Link testLink;
                                testLink.side = side;
                                testLink.edge = e;
                                testLink.triangle = k;
                                BreakOnMultipleAdjacencyLinkage(links, links+linkCount, testLink);
                                */
#endif
                                break;
                            }
                        }
                    }
                }

                originTriangle.linkCount = triLinkCount;
                originTriangle.firstLink = linkCount - triLinkCount;
            }

            m_profiler.FreeMemory(LinkMemory, tile.linkCount * sizeof(Tile::Link));
            m_profiler.AddStat(LinkCount, -(int)tile.linkCount);

            tile.CopyLinks(links, linkCount);

            m_profiler.AddMemory(LinkMemory, tile.linkCount * sizeof(Tile::Link));
            m_profiler.AddStat(LinkCount, tile.linkCount);
        }

        m_profiler.StopTimer(NetworkConstruction);
    }

    bool MeshGrid::CalculateMidEdge(const TriangleID triangleID1, const TriangleID triangleID2, Vec3& result) const
    {
        if (triangleID1 == triangleID2)
        {
            return false;
        }

        if (const TileID tileID = ComputeTileID(triangleID1))
        {
            const TileContainer& container = m_tiles[tileID - 1];
            const Tile& tile = container.tile;
            const uint16 triangleIdx = ComputeTriangleIndex(triangleID1);
            const Tile::Triangle& triangle = tile.triangles[triangleIdx];

            uint16 vi0 = 0, vi1 = 0;

            for (size_t l = 0; l < triangle.linkCount; ++l)
            {
                const Tile::Link& link = tile.links[triangle.firstLink + l];

                if (link.side == Tile::Link::Internal)
                {
                    TriangleID linkedTriID = ComputeTriangleID(tileID, link.triangle);
                    if (linkedTriID == triangleID2)
                    {
                        vi0 = link.edge;
                        vi1 = (link.edge + 1) % 3;
                        assert(vi0 < 3);
                        assert(vi1 < 3);
                        break;
                    }
                }
                else if (link.side != Tile::Link::OffMesh)
                {
                    TileID neighbourTileID = GetNeighbourTileID(container.x, container.y, container.z, link.side);
                    TriangleID linkedTriID = ComputeTriangleID(neighbourTileID, link.triangle);
                    if (linkedTriID == triangleID2)
                    {
                        vi0 = link.edge;
                        vi1 = (link.edge + 1) % 3;
                        assert(vi0 < 3);
                        assert(vi1 < 3);
                        break;
                    }
                }
            }

            if (vi0 != vi1)
            {
                vector3_t v0, v1, v2;
                GetVertices(triangleID1, v0, v1, v2);
                vector3_t vertices[3] = {v0, v1, v2};

                PREFAST_ASSUME(vi0 < 3 && vi1 < 3);
                result = (vertices[vi0] + vertices[vi1]).GetVec3() * 0.5f;
                return true;
            }
        }

        return false;
    }

    //////////////////////////////////////////////////////////////////////////

#if MNM_USE_EXPORT_INFORMATION

    void MeshGrid::ResetAccessibility(uint8 accessible)
    {
        for (TileMap::iterator tileIt = m_tileMap.begin(); tileIt != m_tileMap.end(); ++tileIt)
        {
            Tile& tile = m_tiles[tileIt->second - 1].tile;

            tile.ResetConnectivity(accessible);
        }
    }

    void MeshGrid::ComputeAccessibility(const AccessibilityRequest& inputRequest)
    {
        struct Node
        {
            Node (TriangleID _id, TriangleID _previousID)
                : id(_id)
                , previousId(_previousID)
            {
            }

            TriangleID id;
            TriangleID previousId;
        };

        std::vector<TriangleID> nextTriangles;
        nextTriangles.reserve(16);

        std::vector<Node>       openNodes;
        openNodes.reserve(m_triangleCount);

        if (const TileID startTileID = ComputeTileID(inputRequest.fromTriangle))
        {
            const uint16 startTriangleIdx = ComputeTriangleIndex(inputRequest.fromTriangle);
            Tile& startTile = m_tiles[startTileID - 1].tile;
            startTile.SetTriangleAccessible(startTriangleIdx);

            openNodes.push_back(Node(inputRequest.fromTriangle, 0));

            while (!openNodes.empty())
            {
                const Node currentNode = openNodes.back();
                openNodes.pop_back();

                if (const TileID tileID = ComputeTileID(currentNode.id))
                {
                    const uint16 triangleIdx = ComputeTriangleIndex(currentNode.id);

                    TileContainer& container = m_tiles[tileID - 1];
                    Tile& tile = container.tile;

                    const Tile::Triangle& triangle = tile.triangles[triangleIdx];

                    nextTriangles.clear();

                    // Collect all accessible nodes from the current one
                    for (size_t l = 0; l < triangle.linkCount; ++l)
                    {
                        const Tile::Link& link = tile.links[triangle.firstLink + l];

                        TriangleID nextTriangleID;

                        if (link.side == Tile::Link::Internal)
                        {
                            nextTriangleID = ComputeTriangleID(tileID, link.triangle);
                        }
                        else if (link.side != Tile::Link::OffMesh)
                        {
                            TileID neighbourTileID = GetNeighbourTileID(container.x, container.y, container.z, link.side);
                            nextTriangleID = ComputeTriangleID(neighbourTileID, link.triangle);
                        }
                        else
                        {
                            OffMeshNavigation::QueryLinksResult links = inputRequest.offMeshNavigation.GetLinksForTriangle(currentNode.id, link.triangle);
                            while (WayTriangleData nextLink = links.GetNextTriangle())
                            {
                                nextTriangles.push_back(nextLink.triangleID);
                            }
                            continue;
                        }

                        nextTriangles.push_back(nextTriangleID);
                    }

                    // Add them to the open list if not visited already
                    for (size_t t = 0; t < nextTriangles.size(); ++t)
                    {
                        const TriangleID nextTriangleID = nextTriangles[t];

                        // Skip if previous triangle
                        if (nextTriangleID == currentNode.previousId)
                        {
                            continue;
                        }

                        // Skip if already visited
                        if (const TileID nextTileID = ComputeTileID(nextTriangleID))
                        {
                            Tile& nextTile = m_tiles[nextTileID - 1].tile;
                            const uint16 nextTriangleIndex = ComputeTriangleIndex(nextTriangleID);
                            if (nextTile.IsTriangleAccessible(nextTriangleIndex))
                            {
                                continue;
                            }

                            nextTile.SetTriangleAccessible(nextTriangleIndex);

                            openNodes.push_back(Node(nextTriangleID, currentNode.id));
                        }
                    }
                }
            }
        }
    }

#endif
}
