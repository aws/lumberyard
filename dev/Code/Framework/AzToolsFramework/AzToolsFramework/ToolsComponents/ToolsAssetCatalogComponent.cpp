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

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/SystemFile.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogComponent.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>

namespace AssetProcessor
{
    void ToolsAssetCatalogComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ToolsAssetCatalogComponent, AZ::Component>()
                ->Version(1);
        }
    }

    void ToolsAssetCatalogComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("AssetCatalogService", 0xc68ffc57));
    }

    void ToolsAssetCatalogComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("AssetDatabaseService", 0x3abf5601));
    }

    void ToolsAssetCatalogComponent::Activate()
    {
        AZ::Data::AssetCatalogRequestBus::Handler::BusConnect();
    }

    void ToolsAssetCatalogComponent::Deactivate()
    {
        AZ::Data::AssetCatalogRequestBus::Handler::BusDisconnect();

        DisableCatalog();
    }

    AZ::Data::AssetStreamInfo ToolsAssetCatalogComponent::GetStreamInfoForLoad(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType)
    {
        AZ::Data::AssetInfo assetInfo;
        AZStd::string rootFilePath;
        bool result = false;

        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(result, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetAssetInfoById, assetId, assetType, assetInfo, rootFilePath);

        if (result)
        {
            AZStd::string sourceFileFullPath;

            AzFramework::StringFunc::Path::Join(rootFilePath.c_str(), assetInfo.m_relativePath.c_str(), sourceFileFullPath);

            AZ::Data::AssetStreamInfo streamInfo;
            streamInfo.m_dataLen = assetInfo.m_sizeBytes;

            // All asset handlers are presently required to use the LoadAssetData override
            // that takes a path, in order to pipe the IO through FileIO / VFS.
            streamInfo.m_isCustomStreamType = true;
            streamInfo.m_streamName = sourceFileFullPath;

            return streamInfo;
        }

        return {};
    }

    void ToolsAssetCatalogComponent::EnableCatalogForAsset(const AZ::Data::AssetType& assetType)
    {
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager is not ready.");
        AZ::Data::AssetManager::Instance().RegisterCatalog(this, assetType);
    }

    void ToolsAssetCatalogComponent::DisableCatalog()
    {
        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterCatalog(this);
        }
    }

    AZ::Data::AssetInfo ToolsAssetCatalogComponent::GetAssetInfoById(const AZ::Data::AssetId& id)
    {
        AZ::Data::AssetInfo assetInfo;
        AZStd::string rootFilePath;
        bool result = false;

        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(result, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetAssetInfoById, id, AZ::Data::AssetType::CreateNull(), assetInfo, rootFilePath);

        if (result)
        {
            return assetInfo;
        }

        return {};
    }
}
