
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

#include <AzToolsFramework/Asset/AssetSeedManager.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Math/Sha1.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzFramework/FileTag/FileTagBus.h>
#include <AzFramework/FileTag/FileTag.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Asset/AssetDebugInfo.h>
#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalogBus.h>
#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalog.h>


const char SeedFileExtension[] = "seed";
const char AssetListFileExtension[] = "assetlist";
const char ScriptCanvas[] = "scriptcanvas";
const char ScriptCanvasCompiled[] = "scriptcanvas_compiled";
const char ScriptCanvasFunction[] = "scriptcanvas_fn";
const char ScriptCanvasFunctionCompiled[] = "scriptcanvas_fn_compiled";

namespace AzToolsFramework
{

    AZStd::string GetSeedPath(AZ::Data::AssetId assetId, AzFramework::PlatformFlags platformFlags)
    {
        using namespace AzToolsFramework;
        AZStd::vector<AzFramework::PlatformId> platformIndices = AzFramework::PlatformHelper::GetPlatformIndicesInterpreted(platformFlags);
        for (const auto& platformId : platformIndices)
        {
            AZStd::string assetPath;
            AssetCatalog::PlatformAddressedAssetCatalogRequestBus::EventResult(assetPath, platformId, &AssetCatalog::PlatformAddressedAssetCatalogRequestBus::Events::GetAssetPathById, assetId);
            if (!assetPath.empty())
            {
                return assetPath;
            }
        }

        AZ_Warning("AssetSeedManager", false, "Unable to find path hint for Seed asset (%s) for the given platformFlags (%d).\n", assetId.ToString<AZStd::string>().c_str(), platformFlags);

        return {};
    }

    AssetSeedManager::~AssetSeedManager()
    {
        m_sourceAssetTypeToRuntimeAssetTypeMap.clear();
    }

    void AssetSeedManager::AddSeedAsset(AZ::Data::AssetId assetId, AzFramework::PlatformFlags platformFlags, AZStd::string path)
    {
        for (auto iter = m_assetSeedList.begin(); iter != m_assetSeedList.end(); ++iter)
        {
            if (iter->m_assetId == assetId)
            {
                if (iter->m_platformFlags == platformFlags)
                {
                    AZ_TracePrintf("AssetSeedManager", "Seed Asset ( %s ) is already present in the asset seed list.\n", assetId.ToString<AZStd::string>().c_str());
                }
                else
                {
                    iter->m_platformFlags = iter->m_platformFlags | platformFlags;
                }
                return;
            }
        }

        if (path.empty())
        {
            path = GetSeedPath(assetId, platformFlags);
        }
        m_assetSeedList.emplace_back(AzFramework::SeedInfo(assetId, platformFlags, path));
    }

    void AssetSeedManager::RemoveSeedAsset(AZ::Data::AssetId assetId, AzFramework::PlatformFlags platformFlags)
    {
        for (auto iter = m_assetSeedList.begin(); iter != m_assetSeedList.end(); ++iter)
        {
            if (iter->m_assetId == assetId)
            {
                if (iter->m_platformFlags == platformFlags)
                {
                    m_assetSeedList.erase(iter);
                }
                else
                {
                    iter->m_platformFlags = iter->m_platformFlags  & (~platformFlags);
                }
                return;
            }
        }

        AZ_TracePrintf("AssetSeedManager", "Seed Asset ( %s ) is not present in the asset seed list.\n", assetId.ToString<AZStd::string>().c_str());
    }

