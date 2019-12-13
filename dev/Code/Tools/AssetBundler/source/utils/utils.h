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
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/IO/SystemFile.h> //AZ_MAX_PATH_LEN
#include <AzCore/Outcome/Outcome.h>
#include <AzFramework/Platform/PlatformDefaults.h>
#include <AzToolsFramework/Asset/AssetUtils.h>

namespace AssetBundler
{
    enum CommandType
    {
        Invalid,
        Seeds,
        AssetLists,
        ComparisonRules,
        Compare,
        BundleSettings,
        Bundles,
        BundleSeed
    };

    ////////////////////////////////////////////////////////////////////////////////////////////
    // General
    extern const char* AppWindowName;
    extern const char* AppWindowNameVerbose;
    extern const char* HelpFlag;
    extern const char* HelpFlagAlias;
    extern const char* VerboseFlag;
    extern const char* SaveFlag;
    extern const char* PlatformArg;
    extern const char* PrintFlag;
    extern const char* AssetCatalogFileArg;
    extern const char* AllowOverwritesFlag;
    ////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Seeds
    extern const char* SeedsCommand;
    extern const char* SeedListFileArg;
    extern const char* AddSeedArg;
    extern const char* RemoveSeedArg;
    extern const char* AddPlatformToAllSeedsFlag;
    extern const char* RemovePlatformFromAllSeedsFlag;
    extern const char* UpdateSeedPathArg;
    extern const char* RemoveSeedPathArg;
    ////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Asset Lists
    extern const char* AssetListsCommand;
    extern const char* AssetListFileArg;
    extern const char* AddDefaultSeedListFilesFlag;
    extern const char* DryRunFlag;
    extern const char* GenerateDebugFileFlag;
    ////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Comparison Rules
    extern const char* ComparisonRulesCommand;
    extern const char* ComparisonRulesFileArg;
    extern const char* ComparisonTypeArg;
    extern const char* ComparisonFilePatternArg;
    extern const char* ComparisonFilePatternTypeArg;
    ////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Compare
    extern const char* CompareCommand;
    extern const char* CompareFirstFileArg;
    extern const char* CompareSecondFileArg;
    extern const char* CompareOutputFileArg;
    extern const char* ComparePrintArg;
    ////////////////////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////////////////
    // Bundle Settings 
    extern const char* BundleSettingsCommand;
    extern const char* BundleSettingsFileArg;
    extern const char* OutputBundlePathArg;
    extern const char* BundleVersionArg;
    extern const char* MaxBundleSizeArg;
    ////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Bundles
    extern const char* BundlesCommand;
    ////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Bundle Seed
    extern const char* BundleSeedCommand;
    ////////////////////////////////////////////////////////////////////////////////////////////

    extern const char* AssetCatalogFilename;
    extern char g_cachedEngineRoot[AZ_MAX_PATH_LEN];
    static const size_t MaxErrorMessageLength = 4096;

    //! This struct stores gem related information
    struct GemInfo
    {
        AZ_CLASS_ALLOCATOR(GemInfo, AZ::SystemAllocator, 0);
        GemInfo(AZStd::string name, AZStd::string relativeFilePath, AZStd::string absoluteFilePath);
        GemInfo() = default;
        AZStd::string m_gemName;
        AZStd::string m_relativeFilePath;
        AZStd::string m_absoluteFilePath;
    };


    // The Warning Absorber is used to absorb warnings 
    // One case that this is being used is during loading of the asset catalog.
    // During loading the asset catalog tries to communicate to the AP which is not required for this application. 
    class WarningAbsorber
        : public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        WarningAbsorber();
        ~WarningAbsorber();

        bool OnWarning(const char* window, const char* message) override;
        bool OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message) override;
    };

    // computes the asset alias and game name either through the asset catalog file provided by the user or using the platform and game folder 
    AZ::Outcome<void, AZStd::string> ComputeAssetAliasAndGameName(const AZStd::string& platformIdentifier, const AZStd::string& assetCatalogFile, AZStd::string& assetAlias, AZStd::string& gameName);

    // Computes the engine root and cache it locally
    bool ComputeEngineRoot();

    // Retrurns the engine root
    const char* GetEngineRoot();

    //! Add the specified platform identifier to the filename
    void AddPlatformIdentifier(AZStd::string& filePath, const AZStd::string& platformIdentifier);

    //! Returns a list of absolute file path of all the default seedFiles for the current game project.
    AZStd::vector<AZStd::string> GetDefaultSeedListFiles(const char* root, const AZStd::vector<AzToolsFramework::AssetUtils::GemInfo>& gemInfoList, AzFramework::PlatformFlags platformFlags);

    //! Given an absolute gem seed file path determines whether the file is valid for the current game project.
    //! This method is for validating gem seed list files only.
    bool IsGemSeedFilePathValid(const char* root, AZStd::string seedAbsoluteFilePath, const AZStd::vector<AzToolsFramework::AssetUtils::GemInfo>& gemInfoList, AzFramework::PlatformFlags platformFlags);

    //! Returns platformFlags of all enabled platforms by parsing all the asset processor config files.
    //! Please note that the game project could be in a different location to the engine therefore we need the assetRoot param.
    AzFramework::PlatformFlags GetEnabledPlatformFlags(const char* root, const char* assetRoot, const char* gameName);

    //! Filepath is a helper class that is used to find the absolute path of a file
    //! if the inputted file path is an absolute path than it does nothing
    //! if the inputted file path is a relative path than based on whether the user
    //! also inputted a root directory it computes the absolute path, 
    //! if root directory is provided it uses that otherwise it uses the engine root as the default root folder.
    class FilePath
    {
    public:
        AZ_CLASS_ALLOCATOR(FilePath, AZ::SystemAllocator, 0);
        explicit FilePath(const AZStd::string& filePath, AZStd::string platformIdentifier = AZStd::string());
        FilePath() = default;
        const AZStd::string& AbsolutePath() const;
        const AZStd::string& OriginalPath() const;
        bool IsValid() const;
    private:
        void ComputeAbsolutePath(AZStd::string& filePath, const AZStd::string& platformIdentifier);

        AZStd::string m_absolutePath;
        AZStd::string m_originalPath;
        bool m_validPath = false;
    };

    void ValidateOutputFilePath(FilePath filePath, const char* format, ...);

    //! ScopedTraceHandler can be used to handle and report errors 
    class ScopedTraceHandler : public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        
        ScopedTraceHandler();
        ~ScopedTraceHandler();

        //! TraceMessageBus Interface
        bool OnError(const char* /*window*/, const char* /*message*/) override;
        //////////////////////////////////////////////////////////
        //! Returns the error count
        int GetErrorCount() const;
        //! Report all the errors
        void ReportErrors();
        //! Clear all the errors
        void ClearErrors();
    private:
        AZStd::vector<AZStd::string> m_errors;
        bool m_reportingError = false;
    };

    AZ::Outcome<AzToolsFramework::AssetFileInfoListComparison::ComparisonType, AZStd::string> ParseComparisonType(const AZStd::string& comparisonType);
}
