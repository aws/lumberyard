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

#include <AzFramework/API/BootstrapReaderBus.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzFramework/Platform/PlatformDefaults.h>
#include <AzToolsFramework/Asset/AssetSeedManager.h>
#include <AzToolsFramework/Asset/AssetBundler.h>
#include <source/Utils/utils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/regex.h>
#include <cctype>



namespace AssetBundler
{
    // General
    const char* AppWindowName = "AssetBundler";
    const char* AppWindowNameVerbose = "AssetBundlerVerbose";
    const char* HelpFlag = "help";
    const char* HelpFlagAlias = "h";
    const char* VerboseFlag = "verbose";
    const char* SaveFlag = "save";
    const char* PlatformArg = "platform";
    const char* PrintFlag = "print";
    const char* AssetCatalogFileArg = "overrideAssetCatalogFile";
    const char* AllowOverwritesFlag = "allowOverwrites";

    // Seeds
    const char* SeedsCommand = "seeds";
    const char* SeedListFileArg = "seedListFile";
    const char* AddSeedArg = "addSeed";
    const char* RemoveSeedArg = "removeSeed";
    const char* AddPlatformToAllSeedsFlag = "addPlatformToSeeds";
    const char* RemovePlatformFromAllSeedsFlag = "removePlatformFromSeeds";
    const char* UpdateSeedPathArg = "updateSeedPath";
    const char* RemoveSeedPathArg = "removeSeedPath";

    // Asset Lists
    const char* AssetListsCommand = "assetLists";
    const char* AssetListFileArg = "assetListFile";
    const char* AddDefaultSeedListFilesFlag = "addDefaultSeedListFiles";
    const char* DryRunFlag = "dryRun";
    const char* GenerateDebugFileFlag = "generateDebugFile";

    // Comparison Rules
    const char* ComparisonRulesCommand = "comparisonRules";
    const char* ComparisonRulesFileArg = "comparisonRulesFile";
    const char* ComparisonTypeArg = "comparisonType";
    const char* ComparisonFilePatternArg = "filePattern";
    const char* ComparisonFilePatternTypeArg = "filePatternType";

    // Compare
    const char* CompareCommand = "compare";
    const char* CompareFirstFileArg = "firstAssetFile";
    const char* CompareSecondFileArg = "secondAssetFile";
    const char* CompareOutputFileArg = "output";
    const char* ComparePrintArg = "print";

    // Bundle Settings 
    const char* BundleSettingsCommand = "bundleSettings";
    const char* BundleSettingsFileArg = "bundleSettingsFile";
    const char* OutputBundlePathArg = "outputBundlePath";
    const char* BundleVersionArg = "bundleVersion";
    const char* MaxBundleSizeArg = "maxSize";

    // Bundles
    const char* BundlesCommand = "bundles";

    // Bundle Seed
    const char* BundleSeedCommand = "bundleSeed";

    const char* AssetCatalogFilename = "assetcatalog.xml";

    char g_cachedEngineRoot[AZ_MAX_PATH_LEN];

    
    const char EngineDirectoryName[] = "Engine";
    const char PlatformsDirectoryName[] = "Platforms";
    const char GemsDirectoryName[] = "Gems";
    const char GemsSeedFileName[] = "seedList";
    const char EngineSeedFileName[] = "SeedAssetList";

    namespace Internal
    {
        void AddPlatformSeeds(AZStd::string rootFolder, AZStd::vector<AZStd::string>& defaultSeedLists, AzFramework::PlatformFlags platformFlags)
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZStd::vector<AzFramework::PlatformId> platformsIdxList = AzFramework::PlatformHelper::GetPlatformIndices(platformFlags);
            
            for (const AzFramework::PlatformId& platformId : platformsIdxList)
            {
                AZStd::string platformDirectory;
                AzFramework::StringFunc::Path::Join(rootFolder.c_str(), AzFramework::PlatformHelper::GetPlatformName(platformId), platformDirectory);
                if (fileIO->Exists(platformDirectory.c_str()))
                {
                    bool recurse = true;
                    AZ::Outcome<AZStd::list<AZStd::string>, AZStd::string> result = AzFramework::FileFunc::FindFileList(platformDirectory,
                        AZStd::string::format("*.%s", AzToolsFramework::AssetSeedManager::GetSeedFileExtension()).c_str(), recurse);

                    if (result.IsSuccess())
                    {
                        AZStd::list<AZStd::string> seedFiles = result.TakeValue();
                        for (AZStd::string& seedFile : seedFiles)
                        {
                            AZStd::string normalizedFilePath = seedFile;
                            AzFramework::StringFunc::Path::Normalize(seedFile);
                            defaultSeedLists.emplace_back(seedFile);
                        }
                    }
                }
            }
        }