    void AssetSeedManager::AddSeedAsset(const AZStd::string& assetPath, AzFramework::PlatformFlags platformFlags)
    {
        if (!m_sourceAssetTypeToRuntimeAssetTypeMap.size())
        {
            PopulateAssetTypeMap();
        }

        using namespace AzFramework::FileTag;
        AZStd::vector<AZStd::string> editorTagsList = { FileTags[static_cast<unsigned int>(FileTagsIndex::EditorOnly)] };
        bool editorOnlyAsset = false;
        QueryFileTagsEventBus::EventResult(editorOnlyAsset, FileTagType::Exclude,
            &QueryFileTagsEventBus::Events::Match, assetPath, editorTagsList);

        AZStd::string seedPath = assetPath;

        if (editorOnlyAsset)
        {
            // If we are here it implies that this is an editor only asset type.
            // Please note that in those cases where we are certain about what the runtime asset type should be for this
            // source asset type, we will fix the fileextension.

            AZStd::string fileExtension;
            AzFramework::StringFunc::Path::GetExtension(seedPath.c_str(), fileExtension, false);
            auto found = m_sourceAssetTypeToRuntimeAssetTypeMap.find(fileExtension);

            if (found != m_sourceAssetTypeToRuntimeAssetTypeMap.end())
            {
                AzFramework::StringFunc::Path::ReplaceExtension(seedPath, found->second.c_str());
                AZ_Warning("AssetSeedManager", false, "( %s ) is an editor only asset. We wil use seed asset( %s ) instead.\n", assetPath.c_str(), seedPath.c_str());
            }
            else
            {
                AZ_Warning("AssetSeedManager", false, "Invalid seed asset ( %s ). This is an editor only asset. Please note that you can open the asset processor database and find \
                    all assets associated with this source asset and add those products instead. \n", seedPath.c_str());
                return;
            }
        }
        AZ::Data::AssetId assetId = GetAssetIdByPath(seedPath, platformFlags);
        if (assetId.IsValid())
        {
            AddSeedAsset(assetId, platformFlags, seedPath);
        }
        else
        {
            AZ_Warning("AssetSeedManager", false, "Unable to add asset ( %s ) to the seed list, could not find it on all requested platforms.\n", seedPath.c_str());
        }
    }

    void AssetSeedManager::RemoveSeedAsset(const AZStd::string& assetKey, AzFramework::PlatformFlags platformFlags)
    {
        AZ::Data::AssetId assetId = GetAssetIdByAssetKey(assetKey, platformFlags);

        if (assetId.IsValid())
        {
            RemoveSeedAsset(assetId, platformFlags);
        }
        else
        {
            AZ_Warning("AssetSeedManager", false, "Asset catalog does not know about the asset ( %s ) on all platforms. Unable to remove this asset from the seed list.\n", assetKey.c_str());
        }
    }

    void AssetSeedManager::AddPlatformToAllSeeds(AzFramework::PlatformId platform)
    {
        using namespace AzToolsFramework::AssetCatalog;

        AzFramework::PlatformFlags platformFlag = AzFramework::PlatformHelper::GetPlatformFlagFromPlatformIndex(platform);
        AZ::Data::AssetInfo assetInfo;
        bool isSpecialPlatform = AzFramework::PlatformHelper::IsSpecialPlatform(platformFlag);

        for (auto& seed : m_assetSeedList)
        {
            if (isSpecialPlatform)
            {
                seed.m_platformFlags |= platformFlag;
            }
            else
            {
                assetInfo = GetAssetInfoById(seed.m_assetId, platform);

                if (assetInfo.m_assetId.IsValid())
                {
                    seed.m_platformFlags |= platformFlag;
                }
            }
        }
    }

    void AssetSeedManager::RemovePlatformFromAllSeeds(AzFramework::PlatformId platform)
    {
        using namespace AzToolsFramework::AssetCatalog;
        AzFramework::PlatformFlags platformFlag = AzFramework::PlatformHelper::GetPlatformFlagFromPlatformIndex(platform);

        for (auto& seed : m_assetSeedList)
        {
            if ((seed.m_platformFlags & (~platformFlag)) == AzFramework::PlatformFlags::Platform_NONE)
            {
                AZ_Warning("AssetSeedManager", false, "Cannot remove platform ( %s ) from Seed ( %s ): Seed only has one platform", AzFramework::PlatformHelper::GetPlatformName(platform), seed.m_assetId.ToString<AZStd::string>().c_str());
            }
            else
            {
                seed.m_platformFlags &= (~platformFlag);
            }
        }
    }

    AZ::Data::AssetId AssetSeedManager::FindAssetIdByPathHint(const AZStd::string& pathHint) const
    {
        for (auto iter = m_assetSeedList.begin(); iter != m_assetSeedList.end(); ++iter)
        {
            if (AzFramework::StringFunc::Equal(iter->m_assetRelativePath.c_str(), pathHint.c_str()))
            {
                return iter->m_assetId;
            }
        }
        return AZ::Data::AssetId();
    }

