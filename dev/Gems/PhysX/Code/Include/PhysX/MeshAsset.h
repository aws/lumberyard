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

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzFramework/Physics/Material.h>

namespace physx
{
    class PxBase;
}

namespace PhysX
{
    namespace Pipeline
    {
        /// Represents a PhysX mesh asset.
        class MeshAsset
            : public AZ::Data::AssetData
        {
        public:
            friend class MeshAssetHandler;

            AZ_CLASS_ALLOCATOR(MeshAsset, AZ::SystemAllocator, 0);
            AZ_RTTI(MeshAsset, "{7A2871B9-5EAB-4DE0-A901-B0D2C6920DDB}", AZ::Data::AssetData);

            physx::PxBase* GetMeshData()
            {
                return m_meshData;
            }

            const AZStd::vector<Physics::MaterialConfiguration>& GetMaterialsData() const
            {
                return m_materials;
            }

            const AZStd::vector<AZStd::string>& GetMaterialSlots() const
            {
                return m_materialSlots;
            }

        private:
            physx::PxBase* m_meshData = nullptr;
            AZStd::vector<Physics::MaterialConfiguration> m_materials;
            AZStd::vector<AZStd::string> m_materialSlots;
        };
    } // namespace Pipeline
} // namespace PhysX
