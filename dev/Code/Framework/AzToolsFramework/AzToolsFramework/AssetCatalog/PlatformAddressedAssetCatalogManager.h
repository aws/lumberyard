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

#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalog.h>
#include <AzFramework/Asset/NetworkAssetNotification_private.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/vector.h>

namespace AzToolsFramework
{
    //! Handle multiple AssetCatalogs with requests addressing each by platform
    class PlatformAddressedAssetCatalogManager : private AzFramework::AssetSystem::NetworkAssetUpdateInterface
    {
    public:
        AZ_TYPE_INFO(PlatformAddressedAssetCatalogManager, "{985263D2-1C04-4811-9EC6-6A069641512A}");
        AZ_CLASS_ALLOCATOR(PlatformAddressedAssetCatalogManager, AZ::SystemAllocator, 0);

        PlatformAddressedAssetCatalogManager();
        PlatformAddressedAssetCatalogManager(AzFramework::PlatformId platformId);
        ~PlatformAddressedAssetCatalogManager();

        using AssetCatalogList = AZStd::vector<AZStd::unique_ptr<AzToolsFramework::PlatformAddressedAssetCatalog>>;

        void LoadCatalogs();
        void LoadSingleCatalog(AzFramework::PlatformId platformId);
        void TakeSingleCatalog(AZStd::unique_ptr<AzToolsFramework::PlatformAddressedAssetCatalog>&& newCatalog);
        //////////////////////////////////////////////////////////////////////////
        // NetworkAssetUpdateInterface
        void AssetChanged(AzFramework::AssetSystem::AssetNotificationMessage message) override;
        void AssetRemoved(AzFramework::AssetSystem::AssetNotificationMessage message) override;
        AZStd::string GetSupportedPlatforms() override;
        //////////////////////////////////////////////////////////////////////////

        //! Uses the existence of each platform's AssetCatalog.xml to determine what will be enabled
        //! in order to make this usable without loading any catalogs
        static AZStd::vector< AzFramework::PlatformId> GetEnabledPlatforms();
        static AZStd::string GetEnabledPlatformsString();

        const AssetCatalogList& GetCatalogList() { return m_assetCatalogList; }
    private:
        AssetCatalogList m_assetCatalogList;
    };

} // namespace AzToolsFramework