    // Returns the AssetId if it was valid for all the platforms in platformFlags
    AZ::Data::AssetId AssetSeedManager::GetAssetIdByPath(const AZStd::string& assetPath, const AzFramework::PlatformFlags& platformFlags) const
    {
        using namespace AzToolsFramework::AssetCatalog;
        AZ::Data::AssetId assetId;
        AZStd::vector<AzFramework::PlatformId> platformIndices = AzFramework::PlatformHelper::GetPlatformIndicesInterpreted(platformFlags);
        bool foundInvalid = false;

        for (const auto& platformNum : platformIndices)
        {
            AZ::Data::AssetId foundAssetId;
            PlatformAddressedAssetCatalogRequestBus::EventResult(foundAssetId, platformNum, &PlatformAddressedAssetCatalogRequestBus::Events::GetAssetIdByPath, assetPath.c_str(), AZ::Data::s_invalidAssetType, false);
            if (!foundAssetId.IsValid())
            {
                AZ_Warning("AssetSeedManager", false, "Asset catalog does not know about the asset ( %s ) on platform ( %s ).", assetPath.c_str(), AzFramework::PlatformHelper::GetPlatformName(platformNum));
                foundInvalid = true;
            }
            else
            {
                assetId = foundAssetId;
            }
        }
        if (foundInvalid)
        {
            return AZ::Data::AssetId();
        }
        return assetId;
    }

    AZ::Data::AssetId AssetSeedManager::GetAssetIdByAssetKey(const AZStd::string& assetKey, const AzFramework::PlatformFlags& platformFlags) const
    {
        AZ::Data::AssetId assetId;
        auto found = assetKey.find(":");
        if (found != AZStd::string::npos)
        {
            assetId.m_subId = static_cast<AZ::u32>(atoi(assetKey.substr(found + 1).c_str()));
            assetId.m_guid = AZ::Uuid(assetKey.substr(0, found).c_str());
            return assetId;
        }

        // if we are here we will first try to query the asset catalog about the asset,
        // if we are unable to find the asset in the catalog than we will try to search it based on path hints.
        assetId = GetAssetIdByPath(assetKey, platformFlags);

        if (!assetId.IsValid())
        {
            assetId = FindAssetIdByPathHint(assetKey);
        }

        return assetId;
    }

