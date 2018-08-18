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

#ifndef CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_MESHGRID_H
#define CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_MESHGRID_H
#pragma once

#include "MNM.h"
#include "Tile.h"
#include "Profiler.h"

#include <SimpleHashLookUp.h>
#include <VectorMap.h>

namespace MNM
{
    struct OffMeshNavigation;
    class IslandConnections;

    ///////////////////////////////////////////////////////////////////////

    // Heuristics for dangers
    enum EWeightCalculationType
    {
        eWCT_None = 0,
        eWCT_Range,           // It returns 1 as a weight for the cost if the location to evaluate
                              //  is in the specific range from the threat, 0 otherwise
        eWCT_InverseDistance, // It returns the inverse of the distance as a weight for the cost
        eWCT_Direction,       // It returns a value between [0,1] as a weight for the cost in relation of
                              // how the location to evaluate is in the threat direction. The range is not taken into
                              // account with this weight calculation type
        eWCT_Last,
    };

    template<EWeightCalculationType WeightType>
    struct DangerWeightCalculation
    {
        MNM::real_t CalculateWeight(const Vec3& locationToEval, const Vec3& startingLocation, const Vec3& dangerPosition, const float rangeSq) const
        {
            return MNM::real_t(.0f);
        }
    };

    template<>
    struct DangerWeightCalculation<eWCT_InverseDistance>
    {
        MNM::real_t CalculateWeight(const Vec3& locationToEval, const Vec3& startingLocation, const Vec3& dangerPosition, const float rangeSq) const
        {
            // TODO: This is currently not used, but if we see we need it, then we should think to change
            // the euclidean distance calculation with a faster approximation (Like the one used in the FindWay function)
            const float distance = (dangerPosition - locationToEval).len();
            bool isInRange = rangeSq > .0f ? sqr(distance) > rangeSq : true;
            return isInRange ? MNM::real_t(1 / distance) : MNM::real_t(.0f);
        }
    };

    template<>
    struct DangerWeightCalculation<eWCT_Range>
    {
        MNM::real_t CalculateWeight(const Vec3& locationToEval, const Vec3& startingLocation, const Vec3& dangerPosition, const float rangeSq) const
        {
            const Vec3 dangerToLocationDir = locationToEval - dangerPosition;
            const float weight = static_cast<float>(fsel(dangerToLocationDir.len2() - rangeSq, 0.0f, 1.0f));
            return MNM::real_t(weight);
        }
    };

    template<>
    struct DangerWeightCalculation<eWCT_Direction>
    {
        real_t CalculateWeight(const Vec3& locationToEval, const Vec3& startingLocation, const Vec3& dangerPosition, const float rangeSq) const
        {
            Vec3 startLocationToNewLocation = (locationToEval - startingLocation);
            startLocationToNewLocation.NormalizeSafe();
            Vec3 startLocationToDangerPosition = (dangerPosition - startingLocation);
            startLocationToDangerPosition.NormalizeSafe();
            float dotProduct = startLocationToNewLocation.dot(startLocationToDangerPosition);
            return max(real_t(.0f), real_t(dotProduct));
        }
    };

    struct DangerArea
    {
        virtual ~DangerArea() {}
        virtual real_t GetDangerHeuristicCost(const Vec3& locationToEval, const Vec3& startingLocation) const = 0;
        virtual const Vec3& GetLocation() const = 0;
    };
    DECLARE_SMART_POINTERS(DangerArea)

    template<EWeightCalculationType WeightType>
    struct DangerAreaT
        : public DangerArea
    {
        DangerAreaT()
            : location(ZERO)
            , effectRangeSq(.0f)
            , cost(0)
            , weightCalculator()
        {}

        DangerAreaT(const Vec3& _location, const float _effectRange, const uint8 _cost)
            : location(_location)
            , effectRangeSq(sqr(_effectRange))
            , cost(_cost)
            , weightCalculator()
        {}

        virtual real_t GetDangerHeuristicCost(const Vec3& locationToEval, const Vec3& startingLocation) const
        {
            return real_t(cost) * weightCalculator.CalculateWeight(locationToEval, startingLocation, location, effectRangeSq);
        }

        virtual const Vec3& GetLocation() const { return location; }

    private:
        const Vec3  location;          // Position of the danger
        const float effectRangeSq;       // If zero the effect is on the whole level
        const unsigned int cost;       // Absolute cost associated with the Danger represented by the DangerAreaT
        const DangerWeightCalculation<WeightType> weightCalculator;
    };

