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

#ifndef CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_TILE_H
#define CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_TILE_H
#pragma once

#include "MNM.h"

namespace MNM
{
    struct Tile
    {
        typedef uint16 Index;
        typedef FixedVec3<uint16, 5> Vertex;
        typedef FixedAABB<uint16, 5> AABB;

        struct Triangle
        {
            Triangle() {}

            Index vertex[3];         // Vertex index into Tile::vertices array
            uint16 linkCount : 4;    // Total link count connected to this triangle (begins at 'firstLink')
            uint16 firstLink : 12;   // First link index into Tile::links array
            StaticIslandID islandID; // Island for this triangle

            AUTO_STRUCT_INFO
        };

        struct BVNode
        {
            uint16 leaf : 1;
            uint16 offset : 15;

            AABB aabb;

            AUTO_STRUCT_INFO
        };

        struct Link
        {
            uint16 side : 4;      // Side of the tile (cube) this link is associated with [0..13] or enumerated link type
                                  // (See NeighbourOffset_MeshGrid and the enumeration below)
            uint16 edge : 2;      // Local edge index of the triangle this link is on [0..2]
                                  // (NOTE: 'edge' is not used for off-mesh links)
            uint16 triangle : 10; // Index into Tile::triangles
                                  // (NOTE: This bit count makes our max triangle count 1024)
                                  // (NOTE: 'triangle' is re-purposed to be the index of the off-mesh index for off-mesh links)

            // This enumeration can be assigned to the 'side' variable
            // which generally would index the 14 element array
            // NeighbourOffset_MeshGrid, found in MeshGrid.cpp.
            // If 'side' is found to be one of the values below,
            // it will not be used as a lookup into the table but instead
            // as a flag so we need to be sure these start after the final
            // index and do not exceed the 'side' variable's bit count.
            enum
            {
                OffMesh  = 0xe,
            };                        // A link to a nonadjacent triangle
            enum
            {
                Internal = 0xf,
            };                        // A link to an adjacent triangle (not exposed to the tile edge)

            AUTO_STRUCT_INFO
        };

        Tile();

        void CopyTriangles(Triangle* triangles, uint16 count);
        void CopyVertices(Vertex* vertices, uint16 count);
        void CopyNodes(BVNode* nodes, uint16 count);
        void CopyLinks(Link* links, uint16 count);

        void AddOffMeshLink(const TriangleID triangleID, const uint16 offMeshIndex);
        void UpdateOffMeshLink(const TriangleID triangleID, const uint16 offMeshIndex);
        void RemoveOffMeshLink(const TriangleID triangleID);

        void Swap(Tile& other);
        void Destroy();

        vector3_t::value_type GetTriangleArea(const TriangleID triangleID) const;

#if MNM_USE_EXPORT_INFORMATION
        void ResetConnectivity(uint8 accessible);
        inline bool IsTriangleAccessible(const uint16 triangleIdx) const
        {
            assert((triangleIdx >= 0) && (triangleIdx < connectivity.triangleCount));
            return (connectivity.trianglesAccessible[triangleIdx] != 0);
        }
        inline bool IsTileAccessible() const
        {
            return (connectivity.tileAccessible != 0);
        }

        inline void SetTriangleAccessible(const uint16 triangleIdx)
        {
            assert((triangleIdx >= 0) && (triangleIdx < connectivity.triangleCount));

            connectivity.tileAccessible = 1;
            connectivity.trianglesAccessible[triangleIdx] = 1;
        }
#endif

        enum DrawFlags
        {
            DrawTriangles      = BIT(0),
            DrawInternalLinks  = BIT(1),
            DrawExternalLinks  = BIT(2),
            DrawOffMeshLinks   = BIT(3),
            DrawMeshBoundaries = BIT(4),
            DrawBVTree         = BIT(5),
            DrawAccessibility  = BIT(6),
            DrawTrianglesId    = BIT(7),
            DrawIslandsId      = BIT(8),
            DrawAll            = ~0ul,
        };

        void Draw(size_t drawFlags, vector3_t origin, TileID tileID, const std::vector<float>& islandAreas) const;

        Triangle*   triangles;
        Vertex*     vertices;
        BVNode*     nodes;
        Link*       links;

        uint16 triangleCount;
        uint16 vertexCount;
        uint16 nodeCount;
        uint16 linkCount;
        uint32 hashValue;

#if DEBUG_MNM_DATA_CONSISTENCY_ENABLED
    private:
        void ValidateTriangleLinks();
#endif

#if MNM_USE_EXPORT_INFORMATION

    private:
        struct TileConnectivity
        {
            TileConnectivity()
                : tileAccessible(1)
                , trianglesAccessible(NULL)
                , triangleCount(0)
            {
            }


            uint8 tileAccessible;
            uint8* trianglesAccessible;
            uint16 triangleCount;
        };

        bool ConsiderExportInformation() const;
        void InitConnectivity(uint16 oldTriangleCount, uint16 newTriangleCount);

        TileConnectivity connectivity;

#endif
    };
}

#if DEBUG_MNM_DATA_CONSISTENCY_ENABLED
struct CompareLink
{
    CompareLink(const MNM::Tile::Link& link)
    {
        m_link.edge = link.edge;
        m_link.side = link.side;
        m_link.triangle = link.triangle;
    }

    bool operator()(const MNM::Tile::Link& other)
    {
        return m_link.edge == other.edge && m_link.side == other.side && m_link.triangle == other.triangle;
    }

    MNM::Tile::Link m_link;
};

inline void BreakOnMultipleAdjacencyLinkage(const MNM::Tile::Link* start, const MNM::Tile::Link* end, const MNM::Tile::Link& linkToTest)
{
    if (std::count_if(start, end, CompareLink(linkToTest)) > 1)
    {
        DEBUG_BREAK;
    }
}
#else
inline void BreakOnMultipleAdjacencyLinkage(const MNM::Tile::Link* start, const MNM::Tile::Link* end, const MNM::Tile::Link& linkToTest)
{
}
#endif

#endif // CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_TILE_H
