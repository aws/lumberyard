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

#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/OutCome/OutCome.h>
#include <AzToolsFramework/Asset/AssetSeedManager.h>
#include <AzFramework/Asset/AssetBundleManifest.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzToolsFramework
{
    constexpr AZ::u64 MaxBundleSizeInMB = 2 * 1024;
    class AssetBundleSettings
    {
    public:
        AZ_TYPE_INFO(AssetBundleSettings, "{B9597C91-540E-41A9-9572-80A629061914}");
        
        static void Reflect(AZ::ReflectContext * context);

        //! Loads the AssetBundleSettings file from the file path
        static AZ::Outcome<AssetBundleSettings, AZStd::string> Load(const AZStd::string& filePath);
        static bool Save(const AssetBundleSettings& assetBundleFileInfo, const AZStd::string& destinationFilePath);

        static bool IsBundleSettingsFile(const AZStd::string& filePath);

        //! Returns the Bundle Settings file extension
        static const char* GetBundleSettingsFileExtension();

        //! Validates that the input path has the proper file extension for a Bundle Settings file.
        //! Input path can be relative or absolute.
        //! Returns void on success, error message on failure.
        static AZ::Outcome<void, AZStd::string> ValidateBundleSettingsFileExtension(const AZStd::string& path);

        //! Returns the Bundle file extension
        static const char* GetBundleFileExtension();

        //! Validates that the input path has the proper file extension for a Bundle.
        //! Input path can be relative or absolute.
        //! Returns void on success, error message on failure.
        static AZ::Outcome<void, AZStd::string> ValidateBundleFileExtension(const AZStd::string& path);

        static AZ::u64 GetMaxBundleSizeInMB();
        static AZStd::string GetPlatformFromAssetInfoFilePath(const AssetBundleSettings& assetBundleSettings);

        AZStd::string m_platform;
        AZStd::string m_assetFileInfoListPath;
        AZStd::string m_bundleFilePath; // the file path where the parent bundle file should get saved to disk.
        int m_bundleVersion = AzFramework::AssetBundleManifest::CurrentBundleVersion;
        AZ::u64 m_maxBundleSizeInMB = MaxBundleSizeInMB;
    };

   /*
   * This class can we used to create a new AssetFileInfoList based on the
   * comparison type specified by the user
   */
    class AssetFileInfoListComparison
    {
    public:
        enum class ComparisonType : AZ::u8
        {
            //! Given two AssetFileInfoLists A and B, creates a new AssetFileInfoList C such that assets in it are either 
            //! because they are present in B but not in A or because their file hash has changed between A and B.
            Delta,
            //! Given two AssetFileInfoLists A and B, creates a new AssetFileInfoList C such that it contains all the assets present in A and B.
            //! If the asset is contained in both A and B, it will have B's information.
            Union,
            //! Given two AssetFileInfoLists A and B, creates a new AssetFileInfoList C such that it contains only the assets that are present in both A and B.
            //! It is not necessary that all the information of assets should match between A and B, we are only checking the assetId
            //! and the asset will have B's information.
            Intersection,
            //! Given two AssetFileInfoLists A and B, creates a new AssetFileInfoList C such that it contains all the assets that are present in B but not in A.
            //! It is not necessary that all the information of assets should match in A and B, we are only checking by assetId.
            Complement,
            //! Given an AssetFileInfoList A, creates a new AssetFileInfoList C based on pattern matching.
            //! Pattern matching can be either wildcard or regex.
            FilePattern,

            //! New types go above NumPatterns- if you add one be sure to add it to ComparisonTypeNames as well
            NumPatterns,
            Default = 255,
        };
        static const char* ComparisonTypeNames[aznumeric_cast<int>(ComparisonType::NumPatterns)];

        enum class FilePatternType : AZ::u8
        {
            //! The pattern is a file wildcard pattern (glob)
            Wildcard,
            //! The pattern is a regular expression pattern
            Regex,

            Default = 255,
        };

        struct ComparisonData
        {
            AZ_TYPE_INFO(ComparisonData, "{B39A7148-AC9D-4038-A85E-2C86A0B2DEF6}");
            ComparisonData(const ComparisonType& type, const AZStd::string& destinationPath, const AZStd::string& filePattern = AZStd::string(), FilePatternType filePatternType = FilePatternType::Default);
            ComparisonData() = default;
            static void Reflect(AZ::ReflectContext * context);
            ComparisonType m_comparisonType = ComparisonType::Default;
            FilePatternType m_filePatternType = FilePatternType::Default;
            AZStd::string m_filePattern;
            AZStd::string m_destinationPath; // absolute path where the result of the comparison will be saved.
        };
        AZ_TYPE_INFO(AssetFileInfoListComparison, "{AC003572-3A33-476C-9B2B-ADDA4F7BB870}");
        AZ_CLASS_ALLOCATOR(AssetFileInfoListComparison, AZ::SystemAllocator, 0);

        AssetFileInfoListComparison() = default;

        static void Reflect(AZ::ReflectContext* context);

        void AddComparisonStep(const ComparisonData& comparisonData);

        AZStd::vector<ComparisonData> GetComparisonList() const;

        void SetDestinationPath(const int& idx, const AZStd::string& path);

        //! This can be used to serialize the AssetFileInfoListComparison to the destination file path. 
        bool Save(const AZStd::string& destinationFilePath) const;

        //! Loads the assetFileInfoListComparison file from the file path
        static AZ::Outcome<AssetFileInfoListComparison, AZStd::string> Load(const AZStd::string& filePath);

        //! Determine whether the file is a token file or not
        static bool IsTokenFile(const AZStd::string& filePath);

        //! Compares the two assetFileInfoLists and returns a AssetFileInfoList
        AZ::Outcome<AssetFileInfoList, AZStd::string> Compare(const AZStd::vector<AZStd::string>& firstAssetFileInfoPathList, const AZStd::vector<AZStd::string>& secondAssetFileInfoPathList = {});

        //! Compares the two assetFileInfoLists saves the assetFileInfoList result in the destination path
        AZ::Outcome<void, AZStd::string> CompareAndSaveResults(const AZStd::vector<AZStd::string>& firstAssetFileInfoPathList, const AZStd::vector<AZStd::string>& secondAssetFileInfoPathList = {});

        //! Saves all previously completed comparisons to disk if output is a file path
        AZ::Outcome<void, AZStd::string> SaveResults() const;

        //! Get the absolute paths of any files marked for save that currently exist on disk
        AZStd::vector<AZStd::string> GetDestructiveOverwriteFilePaths();

        AssetFileInfoList GetComparisonResults(const AZStd::string& comparisonKey);

        static const char* GetComparisonRulesFileExtension();

    protected:

        AssetFileInfoList Delta(const AssetFileInfoList& firstAssetFileInfoList, const AssetFileInfoList& secondAssetFileInfoList) const;
        AssetFileInfoList Union(const AssetFileInfoList& firstAssetFileInfoList, const AssetFileInfoList& secondAssetFileInfoList) const;
        AssetFileInfoList Intersection(const AssetFileInfoList& firstAssetFileInfoList, const AssetFileInfoList& secondAssetFileInfoList) const;
        AssetFileInfoList Complement(const AssetFileInfoList& firstAssetFileInfoList, const AssetFileInfoList& secondAssetFileInfoList) const;
        AssetFileInfoList FilePattern(const AssetFileInfoList& assetFileInfoList, int index) const;
        AZ::Outcome<AssetFileInfoList, AZStd::string> PopulateAssetFileInfo(const AZStd::string& assetFileInfoPath);

        AZStd::vector<ComparisonData> m_comparisonDataList;
        //This should not be serialized to disk because this is storing internal state that can change.
        AZStd::unordered_map<AZStd::string, AssetFileInfoList> m_assetFileInfoMap;
    };

    /*
    * Some Helpers for dealing with filenames in the AssetBundler tools
    */
    // Split the file name to get base name and platform identifier
    void SplitFilename(const AZStd::string& filePath, AZStd::string& baseFileName, AZStd::string& platformIdentifier);

    //! Removes the platform identifier from the filename if present
    void RemovePlatformIdentifier(AZStd::string& filePath);

    //! Returns the platform identifier from the filename, will return an empty string if none found
    AZStd::string GetPlatformIdentifier(const AZStd::string& filePath);
} // namespace AzToolsFramework