    enum
    {
        max_danger_amount = 5,
    };

    typedef CryFixedArray<MNM::DangerAreaConstPtr, max_danger_amount> DangerousAreasList;

    ///////////////////////////////////////////////////////////////////////

    struct MeshGrid
    {
        enum
        {
            x_bits = 11,
        };                     // must add up to a 32 bit - the "tileName"
        enum
        {
            y_bits = 11,
        };
        enum
        {
            z_bits = 10,
        };

        enum
        {
            max_x = (1 << x_bits) - 1,
        };
        enum
        {
            max_y = (1 << y_bits) - 1,
        };
        enum
        {
            max_z = (1 << z_bits) - 1,
        };

        struct Params
        {
            Params()
                : origin(ZERO)
                , tileSize(16)
                , tileCount(1024)
                , voxelSize(0.1f)
            {
            }

            Vec3   origin;
            Vec3i  tileSize;
            Vec3   voxelSize;
            uint32 tileCount;
        };

        enum
        {
            SideCount = 14,
        };

        enum EPredictionType
        {
            ePredictionType_TriangleCenter = 0,
            ePredictionType_Advanced,
            ePredictionType_Latest,
        };

        struct WayQueryRequest
        {
            WayQueryRequest(const IAIPathAgent* pRequester, TriangleID _from, const vector3_t& _fromLocation, TriangleID _to,
                const vector3_t& _toLocation, const OffMeshNavigation& _offMeshNavigation,
                const DangerousAreasList& dangerousAreas)
                : m_from(_from)
                , m_to(_to)
                , m_fromLocation(_fromLocation)
                , m_toLocation(_toLocation)
                , m_offMeshNavigation(_offMeshNavigation)
                , m_dangerousAreas(dangerousAreas)
                , m_pRequester(pRequester)
            {}

            virtual ~WayQueryRequest(){}

            virtual bool CanUseOffMeshLink(const OffMeshLinkID linkID, float* costMultiplier) const;
            bool IsPointValidForAgent(const Vec3& pos, uint32 flags) const;

            ILINE const TriangleID From() const { return m_from; }
            ILINE const TriangleID To() const { return m_to; }
            ILINE const OffMeshNavigation& GetOffMeshNavigation() const { return m_offMeshNavigation; }
            ILINE const DangerousAreasList& GetDangersInfos() { return m_dangerousAreas; }
            ILINE const vector3_t& GetFromLocation() const {return m_fromLocation; };
            ILINE const vector3_t& GetToLocation() const {return m_toLocation; };

        private:
            const TriangleID         m_from;
            const TriangleID         m_to;
            const vector3_t          m_fromLocation;
            const vector3_t          m_toLocation;
            const OffMeshNavigation& m_offMeshNavigation;
            DangerousAreasList       m_dangerousAreas;
        protected:
            const IAIPathAgent*      m_pRequester;
        };

        struct WayQueryWorkingSet
        {
            WayQueryWorkingSet()
            {
                nextLinkedTriangles.reserve(32);
            }
            typedef std::vector<WayTriangleData> TNextLinkedTriangles;

            void Reset()
            {
                aStarOpenList.Reset();
                nextLinkedTriangles.clear();
                nextLinkedTriangles.reserve(32);
            }

            TNextLinkedTriangles nextLinkedTriangles;
            AStarOpenList aStarOpenList;
        };

        struct WayQueryResult
        {
            WayQueryResult(const size_t wayMaxSize = 512)
                : m_wayMaxSize(wayMaxSize)
            {
                m_pWayTriData.reset(new WayTriangleData[m_wayMaxSize]);
            }

            ~WayQueryResult()
            {
            }

            void Reset()
            {
                m_pWayTriData.reset(new WayTriangleData[m_wayMaxSize]);
                m_waySize = 0;
            }

