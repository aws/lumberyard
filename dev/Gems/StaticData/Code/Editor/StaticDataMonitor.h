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

#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>

#include <AzFramework/Asset/AssetCatalogBus.h>

#include <StaticData/StaticDataBus.h>
#include <IStaticDataMonitor.h>

#include <CrySystemBus.h>

namespace CloudCanvas
{
    namespace StaticData
    {

        class StaticDataMonitor :  
            public StaticDataMonitorRequestBus::Handler
            , public AzFramework::AssetCatalogEventBus::Handler
            , public AZ::Data::AssetBus::MultiHandler
            , public StaticDataUpdateBus::Handler
            , public CrySystemEventBus::Handler
        {
        public:
            StaticDataMonitor();
            ~StaticDataMonitor();

            void Initialize();
            void RemoveAll() override;

            void AddPath(const AZStd::string& sanitizedPath, bool isFile) override;
            void RemovePath(const AZStd::string& sanitizedPath) override;

            void AddAsset(const AZ::Data::AssetId& sanitizedPath) override;
            void RemoveAsset(const AZ::Data::AssetId& sanitizedPath) override;

            //AssetCatalogEventBus
            void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId);
            void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId);
            void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId);

            // AssetDatabaseBus
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

            // StaticDataUpdateBus
            void StaticDataFileAdded(const AZStd::string& filePath) override;

            void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams) override;
        private:

            bool IsMonitored(const AZ::Data::AssetId& assetId);
            void OnFileChanged(const AZStd::string& filePath);

            AZStd::string GetSanitizedName(const char* pathName) const override; // Do any sort of path sanitizing so output events line up
            static AZStd::string GetAssetFilenameFromAssetId(const AZ::Data::AssetId& assetId);

            AZStd::unordered_set<AZ::Data::AssetId> m_monitoredAssets;
            AZStd::unordered_set<AZStd::string> m_monitoredPaths;
        };
    }
}