    // Returns the asset info if it exists and is the same for the platform specified by platformIndex
    AZ::Data::AssetInfo AssetSeedManager::GetAssetInfoById(const AZ::Data::AssetId& assetId, const AzFramework::PlatformId& platformIndex) const
    {
        using namespace AzToolsFramework::AssetCatalog;
        AZ::Data::AssetInfo assetInfo;
        PlatformAddressedAssetCatalogRequestBus::EventResult(assetInfo, static_cast<AzFramework::PlatformId>(platformIndex), &PlatformAddressedAssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
        if (!assetInfo.m_assetId.IsValid())
        {
            AZ_Error("AssetSeedManager", false, "Could not find asset with id ( %s ) on platform ( %s ).", assetId.ToString<AZStd::string>().c_str(), AzFramework::PlatformHelper::GetPlatformName(platformIndex));
        }
        return assetInfo;
    }

    AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> AssetSeedManager::GetAllProductDependencies(
        const AZ::Data::AssetId& assetId,
        const AzFramework::PlatformId& platformIndex,
        const AZStd::unordered_set<AZ::Data::AssetId>& exclusionList,
        AssetFileDebugInfoList* optionalDebugList,
        AZStd::unordered_set<AZ::Data::AssetId>* cyclicalDependencySet) const
    {
        using namespace AzToolsFramework::AssetCatalog;

        if (optionalDebugList)
        {
            return AssetFileDebugInfoList::GetAllProductDependenciesDebug(assetId, platformIndex, optionalDebugList, cyclicalDependencySet, exclusionList);
        }

        // If not gathering debug info, then the recursion can happen in SQL. Call GetAllProductDependencies for the faster method of gathering.
        AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> getDependenciesResult = AZ::Failure(AZStd::string());
        PlatformAddressedAssetCatalogRequestBus::EventResult(getDependenciesResult, platformIndex, &PlatformAddressedAssetCatalogRequestBus::Events::GetAllProductDependenciesFilter, assetId, exclusionList);
        return getDependenciesResult;
    }

    void AssetSeedManager::PopulateAssetTypeMap()
    {
        AZStd::string sliceFileExtension;
        AzFramework::StringFunc::Path::GetExtension(AZ::SliceAsset::GetFileFilter(), sliceFileExtension, false);

        AZStd::string dynamicSliceFileExtension;
        AzFramework::StringFunc::Path::GetExtension(AZ::DynamicSliceAsset::GetFileFilter(), dynamicSliceFileExtension, false);

        m_sourceAssetTypeToRuntimeAssetTypeMap[sliceFileExtension] = dynamicSliceFileExtension;
        m_sourceAssetTypeToRuntimeAssetTypeMap[ScriptCanvas] = ScriptCanvasCompiled;
        m_sourceAssetTypeToRuntimeAssetTypeMap[ScriptCanvasFunction] = ScriptCanvasFunctionCompiled;
    }

    const AzFramework::AssetSeedList& AssetSeedManager::GetAssetSeedList() const
    {
        return m_assetSeedList;
    }

    AssetSeedManager::AssetsInfoList AssetSeedManager::GetDependenciesInfo(AzFramework::PlatformId platformIndex, const AZStd::unordered_set<AZ::Data::AssetId>& exclusionList, AssetFileDebugInfoList* optionalDebugList) const
    {
        if (!m_assetSeedList.size())
        {
            AZ_TracePrintf("AssetSeedManager", "Asset Seed list is empty.\n");
            return {};
        }

        if (!AzToolsFramework::PlatformAddressedAssetCatalog::CatalogExists(platformIndex))
        {
            // There's no catalog loaded for this platform, so there won't be any assets
            return {};
        }

        AssetSeedManager::AssetsInfoList assetsInfoList;
        AZStd::unordered_set<AZ::Data::AssetId> assetIdSet;
        AZStd::unordered_set<AZ::Data::AssetId> cyclicalDependencySet;
        for (int idx = 0; idx < m_assetSeedList.size(); idx++)
        {
            if(!AzFramework::PlatformHelper::HasPlatformFlag(m_assetSeedList[idx].m_platformFlags,platformIndex))
            {
                // This asset is not valid for the platformIndex
                continue;
            }

            AZ::Data::AssetInfo seedAssetInfo = GetAssetInfoById(m_assetSeedList[idx].m_assetId, platformIndex);

            if (optionalDebugList && 
                optionalDebugList->m_fileDebugInfoList.find(seedAssetInfo.m_assetId) == optionalDebugList->m_fileDebugInfoList.end())
            {
                optionalDebugList->m_fileDebugInfoList[seedAssetInfo.m_assetId].m_assetId = seedAssetInfo.m_assetId;
            }

            if (assetIdSet.find(seedAssetInfo.m_assetId) != assetIdSet.end())
            {
                // do not want duplicate enteries in the assets info list
                continue;
            }

            if(exclusionList.find(seedAssetInfo.m_assetId) != exclusionList.end())
            {
                continue;
            }

            assetsInfoList.emplace_back(AZStd::move(seedAssetInfo));
            assetIdSet.insert(seedAssetInfo.m_assetId);

            cyclicalDependencySet.clear();
            AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> getDependenciesResult =
                GetAllProductDependencies(m_assetSeedList[idx].m_assetId, platformIndex, exclusionList, optionalDebugList, &cyclicalDependencySet);
            if (getDependenciesResult.IsSuccess())
            {
                AZStd::vector<AZ::Data::ProductDependency> entries = getDependenciesResult.TakeValue();

                for (const AZ::Data::ProductDependency& productDependency : entries)
                {
                    if (productDependency.m_assetId.IsValid() && assetIdSet.find(productDependency.m_assetId) == assetIdSet.end())
                    {
                        assetIdSet.insert(productDependency.m_assetId);

                        AZ::Data::AssetInfo assetInfo = GetAssetInfoById(productDependency.m_assetId, platformIndex);
                        assetsInfoList.emplace_back(AZStd::move(assetInfo));
                    }
                }
            }
            else
            {
                AZ_Error("AssetSeedManager", false, "Unable to retrieve all product dependencies for asset ( %s ) with asset id ( %s ).\n", seedAssetInfo.m_relativePath.c_str(), m_assetSeedList[idx].m_assetId.ToString<AZStd::string>().c_str());
            }
        }

        return assetsInfoList;

    }

    AssetFileInfoList AssetSeedManager::GetDependencyList(AzFramework::PlatformId platformIndex, const AZStd::unordered_set<AZ::Data::AssetId>& exclusionList, AssetFileDebugInfoList* optionalDebugList) const
    {     
        AssetSeedManager::AssetsInfoList  assetInfoList = AZStd::move(GetDependenciesInfo(platformIndex, exclusionList, optionalDebugList));

        AssetFileInfoList assetFileInfoList;
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO != nullptr, "AZ::IO::FileIOBase must be ready for use.\n");
        AZStd::string assetRoot = PlatformAddressedAssetCatalog::GetAssetRootForPlatform(platformIndex);

        for (const AZ::Data::AssetInfo& assetInfo : assetInfoList)
        {
            if (assetInfo.m_assetId.IsValid())
            {
                if (!assetInfo.m_relativePath.empty())
                {
                    AZStd::string assetPath;
                    AzFramework::StringFunc::Path::Join(assetRoot.c_str(), assetInfo.m_relativePath.c_str(), assetPath);
                    if (!fileIO->Exists(assetPath.c_str()))
                    {
                        AZ_Warning("AssetSeedManager", false, "Asset ( %s ) does not exist in the cache folder.\n", assetPath.c_str());
                        continue;
                    }

                    AZ::IO::FileIOStream fileStream;
                    if (!fileStream.Open(assetPath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary))
                    {
                        AZ_Warning("AssetSeedManager", false, "Failed to open asset ( %s ).\n", assetPath.c_str());
                        continue;
                    }

                    AZ::IO::SizeType length = fileStream.GetLength();
                    AZ::Sha1 hash;
                    AZStd::array<AZ::u32, AssetFileInfo::s_arraySize> digest = { {0} };
                    AZ::u32 digestArray[AssetFileInfo::s_arraySize] = { 0 };
                    // If there's no length, there's no data to hash.
                    // It's valid to have 0 length files, these can be used as markers for the file system.
                    if (length)
                    {
                        AZStd::vector<uint8_t> buffer;
                        buffer.resize_no_construct(length);

                        if (fileStream.Read(length, buffer.data()) != length)
                        {
                            AZ_Warning("AssetSeedManager", false, "Failed to read entire asset file ( %s ).\n", assetPath.c_str());
                            continue;
                        }

                        hash.ProcessBytes(buffer.data(), buffer.size());
                        hash.GetDigest(digestArray);

                        for (int idx = 0; idx < digest.size(); idx++)
                        {
                            digest[idx] = digestArray[idx];
                        }
                    }

                    uint64_t modTime = fileIO->ModificationTime(assetInfo.m_relativePath.c_str());

                    AssetFileInfo assetFileInfo(assetInfo.m_assetId, assetInfo.m_relativePath, modTime, digest);

                    if (optionalDebugList)
                    {
                        optionalDebugList->m_fileDebugInfoList[assetInfo.m_assetId].m_assetRelativePath = assetInfo.m_relativePath;
                        optionalDebugList->m_fileDebugInfoList[assetInfo.m_assetId].m_fileSize = length;
                    }

                    assetFileInfoList.m_fileInfoList.push_back(assetFileInfo);
                }
                else
                {
                    AZ_Warning("AssetSeedManager", false, "Asset with asset id ( %s ) is missing relative path information in the asset catalog.\n", assetInfo.m_assetId.ToString<AZStd::string>().c_str());
                }
            }
        }

        return assetFileInfoList;
    }