            ILINE WayTriangleData* GetWayData() const { return m_pWayTriData.get(); }
            ILINE size_t GetWaySize() const { return m_waySize; }
            ILINE size_t GetWayMaxSize() const { return m_wayMaxSize; }

            ILINE void SetWaySize(const size_t waySize) { m_waySize = waySize; }

            ILINE void Clear() { m_waySize = 0; }
        private:

            AZStd::shared_ptr<WayTriangleData> m_pWayTriData;
            size_t m_wayMaxSize;
            size_t m_waySize;
        };

        enum EWayQueryResult
        {
            eWQR_Continuing = 0,
            eWQR_Done,
        };

        MeshGrid();
        ~MeshGrid();

        void Init(const Params& params);

        static inline size_t ComputeTileName(size_t x, size_t y, size_t z)
        {
            return (x & ((1 << x_bits) - 1)) |
                   ((y & ((1 << y_bits) - 1)) << x_bits) |
                   ((z & ((1 << z_bits) - 1)) << (x_bits + y_bits));
        }

        static inline void ComputeTileXYZ(size_t tileName, size_t& x, size_t& y, size_t& z)
        {
            x = tileName & ((1 << x_bits) - 1);
            y = (tileName >> x_bits) & ((1 << y_bits) - 1);
            z = (tileName >> (x_bits + y_bits)) & ((1 << z_bits) - 1);
        }

        size_t GetTriangles(aabb_t aabb, TriangleID* triangles, size_t maxTriCount, float minIslandArea = 0.f) const;
        TriangleID GetTriangleAt(const vector3_t& location, const real_t verticalDownwardRange, const real_t verticalUpwardRange, float minIslandArea = 0.f) const;
        TriangleID GetClosestTriangle(const vector3_t& location, real_t vrange, real_t hrange, real_t* distSq = 0, vector3_t* closest = 0, float minIslandArea = 0.f) const;


        TriangleID GetTriangleEdgeAlongLine(const vector3_t& startLocation, const vector3_t& endLocation,
            const real_t verticalDownwardRange, const real_t verticalUpwardRange,
            vector3_t& hit, float minIslandArea = 0.f) const;
        bool IsTriangleAcceptableForLocation(const vector3_t& location, TriangleID triangleID) const;


        bool GetVertices(TriangleID triangleID, vector3_t& v0, vector3_t& v1, vector3_t& v2) const;
        bool GetVertices(TriangleID triangleID, vector3_t* verts) const;
        // Sets a bit in linkedEdges for each edge on this triangle that has a link.
        // Bit 0: v0->v1, Bit 1: v1->v2, Bit 2: v2->v0
        bool GetLinkedEdges(TriangleID triangleID, size_t& linkedEdges) const;
        bool GetTriangle(TriangleID triangleID, Tile::Triangle& triangle) const;

        bool PushPointInsideTriangle(const TriangleID triangleID, vector3_t& location, real_t amount) const;

        void IncrementCountOfPathsPassingThroughTriangleId(TriangleID triangleID);
        void DecrementCountOfPathsPassingThroughTriangleId(TriangleID triangleID);

        inline size_t GetTriangleCount() const
        {
            return m_triangleCount;
        }

        void AddOffMeshLinkToTile(const TileID tileID, const TriangleID triangleID, const uint16 offMeshIndex);
        void UpdateOffMeshLinkForTile(const TileID tileID, const TriangleID triangleID, const uint16 offMeshIndex);
        void RemoveOffMeshLinkFromTile(const TileID tileID, const TriangleID triangleID);

        MeshGrid::EWayQueryResult FindWay(WayQueryRequest& inputRequest, WayQueryWorkingSet& workingSet, WayQueryResult& result) const;
        real_t CalculateHeuristicCostForDangers(const vector3_t& locationToEval, const vector3_t& startingLocation, const Vec3& meshOrigin, const DangerousAreasList& dangersInfos) const;
        void PullString(const vector3_t from, const TriangleID fromTriID, const vector3_t to, const TriangleID toTriID, vector3_t& middlePoint) const;

        struct RayHit
        {
            TriangleID triangleID;
            real_t distance;
            size_t edge;
        };

