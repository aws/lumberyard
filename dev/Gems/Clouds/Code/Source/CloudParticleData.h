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

namespace CloudsGem
{
    /** 
     * CloudAsset
     */
    class CloudAsset : public AZ::Data::AssetData
    {
    public:

        AZ_RTTI(CloudAsset, "{E043B218-F9EA-4820-AA0A-C5149F0089E8}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(CloudAsset, AZ::SystemAllocator, 0);

        CloudAsset();
        ~CloudAsset() {};

        static void Reflect(AZ::ReflectContext* reflection);

        const AZStd::vector<CloudParticle>& GetParticles() const { return m_particles; }
        AZStd::vector<CloudParticle>& GetParticles() { return m_particles; }
        Vec3 GetOffset() { return AZVec3ToLYVec3(m_offset); }
        void SetOffset(const Vec3& offset) { m_offset = LYVec3ToAZVec3(offset); }
        AABB GetBounds() { return AZAabbToLyAABB(m_bounds); }
        void SetBounds(const AABB& bounds) { m_bounds = LyAABBToAZAabb(bounds); }
        bool HasParticles() { return !m_particles.empty(); }

    protected:

        CloudAsset(const CloudAsset&) = delete;

        AZ::Aabb m_bounds;
        AZ::Vector3 m_offset;
        AZStd::vector<CloudParticle> m_particles;
  	};
}