    bool AssetSeedManager::SaveAssetFileInfo(const AZStd::string& destinationFilePath, AzFramework::PlatformFlags platformFlags, const AZStd::unordered_set<AZ::Data::AssetId>& exclusionList, const AZStd::string& debugFilePath)
    {
        auto fileExtensionOutcome = ValidateAssetListFileExtension(destinationFilePath);
        if (!fileExtensionOutcome.IsSuccess())
        {
            AZ_Error("AssetSeedManager", false, fileExtensionOutcome.GetError().c_str());
            return false;
        }

        if (AZ::IO::FileIOBase::GetInstance()->Exists(destinationFilePath.c_str()) && AZ::IO::FileIOBase::GetInstance()->IsReadOnly(destinationFilePath.c_str()))
        {
            AZ_Error("AssetSeedManager", false, "Unable to save asset info file (%s): file is marked Read-Only.\n", destinationFilePath.c_str());
            return false;
        }

        AZStd::vector<AzFramework::PlatformId> platformIndices = AzFramework::PlatformHelper::GetPlatformIndicesInterpreted(platformFlags);
        if (platformIndices.size() != 1)
        {
            AZ_Warning("AssetSeedManager", false, "AssetSeedManager::SaveAssetFileInfo can only operate on one platform at a time");
            return false;
        }

        bool useDebugInfoList = !debugFilePath.empty();
        AssetFileDebugInfoList debugInfo;
        AssetFileInfoList assetFileInfoList = GetDependencyList(platformIndices[0], exclusionList, useDebugInfoList ? &debugInfo : nullptr);

        bool debugSaveResult = true;
        if (useDebugInfoList)
        {
            debugInfo.BuildHumanReadableString();
            debugSaveResult = AZ::Utils::SaveObjectToFile(debugFilePath, AZ::DataStream::StreamType::ST_XML, &debugInfo);
        }

        if (assetFileInfoList.m_fileInfoList.empty())
        {
            // Don't save an empty list
            return debugSaveResult;
        }

        bool assetFileInfoListSaveResult = AZ::Utils::SaveObjectToFile(destinationFilePath, AZ::DataStream::StreamType::ST_XML, &assetFileInfoList);
        return assetFileInfoListSaveResult && debugSaveResult;
    }

