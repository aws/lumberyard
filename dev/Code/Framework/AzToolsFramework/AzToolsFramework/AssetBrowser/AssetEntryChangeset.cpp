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

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetEntryChangeset.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetEntryChangeset::AssetEntryChangeset(
            AZStd::shared_ptr<AssetDatabaseConnection> databaseConnection,
            AZStd::shared_ptr<RootAssetBrowserEntry> rootEntry)
            : m_databaseConnection(databaseConnection)
            , m_rootEntry(rootEntry)
            , m_fullUpdate(false)
            , m_updated(false)
        {}

        AssetEntryChangeset::~AssetEntryChangeset() = default;

        void AssetEntryChangeset::PopulateEntries()
        {
            m_fullUpdate = true;
        }

        void AssetEntryChangeset::AddEntry(const AZ::Data::AssetId& assetId)
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_mutex);
            m_toAdd.push_back(assetId);
        }

        void AssetEntryChangeset::RemoveEntry(const AZ::Data::AssetId& assetId)
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_mutex);
            m_toRemove.push_back(assetId);
        }

        void AssetEntryChangeset::RemoveSource(const AZ::Uuid& sourceUUID) 
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_mutex);
            m_sourcesToRemove.push_back(sourceUUID);
        }

        void AssetEntryChangeset::Synchronize()
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_mutex);

            if (m_updated)
            {
                m_rootEntry->Update(m_relativePath.c_str());
                m_updated = false;
            }

            for (auto& entry : m_entriesToAdd)
            {
                m_rootEntry->AddAsset(entry);
            }
            m_entriesToAdd.clear();

            for (auto& assetId : m_toRemove)
            {
                m_rootEntry->RemoveProduct(assetId);
            }
            m_toRemove.clear();

            for (auto sourceUuid : m_sourcesToRemove)
            {
                m_rootEntry->RemoveSource(sourceUuid);
            }
            m_sourcesToRemove.clear();
        }

        void AssetEntryChangeset::Update()
        {
            if (m_fullUpdate)
            {
                AZStd::lock_guard<AZStd::mutex> locker(m_mutex);

                m_fullUpdate = false;
                m_updated = true;
                {                    
                    QueryEntireDatabase();
                    m_toRemove.clear();
                    m_toAdd.clear();
                }
            }
            else
            {
                AZStd::vector<AZ::Data::AssetId> toAdd;
                {
                    AZStd::lock_guard<AZStd::mutex> locker(m_mutex);
                    toAdd = m_toAdd;
                    m_toAdd.clear();
                }

                for (auto& assetId : toAdd)
                {
                    QueryAsset(assetId);
                }
            }
        }

        void AssetEntryChangeset::QueryEntireDatabase()
        {
            // If the current engine root folder is external to the current root folder, then we need to set the m_relativePath to the actual external engine
            // root folder instead, since that is where the Gems, Editor, and Engine folders will reside
            bool isEngineRootExternal = false;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(isEngineRootExternal, &AzToolsFramework::ToolsApplicationRequests::IsEngineRootExternal);
            if (isEngineRootExternal)
            {
                AZStd::string engineRoot;
                AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(engineRoot, &AzToolsFramework::ToolsApplicationRequests::GetEngineRootPath);
                AzFramework::StringFunc::AssetDatabasePath::Normalize(engineRoot);
                m_relativePath = engineRoot;
            }
            else
            {
                // Fallback to querying the asset database for the root folder
                m_databaseConnection->QueryScanFolderByDisplayName(
                    "root",
                    [=](ScanFolderDatabaseEntry& scanFolderDatabaseEntry)
                {
                    m_relativePath = scanFolderDatabaseEntry.m_scanFolder.c_str();
                    return true;
                });
            }

            m_databaseConnection->QueryCombined(
                [&](CombinedDatabaseEntry& combinedDatabaseEntry)
                {
                    m_entriesToAdd.push_back(combinedDatabaseEntry);
                    return true;
                }, AZ::Uuid::CreateNull(),
                nullptr,
                AzToolsFramework::AssetSystem::GetHostAssetPlatform(),
                AssetSystem::JobStatus::Any);

            // query all sources in case they didn't produce any products
            AZStd::vector<SourceDatabaseEntry> sources;
            m_databaseConnection->QuerySourcesTable(
                [&](SourceDatabaseEntry& sourceDatabaseEntry)
                {
                    sources.push_back(sourceDatabaseEntry);
                    return true;
                });

            for (auto& source : sources)
            {
                if (AZStd::find_if(m_entriesToAdd.begin(), m_entriesToAdd.end(),
                        [&source](const CombinedDatabaseEntry& combinedDatabaseEntry)
                        {
                            return combinedDatabaseEntry.m_sourceID == source.m_sourceID;
                        }) == m_entriesToAdd.end())
                {
                    CombinedDatabaseEntry combinedDatabaseEntry;

                    m_databaseConnection->QueryScanFolderByScanFolderID(
                        source.m_scanFolderPK,
                        [&combinedDatabaseEntry](ScanFolderDatabaseEntry& scanFolderDatabaseEntry)
                        {
                            combinedDatabaseEntry.m_scanFolderID = scanFolderDatabaseEntry.m_scanFolderID;
                            combinedDatabaseEntry.m_scanFolder = scanFolderDatabaseEntry.m_scanFolder;
                            combinedDatabaseEntry.m_displayName = scanFolderDatabaseEntry.m_displayName;
                            combinedDatabaseEntry.m_portableKey = scanFolderDatabaseEntry.m_portableKey;
                            combinedDatabaseEntry.m_outputPrefix = scanFolderDatabaseEntry.m_outputPrefix;
                            return true;
                        });

                    combinedDatabaseEntry.m_sourceID = source.m_sourceID;
                    combinedDatabaseEntry.m_scanFolderPK = source.m_scanFolderPK;
                    combinedDatabaseEntry.m_sourceName = source.m_sourceName;
                    combinedDatabaseEntry.m_sourceGuid = source.m_sourceGuid;

                    m_entriesToAdd.push_back(combinedDatabaseEntry);
                }
            }
        }

        void AssetEntryChangeset::QueryAsset(AZ::Data::AssetId assetId)
        {
            m_databaseConnection->QueryCombinedBySourceGuidProductSubId(
                assetId.m_guid,
                assetId.m_subId,
                [&](CombinedDatabaseEntry& combinedDatabaseEntry)
                {
                    AZStd::lock_guard<AZStd::mutex> locker(m_mutex);
                    m_entriesToAdd.push_back(combinedDatabaseEntry);
                    return true;
                }, AZ::Uuid::CreateNull(),
                nullptr,
                AzToolsFramework::AssetSystem::GetHostAssetPlatform(),
                AssetSystem::JobStatus::Any);
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework