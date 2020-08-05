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

#ifndef CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_VOXELIZER_H
#define CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_VOXELIZER_H
#pragma once


#include "DynamicSpanGrid.h"
#include <AzFramework/Physics/Casts.h>

namespace MNM
{
    class Voxelizer
    {
    public:
        Voxelizer();

        void Reset();
        void Start(const AABB& volume, const Vec3& voxelSize);
        void RasterizeTriangle(const Vec3 v0, const Vec3 v1, const Vec3 v2);

        inline const DynamicSpanGrid& GetSpanGrid() const
        {
            return m_spanGrid;
        }

    protected:
        template<typename Ty>
        inline Vec3_tpl<Ty> Maximize(const Vec3_tpl<Ty> a, const Vec3_tpl<Ty> b)
        {
            return Vec3_tpl<Ty>(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
        }

        template<typename Ty>
        inline Vec3_tpl<Ty> Maximize(const Vec3_tpl<Ty> a, const Vec3_tpl<Ty> b, const Vec3_tpl<Ty> c)
        {
            return Vec3_tpl<Ty>(max(max(a.x, b.x), c.x), max(max(a.y, b.y), c.y), max(max(a.z, b.z), c.z));
        }

        template<typename Ty>
        inline Vec3_tpl<Ty> Minimize(const Vec3_tpl<Ty> a, const Vec3_tpl<Ty> b)
        {
            return Vec3_tpl<Ty>(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
        }

        template<typename Ty>
        inline Vec3_tpl<Ty> Minimize(const Vec3_tpl<Ty> a, const Vec3_tpl<Ty> b, const Vec3_tpl<Ty> c)
        {
            return Vec3_tpl<Ty>(min(min(a.x, b.x), c.x), min(min(a.y, b.y), c.y), min(min(a.z, b.z), c.z));
        }

        inline void Evaluate2DEdge(Vec2& edgeNormal, float& distanceEdge, bool cw, const Vec2 edge,
            const Vec2 vertex, const Vec2 ext)
        {
            edgeNormal = cw ? Vec2(edge.y, -edge.x) : Vec2(-edge.y, edge.x);
            distanceEdge = edgeNormal.Dot(Vec2(edgeNormal.x >= 0.0f ? ext.x : 0.0f, edgeNormal.y >= 0.0f ? ext.y : 0.0f) - vertex);
        }

        AABB m_volumeAABB;
        Vec3 m_voxelSize;
        Vec3 m_voxelConv;
        Vec3i m_voxelSpaceSize;

        DynamicSpanGrid m_spanGrid;

    };


    class WorldVoxelizer
        : public Voxelizer
    {
    public:
        size_t ProcessGeometry(uint32 hashValueSeed = 0, uint32 hashTest = 0, uint32* hashValue = 0,
            NavigationMeshEntityCallback pEntityCallback = NULL);
        void CalculateWaterDepth();

    protected:
        /// Fills in the overlapHits will all colliders in the provided AABB 
        void GetAZCollidersInAABB(const AABB& aabb, AZStd::vector<Physics::OverlapHit>& overlapHits);

        /// Rasterize all the collider geometry in the provided bounding box
        size_t RasterizeAZColliderGeometry(const AABB& aabb, const AZStd::vector<Physics::OverlapHit>& overlapHits);

    private:
        void VoxelizeGeometry(const Vec3* vertices, size_t triCount, const Matrix34& worldTM);
        void VoxelizeGeometry(const strided_pointer<Vec3>& vertices, const index_t* indices, size_t triCount,
            const Matrix34& worldTM);
        void VoxelizeGeometry(const Vec3* vertices, const uint32* indices, size_t triCount, const Matrix34& worldTM);
        AABB ComputeTerrainAABB(IGeometry* geometry);
        size_t VoxelizeTerrain(IGeometry* geometry, const Matrix34& worldTM);
        size_t VoxelizeGeometry(IGeometry* geometry, const Matrix34& worldTM);
    };
}

#endif // CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_VOXELIZER_H
