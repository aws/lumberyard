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

#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Asset/AssetManager.h>

namespace ScriptCanvas
{
    class AssetRegistry
    {
    public:
        AZ_CLASS_ALLOCATOR(AssetRegistry, AZ::SystemAllocator, 0);

        AssetRegistry() = default;
        ~AssetRegistry() = default;

        template <typename AssetType, typename HandlerType>
        void Register()
        {
            AZ::Data::AssetType assetType(azrtti_typeid<AssetType>());
            if (AZ::Data::AssetManager::Instance().GetHandler(assetType))
            {
                return; // Asset Type already handled
            }

            m_assetHandlers[assetType] = AZStd::make_unique<HandlerType>();
            AZ::Data::AssetManager::Instance().RegisterHandler(m_assetHandlers[assetType].get(), assetType);

            // Use AssetCatalog service to register ScriptCanvas asset type and extension
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddAssetType, assetType);
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, assetType);
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddExtension, AssetType::GetFileExtension());
        }

        void Unregister();

        template <typename AssetType>
        AZ::Data::AssetHandler* GetAssetHandler()
        {
            AZ::Data::AssetType assetType(azrtti_typeid<AssetType>());
            return GetAssetHandler(assetType);
        }

        AZ::Data::AssetHandler* GetAssetHandler(AZ::Data::AssetType type);

    private:
        AssetRegistry(const AssetRegistry&) = delete;

        AZStd::unordered_map<AZ::Data::AssetType, AZStd::unique_ptr<AZ::Data::AssetHandler>> m_assetHandlers;
    };
}
