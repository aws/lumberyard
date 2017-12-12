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

#include "CloudDescription.h"

namespace CloudsGem
{
    class CloudAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(CloudAsset, "{E043B218-F9EA-4820-AA0A-C5149F0089E8}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(CloudAsset, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflection)
        {
            CloudParticle::Reflect(reflection);
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<CloudAsset>()
                    ->Version(1)
                    ->Field("TextureRows", &CloudAsset::m_textureRows)
                    ->Field("TextureCols", &CloudAsset::m_textureCols)
                    ->Field("MaterialName", &CloudAsset::m_materialName)
                    ->Field("Bounds", &CloudAsset::m_bounds)
                    ->Field("Offset", &CloudAsset::m_offset)
                    ->Field("Particles", &CloudAsset::m_particles);
            }
        }

        const AZStd::string& GetMaterialName() { return m_materialName; }
        void SetMaterialName(const AZStd::string& materialName) { m_materialName = materialName; }

        int GetTextureRows() { return m_textureRows;  }
        void SetTextureRows(int rows) { m_textureRows = rows;  }

        int GetTextureColumns() { return m_textureCols; }
        void SetTextureColumns(int cols) { m_textureCols = cols; }

        const AZStd::vector<CloudParticle>& GetParticles() const { return m_particles; }
        AZStd::vector<CloudParticle>& GetParticles() { return m_particles; }

        Vec3 GetOffset() { return AZVec3ToLYVec3(m_offset); }
        void SetOffset(const Vec3& offset) { m_offset = LYVec3ToAZVec3(offset); }

        AABB GetBounds() { return AZAabbToLyAABB(m_bounds); }
        void SetBounds(const AABB& bounds) { m_bounds = LyAABBToAZAabb(bounds); }

        bool HasParticles() { return !m_particles.empty(); }

        IMaterial* GetMaterial() { return m_material; }
        void SetMaterial(IMaterial* material) { m_material = material;  }

        void SetQuadTree(CloudQuadTree* cloudTree) { m_pCloudTree = cloudTree; }
        CloudQuadTree* GetQuadTree() { return m_pCloudTree;  }

    protected:

        AZStd::string m_materialName;               //! Name of material used to render clouds
        int m_textureRows{ 0 };                     //! Number of rows in the cloud atlas texture
        int m_textureCols{ 0 };                     //! Number of cols in the cloud atlas texture
        AZ::Aabb m_bounds;                          //! Bounds of the particle system
        AZ::Vector3 m_offset;                       //! Offset from center
        AZStd::vector<CloudParticle> m_particles;   //! Vector of particles used by the system
        IMaterial* m_material{ nullptr };           //! Material used by the cloud
        CloudQuadTree* m_pCloudTree{ nullptr };     //! Quad tree for intersection testing
    };
}
