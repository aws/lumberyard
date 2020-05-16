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
#include <AzFramework/IO/LocalFileIO.h>
#include <source/Utils/utils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/regex.h>
#include <cctype>

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
#include <QDir>
#include <QString>
#include <QStringList>
AZ_POP_DISABLE_WARNING

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
    const char* DefaultProjectTemplatePath = "ProjectTemplates/DefaultTemplate/${ProjectName}";
    const char* ProjectName = "${ProjectName}";
    const char* DependenciesFileSuffix = "_Dependencies";
    const char* DependenciesFileExtension = "xml";

    // Asset Lists
    const char* AssetListsCommand = "assetLists";
    const char* AssetListFileArg = "assetListFile";
    const char* AddDefaultSeedListFilesFlag = "addDefaultSeedListFiles";
    const char* DryRunFlag = "dryRun";
    const char* GenerateDebugFileFlag = "generateDebugFile";
    const char* SkipArg = "skip";

    // Comparison Rules
    const char* ComparisonRulesCommand = "comparisonRules";
    const char* ComparisonRulesFileArg = "comparisonRulesFile";
    const char* ComparisonTypeArg = "comparisonType";
    const char* ComparisonFilePatternArg = "filePattern";
    const char* ComparisonFilePatternTypeArg = "filePatternType";
    const char* AddComparisonStepArg = "addComparison";
    const char* RemoveComparisonStepArg = "removeComparison";
    const char* MoveComparisonStepArg = "moveComparison";

    // Compare
    const char* CompareCommand = "compare";
    const char* CompareFirstFileArg = "firstAssetFile";
    const char* CompareSecondFileArg = "secondAssetFile";
    const char* CompareOutputFileArg = "output";
    const char* ComparePrintArg = "print";
    const char* IntersectionCountArg = "intersectionCount";

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
            AZStd::vector<AzFramework::PlatformId> platformsIdxList = AzFramework::PlatformHelper::GetPlatformIndicesInterpreted(platformFlags);
            
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
                AZStd::to_lower(gameFolder.begin(), gameFolder.end());
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

        // Add all seed list files of enabled gems for the given project
        AZStd::vector<AZStd::string> defaultSeedLists = GetGemSeedListFiles(gemInfoList, platformFlag);

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

    AZStd::string GetProjectDependenciesFile(const char* root)
    {
        AZ::Outcome<AZStd::string, AZStd::string> outcome = GetCurrentProjectName();
        if (!outcome.IsSuccess())
        {
            AZ_Error(AssetBundler::AppWindowName, false, outcome.TakeError().c_str());
            return "";
        }

        AZStd::string projectName = outcome.TakeValue();
        AZStd::string projectDependenciesFilePath = projectName + DependenciesFileSuffix;
        AzFramework::StringFunc::Path::ConstructFull(root, projectName.c_str(), projectDependenciesFilePath.c_str(), DependenciesFileExtension, projectDependenciesFilePath, true);
        return projectDependenciesFilePath;
    }

    AZStd::string GetProjectDependenciesFileTemplate(const char* root)
    {
        AZStd::string projectDependenciesFileTemplate = ProjectName;
        projectDependenciesFileTemplate += DependenciesFileSuffix;
        AzFramework::StringFunc::Path::ConstructFull(root, DefaultProjectTemplatePath, projectDependenciesFileTemplate.c_str(), DependenciesFileExtension, projectDependenciesFileTemplate, true);
        return projectDependenciesFileTemplate;
    }

    AZStd::vector<AZStd::string> GetGemSeedListFiles(const AZStd::vector<AzToolsFramework::AssetUtils::GemInfo>& gemInfoList, AzFramework::PlatformFlags platformFlags)
    {
        AZStd::vector<AZStd::string> gemSeedListFiles;
        for (const AzToolsFramework::AssetUtils::GemInfo& gemInfo : gemInfoList)
        {
            AZStd::string absoluteGemSeedFilePath;
            AzFramework::StringFunc::Path::ConstructFull(gemInfo.m_absoluteFilePath.c_str(), GemsSeedFileName, AzToolsFramework::AssetSeedManager::GetSeedFileExtension(), absoluteGemSeedFilePath, true);

            if (AZ::IO::FileIOBase::GetInstance()->Exists(absoluteGemSeedFilePath.c_str()))
            {
                gemSeedListFiles.emplace_back(absoluteGemSeedFilePath);
            }

            Internal::AddPlatformsDirectorySeeds(gemInfo.m_absoluteFilePath, gemSeedListFiles, platformFlags);
        }

        return gemSeedListFiles;
    }

    AZStd::unordered_map<AZStd::string, AZStd::string> GetGemSeedListFilePathToGemNameMap(const AZStd::vector<AzToolsFramework::AssetUtils::GemInfo>& gemInfoList, AzFramework::PlatformFlags platformFlags)
    {
        AZStd::unordered_map<AZStd::string, AZStd::string> filePathToGemNameMap;
        for (const AzToolsFramework::AssetUtils::GemInfo& gemInfo : gemInfoList)
        {
            AZStd::string absoluteGemSeedFilePath;
            AzFramework::StringFunc::Path::ConstructFull(gemInfo.m_absoluteFilePath.c_str(), GemsSeedFileName, AzToolsFramework::AssetSeedManager::GetSeedFileExtension(), absoluteGemSeedFilePath, true);

            AZStd::string gemName = gemInfo.m_gemName + " Gem";
            if (AZ::IO::FileIOBase::GetInstance()->Exists(absoluteGemSeedFilePath.c_str()))
            {
                filePathToGemNameMap[absoluteGemSeedFilePath] = gemName;
            }

            AZStd::vector<AZStd::string> platformSpecificSeedListFiles;
            Internal::AddPlatformsDirectorySeeds(gemInfo.m_absoluteFilePath, platformSpecificSeedListFiles, platformFlags);
            for (const AZStd::string& platformSpecificSeedListFile : platformSpecificSeedListFiles)
            {
                filePathToGemNameMap[platformSpecificSeedListFile] = gemName;
            }
        }

        return filePathToGemNameMap;
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

    AZ::Outcome<AZStd::string, AZStd::string> GetCurrentProjectName()
    {
        AZStd::string gameName;
        bool result;
        AzFramework::BootstrapReaderRequestBus::BroadcastResult(result, &AzFramework::BootstrapReaderRequestBus::Events::SearchConfigurationForKey, "sys_game_folder", false, gameName);

        if (result)
        {
            return AZ::Success(gameName);
        }
        else
        {
            return AZ::Failure(AZStd::string("Unable to locate current project name in bootstrap.cfg"));
        }
    }

    AZ::Outcome<AZStd::string, AZStd::string> GetProjectFolderPath(const AZStd::string& engineRoot, const AZStd::string& projectName)
    {
        AZStd::string projectFolderPath;
        bool success = AzFramework::StringFunc::Path::ConstructFull(engineRoot.c_str(), projectName.c_str(), projectFolderPath);

        if (success && AZ::IO::FileIOBase::GetInstance()->Exists(projectFolderPath.c_str()))
        {
            return AZ::Success(projectFolderPath);
        }
        else
        {
            return AZ::Failure(AZStd::string::format( "Unable to locate the current Project folder: %s", projectName.c_str()));
        }
    }

    AZ::Outcome<AZStd::string, AZStd::string> GetProjectCacheFolderPath(const AZStd::string& engineRoot, const AZStd::string& projectName)
    {
        AZStd::string projectCacheFolderPath;

        bool success = AzFramework::StringFunc::Path::ConstructFull(engineRoot.c_str(), "Cache", projectCacheFolderPath);
        if (!success || !AZ::IO::FileIOBase::GetInstance()->Exists(projectCacheFolderPath.c_str()))
        {
            return AZ::Failure(AZStd::string::format(
                "Unable to locate the Cache in the engine directory: %s. Please run the Asset Processor to generate a Cache and build assets.", 
                engineRoot.c_str()));
        }

        success = AzFramework::StringFunc::Path::ConstructFull(projectCacheFolderPath.c_str(), projectName.c_str(), projectCacheFolderPath);
        if (success && AZ::IO::FileIOBase::GetInstance()->Exists(projectCacheFolderPath.c_str()))
        {
            return AZ::Success(projectCacheFolderPath);
        }
        else
        {
            return AZ::Failure(AZStd::string::format(
                "Unable to locate the current Project in the Cache folder: %s. Please run the Asset Processor to generate a Cache and build assets.",
                projectName.c_str()));
        }
    }

    AZ::Outcome<void, AZStd::string> GetPlatformNamesFromCacheFolder(const AZStd::string& projectCacheFolder, AZStd::vector<AZStd::string>& platformNames)
    {
        QDir projectCacheDir(QString(projectCacheFolder.c_str()));
        auto tempPlatformList = projectCacheDir.entryList(QDir::Filter::Dirs | QDir::Filter::NoDotAndDotDot);

        if (tempPlatformList.empty())
        {
            return AZ::Failure(AZStd::string("Cache is empty. Please run the Asset Processor to generate a Cache and build assets."));
        }

        for (const QString& platform : tempPlatformList)
        {
            platformNames.push_back(AZStd::string(platform.toUtf8().data()));
        }

        return AZ::Success();
    }

    AZ::Outcome<AZStd::string, AZStd::string> GetAssetCatalogFilePath(const char* pathToCacheFolder, const char* platformIdentifier, const char* projectName)
    {
        AZStd::string assetCatalogFilePath;
        bool success = AzFramework::StringFunc::Path::ConstructFull(pathToCacheFolder, platformIdentifier, assetCatalogFilePath, true);

        if (!success)
        {
            return AZ::Failure(AZStd::string::format(
                "Unable to find platform folder %s in cache found at: %s. Please run the Asset Processor to generate platform-specific cache folders and build assets.", 
                platformIdentifier, 
                pathToCacheFolder));
        }

        // Project name is lower case in the platform-specific cache folder
        AZStd::string lowerCaseProjectName = AZStd::string(projectName);
        AZStd::to_lower(lowerCaseProjectName.begin(), lowerCaseProjectName.end());
        success = AzFramework::StringFunc::Path::ConstructFull(assetCatalogFilePath.c_str(), lowerCaseProjectName.c_str(), assetCatalogFilePath)
            && AzFramework::StringFunc::Path::ConstructFull(assetCatalogFilePath.c_str(), AssetCatalogFilename, assetCatalogFilePath);

        if (!success)
        {
            return AZ::Failure(AZStd::string("Unable to find the asset catalog. Please run the Asset Processor to generate a Cache and build assets."));
        }

        return AZ::Success(assetCatalogFilePath);
    }

    AZStd::string GetPlatformSpecificCacheFolderPath(const AZStd::string& projectSpecificCacheFolderAbsolutePath, const AZStd::string& platform, const AZStd::string& projectName)
    {
        // C:/dev/Cache/ProjectName -> C:/dev/Cache/ProjectName/platform
        AZStd::string platformSpecificCacheFolderPath;
        AzFramework::StringFunc::Path::ConstructFull(projectSpecificCacheFolderAbsolutePath.c_str(), platform.c_str(), platformSpecificCacheFolderPath, true);

        // C:/dev/Cache/ProjectName/platform -> C:/dev/Cache/ProjectName/platform/projectname
        AZStd::string lowerCaseProjectName = AZStd::string(projectName);
        AZStd::to_lower(lowerCaseProjectName.begin(), lowerCaseProjectName.end());
        AzFramework::StringFunc::Path::ConstructFull(platformSpecificCacheFolderPath.c_str(), lowerCaseProjectName.c_str(), platformSpecificCacheFolderPath, true);

        return platformSpecificCacheFolderPath;
    }

    AZStd::string GenerateKeyFromAbsolutePath(const AZStd::string& absoluteFilePath)
    {
        AZStd::string key(absoluteFilePath);
        AzFramework::StringFunc::Path::Normalize(key);
        AzFramework::StringFunc::Path::StripDrive(key);
        return key;
    }

    void ConvertToRelativePath(const AZStd::string& parentFolderPath, AZStd::string& absoluteFilePath)
    {
        // Qt and AZ return different Drive Letter formats, so strip them away before doing a comparison
        AZStd::string parentFolderPathWithoutDrive(parentFolderPath);
        AzFramework::StringFunc::Path::StripDrive(parentFolderPathWithoutDrive);
        AzFramework::StringFunc::Path::Normalize(parentFolderPathWithoutDrive);

        AzFramework::StringFunc::Path::StripDrive(absoluteFilePath);
        AzFramework::StringFunc::Path::Normalize(absoluteFilePath);

        AzFramework::StringFunc::Replace(absoluteFilePath, parentFolderPathWithoutDrive.c_str(), "");
    }

    AZ::Outcome<void, AZStd::string> MakePath(const AZStd::string& path)
    {
        // Create the folder if it does not already exist
        if (!AZ::IO::FileIOBase::GetInstance()->Exists(path.c_str()))
        {
            auto result = AZ::IO::FileIOBase::GetInstance()->CreatePath(path.c_str());
            if (!result)
            {
                return AZ::Failure(AZStd::string::format("Path creation failed. Input path: %s \n", path.c_str()));
            }
        }

        return AZ::Success();
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
        AZ_UNUSED(window);
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

    AZ::Outcome<AzToolsFramework::AssetFileInfoListComparison::ComparisonType, AZStd::string> ParseComparisonType(const AZStd::string& comparisonType)
    {
        using namespace AzToolsFramework;

        const size_t numTypes = AZ_ARRAY_SIZE(AssetFileInfoListComparison::ComparisonTypeNames);

        int comparisonTypeIndex = 0;
        if (AzFramework::StringFunc::LooksLikeInt(comparisonType.c_str(), &comparisonTypeIndex))
        {
            // User passed in a number
            if (comparisonTypeIndex < numTypes)
            {
                return AZ::Success(static_cast<AssetFileInfoListComparison::ComparisonType>(comparisonTypeIndex));
            }
        }
        else
        {
            // User passed in the name of a ComparisonType
            for (size_t i = 0; i < numTypes; ++i)
            {
                if (AzFramework::StringFunc::Equal(comparisonType.c_str(), AssetFileInfoListComparison::ComparisonTypeNames[i]))
                {
                    return AZ::Success(static_cast<AssetFileInfoListComparison::ComparisonType>(i));
                }
            }
        }

        // Failure case
        AZStd::string failureMessage = AZStd::string::format("Invalid Comparison Type ( %s ).  Valid types are: ", comparisonType.c_str());
        for (size_t i = 0; i < numTypes - 1; ++i)
        {
            failureMessage.append(AZStd::string::format("%s, ", AssetFileInfoListComparison::ComparisonTypeNames[i]));
        }
        failureMessage.append(AZStd::string::format("and %s.", AssetFileInfoListComparison::ComparisonTypeNames[numTypes - 1]));
        return AZ::Failure(failureMessage);
    }

    AZ::Outcome<AzToolsFramework::AssetFileInfoListComparison::FilePatternType, AZStd::string> ParseFilePatternType(const AZStd::string& filePatternType)
    {
        using namespace AzToolsFramework;

        const size_t numTypes = AZ_ARRAY_SIZE(AssetFileInfoListComparison::FilePatternTypeNames);

        int filePatternTypeIndex = 0;
        if (AzFramework::StringFunc::LooksLikeInt(filePatternType.c_str(), &filePatternTypeIndex))
        {
            // User passed in a number
            if (filePatternTypeIndex < numTypes)
            {
                return AZ::Success(static_cast<AssetFileInfoListComparison::FilePatternType>(filePatternTypeIndex));
            }
        }
        else
        {
            // User passed in the name of a FilePatternType
            for (size_t i = 0; i < numTypes; ++i)
            {
                if (AzFramework::StringFunc::Equal(filePatternType.c_str(), AssetFileInfoListComparison::FilePatternTypeNames[i]))
                {
                    return AZ::Success(static_cast<AssetFileInfoListComparison::FilePatternType>(i));
                }
            }
        }

        // Failure case
        AZStd::string failureMessage = AZStd::string::format("Invalid File Pattern Type ( %s ).  Valid types are: ", filePatternType.c_str());
        for (size_t i = 0; i < numTypes - 1; ++i)
        {
            failureMessage.append(AZStd::string::format("%s, ", AssetFileInfoListComparison::FilePatternTypeNames[i]));
        }
        failureMessage.append(AZStd::string::format("and %s.", AssetFileInfoListComparison::FilePatternTypeNames[numTypes - 1]));
        return AZ::Failure(failureMessage);
    }

}