    AZ::Outcome<AssetFileInfoList, AZStd::string> AssetSeedManager::LoadAssetFileInfo(const AZStd::string& assetListFileAbsolutePath)
    {
        auto fileExtensionOutcome = ValidateAssetListFileExtension(assetListFileAbsolutePath);
        if (!fileExtensionOutcome.IsSuccess())
        {
            return AZ::Failure(fileExtensionOutcome.GetError());
        }

        if (!AZ::IO::FileIOBase::GetInstance()->Exists(assetListFileAbsolutePath.c_str()))
        {
            return AZ::Failure(AZStd::string::format("Unable to load Asset List file ( %s ): file does not exist.", assetListFileAbsolutePath.c_str()));
        }

        AssetFileInfoList assetFileInfoList;
        if (!AZ::Utils::LoadObjectFromFileInPlace(assetListFileAbsolutePath.c_str(), assetFileInfoList))
        {
            return AZ::Failure(AZStd::string::format("Unable to load Asset List file ( %s ).", assetListFileAbsolutePath.c_str()));
        }

        return AZ::Success(assetFileInfoList);
    }

    const char* AssetSeedManager::GetSeedFileExtension()
    {
        return SeedFileExtension;
    }

    AZ::Outcome<void, AZStd::string> AssetSeedManager::ValidateSeedFileExtension(const AZStd::string& path)
    {
        if (!AzFramework::StringFunc::EndsWith(path, SeedFileExtension))
        {
            return AZ::Failure(AZStd::string::format(
                "Invalid Seed List file path ( %s ). Invalid file extension, Seed List files can only have ( .%s ) extension.\n",
                path.c_str(),
                SeedFileExtension));
        }

        return AZ::Success();
    }

    const char* AssetSeedManager::GetAssetListFileExtension()
    {
        return AssetListFileExtension;
    }

    AZ::Outcome<void, AZStd::string> AssetSeedManager::ValidateAssetListFileExtension(const AZStd::string& path)
    {
        if (!AzFramework::StringFunc::EndsWith(path, AssetListFileExtension))
        {
            return AZ::Failure(AZStd::string::format(
                "Invalid Asset List file path ( %s ). Invalid file extension, Asset List files can only have ( .%s ) extension.\n",
                path.c_str(),
                AssetListFileExtension));
        }

        return AZ::Success();
    }

