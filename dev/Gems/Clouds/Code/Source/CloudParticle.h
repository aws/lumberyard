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

#include <MathConversion.h>

namespace CloudsGem
{
    class CloudParticle
    {
    public:
        AZ_TYPE_INFO(CloudParticle, "{10EFB89A-0AD3-41D7-969D-D9124781174F}");

        CloudParticle() = default;
        CloudParticle(const CloudParticle& rhs) = default;
        ~CloudParticle() = default;

        static void Reflect(AZ::ReflectContext* context);

        float GetRadius() const { return m_radius; }
        const Vec3 GetPosition() const { return AZVec3ToLYVec3(m_position); }
        float GetSquareSortDistance() const { return m_squareSortDistance; }
        const Vec3 GetUV(int index) { return index == 0 ? AZVec3ToLYVec3(m_topLeftUV) : AZVec3ToLYVec3(m_bottomRightUV); }
        float GetSize() { return m_size; }
        float GetSizeVariance() { return m_sizeVariance; }
        uint32 GetTextureId() { return m_textureId; }

        void SetRadius(float rad) { m_radius = rad; }
        void SetPosition(const Vec3& pos) { m_position = LYVec3ToAZVec3(pos); }
        void SetSquareSortDistance(float fSquareDistance) { m_squareSortDistance = fSquareDistance; }
        void SetUVs(const AZ::Vector3& topLeft, const AZ::Vector3& bottomRight) { m_topLeftUV = topLeft; m_bottomRightUV = bottomRight; }
        void SetSize(float size) { m_size = size; }
        void SetSizeVariance(float sizeVariance) { m_sizeVariance = sizeVariance; }
        void SetTextureId(uint32 id) { m_textureId = id; }

        bool operator<(const CloudParticle& p) const { return m_squareSortDistance < p.m_squareSortDistance; }
        bool operator>(const CloudParticle& p) const { return m_squareSortDistance > p.m_squareSortDistance; }

    protected:

        uint32 m_textureId{ 0 };
        AZ::Vector3 m_position{0.f, 0.f, 0.f};
        float m_radius{ 0 };
        AZ::Vector3 m_topLeftUV;
        AZ::Vector3 m_bottomRightUV;
        float m_size{ 1 };
        float m_sizeVariance{ 1 };
        float m_squareSortDistance{ 0 };
    };
    using CloudParticles = AZStd::vector<CloudParticle>;

    class CloudParticleData
    {
    public:
        AZ_TYPE_INFO(CloudParticleData, "{1BEF53A4-38E0-4DB6-A51F-2D4308E73983}");

        CloudParticleData();

        static void Reflect(AZ::ReflectContext* reflection);

        const AZStd::vector<CloudParticle>& GetParticles() const { return m_particles; }
        AZStd::vector<CloudParticle>& GetParticles() { return m_particles; }
        Vec3 GetOffset() { return AZVec3ToLYVec3(m_offset); }
        void SetOffset(const Vec3& offset) { m_offset = LYVec3ToAZVec3(offset); }
        AABB GetBounds() { return AZAabbToLyAABB(m_bounds); }
        void SetBounds(const AABB& bounds) { m_bounds = LyAABBToAZAabb(bounds); }
        bool HasParticles() { return !m_particles.empty(); }

    protected:

        AZ::Aabb m_bounds;
        AZ::Vector3 m_offset;
        CloudParticles m_particles;
    };
}