        /*
            ********************************************************************************************
            RayCastRequestBase holds the actual request information needed to perform a RayCast request.
            It needs to be constructed through the RayCastRequest that assures the presence of the way.
            ********************************************************************************************
        */

        struct RaycastRequestBase
        {
        protected:
            // This class can't be instantiated directly.
            RaycastRequestBase(const size_t _maxWayTriCount)
                : maxWayTriCount(_maxWayTriCount)
                , way(NULL)
                , wayTriCount(0)
            {}

        public:
            RayHit       hit;
            TriangleID*  way;
            size_t       wayTriCount;
            const size_t maxWayTriCount;
        };

        template<size_t MaximumNumberOfTrianglesInWay>
        struct RayCastRequest
            : public RaycastRequestBase
        {
            RayCastRequest()
                : RaycastRequestBase(MaximumNumberOfTrianglesInWay)
            {
                way = &(wayArray[0]);
                wayTriCount = 0;
            }
        private:
            TriangleID wayArray[MaximumNumberOfTrianglesInWay];
        };

        // ********************************************************************************************

        enum ERayCastResult
        {
            eRayCastResult_NoHit = 0,
            eRayCastResult_Hit,
            eRayCastResult_RayTooLong,
            eRayCastResult_Unacceptable,
            eRayCastResult_InvalidStart,
            eRayCastResult_InvalidEnd,
        };

        ERayCastResult RayCast(const vector3_t& from, TriangleID fromTri, const vector3_t& to, TriangleID toTri,
            RaycastRequestBase& wayRequest) const;
        ERayCastResult RayCast_new(const vector3_t& from, TriangleID fromTriangleID, const vector3_t& to, TriangleID toTriangleID, RaycastRequestBase& wayRequest) const;
        ERayCastResult RayCast_old(const vector3_t& from, TriangleID fromTri, const vector3_t& to, TriangleID toTri, RaycastRequestBase& wayRequest) const;

        //! Computes intersection (if any) of a world-space segment with the mesh grid
        /*!
        \param segP0 First point of world segment
        \param segP1 Second point of world segment
        \return A 3-tuple: boolean indicating collision, time of collision, point of collision
        */
        std::tuple<bool, float, Vec3> RayCastWorld(const Vec3& segP0, const Vec3& segP1) const;

        TileID SetTile(size_t x, size_t y, size_t z, Tile& tile);
        void ClearTile(TileID tileID, bool clearNetwork = true);

        ILINE void SetTotalIslands(uint32 totalIslands) { m_islands.resize(totalIslands); }
        ILINE uint32 GetTotalIslands() const { return m_islands.size(); }
        float GetIslandArea(StaticIslandID islandID) const;
        float GetIslandAreaForTriangle(TriangleID triangle) const;

        void CreateNetwork();
        void ConnectToNetwork(TileID tileID);

        inline bool Empty() const
        {
            return m_tileCount == 0;
        }

        inline size_t GetTileCount() const
        {
            return m_tileCount;
        }

        TileID GetTileID(size_t x, size_t y, size_t z) const;
        const Tile& GetTile(TileID) const;
        Tile& GetTile(TileID);
        const vector3_t GetTileContainerCoordinates(TileID) const;


        void Swap(MeshGrid& other);

        inline const Params& GetParams() const
        {
            return m_params;
        }

        inline void OffsetOrigin(const Vec3& offset)
        {
            m_params.origin += offset;
        }

        void Draw(size_t drawFlags, TileID excludeID = 0) const;

        bool CalculateMidEdge(const TriangleID triangleID1, const TriangleID triangleID2, Vec3& result) const;

        enum ProfilerTimers
        {
            NetworkConstruction = 0,
        };

        enum ProfilerMemoryUsers
        {
            TriangleMemory = 0,
            VertexMemory,
            BVTreeMemory,
            LinkMemory,
            GridMemory,
        };

        enum ProfilerStats
        {
            TileCount = 0,
            TriangleCount,
            VertexCount,
            BVTreeNodeCount,
            LinkCount,
        };

        typedef Profiler<ProfilerMemoryUsers, ProfilerTimers, ProfilerStats> ProfilerType;
        const ProfilerType& GetProfiler() const;

#if MNM_USE_EXPORT_INFORMATION
        enum EAccessibilityRequestValue
        {
            eARNotAccessible = 0,
            eARAccessible = 1
        };

