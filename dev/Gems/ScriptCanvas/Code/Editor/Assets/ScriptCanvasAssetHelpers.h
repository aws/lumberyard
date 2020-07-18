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

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzCore/Component/EntityUtils.h>

namespace ScriptCanvasEditor
{
    namespace AssetHelpers
    {
        // Simplified function to trace messages
        static void PrintInfo(const char* format, ...)
        {
            // TODO: Turn into something nice with enable_if
            static bool s_enabled = false;
            if (s_enabled)
            {
                char sBuffer[1024];
                va_list ArgList;
                va_start(ArgList, format);
                azvsnprintf(sBuffer, sizeof(sBuffer), format, ArgList);
                sBuffer[sizeof(sBuffer) - 1] = '\0';
                va_end(ArgList);

                AZ_TracePrintf("Script Canvas", "%s\n", sBuffer);
            }
        }

        // Simplifies the conversion of an AssetId to a string to avoid overly verbose trace calls
        static AZStd::string AssetIdToString(AZ::Data::AssetId assetId) { return assetId.ToString<AZStd::string>(); }

        // Given the full path to the asset, attempt to get the AssetInfo
        static bool GetAssetInfo(AZStd::string_view fullPath, AZ::Data::AssetInfo& outAssetInfo)
        {
            AZStd::string watchFolder;
            AZ::Data::AssetInfo catalogAssetInfo;
            bool sourceInfoFound{};

            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(sourceInfoFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, fullPath.data(), catalogAssetInfo, watchFolder);
            auto saveAssetId = sourceInfoFound ? catalogAssetInfo.m_assetId : AZ::Data::AssetId(AZ::Uuid::CreateRandom());
            if (sourceInfoFound)
            {
                outAssetInfo = catalogAssetInfo;
                return true;
            }

            return false;
        }
        
        // Given the AssetId to the asset, attempt to get the AssetInfo
        static AZ::Data::AssetInfo GetAssetInfo(AZ::Data::AssetId assetId)
        {
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
            return assetInfo;
        }

        // Find the AssetType for a given asset
        static AZ::Data::AssetType GetAssetType(AZ::Data::AssetId assetId)
        {
            AZ::Data::AssetInfo assetInfo = GetAssetInfo(assetId);
            return assetInfo.m_assetType;
        }

        // Find the AssetType for a given asset by path
        static AZ::Data::AssetType GetAssetType(const char* assetPath)
        {
            AZStd::string watchFolder;
            AZ::Data::AssetInfo assetInfo;
            bool sourceInfoFound{};
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(sourceInfoFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, assetPath, assetInfo, watchFolder);
            if (sourceInfoFound)
            {
                return GetAssetType(assetInfo.m_assetId);
            }
            return AZ::Data::AssetType::CreateNull();
        }

        // Get AssetInfo from the AssetSystem as opposed to the catalog
        static AZ::Data::AssetInfo GetAssetInfo(AZ::Data::AssetId assetId, AZStd::string& rootFilePath)
        {
            AZ::Data::AssetInfo assetInfo;
            AzToolsFramework::AssetSystemRequestBus::Broadcast(&AzToolsFramework::AssetSystemRequestBus::Events::GetAssetInfoById, assetId, GetAssetType(assetId), assetInfo, rootFilePath);
            return assetInfo;
        }

        static AZ::Data::AssetInfo GetSourceInfo(const AZStd::string& sourceFilePath, AZStd::string& watchFolder)
        {
            AZ::Data::AssetInfo assetInfo;
            AzToolsFramework::AssetSystemRequestBus::Broadcast(&AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, sourceFilePath.c_str(), assetInfo, watchFolder);

            return assetInfo;
        }

        static AZ::Data::AssetInfo GetSourceInfoByProductId(AZ::Data::AssetId assetId, AZ::Data::AssetType assetType)
        {
            bool infoFound = false;
            AZStd::string watchFolder;
            AZ::Data::AssetInfo assetInfo;

            AzToolsFramework::AssetSystemRequestBus::Events* assetSystem = AzToolsFramework::AssetSystemRequestBus::FindFirstHandler();

            if (assetSystem == nullptr)
            {
                return AZ::Data::AssetInfo();
            }

            if (!assetSystem->GetAssetInfoById(assetId, assetType, assetInfo, watchFolder))
            {
                return AZ::Data::AssetInfo();
            }

            AZStd::string sourcePath;

            if (!assetSystem->GetFullSourcePathFromRelativeProductPath(assetInfo.m_relativePath, sourcePath))
            {
                return AZ::Data::AssetInfo();
            }

            if (!assetSystem->GetSourceInfoBySourcePath(sourcePath.c_str(), assetInfo, watchFolder))
            {
                return AZ::Data::AssetInfo();
            }

            return assetInfo;
        }

        static void DumpAssetInfo(AZ::Data::AssetId assetId, const char* extra)
        {
            static bool s_enabled = false;
            if (!s_enabled)
            {
                return;
            }

            AZ::Data::AssetInfo assetInfo = GetAssetInfo(assetId);
            if (assetInfo.m_assetId.IsValid())
            {
                AZ_TracePrintf("Script Canvas", "-------------------------------------\n");
                AZ_TracePrintf("Script Canvas", "AssetId: %s", AssetIdToString(assetId).c_str());
                AZ_TracePrintf("Script Canvas", AZStd::string::format("AssetType: %s", assetInfo.m_assetType.ToString<AZStd::string>().c_str()).c_str());
                AZ_TracePrintf("Script Canvas", AZStd::string::format("RelativePath: %s", assetInfo.m_relativePath.c_str()).c_str());
                AZ_TracePrintf("Script Canvas", AZStd::string::format("Size in Byes: %ul", assetInfo.m_sizeBytes).c_str());
                AZ_TracePrintf("Script Canvas", AZStd::string::format("%s\n", extra).c_str());
                AZ_TracePrintf("Script Canvas", "-------------------------------------\n");
            }
            else
            {
                AZ_TracePrintf("Script Canvas", "Cannot DumpAssetInfo for Asset with ID: %s\n", AssetIdToString(assetId).c_str());
            }

        }

    }
}
