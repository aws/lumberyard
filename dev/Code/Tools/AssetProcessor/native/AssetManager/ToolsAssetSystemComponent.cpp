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

#include <native/AssetManager/ToolsAssetSystemComponent.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/wildcard.h>
#include <AzCore/std/string/tokenize.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <native/utilities/assetUtils.h>
#include <QDir>

namespace AssetProcessor
{
    void ToolsAssetSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ToolsAssetSystemComponent, AZ::Component>()
                ->Version(1)
                ;
        }
    }

    void ToolsAssetSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("AssetCatalogService", 0xc68ffc57));
    }

    void ToolsAssetSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("AssetDatabaseService", 0x3abf5601));
    }

    void ToolsAssetSystemComponent::Activate()
    {
        AssetUtilities::ComputeProjectCacheRoot(m_cacheRootDir);

        AssetBuilderSDK::ToolsAssetSystemBus::Handler::BusConnect();
        AZ::Data::AssetCatalogRequestBus::Handler::BusConnect();
    }

    void ToolsAssetSystemComponent::Deactivate()
    {
        AssetBuilderSDK::ToolsAssetSystemBus::Handler::BusDisconnect();
        AZ::Data::AssetCatalogRequestBus::Handler::BusDisconnect();

        DisableCatalog();
        m_sourceAssetTypes.clear();

        m_assetDatabaseConnection.reset();
    }

    AZ::Data::AssetStreamInfo ToolsAssetSystemComponent::GetStreamInfoForLoad(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType)
    {
        AZStd::string rootFilePath;
        auto assetInfo = GetAssetInfoByIdInternal(assetId, assetType, rootFilePath);

        if (!assetInfo.m_relativePath.empty())
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

    void ToolsAssetSystemComponent::EnableCatalogForAsset(const AZ::Data::AssetType& assetType)
    {
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager is not ready.");
        AZ::Data::AssetManager::Instance().RegisterCatalog(this, assetType);
    }

    void ToolsAssetSystemComponent::DisableCatalog()
    {
        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterCatalog(this);
        }
    }

    AZ::Data::AssetInfo ToolsAssetSystemComponent::GetAssetInfoById(const AZ::Data::AssetId& id)
    {
        AZStd::string rootPath;
        return GetAssetInfoByIdInternal(id, AZ::Data::AssetType::CreateNull(), rootPath);
    }

    void ToolsAssetSystemComponent::RegisterSourceAssetType(const AZ::Data::AssetType& assetType, const char* assetFileFilter)
    {
        m_sourceAssetTypes.insert({ assetType, assetFileFilter });
    }

    void ToolsAssetSystemComponent::UnregisterSourceAssetType(const AZ::Data::AssetType& assetType)
    {
        m_sourceAssetTypes.erase(assetType);
    }

    bool ToolsAssetSystemComponent::GetSourceFileInfoFromAssetId(const AZ::Data::AssetId &assetId, AZStd::string& watchFolder, AZStd::string& relativePath)
    {
        AZStd::string sourceFileFullPath;

        // Check the database first
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_databaseMutex);
            AzToolsFramework::AssetDatabase::SourceDatabaseEntry entry;

            if (CheckDatabaseConnection() && m_assetDatabaseConnection->GetSourceBySourceGuid(assetId.m_guid, entry))
            {
                AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry scanEntry;
                if (m_assetDatabaseConnection->GetScanFolderByScanFolderID(entry.m_scanFolderPK, scanEntry))
                {
                    watchFolder = scanEntry.m_scanFolder;
                    relativePath = entry.m_sourceName;

                    return true;
                }
            }
        }

        // Source file isn't in the database yet, see if the APM has it in the job queue
        if (sourceFileFullPath.empty())
        {
            bool result = false;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(result, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceAssetInfoById, assetId.m_guid, watchFolder, relativePath);

            return result;
        }

        return false;
    }

    bool ToolsAssetSystemComponent::CheckDatabaseConnection()
    {
        if (!m_assetDatabaseConnection)
        {
            AZStd::string databaseLocation;
            AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Broadcast(&AzToolsFramework::AssetDatabase::AssetDatabaseRequests::GetAssetDatabaseLocation, databaseLocation);

            if (!databaseLocation.empty())
            {
                m_assetDatabaseConnection = AZStd::make_unique<AssetProcessor::AssetDatabaseConnection>();
                m_assetDatabaseConnection->OpenDatabase();

                return true;
            }

            return false;
        }

        return true;
    }

    AZ::Data::AssetInfo ToolsAssetSystemComponent::GetAssetInfoByIdInternal(const AZ::Data::AssetId &id, const AZ::Data::AssetType& assetType, AZStd::string& rootFilePath)
    {
        // If the assetType wasn't provided, try to guess it
        if (assetType.IsNull())
        {
            return GetAssetInfoByIdOnly(id, rootFilePath);
        }

        // Return the source file info
        if (m_sourceAssetTypes.find(assetType) != m_sourceAssetTypes.end())
        {
            AZStd::string watchFolder;
            AZStd::string relativePath;

            if (GetSourceFileInfoFromAssetId(id, watchFolder, relativePath))
            {
                AZ::Data::AssetInfo assetInfo;
                AZStd::string sourceFileFullPath;
                AzFramework::StringFunc::Path::Join(watchFolder.c_str(), relativePath.c_str(), sourceFileFullPath);

                assetInfo.m_assetId = id;
                assetInfo.m_assetType = assetType;
                assetInfo.m_relativePath = relativePath;
                assetInfo.m_sizeBytes = AZ::IO::SystemFile::Length(sourceFileFullPath.c_str());

                rootFilePath = watchFolder;

                return assetInfo;
            }
        }
        else // Return the product file info
        {
            return GetProductAssetInfo(id, rootFilePath);
        }

        return {};
    }

    AZ::Data::AssetInfo ToolsAssetSystemComponent::GetProductAssetInfo(const AZ::Data::AssetId &id, AZStd::string &rootFilePath)
    {
        AZ::Data::AssetInfo assetInfo;
        AZStd::lock_guard<AZStd::mutex> lock(m_databaseMutex);

        if (!CheckDatabaseConnection())
        {
            return {};
        }

        m_assetDatabaseConnection->QueryCombinedBySourceGuidProductSubId(id.m_guid, id.m_subId, [this, &assetInfo, &id, &rootFilePath](AzToolsFramework::AssetDatabase::CombinedDatabaseEntry& entry) -> bool
        {
            QString fullPath = m_cacheRootDir.absoluteFilePath(assetInfo.m_relativePath.c_str());

            assetInfo.m_assetId = id;
            assetInfo.m_assetType = entry.m_assetType;
            assetInfo.m_relativePath = entry.m_productName;
            assetInfo.m_sizeBytes = AZ::IO::SystemFile::Length(fullPath.toStdString().c_str());

            rootFilePath = m_cacheRootDir.absolutePath().toStdString().c_str();

            return false;
        });

        return assetInfo;
    }

    AZ::Data::AssetInfo ToolsAssetSystemComponent::GetAssetInfoByIdOnly(const AZ::Data::AssetId& id, AZStd::string& rootFilePath)
    {
        AZStd::string watchFolder;
        AZStd::string relativePath;
        AZ::Data::AssetInfo assetInfo;

        if (GetSourceFileInfoFromAssetId(id, watchFolder, relativePath))
        {
            // Go through the list of source assets and see if this asset's file path matches any of the filters
            for (const auto& pair : m_sourceAssetTypes)
            {
                AZStd::vector<AZStd::string> tokens;
                AZStd::tokenize(pair.second, AZStd::string(";"), tokens);

                for (const auto& pattern : tokens)
                {
                    if (AZStd::wildcard_match(pattern, relativePath))
                    {
                        AZStd::string sourceFileFullPath;
                        AzFramework::StringFunc::Path::Join(watchFolder.c_str(), relativePath.c_str(), sourceFileFullPath);

                        assetInfo.m_assetId = id;
                        assetInfo.m_assetType = pair.first;
                        assetInfo.m_relativePath = relativePath;
                        assetInfo.m_sizeBytes = AZ::IO::SystemFile::Length(sourceFileFullPath.c_str());

                        rootFilePath = watchFolder;

                        return assetInfo;
                    }
                }
            }

            // If we get to here, we're going to assume it's a product type
            return GetProductAssetInfo(id, rootFilePath);
        }
        else
        {
            // Asset isn't in the DB or in the APM queue, we don't know what this asset ID is
            return {};
        }
    }

}
