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

#include "StaticDataMonitor.h"
#include <StaticDataManager.h>
#include <AzCore/IO/FileIO.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/Asset/AssetManagerBus.h>

namespace CloudCanvas
{
    namespace StaticData
    {

        StaticDataMonitor::StaticDataMonitor() 
        {
            StaticDataMonitorRequestBus::Handler::BusConnect();
            AzFramework::AssetCatalogEventBus::Handler::BusConnect();
            StaticDataUpdateBus::Handler::BusConnect();
            CrySystemEventBus::Handler::BusConnect();
        }

        StaticDataMonitor::~StaticDataMonitor()
        {
            CloudCanvas::StaticData::StaticDataMonitorRequestBus::Handler::BusDisconnect();
            AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();
            StaticDataUpdateBus::Handler::BusDisconnect();
            CrySystemEventBus::Handler::BusDisconnect();
        }

        void StaticDataMonitor::OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams)
        {
            Initialize();
        }

        void StaticDataMonitor::Initialize()
        {
            StaticDataRequestBus::Broadcast(&IStaticDataManager::ReloadTagType, "csv");
        }

        AZStd::string StaticDataMonitor::GetSanitizedName(const char* pathName) const
        {
             AZStd::string pathString{ pathName };
             AzFramework::StringFunc::Path::Normalize(pathString);
             return pathString;
        }

        void StaticDataMonitor::StaticDataFileAdded(const AZStd::string& filePath)
        {
            AddPath(filePath, true);
        }

        void StaticDataMonitor::AddAsset(const AZ::Data::AssetId& assetId)
        {
            if (m_monitoredAssets.find(assetId) != m_monitoredAssets.end())
            {
                AZ_TracePrintf("StaticData", "Already watching asset %d", assetId);
                return;
            }
            m_monitoredAssets.insert(assetId);
            AZ::Data::AssetBus::MultiHandler::BusConnect(assetId);
        }
        void StaticDataMonitor::AddPath(const AZStd::string& inputName, bool isFile)
        {
            AZStd::string resolvedName = StaticDataManager::ResolveAndSanitize(inputName.c_str());

            if (isFile)
            {
                AZ::Data::AssetId staticAssetId;
                EBUS_EVENT_RESULT(staticAssetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, inputName.c_str(), AZ::AzTypeInfo<StaticDataAsset>::Uuid(), true);
                if (staticAssetId.IsValid())
                {
                    AddAsset(staticAssetId);
                }
            }
            else
            {
                m_monitoredPaths.insert(resolvedName);
                AZ::IO::LocalFileIO localFileIO;
                bool foundOK = localFileIO.FindFiles(resolvedName.c_str(), resolvedName.c_str(), [&](const char* filePath) -> bool
                {
                    AZStd::string thisFile{ filePath };
                    AddPath(thisFile, true);
                    return true; // continue iterating
                });
            } 
        }

        void StaticDataMonitor::RemoveAsset(const AZ::Data::AssetId& assetId)
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect(assetId);
            m_monitoredAssets.erase(assetId);
        }

        void StaticDataMonitor::RemovePath(const AZStd::string& inputName)
        {
            m_monitoredPaths.erase(inputName);
        }

        void StaticDataMonitor::OnFileChanged(const AZStd::string& fileName)
        {
            EBUS_EVENT(CloudCanvas::StaticData::StaticDataRequestBus, LoadRelativeFile, fileName.c_str());
        }

        void StaticDataMonitor::RemoveAll()
        {

            auto fileList = m_monitoredAssets;
            for (auto thisAsset : fileList)
            {
                RemoveAsset(thisAsset);
            }

            auto directoryList = m_monitoredPaths;
            for (auto thisDir : directoryList)
            {
                RemovePath(thisDir.c_str());
            }
        }

        bool StaticDataMonitor::IsMonitored(const AZ::Data::AssetId& assetId)
        {
            if (m_monitoredAssets.find(assetId) != m_monitoredAssets.end())
            {
                return true;
            }

            const AZStd::string fileName = GetAssetFilenameFromAssetId(assetId);
            if (!fileName.empty())
            {
                AZStd::string sanitizedName{ GetSanitizedName(fileName.c_str()) };
                AZStd::string directoryPath = StaticDataManager::GetDirectoryFromFullPath(sanitizedName);

                if (m_monitoredPaths.find(directoryPath) != m_monitoredPaths.end())
                {
                    return true;
                }
            }

            return false;
        }

        void StaticDataMonitor::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> readyAsset)
        {
            OnCatalogAssetChanged(readyAsset.GetId());
        }
        void StaticDataMonitor::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
        {
            if (!IsMonitored(assetId))
            {
                return;
            }

            const AZStd::string fileName = GetAssetFilenameFromAssetId(assetId);
            if (!fileName.empty())
            {
                OnFileChanged(fileName);
            }
        }

        void StaticDataMonitor::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
        {
            OnCatalogAssetChanged(assetId);
        }


        void StaticDataMonitor::OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId)
        {
            if (!IsMonitored(assetId))
            {
                return;
            }
            const AZStd::string fileName = GetAssetFilenameFromAssetId(assetId);
            if (!fileName.empty())
            {
                OnFileChanged(fileName);
            }
        }

        AZStd::string StaticDataMonitor::GetAssetFilenameFromAssetId(const AZ::Data::AssetId& assetId)
        {
            const char* cachePath = AZ::IO::FileIOBase::GetInstance()->GetAlias("@assets@");
            if (!cachePath)
            {
                return {};
            }
            AZStd::string assetCachePath{ cachePath };
            AZStd::string filename;
            AzFramework::StringFunc::AssetDatabasePath::Normalize(assetCachePath);

            AZStd::string relativePath;
            EBUS_EVENT_RESULT(relativePath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, assetId);
            AzFramework::StringFunc::AssetDatabasePath::Join(assetCachePath.c_str(), relativePath.c_str(), filename, true);

            return filename;
        }
    }
}