    const AZStd::string& AssetSeedManager::GetReadablePlatformList(const AzFramework::SeedInfo& seed)
    {
        auto readablePlatformListIter = m_platformFlagsToReadablePlatformList.find(seed.m_platformFlags);
        if (readablePlatformListIter != m_platformFlagsToReadablePlatformList.end())
        {
            return readablePlatformListIter->second;
        }

        AZStd::vector<AZStd::string> platformNames = AzFramework::PlatformHelper::GetPlatforms(seed.m_platformFlags);
        AZStd::string readablePlatformList;
        AzFramework::StringFunc::Join(readablePlatformList, platformNames.begin(), platformNames.end(), ", ");

        m_platformFlagsToReadablePlatformList[seed.m_platformFlags] = readablePlatformList;
        return m_platformFlagsToReadablePlatformList.at(seed.m_platformFlags);
    }

    void AssetSeedManager::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AssetSeedManager>()
                ->Version(1)
                ->Field("assetSeedList", &AssetSeedManager::m_assetSeedList);

            AssetFileInfo::Reflect(serializeContext);
            AssetFileInfoList::Reflect(serializeContext);
            AssetFileDebugInfo::Reflect(serializeContext);
            AssetFileDebugInfoList::Reflect(serializeContext);
        }
    }

    bool AssetSeedManager::Save(const AZStd::string& destinationPath)
    {
        auto fileExtensionOutcome = ValidateSeedFileExtension(destinationPath);
        if (!fileExtensionOutcome.IsSuccess())
        {
            AZ_Error("AssetSeedManager", false, fileExtensionOutcome.GetError().c_str());
            return false;
        }

        if (AZ::IO::FileIOBase::GetInstance()->Exists(destinationPath.c_str()) && AZ::IO::FileIOBase::GetInstance()->IsReadOnly(destinationPath.c_str()))
        {
            AZ_Error("AssetSeedManager", false, "Unable to save seed file (%s): file is marked Read-Only.\n", destinationPath.c_str());
            return false;
        }

        return AZ::Utils::SaveObjectToFile(destinationPath, AZ::DataStream::StreamType::ST_XML, &m_assetSeedList);
    }

    void AssetSeedManager::UpdateSeedPath()
    {
        for (AzFramework::SeedInfo& seedInfo : m_assetSeedList)
        {
            AZStd::string assetPath = GetSeedPath(seedInfo.m_assetId, seedInfo.m_platformFlags);
            if (!assetPath.empty())
            {
                seedInfo.m_assetRelativePath = assetPath;
            }
        }
    }

    void AssetSeedManager::RemoveSeedPath()
    {
        for (AzFramework::SeedInfo& seedInfo : m_assetSeedList)
        {
            seedInfo.m_assetRelativePath.clear();
        }
    }

    bool AssetSeedManager::Load(const AZStd::string& sourceFilePath)
    {
        auto fileExtensionOutcome = ValidateSeedFileExtension(sourceFilePath);
        if (!fileExtensionOutcome.IsSuccess())
        {
            AZ_Error("AssetSeedManager", false, fileExtensionOutcome.GetError().c_str());
            return false;
        }

        AzFramework::AssetSeedList assetSeedList;
        if (!AZ::Utils::LoadObjectFromFileInPlace(sourceFilePath.c_str(), assetSeedList))
        {
            return false;
        }

        for (AzFramework::SeedInfo& seedInfo : assetSeedList)
        {
            AddSeedAsset(seedInfo.m_assetId, seedInfo.m_platformFlags, seedInfo.m_assetRelativePath);
        }

        return true;
    }

    AssetFileInfo::AssetFileInfo(AZ::Data::AssetId assetId, AZStd::string assetRelativePath, uint64_t modTime, AZStd::array<AZ::u32, s_arraySize> hash)
        : m_assetId(assetId)
        , m_modificationTime(modTime)
        , m_assetRelativePath(AZStd::move(assetRelativePath))
        , m_hash(AZStd::move(hash))
    {
    }

    void AssetFileInfo::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AssetFileInfo>()
                ->Version(1)
                ->Field("assetId", &AssetFileInfo::m_assetId)
                ->Field("assetRelativePath", &AssetFileInfo::m_assetRelativePath)
                ->Field("modificationTime", &AssetFileInfo::m_modificationTime)
                ->Field("hash", &AssetFileInfo::m_hash);
        }
    }

    void AssetFileInfoList::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AssetFileInfoList>()
                ->Version(1)
                ->Field("fileInfoList", &AssetFileInfoList::m_fileInfoList);
        }
    }

} // namespace AzToolsFramework
