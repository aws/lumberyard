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

#include <PhysXSystemComponent.h>

namespace PhysX
{
    namespace Pipeline
    {
        /**
         * Represents a PhysX mesh asset.
         */
        class PhysXMeshAsset
            : public AZ::Data::AssetData
        {
        public:
            friend class PhysXMeshAssetHandler;

            AZ_CLASS_ALLOCATOR(PhysXMeshAsset, PhysXAllocator, 0);
            AZ_RTTI(PhysXMeshAsset, "{7A2871B9-5EAB-4DE0-A901-B0D2C6920DDB}", AZ::Data::AssetData);

            physx::PxBase* GetMeshData()
            {
                return m_meshData;
            }

            static const char* m_assetFileExtention;

        private:
            physx::PxBase* m_meshData = nullptr;
        };

        /**
         * Asset handler for loading and initializing PhysXMeshAsset assets.
         */
        class PhysXMeshAssetHandler
            : public AZ::Data::AssetHandler
            , private AZ::AssetTypeInfoBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(PhysXMeshAssetHandler, PhysXAllocator, 0);

            PhysXMeshAssetHandler();
            ~PhysXMeshAssetHandler();

            void Register();
            void Unregister();

            // AZ::Data::AssetHandler
            AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override;
            bool LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
            bool LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const char* assetPath, const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
            void DestroyAsset(AZ::Data::AssetPtr ptr) override;
            void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override;

            // AZ::AssetTypeInfoBus
            AZ::Data::AssetType GetAssetType() const override;
            void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) override;
            const char* GetAssetTypeDisplayName() const;
            const char* GetBrowserIcon() const override;
            const char* GetGroup() const override;
            AZ::Uuid GetComponentTypeId() const override;
        };
    } // namespace Pipeline
} // namespace PhysX
