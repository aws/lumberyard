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

#include <AzToolsFramework/Asset/AssetBundler.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/string/wildcard.h>
#include <AzCore/std/string/regex.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/IO/FileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AzToolsFramework
{
    const char AssetBundleSettingsFileExtension[] = "bundlesettings";
    const char BundleFileExtension[] = "pak";
    const char ComparisonRulesFileExtension[] = "rules";
    const char* AssetFileInfoListComparison::ComparisonTypeNames[] = { "delta", "union", "intersection", "complement", "filepattern"};
    void AssetBundleSettings::Reflect(AZ::ReflectContext * context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AssetBundleSettings>()
                ->Version(2)
                ->Field("AssetFileInfoListPath", &AssetBundleSettings::m_assetFileInfoListPath)
                ->Field("BundleFilePath", &AssetBundleSettings::m_bundleFilePath)
                ->Field("BundleVersion", &AssetBundleSettings::m_bundleVersion)
                ->Field("maxBundleSize", &AssetBundleSettings::m_maxBundleSizeInMB);
        }
    }

    AZ::Outcome<AssetBundleSettings, AZStd::string> AssetBundleSettings::Load(const AZStd::string& filePath)
    {
        auto fileExtensionOutcome = ValidateBundleSettingsFileExtension(filePath);
        if (!fileExtensionOutcome.IsSuccess())
        {
            return AZ::Failure(fileExtensionOutcome.GetError());
        }

        AssetBundleSettings assetBundleSettings;

        if (!AZ::Utils::LoadObjectFromFileInPlace(filePath.c_str(), assetBundleSettings))
        {
            return AZ::Failure(AZStd::string::format("Failed to load AssetBundleFileInfo file (%s) from disk.\n", filePath.c_str()));
        }
        
        assetBundleSettings.m_platform = GetPlatformFromAssetInfoFilePath(assetBundleSettings);


        return AZ::Success(assetBundleSettings);
    }

    bool AssetBundleSettings::Save(const AssetBundleSettings& assetBundleSettings, const AZStd::string& destinationFilePath)
    {
        auto fileExtensionOutcome = ValidateBundleSettingsFileExtension(destinationFilePath);
        if (!fileExtensionOutcome.IsSuccess())
        {
            AZ_Error("AssetBundler", false, fileExtensionOutcome.GetError().c_str());
            return false;
        }

        if (AZ::IO::FileIOBase::GetInstance()->Exists(destinationFilePath.c_str()) && AZ::IO::FileIOBase::GetInstance()->IsReadOnly(destinationFilePath.c_str()))
        {
            AZ_Error("AssetBundler", false, "Unable to save bundle settings file (%s): file is marked Read-Only.\n", destinationFilePath.c_str());
            return false;
        }
        return AZ::Utils::SaveObjectToFile(destinationFilePath.c_str(), AZ::DataStream::StreamType::ST_XML, &assetBundleSettings);
    }


    bool AssetBundleSettings::IsBundleSettingsFile(const AZStd::string& filePath)
    {
        return AzFramework::StringFunc::EndsWith(filePath, AssetBundleSettingsFileExtension);
    }

    const char* AssetBundleSettings::GetBundleSettingsFileExtension()
    {
        return AssetBundleSettingsFileExtension;
    }

    AZ::Outcome<void, AZStd::string> AssetBundleSettings::ValidateBundleSettingsFileExtension(const AZStd::string& path)
    {
        if (!AzFramework::StringFunc::EndsWith(path, AssetBundleSettingsFileExtension))
        {
            return AZ::Failure(AZStd::string::format(
                "Invalid Bundle Settings file path ( %s ). Invalid file extension, Bundle Settings files can only have ( .%s ) extension.\n",
                path.c_str(),
                AssetBundleSettingsFileExtension));
        }

        return AZ::Success();
    }

    const char* AssetBundleSettings::GetBundleFileExtension()
    {
        return BundleFileExtension;
    }

    AZ::Outcome<void, AZStd::string> AssetBundleSettings::ValidateBundleFileExtension(const AZStd::string& path)
    {
        if (!AzFramework::StringFunc::EndsWith(path, BundleFileExtension))
        {
            return AZ::Failure(AZStd::string::format(
                "Invalid Bundle file path ( %s ). Invalid file extension, Bundles can only have ( .%s ) extension.\n",
                path.c_str(),
                BundleFileExtension));
        }

        return AZ::Success();
    }

    AZ::u64 AssetBundleSettings::GetMaxBundleSizeInMB()
    {
        return MaxBundleSizeInMB;
    }

    AZStd::string AssetBundleSettings::GetPlatformFromAssetInfoFilePath(const AssetBundleSettings& assetBundleSettings)
    {
        return GetPlatformIdentifier(assetBundleSettings.m_assetFileInfoListPath);
    }

    bool AssetFileInfoListComparison::IsTokenFile(const AZStd::string& filePath)
    {
        return filePath[0] == '$';
    }

    void AssetFileInfoListComparison::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            ComparisonData::Reflect(serializeContext);
            serializeContext->Class<AssetFileInfoListComparison>()
                ->Version(1)
                ->Field("ComparisonDataListType", &AssetFileInfoListComparison::m_comparisonDataList);
        }
    }

    bool AssetFileInfoListComparison::Save(const AZStd::string& destinationFilePath) const
    {
        if (!AzFramework::StringFunc::EndsWith(destinationFilePath, ComparisonRulesFileExtension))
        {
            AZ_Error("AssetBundler", false, "Unable to save comparison file (%s). Invalid file extension, comparison files can only have (.%s) extension.\n", destinationFilePath.c_str(), ComparisonRulesFileExtension);
            return false;
        }

        if (AZ::IO::FileIOBase::GetInstance()->Exists(destinationFilePath.c_str()) && AZ::IO::FileIOBase::GetInstance()->IsReadOnly(destinationFilePath.c_str()))
        {
            AZ_Error("AssetBundler", false, "Unable to save comparison file (%s): file is marked Read-Only.\n", destinationFilePath.c_str());
            return false;
        }
        return AZ::Utils::SaveObjectToFile(destinationFilePath, AZ::DataStream::StreamType::ST_XML, this);
    }

    AZ::Outcome<AssetFileInfoListComparison, AZStd::string> AssetFileInfoListComparison::Load(const AZStd::string& filePath)
    {
        if (!AzFramework::StringFunc::EndsWith(filePath, ComparisonRulesFileExtension))
        {
            return AZ::Failure(AZStd::string::format("Unable to load comparison file (%s). Invalid file extension, comparison files can only have (.%s) extension.\n", filePath.c_str(), ComparisonRulesFileExtension));
        }

        AssetFileInfoListComparison assetFileInfoListComparison;
        if (!AZ::Utils::LoadObjectFromFileInPlace(filePath.c_str(), assetFileInfoListComparison))
        {
            return AZ::Failure(AZStd::string::format("Failed to load AssetFileInfoComparison file (%s) from disk.\n", filePath.c_str()));
        }
        return AZ::Success(AZStd::move(assetFileInfoListComparison));
    }

    AZ::Outcome<void, AZStd::string> AssetFileInfoListComparison::CompareAndSaveResults(const AZStd::vector<AZStd::string>& firstAssetFileInfoPathList, const AZStd::vector<AZStd::string>& secondAssetFileInfoPathList)
    {
        AZ::Outcome<AssetFileInfoList, AZStd::string> result = Compare(firstAssetFileInfoPathList, secondAssetFileInfoPathList);

        if (result.IsSuccess())
        {
            return SaveResults();
        }

        return AZ::Failure(AZStd::string::format("%s", result.GetError().c_str()));
    }

    AZ::Outcome<void, AZStd::string> AssetFileInfoListComparison::SaveResults() const
    {
        for (auto iter = m_assetFileInfoMap.begin(); iter != m_assetFileInfoMap.end(); iter++)
        {
            if (!IsTokenFile(iter->first))
            {
                if (!AZ::Utils::SaveObjectToFile(iter->first, AZ::DataStream::StreamType::ST_XML, &iter->second))
                {
                    return AZ::Failure(AZStd::string::format("Failed to serialize AssetFileInfoList file ( %s )to disk.\n", iter->first.c_str()));
                }
            }
        }
        return AZ::Success();
    }

    AZStd::vector<AZStd::string> AssetFileInfoListComparison::GetDestructiveOverwriteFilePaths()
    {
        AZStd::vector<AZStd::string> existingPaths;

        for (auto iter = m_assetFileInfoMap.begin(); iter != m_assetFileInfoMap.end(); iter++)
        {
            if (!IsTokenFile(iter->first) && AZ::IO::FileIOBase::GetInstance()->Exists(iter->first.c_str()))
            {
                existingPaths.emplace_back(iter->first);
            }
        }

        return existingPaths;
    }

    AssetFileInfoList AssetFileInfoListComparison::GetComparisonResults(const AZStd::string& comparisonKey)
    {
        auto comparisonResults = m_assetFileInfoMap.find(comparisonKey);
        if (comparisonResults == m_assetFileInfoMap.end())
        {
            return AssetFileInfoList();
        }
        return comparisonResults->second;
    }


    const char* AssetFileInfoListComparison::GetComparisonRulesFileExtension()
    {
        return ComparisonRulesFileExtension;
    }

    AZ::Outcome<AssetFileInfoList, AZStd::string>  AssetFileInfoListComparison::PopulateAssetFileInfo(const AZStd::string& assetFileInfoPath)
    {
        AssetFileInfoList assetFileInfoList;
        if (assetFileInfoPath.empty())
        {
            return AZ::Failure(AZStd::string::format("File path for the first asset file info list is empty.\n"));
        }
        if (!IsTokenFile(assetFileInfoPath))
        {
            if (!AZ::IO::FileIOBase::GetInstance()->Exists(assetFileInfoPath.c_str()))
            {
                return AZ::Failure(AZStd::string::format("File ( %s ) does not exists on disk.\n", assetFileInfoPath.c_str()));
            }

            if (!AZ::Utils::LoadObjectFromFileInPlace(assetFileInfoPath.c_str(), assetFileInfoList))
            {
                return AZ::Failure(AZStd::string::format("Failed to deserialize file ( %s ).\n", assetFileInfoPath.c_str()));
            }
        }
        else
        {
            auto found = m_assetFileInfoMap.find(assetFileInfoPath);
            if (found != m_assetFileInfoMap.end())
            {
                assetFileInfoList = found->second;
            }
            else
            {
                return AZ::Failure(AZStd::string::format("Failed to find AssetFileInfoList that matches the TAG ( %s ).\n", assetFileInfoPath.c_str()));
            }
        }

        if (!assetFileInfoList.m_fileInfoList.size())
        {
            return AZ::Failure(AZStd::string::format("File ( %s ) does not contain any assets.\n", assetFileInfoPath.c_str()));
        }

        return AZ::Success(assetFileInfoList);
    }

    AZ::Outcome<AssetFileInfoList, AZStd::string> AssetFileInfoListComparison::Compare(const AZStd::vector<AZStd::string>& firstAssetFileInfoPathList, const AZStd::vector<AZStd::string>& secondAssetFileInfoPathList)
    {
        AssetFileInfoList lastAssetFileInfoList;
        int secondAssetFileInfoIndex = 0;
        for (int index = 0; index < firstAssetFileInfoPathList.size(); index++)
        {
            AZ::Outcome<AssetFileInfoList, AZStd::string> firstResult = PopulateAssetFileInfo(firstAssetFileInfoPathList[index]);
            if (!firstResult.IsSuccess())
            {
                return AZ::Failure(firstResult.GetError());
            }
            AssetFileInfoList firstAssetFileInfoList = firstResult.TakeValue();
            AssetFileInfoList secondAssetFileInfoList;
            if (m_comparisonDataList[index].m_comparisonType != ComparisonType::FilePattern)
            {
                if (secondAssetFileInfoPathList.empty())
                {
                    return AZ::Failure(AZStd::string::format("Second Asset Info List is empty. Cannot perform comparison operation of type (%d).\n", static_cast<int>(m_comparisonDataList[index].m_comparisonType)));
                }

                if (secondAssetFileInfoIndex >= secondAssetFileInfoPathList.size())
                {
                    return AZ::Failure(AZStd::string::format("Second Asset Info List does not contain any entries to perform the comparison operation of type (%d).\n", static_cast<int>(m_comparisonDataList[index].m_comparisonType)));
                }

                AZ::Outcome<AssetFileInfoList, AZStd::string> secondResult = PopulateAssetFileInfo(secondAssetFileInfoPathList[secondAssetFileInfoIndex]);
                secondAssetFileInfoIndex++;
                if (!secondResult.IsSuccess())
                {
                    return AZ::Failure(secondResult.GetError());
                }

                secondAssetFileInfoList = secondResult.TakeValue();
            }

            AZ::Outcome<AssetFileInfoList, AZStd::string> result = AZ::Failure(AZStd::string());
            switch (m_comparisonDataList[index].m_comparisonType)
            {
            case ComparisonType::Delta:
            {
                result = Delta(firstAssetFileInfoList, secondAssetFileInfoList);
                break;
            }
            case ComparisonType::Union:
            {
                result = Union(firstAssetFileInfoList, secondAssetFileInfoList);
                break;
            }
            case ComparisonType::Intersection:
            {
                result = Intersection(firstAssetFileInfoList, secondAssetFileInfoList);
                break;
            }
            case ComparisonType::Complement:
            {
                result = Complement(firstAssetFileInfoList, secondAssetFileInfoList);
                break;
            }
            case ComparisonType::FilePattern:
            {
                result = FilePattern(firstAssetFileInfoList, index);
                break;
            }
            default:
                return AZ::Failure(AZStd::string::format("Invalid comparison type %d specified.\n", static_cast<int>(m_comparisonDataList[index].m_comparisonType)));
            }

            if (result.IsSuccess())
            {
                lastAssetFileInfoList = result.GetValue();
                m_assetFileInfoMap[m_comparisonDataList[index].m_destinationPath] = result.TakeValue();
            }
        }
        
        return AZ::Success(lastAssetFileInfoList);
    }

    AssetFileInfoList AssetFileInfoListComparison::Delta(const AssetFileInfoList& firstAssetFileInfoList, const AssetFileInfoList& secondAssetFileInfoList) const
    {
        AZStd::unordered_map<AZ::Data::AssetId, AzToolsFramework::AssetFileInfo> assetIdToAssetFileInfoMap;

        // Populate the map with entries from the secondAssetFileInfoList
        for (const AssetFileInfo& assetFileInfo : secondAssetFileInfoList.m_fileInfoList)
        {
            assetIdToAssetFileInfoMap[assetFileInfo.m_assetId] = assetFileInfo;
        }

        for (const AssetFileInfo& assetFileInfo : firstAssetFileInfoList.m_fileInfoList)
        {
            auto found = assetIdToAssetFileInfoMap.find(assetFileInfo.m_assetId);
            if (found != assetIdToAssetFileInfoMap.end())
            {
                bool isHashEqual = true;
                // checking the file hash
                for (int idx = 0; idx < AzToolsFramework::AssetFileInfo::s_arraySize; idx++)
                {
                    if (found->second.m_hash[idx] != assetFileInfo.m_hash[idx])
                    {
                        isHashEqual = false;
                        break;
                    }
                }

                if (isHashEqual)
                {
                    assetIdToAssetFileInfoMap.erase(found);
                }
            }
        }

        AssetFileInfoList assetFileInfoList;
        for (auto iter = assetIdToAssetFileInfoMap.begin(); iter != assetIdToAssetFileInfoMap.end(); iter++)
        {
            assetFileInfoList.m_fileInfoList.emplace_back(AZStd::move(iter->second));
        }

        return assetFileInfoList;
    }

    AssetFileInfoList AssetFileInfoListComparison::Union(const AssetFileInfoList& firstAssetFileInfoList, const AssetFileInfoList& secondAssetFileInfoList) const
    {
        AZStd::unordered_map<AZ::Data::AssetId, AzToolsFramework::AssetFileInfo> assetIdToAssetFileInfoMap;

        // Populate the map with entries from the secondAssetFileInfoList
        for (const AssetFileInfo& assetFileInfo : secondAssetFileInfoList.m_fileInfoList)
        {
            assetIdToAssetFileInfoMap[assetFileInfo.m_assetId] = assetFileInfo;
        }

        for (const AssetFileInfo& assetFileInfo : firstAssetFileInfoList.m_fileInfoList)
        {
            auto found = assetIdToAssetFileInfoMap.find(assetFileInfo.m_assetId);
            if (found == assetIdToAssetFileInfoMap.end())
            {
                assetIdToAssetFileInfoMap[assetFileInfo.m_assetId] = assetFileInfo;
            }
        }

        //populate AssetFileInfoList from map
        AssetFileInfoList assetFileInfoList;
        for (auto iter = assetIdToAssetFileInfoMap.begin(); iter != assetIdToAssetFileInfoMap.end(); iter++)
        {
            assetFileInfoList.m_fileInfoList.emplace_back(AZStd::move(iter->second));
        }

        return assetFileInfoList;
    }

    AssetFileInfoList AssetFileInfoListComparison::Intersection(const AssetFileInfoList& firstAssetFileInfoList, const AssetFileInfoList& secondAssetFileInfoList) const
    {
        AZStd::unordered_map<AZ::Data::AssetId, AzToolsFramework::AssetFileInfo> assetIdToAssetFileInfoMap;

        // Populate the map with entries from the secondAssetFileInfoList
        for (const AssetFileInfo& assetFileInfo : secondAssetFileInfoList.m_fileInfoList)
        {
            assetIdToAssetFileInfoMap[assetFileInfo.m_assetId] = assetFileInfo;
        }

        AssetFileInfoList assetFileInfoList;
        for (const AssetFileInfo& assetFileInfo : firstAssetFileInfoList.m_fileInfoList)
        {
            auto found = assetIdToAssetFileInfoMap.find(assetFileInfo.m_assetId);
            if (found != assetIdToAssetFileInfoMap.end())
            {
                assetFileInfoList.m_fileInfoList.emplace_back(AZStd::move(found->second));
            }
        }

        return assetFileInfoList;
    }

    AssetFileInfoList AssetFileInfoListComparison::Complement(const AssetFileInfoList& firstAssetFileInfoList, const AssetFileInfoList& secondAssetFileInfoList) const
    {
        AZStd::unordered_map<AZ::Data::AssetId, AzToolsFramework::AssetFileInfo> assetIdToAssetFileInfoMap;
        
        // Populate the map with entries from the firstAssetFileInfoList
        for (const AssetFileInfo& assetFileInfo : firstAssetFileInfoList.m_fileInfoList)
        {
            assetIdToAssetFileInfoMap[assetFileInfo.m_assetId] = assetFileInfo;
        }

        AssetFileInfoList assetFileInfoList;
        for (const AssetFileInfo& assetFileInfo : secondAssetFileInfoList.m_fileInfoList)
        {
            auto found = assetIdToAssetFileInfoMap.find(assetFileInfo.m_assetId);
            if (found == assetIdToAssetFileInfoMap.end())
            {
                assetFileInfoList.m_fileInfoList.emplace_back(AZStd::move(assetFileInfo));
            }
        }

        return assetFileInfoList;
    }

    AssetFileInfoList AssetFileInfoListComparison::FilePattern(const AssetFileInfoList& assetFileInfoList, int index) const
    {
        bool isWildCard = m_comparisonDataList[index].m_filePatternType == FilePatternType::Wildcard;

        AssetFileInfoList assetFileInfoListResult;
        for (const AssetFileInfo& assetFileInfo : assetFileInfoList.m_fileInfoList)
        {
            if (isWildCard)
            {
                if (AZStd::wildcard_match(m_comparisonDataList[index].m_filePattern, assetFileInfo.m_assetRelativePath))
                {
                    assetFileInfoListResult.m_fileInfoList.push_back(assetFileInfo);
                }
            }
            else
            {
                AZStd::regex regex(m_comparisonDataList[index].m_filePattern.c_str(), AZStd::regex::extended);
                if (AZStd::regex_match(assetFileInfo.m_assetRelativePath.c_str(), regex))
                {
                    assetFileInfoListResult.m_fileInfoList.push_back(assetFileInfo);
                }
            }
        }

        return assetFileInfoListResult;
    }

    void AssetFileInfoListComparison::AddComparisonStep(const ComparisonData& comparisonData)
    {
        m_comparisonDataList.emplace_back(comparisonData);
    }

    AZStd::vector<AssetFileInfoListComparison::ComparisonData> AzToolsFramework::AssetFileInfoListComparison::GetComparisonList() const
    {
        return m_comparisonDataList;
    }

    void AssetFileInfoListComparison::SetDestinationPath(const int& idx, const AZStd::string& path)
    {
        if (idx >= m_comparisonDataList.size())
        {
            return;
        }
        m_comparisonDataList[idx].m_destinationPath = path;
    }

    AssetFileInfoListComparison::ComparisonData::ComparisonData(const ComparisonType& type, const AZStd::string& destinationPath, const AZStd::string& filePattern, FilePatternType filePatternType)
        : m_comparisonType(type)
        , m_destinationPath(destinationPath)
        , m_filePattern(filePattern)
        , m_filePatternType(filePatternType)
    {
    }

    void AssetFileInfoListComparison::ComparisonData::Reflect(AZ::ReflectContext * context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ComparisonData>()
                ->Version(1)
                ->Field("comparisonType", &ComparisonData::m_comparisonType)
                ->Field("filePattern", &ComparisonData::m_filePattern)
                ->Field("filePatternType", &ComparisonData::m_filePatternType)
                ->Field("destinationPath", &ComparisonData::m_destinationPath);
        }
    }

    /*
    * Asset bundler Path and file name utils
    */
    void SplitFilename(const AZStd::string& filePath, AZStd::string& baseFileName, AZStd::string& platformIdentifier)
    {
        AZStd::string fileName;
        AzFramework::StringFunc::Path::GetFileName(filePath.c_str(), fileName);

        for (const AZStd::string& platformName : AzFramework::PlatformHelper::GetPlatforms(AzFramework::PlatformFlags::AllPlatforms))
        {
            AZStd::string appendedPlatform = AZStd::string::format("_%s", platformName.c_str());
            if (AzFramework::StringFunc::EndsWith(fileName, appendedPlatform))
            {
                AzFramework::StringFunc::RChop(fileName, appendedPlatform.size());
                baseFileName = fileName;
                platformIdentifier = platformName;
                return;
            }
        }
    }

    void RemovePlatformIdentifier(AZStd::string& filePath)
    {
        AZStd::string baseFileName;
        AZStd::string platform;
        SplitFilename(filePath, baseFileName, platform);

        if (platform.empty())
        {
            return;
        }

        AZStd::string extension;
        AzFramework::StringFunc::Path::GetExtension(filePath.c_str(), extension);

        AzFramework::StringFunc::Path::ReplaceFullName(filePath, baseFileName.c_str(), extension.c_str());
    }

    AZStd::string GetPlatformIdentifier(const AZStd::string& filePath)
    {
        AZStd::string baseFileName;
        AZStd::string platform;
        SplitFilename(filePath, baseFileName, platform);
        return platform;
    }
} // namespace AzToolsFramework