        void AddPlatformsDirectorySeeds(const AZStd::string& rootFolder, AZStd::vector<AZStd::string>& defaultSeedLists, AzFramework::PlatformFlags platformFlags)
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "AZ::IO::FileIOBase must be ready for use.\n");

            // Check whether platforms directory exists inside the root, if yes than add
            // * All seed files from the platforms directory
            // * All platform specific seed files based on the platform flags specified. 
            AZStd::string platformsDirectory;
            AzFramework::StringFunc::Path::Join(rootFolder.c_str(), PlatformsDirectoryName, platformsDirectory);
            if (fileIO->Exists(platformsDirectory.c_str()))
            {
                fileIO->FindFiles(platformsDirectory.c_str(),
                    AZStd::string::format("*.%s", AzToolsFramework::AssetSeedManager::GetSeedFileExtension()).c_str(),
                    [&](const char* fileName)
                    {
                        AZStd::string normalizedFilePath = fileName;
                        AzFramework::StringFunc::Path::Normalize(normalizedFilePath);
                        defaultSeedLists.emplace_back(normalizedFilePath);
                        return true;
                    });
            }

            AddPlatformSeeds(platformsDirectory, defaultSeedLists, platformFlags);
        }
    }

    bool ComputeEngineRoot()
    {
        if (g_cachedEngineRoot[0])
        {
            return true;
        }

        const char* engineRoot = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(engineRoot, &AzFramework::ApplicationRequests::GetEngineRoot);
        if (!engineRoot)
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Unable to locate engine root.\n");
            return false;
        }

        azstrcpy(g_cachedEngineRoot, AZ_MAX_PATH_LEN, engineRoot);
        return true;
    }

    const char* GetEngineRoot()
    {
        if (!g_cachedEngineRoot[0])
        {
            ComputeEngineRoot();
        }

        return g_cachedEngineRoot;
    }

    AZ::Outcome<void, AZStd::string> ComputeAssetAliasAndGameName(const AZStd::string& platformIdentifier, const AZStd::string& assetCatalogFile, AZStd::string& assetAlias, AZStd::string& gameName)
    {
        AZStd::string assetPath;
        AZStd::string gameFolder;
        if (!ComputeEngineRoot())
        {
            return AZ::Failure(AZStd::string("Unable to compute engine root.\n"));
        }
        if (assetCatalogFile.empty())
        {
            if (gameName.empty())
            {
                bool checkPlatform = false;
                bool result;
                AzFramework::BootstrapReaderRequestBus::BroadcastResult(result, &AzFramework::BootstrapReaderRequestBus::Events::SearchConfigurationForKey, "sys_game_folder", checkPlatform, gameFolder);

                if (!result)
                {
                    return AZ::Failure(AZStd::string("Unable to locate game name in bootstrap.\n"));
                }

                gameName = gameFolder;
            }
            else
            {
                gameFolder = gameName;
            }

            // Appending Cache/%gamename%/%platform%/%gameName% to the engine root
            bool success = AzFramework::StringFunc::Path::ConstructFull(g_cachedEngineRoot, "Cache", assetPath) &&
                AzFramework::StringFunc::Path::ConstructFull(assetPath.c_str(), gameFolder.c_str(), assetPath) &&
                AzFramework::StringFunc::Path::ConstructFull(assetPath.c_str(), platformIdentifier.c_str(), assetPath);
            if (success)
            {
                AZStd::transform(gameFolder.begin(), gameFolder.end(), gameFolder.begin(), tolower);
                success = AzFramework::StringFunc::Path::ConstructFull(assetPath.c_str(), gameFolder.c_str(), assetPath); // game name is lowercase
            }

            if (success)
            {
                assetAlias = assetPath;
            }
        }
        else if (AzFramework::StringFunc::Path::GetFullPath(assetCatalogFile.c_str(), assetPath))
        {
            AzFramework::StringFunc::Strip(assetPath, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, false, true);
            assetAlias = assetPath;

            // 3rd component from reverse should give us the correct case game name because the assetalias directory 
            // looks like ./Cache/%GameName%/%platform%/%gameName%/assetcatalog.xml
            // GetComponent util method returns the component with the separator appended at the end 
            // therefore we need to strip the separator to get the game name string
            if (!AzFramework::StringFunc::Path::GetComponent(assetPath.c_str(), gameFolder, 3, true))
            {
                return AZ::Failure(AZStd::string::format("Unable to retreive game name from assetCatalog file (%s).\n", assetCatalogFile.c_str()));
            }

            if (!AzFramework::StringFunc::Strip(gameFolder, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, false, true))
            {
                return AZ::Failure(AZStd::string::format("Unable to strip separator from game name (%s).\n", gameFolder.c_str()));
            }

            if (!gameName.empty() && !AzFramework::StringFunc::Equal(gameFolder.c_str(), gameName.c_str()))
            {
                return AZ::Failure(AZStd::string::format("Game name retrieved from the assetCatalog file (%s) does not match the inputted game name (%s).\n", gameFolder.c_str(), gameName.c_str()));
            }
            else
            {
                gameName = gameFolder;
            }
        }

        return AZ::Success();
    }

    void AddPlatformIdentifier(AZStd::string& filePath, const AZStd::string& platformIdentifier)
    {
        AZStd::string fileName;
        AzFramework::StringFunc::Path::GetFileName(filePath.c_str(), fileName);

        AZStd::string extension;
        AzFramework::StringFunc::Path::GetExtension(filePath.c_str(), extension);
        AZStd::string platformSuffix = AZStd::string::format("_%s", platformIdentifier.c_str());

        fileName = AZStd::string::format("%s%s", fileName.c_str(), platformSuffix.c_str());
        AzFramework::StringFunc::Path::ReplaceFullName(filePath, fileName.c_str(), extension.c_str());
    }

    AZStd::vector<AZStd::string> GetDefaultSeedListFiles(const char* root, const AZStd::vector<AzToolsFramework::AssetUtils::GemInfo>& gemInfoList, AzFramework::PlatformFlags platformFlag)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO, "AZ::IO::FileIOBase must be ready for use.\n");

        AZStd::vector<AZStd::string> defaultSeedLists;
        // Add all seed list files of enabled gems for the given project 
        for (const AzToolsFramework::AssetUtils::GemInfo& gemInfo : gemInfoList)
        {
            AZStd::string absoluteGemSeedFilePath;
            AzFramework::StringFunc::Path::ConstructFull(gemInfo.m_absoluteFilePath.c_str(), GemsSeedFileName, AzToolsFramework::AssetSeedManager::GetSeedFileExtension(), absoluteGemSeedFilePath, true);

            if (fileIO->Exists(absoluteGemSeedFilePath.c_str()))
            {
                defaultSeedLists.emplace_back(absoluteGemSeedFilePath);
            }

            Internal::AddPlatformsDirectorySeeds(gemInfo.m_absoluteFilePath, defaultSeedLists, platformFlag);
        }

        // Add the engine seed list file
        AZStd::string engineDirectory;
        AzFramework::StringFunc::Path::Join(root, EngineDirectoryName, engineDirectory);
        AZStd::string absoluteEngineSeedFilePath;
        AzFramework::StringFunc::Path::ConstructFull(engineDirectory.c_str(), EngineSeedFileName, AzToolsFramework::AssetSeedManager::GetSeedFileExtension(), absoluteEngineSeedFilePath, true);
        if (fileIO->Exists(absoluteEngineSeedFilePath.c_str()))
        {
            defaultSeedLists.emplace_back(absoluteEngineSeedFilePath);
        }

        Internal::AddPlatformsDirectorySeeds(engineDirectory, defaultSeedLists, platformFlag);

        // Add the current project default seed list file
        AZStd::string projectName;
        bool checkPlatform = false;
        bool result = false;
        AzFramework::BootstrapReaderRequestBus::BroadcastResult(result, &AzFramework::BootstrapReaderRequestBus::Events::SearchConfigurationForKey, "sys_game_folder", checkPlatform, projectName);
        if (result)
        {
            AZStd::string absoluteProjectDefaultSeedFilePath;
            AzFramework::StringFunc::Path::ConstructFull(root, projectName.c_str(), EngineSeedFileName, AzToolsFramework::AssetSeedManager::GetSeedFileExtension(), absoluteProjectDefaultSeedFilePath, true);
            if (fileIO->Exists(absoluteProjectDefaultSeedFilePath.c_str()))
            {
                defaultSeedLists.emplace_back(AZStd::move(absoluteProjectDefaultSeedFilePath));
            }
        }
        return defaultSeedLists;
    }

    bool IsGemSeedFilePathValid(const char* root, AZStd::string seedAbsoluteFilePath, const AZStd::vector<AzToolsFramework::AssetUtils::GemInfo>& gemInfoList, AzFramework::PlatformFlags platformFlags)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO, "AZ::IO::FileIOBase must be ready for use.\n");

        if (!fileIO->Exists(seedAbsoluteFilePath.c_str()))
        {
            return false;
        }

        AZStd::string gemsFolder;
        AzFramework::StringFunc::Path::ConstructFull(root, GemsDirectoryName, gemsFolder, true);
        AzFramework::StringFunc::Path::Normalize(seedAbsoluteFilePath);
        if (!AzFramework::StringFunc::StartsWith(seedAbsoluteFilePath, gemsFolder))
        {
            // if we are here it implies that this seed file does not live under the gems directory and
            // therefore we do not have to validate it
            return true;
        }

        for (const AzToolsFramework::AssetUtils::GemInfo& gemInfo : gemInfoList)
        {
            if (AzFramework::StringFunc::StartsWith(seedAbsoluteFilePath, gemInfo.m_absoluteFilePath))
            {
                AZStd::vector<AZStd::string> seeds;
                AZStd::string absoluteGemSeedFilePath;
                AzFramework::StringFunc::Path::ConstructFull(gemInfo.m_absoluteFilePath.c_str(), GemsSeedFileName, AzToolsFramework::AssetSeedManager::GetSeedFileExtension(), absoluteGemSeedFilePath, true);

                seeds.emplace_back(absoluteGemSeedFilePath);

                Internal::AddPlatformsDirectorySeeds(gemInfo.m_absoluteFilePath, seeds, platformFlags);

                for (AZStd::string& seedFile : seeds)
                {
                    AzFramework::StringFunc::Path::Normalize(seedFile);
                    if (AzFramework::StringFunc::Equal(seedFile.c_str(), seedAbsoluteFilePath.c_str()))
                    {
                        // this is a valid seed file for the given gem and platform flags
                        return true;
                    }
                }
            }
        }

        return false;
    }

    AzFramework::PlatformFlags GetEnabledPlatformFlags(const char* root, const char* assetRoot, const char* gameName)
    {
        QStringList configFiles = AzToolsFramework::AssetUtils::GetConfigFiles(root, assetRoot, gameName, true, true);

        QStringList enabaledPlatformList = AzToolsFramework::AssetUtils::GetEnabledPlatforms(configFiles);
        AzFramework::PlatformFlags platformFlags = AzFramework::PlatformFlags::Platform_NONE;
        for (const QString& enabledPlatform : enabaledPlatformList)
        {
            AzFramework::PlatformFlags platformFlag = AzFramework::PlatformHelper::GetPlatformFlag(enabledPlatform.toUtf8().data());

            if (platformFlag != AzFramework::PlatformFlags::Platform_NONE)
            {
                platformFlags = platformFlags | platformFlag;
            }
            else
            {
                AZ_Warning(AssetBundler::AppWindowName, false, "Platform Helper is not aware of the platform (%s).\n ", enabledPlatform.toUtf8().data());
            }
        }
        
        return platformFlags;
    }

    void ValidateOutputFilePath(FilePath filePath, const char* format, ...)
    {
        if (!filePath.IsValid())
        {
            char message[MaxErrorMessageLength] = {};
            va_list args;
            va_start(args, format);
            azvsnprintf(message, MaxErrorMessageLength, format, args);
            va_end(args);
            AZ_Error(AssetBundler::AppWindowName, false, message);
        }
    }


    WarningAbsorber::WarningAbsorber()
    {
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
    }

    WarningAbsorber::~WarningAbsorber()
    {
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
    }

    bool WarningAbsorber::OnWarning(const char* window, const char* message)
    {
        AZ_UNUSED(window);
        AZ_UNUSED(message);
        return true; // do not forward
    }

    bool WarningAbsorber::OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message)
    {
        AZ_UNUSED(window);
        AZ_UNUSED(fileName);
        AZ_UNUSED(line);
        AZ_UNUSED(func);
        AZ_UNUSED(message);
        return true; // do not forward
    }

    FilePath::FilePath(const AZStd::string& filePath, AZStd::string platformIdentifier)
    {
        AZStd::string platform = platformIdentifier;
        if (!platform.empty())
        {
            AZStd::string filePlatform = AzToolsFramework::GetPlatformIdentifier(filePath);
            if (!filePlatform.empty())
            {
                // input file path already has a platform, no need to append a platform id
                platform = AZStd::string();

                if (!AzFramework::StringFunc::Equal(filePlatform.c_str(), platformIdentifier.c_str(), true))
                {
                    // Platform identifier does not match the current platform
                    return;
                }
            }
        }

        if (!filePath.empty())
        {
            m_validPath = true;
            m_originalPath = m_absolutePath = filePath;
            AzFramework::StringFunc::Path::Normalize(m_originalPath);
            ComputeAbsolutePath(m_absolutePath, platform);
        }
    }

    const AZStd::string& FilePath::AbsolutePath() const
    {
        return m_absolutePath;
    }

    const AZStd::string& FilePath::OriginalPath() const
    {
        return m_originalPath;
    }

    bool FilePath::IsValid() const
    {
        return m_validPath;
    }

    void FilePath::ComputeAbsolutePath(AZStd::string& filePath, const AZStd::string& platformIdentifier)
    {
        if (AzToolsFramework::AssetFileInfoListComparison::IsTokenFile(filePath))
        {
            return;
        }

        if (!platformIdentifier.empty())
        {
            AssetBundler::AddPlatformIdentifier(filePath, platformIdentifier);
        }

        if (!AzFramework::StringFunc::Path::IsRelative(filePath.c_str()))
        {
            // it is already an absolute path
            AzFramework::StringFunc::Path::Normalize(filePath);
            return;
        }

        const char* appRoot = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(appRoot, &AzFramework::ApplicationRequests::GetAppRoot);
        AzFramework::StringFunc::Path::ConstructFull(appRoot, m_absolutePath.c_str(), m_absolutePath, true);
    }

    ScopedTraceHandler::ScopedTraceHandler()
    {
        BusConnect();
    }

    ScopedTraceHandler::~ScopedTraceHandler()
    {
        BusDisconnect();
    }

    bool ScopedTraceHandler::OnError(const char* window, const char* message)
    {
        if (m_reportingError)
        {
            // if we are reporting error than we dont want to store errors again.
            return false;
        }

        m_errors.emplace_back(message);
        return true;
    }

    int ScopedTraceHandler::GetErrorCount() const
    {
        return static_cast<int>(m_errors.size());
    }

    void ScopedTraceHandler::ReportErrors()
    {
        m_reportingError = true;
        for (const AZStd::string& error : m_errors)
        {
            AZ_Error(AssetBundler::AppWindowName, false, error.c_str());
        }

        ClearErrors();
        m_reportingError = false;
    }

    void ScopedTraceHandler::ClearErrors()
    {
        m_errors.clear();
        m_errors.swap(AZStd::vector<AZStd::string>());
    }

    AZ::Outcome<AzToolsFramework::AssetFileInfoListComparison::ComparisonType, AZStd::string> ParseComparisonType(const AZStd::string& comparisonStr)
    {
        for (size_t i = 0; i < AZ_ARRAY_SIZE(AzToolsFramework::AssetFileInfoListComparison::ComparisonTypeNames); ++i)
        {
            if (AzFramework::StringFunc::Equal(comparisonStr.c_str(), AzToolsFramework::AssetFileInfoListComparison::ComparisonTypeNames[i]))
            {
                return AZ::Success(static_cast<AzToolsFramework::AssetFileInfoListComparison::ComparisonType>(i));
            }
        }
        if (AzFramework::StringFunc::LooksLikeInt(comparisonStr.c_str()))
        {
            return AZ::Success(static_cast<AzToolsFramework::AssetFileInfoListComparison::ComparisonType>(AZStd::stoi(comparisonStr)));
        }
        AZStd::string failureMessage{ "Invalid comparison type.  Valid types are: " };
        const size_t numTypes = AZ_ARRAY_SIZE(AzToolsFramework::AssetFileInfoListComparison::ComparisonTypeNames);
        for (size_t i = 0; i < numTypes - 1; ++i)
        {
            failureMessage.append(AZStd::string::format("%s, ", AzToolsFramework::AssetFileInfoListComparison::ComparisonTypeNames[i]));
        }
        failureMessage.append(AZStd::string::format("and %s.", AzToolsFramework::AssetFileInfoListComparison::ComparisonTypeNames[numTypes - 1]));
        return AZ::Failure(failureMessage);
    }

}