        struct AccessibilityRequest
        {
            AccessibilityRequest(TriangleID _fromTriangleId, const OffMeshNavigation& _offMeshNavigation)
                : fromTriangle(_fromTriangleId)
                , offMeshNavigation(_offMeshNavigation)
            {}

            const TriangleID fromTriangle;
            const OffMeshNavigation& offMeshNavigation;
        };

        void ResetAccessibility(uint8 accessible);
        void ComputeAccessibility(const AccessibilityRequest& inputRequest);
#endif

        void ResetConnectedIslandsIDs();
        void ComputeStaticIslandsAndConnections(const NavigationMeshID meshID, const MNM::OffMeshNavigation& offMeshNavigation, MNM::IslandConnections& islandConnections);

        TileID GetNeighbourTileID(size_t x, size_t y, size_t z, size_t side) const;

    private:

        struct Island
        {
            Island() {}
            Island(StaticIslandID _id)
                : id(_id)
                , area(0.f)
            {}

            StaticIslandID id;
            float area;
        };

        Island& GetNewIsland();
        void QueueIslandConnectionSetup(const StaticIslandID islandID, const TriangleID startingTriangleID, const uint16 offMeshLinkIndex);
        void ResolvePendingIslandConnectionRequests(const NavigationMeshID meshID, const MNM::OffMeshNavigation& offMeshNavigation, MNM::IslandConnections& islandConnections);
        void SearchForIslandConnectionsToRefresh(const TileID tileID);
        void ComputeStaticIslands();

        void PredictNextTriangleEntryPosition(const TriangleID bestNodeTriangleID, const vector3_t& startPosition, const TriangleID nextTriangleID, const unsigned int vertexIndex, const vector3_t finalLocation, vector3_t& outPosition) const;

    protected:
        void Grow(size_t amount);
        void ComputeAdjacency(size_t x, size_t y, size_t z, const real_t& toleranceSq, Tile& tile);
        void ReComputeAdjacency(size_t x, size_t y, size_t z, const real_t& toleranceSq, Tile& tile,
            size_t side, size_t tx, size_t ty, size_t tz, TileID targetID);

        bool IsLocationInTriangle(const vector3_t& location, const TriangleID triangleID) const;
        typedef VectorMap<TriangleID, TriangleID> RaycastCameFromMap;
        ERayCastResult ReconstructRaycastResult(const TriangleID fromTriangleID, const TriangleID toTriangleID, const RaycastCameFromMap& comeFrom, RaycastRequestBase& raycastRequest) const;

        struct TileContainer
        {
            TileContainer()
                : x(0)
                , y(0)
                , z(0)
            {
            }

            uint32 x: x_bits;
            uint32 y: y_bits;
            uint32 z: z_bits;

            Tile tile;
        };

        TileContainer* m_tiles;
        size_t m_tileCount;
        size_t m_tileCapacity;
        size_t m_triangleCount;

        typedef std::vector<size_t> Frees;
        Frees m_frees;

        // replace with something more efficient will speed up path-finding, ray-casting and
        // connectivity computation
        typedef std::map<uint32, uint32> TileMap;
        TileMap m_tileMap;

        std::vector<Island> m_islands;

        struct IslandConnectionRequest
        {
            IslandConnectionRequest(const StaticIslandID _startingIslandID, const TriangleID _startingTriangleID, const uint16 _offMeshLinkIndex)
                : startingIslandID(_startingIslandID)
                , startingTriangleID(_startingTriangleID)
                , offMeshLinkIndex(_offMeshLinkIndex)
            {
            }

            StaticIslandID startingIslandID;
            TriangleID startingTriangleID;
            uint16 offMeshLinkIndex;
        };
        std::vector<IslandConnectionRequest> m_islandConnectionRequests;

        Params m_params;
        ProfilerType m_profiler;

        static const real_t kMinPullingThreshold;
        static const real_t kMaxPullingThreshold;
        static const real_t kAdjecencyCalculationToleranceSq;
    };
}

#endif // CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_MESHGRID_H
