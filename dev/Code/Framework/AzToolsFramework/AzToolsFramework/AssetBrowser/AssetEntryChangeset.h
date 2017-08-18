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

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Asset/AssetCommon.h>

namespace AzToolsFramework
{
    namespace AssetDatabase
    {
        class AssetDatabaseConnection;
        class CombinedDatabaseEntry;
    }

    using namespace AssetDatabase;

    namespace AssetBrowser
    {
        class RootAssetBrowserEntry;

        class AssetEntryChangeset
        {
        public:
            AssetEntryChangeset(
                AZStd::shared_ptr<AssetDatabaseConnection> databaseConnection, 
                AZStd::shared_ptr<RootAssetBrowserEntry> rootEntry);
            ~AssetEntryChangeset();

            void PopulateEntries();
            void AddEntry(const AZ::Data::AssetId& assetId);
            void RemoveEntry(const AZ::Data::AssetId& assetId);
            void RemoveSource(const AZ::Uuid& sourceUUID);
            void Update();
            void Synchronize();

        private:
            AZStd::shared_ptr<AssetDatabaseConnection> m_databaseConnection;

            AZStd::shared_ptr<RootAssetBrowserEntry> m_rootEntry;

            //! protects read/write to m_entries
            AZStd::mutex m_mutex;

            AZStd::atomic_bool m_fullUpdate;
            bool m_updated;
            
            AZStd::vector<AZ::Data::AssetId> m_toAdd;
            AZStd::vector<AZ::Data::AssetId> m_toAddAccumulator;
            AZStd::vector<AZ::Data::AssetId> m_toRemove;
            AZStd::vector<AZ::Data::AssetId> m_toUpdate;
            AZStd::vector<AZ::Uuid> m_sourcesToRemove;

            AZStd::vector<CombinedDatabaseEntry> m_entriesToAdd;

            AZStd::string m_relativePath;

            void QueryEntireDatabase();
            void QueryAsset(AZ::Data::AssetId assetId);
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework