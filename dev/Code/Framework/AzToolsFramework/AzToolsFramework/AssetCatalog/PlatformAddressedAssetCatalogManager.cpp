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

#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalogManager.h>



namespace AzToolsFramework
{
    PlatformAddressedAssetCatalogManager::PlatformAddressedAssetCatalogManager()
    {
        LoadCatalogs();
    }

    PlatformAddressedAssetCatalogManager::PlatformAddressedAssetCatalogManager(AzFramework::PlatformId platformId)
    {
        LoadSingleCatalog(platformId);
    }
  
    void PlatformAddressedAssetCatalogManager::LoadCatalogs()
    {
        m_assetCatalogList.clear();
        for (int platformNum = AzFramework::PlatformId::PC; platformNum < AzFramework::PlatformId::NumPlatforms; ++platformNum)
        {
            if (PlatformAddressedAssetCatalog::CatalogExists(static_cast<AzFramework::PlatformId>(platformNum)))
            {
                m_assetCatalogList.emplace_back(AZStd::make_unique<PlatformAddressedAssetCatalog>(static_cast<AzFramework::PlatformId>(platformNum)));
            }
        }
    }

    void PlatformAddressedAssetCatalogManager::LoadSingleCatalog(AzFramework::PlatformId platformId)
    {
        m_assetCatalogList.clear();
        m_assetCatalogList.emplace_back(AZStd::make_unique<PlatformAddressedAssetCatalog>(platformId));
    }
} // namespace AzToolsFramework
