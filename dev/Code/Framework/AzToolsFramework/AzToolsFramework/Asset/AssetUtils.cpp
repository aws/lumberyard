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

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Module/ModuleManagerBus.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Asset/AssetUtils.h>
#include <GemRegistry/IGemRegistry.h>
AZ_PUSH_DISABLE_WARNING(4127 4251 4800, "-Wunknown-warning-option")
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QDirIterator>
AZ_POP_DISABLE_WARNING


namespace AzToolsFramework
{
    namespace AssetUtils
    {
        namespace Internal
        {

            const char AssetConfigPlatfomrDir[] = "AssetProcessorConfig";

            QStringList FindWildcardMatches(const QString& sourceFolder, QString relativeName)
            {
                if (relativeName.isEmpty())
                {
                    return QStringList();
                }

                const int pathLen = sourceFolder.length() + 1;

                relativeName.replace('\\', '/');

                QStringList returnList;
                QRegExp nameMatch{ relativeName, Qt::CaseInsensitive, QRegExp::Wildcard };
                QDirIterator diretoryIterator(sourceFolder, QDir::AllEntries | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
                QStringList files;
                while (diretoryIterator.hasNext())
                {
                    diretoryIterator.next();
                    if (!diretoryIterator.fileInfo().isFile())
                    {
                        continue;
                    }
                    QString pathMatch{ diretoryIterator.filePath().mid(pathLen) };
                    if (nameMatch.exactMatch(pathMatch))
                    {
                        returnList.append(diretoryIterator.filePath());
                    }
                }
                return returnList;
            }

            void ReadPlatformInfosFromConfigFile(QString configFile, QStringList& enabledPlatforms)
            {
                // in the inifile the platform can be missing (commented out)
                // in which case it is disabled implicitly by not being there
                // or it can be 'disabled' which means that it is explicitly disabled.
                // or it can be 'enabled' which means that it is explicitly enabled.

                if (!QFile::exists(configFile))
                {
                    return;
                }

                QSettings loader(configFile, QSettings::IniFormat);

                // Read in enabled platforms
                loader.beginGroup("Platforms");
                QStringList keys = loader.allKeys();
                for (int idx = 0; idx < keys.count(); idx++)
                {
                    QString val = loader.value(keys[idx]).toString();
                    QString platform = keys[idx].toLower().trimmed();

                    val = val.toLower().trimmed();

                    if (val == "enabled")
                    {
                        if (!enabledPlatforms.contains(val))
                        {
                            enabledPlatforms.push_back(platform);
                        }
                    }
                    else if (val == "disabled")
                    {
                        // disable platform explicitly.
                        int index = enabledPlatforms.indexOf(platform);
                        if (index != -1)
                        {
                            enabledPlatforms.removeAt(index);
                        }
                    }
                }
                loader.endGroup();
            }

            void AddGemConfigFiles(const AZStd::vector<AzToolsFramework::AssetUtils::GemInfo>& gemInfoList, QStringList& configFiles)
            {
                // there can only be one gam gem per project, so if we find one, we cache the name of it so that
                // later we can add it to the very end of the list, giving it the ability to override all other config files.
                QString gameConfigPath;

                for (const GemInfo& gemElement : gemInfoList)
                {
                    QString gemAbsolutePath(gemElement.m_absoluteFilePath.c_str());

                    QDir gemDir(gemAbsolutePath);
                    QString absPathToConfigFile = gemDir.absoluteFilePath("AssetProcessorGemConfig.ini");
                    if (gemElement.m_isGameGem)
                    {
                        gameConfigPath = absPathToConfigFile;
                    }
                    else
                    {
                        configFiles.push_back(absPathToConfigFile);
                    }
                }

                // if a 'game gem' was discovered during the above loop, we want to append it to the END of the list
                if (!gameConfigPath.isEmpty())
                {
                    configFiles.push_back(gameConfigPath);
                }
            }

            bool AddPlatformConfigFilePaths(const char* root, QStringList& configFilePaths)
            {
                QString configWildcardName{ "*" };
                configWildcardName.append(AssetProcessorPlatformConfigFileName);
                QDir sourceRoot(root);
                QStringList platformList = FindWildcardMatches(sourceRoot.filePath(AssetConfigPlatfomrDir), configWildcardName);
                for (const auto& thisConfig : platformList)
                {
                    configFilePaths.append(thisConfig);
                }
                return (platformList.size() > 0);
            }
        }

        const char* AssetProcessorPlatformConfigFileName = "AssetProcessorPlatformConfig.ini";
        const char* AssetProcessorGamePlatformConfigFileName = "AssetProcessorGamePlatformConfig.ini";
        const char GemsDirectoryName[] = "Gems";

        GemInfo::GemInfo(AZStd::string name, AZStd::string relativeFilePath, AZStd::string absoluteFilePath, AZStd::string identifier, bool isGameGem, bool assetOnlyGem)
            : m_gemName(name)
            , m_relativeFilePath(relativeFilePath)
            , m_absoluteFilePath(absoluteFilePath)
            , m_identifier(identifier)
            , m_isGameGem(isGameGem)
            , m_assetOnly(assetOnlyGem)
        {
        }

        QStringList GetEnabledPlatforms(QStringList configFiles)
        {
            QStringList enabledPlatforms;

            // note that the current host platform is enabled by default.
            enabledPlatforms.push_back(AzToolsFramework::AssetSystem::GetHostAssetPlatform());

            for (const QString& configFile : configFiles)
            {
                Internal::ReadPlatformInfosFromConfigFile(configFile, enabledPlatforms);
            }
            return enabledPlatforms;
        }

        QStringList GetConfigFiles(const char* root, const char* assetRoot, const char* gameName, bool addPlatformConfigs, bool addGemsConfigs)
        {
            QStringList configFiles;
            QDir configRoot(root);

            QString rootConfigFile = configRoot.filePath(AssetProcessorPlatformConfigFileName);

            configFiles.push_back(rootConfigFile);
            if (addPlatformConfigs)
            {
                Internal::AddPlatformConfigFilePaths(root, configFiles);
            }

            if (addGemsConfigs)
            {
                AZStd::vector<AzToolsFramework::AssetUtils::GemInfo> gemInfoList;
                if (!GetGemsInfo(root, assetRoot, gameName, gemInfoList))
                {
                    AZ_Error("AzToolsFramework::AssetUtils", false, "Failed to read gems for game project (%s).\n", gameName);
                    return {};
                }

                Internal::AddGemConfigFiles(gemInfoList, configFiles);
            }

            QDir assetRootDir(assetRoot);
            assetRootDir.cd(gameName);

            QString projectConfigFile = assetRootDir.filePath(AssetProcessorGamePlatformConfigFileName);

            configFiles.push_back(projectConfigFile);

            return configFiles;
        }

        bool GetGemsInfo(const char* root, const char* assetRoot, const char* gameName, AZStd::vector<GemInfo>& gemInfoList)
        {
            Gems::IGemRegistry* registry = nullptr;
            Gems::IProjectSettings* projectSettings = nullptr;
            AZ::ModuleManagerRequests::LoadModuleOutcome result = AZ::Failure(AZStd::string("Failed to connect to ModuleManagerRequestBus.\n"));
            AZ::ModuleManagerRequestBus::BroadcastResult(result, &AZ::ModuleManagerRequestBus::Events::LoadDynamicModule, "GemRegistry", AZ::ModuleInitializationSteps::Load, false);
            if (!result.IsSuccess())
            {
                AZ_Error("AzToolsFramework::AssetUtils", false, "Could not load the GemRegistry module - %s.\n", result.GetError().c_str());
                return false;
            }

            // Use shared_ptr aliasing ctor to use the refcount/deleter from the moduledata pointer, but we only need to store the dynamic module handle.
            auto registryModule = AZStd::shared_ptr<AZ::DynamicModuleHandle>(result.GetValue(), result.GetValue()->GetDynamicModuleHandle());
            auto CreateGemRegistry = registryModule->GetFunction<Gems::RegistryCreatorFunction>(GEMS_REGISTRY_CREATOR_FUNCTION_NAME);
            Gems::RegistryDestroyerFunction registryDestroyerFunc = registryModule->GetFunction<Gems::RegistryDestroyerFunction>(GEMS_REGISTRY_DESTROYER_FUNCTION_NAME);
            if (!CreateGemRegistry || !registryDestroyerFunc)
            {
                AZ_Error("AzToolsFramework::AssetUtils", false, "Failed to load GemRegistry functions.\n");
                return false;
            }

            registry = CreateGemRegistry();
            if (!registry)
            {
                AZ_Error("AzToolsFramework::AssetUtils", false, "Failed to create Gems::GemRegistry.\n");
                return false;
            }

            registry->AddSearchPath({ root, GemsDirectoryName }, false);


            registry->AddSearchPath({ assetRoot, GemsDirectoryName }, false);

            const char* engineRootPath = nullptr;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(engineRootPath, &AzToolsFramework::ToolsApplicationRequestBus::Events::GetEngineRootPath);

            if (engineRootPath && azstricmp(engineRootPath, root))
            {
                registry->AddSearchPath({ engineRootPath, GemsDirectoryName }, false);
            }
            projectSettings = registry->CreateProjectSettings();
            if (!projectSettings)
            {
                registryDestroyerFunc(registry);
                AZ_Error("AzToolsFramework::AssetUtils", false, "Failed to create Gems::ProjectSettings.\n");
                return false;
            }

            if (!projectSettings->Initialize(assetRoot, gameName))
            {
                registry->DestroyProjectSettings(projectSettings);
                registryDestroyerFunc(registry);

                AZ_Error("AzToolsFramework::AssetUtils", false, "Failed to initialize Gems::ProjectSettings.\n");
                return false;
            }

            auto loadProjectOutcome = registry->LoadProject(*projectSettings, true);
            if (!loadProjectOutcome.IsSuccess())
            {
                registry->DestroyProjectSettings(projectSettings);
                registryDestroyerFunc(registry);

                AZ_Error("AzToolsFramework::AssetUtils", false, "Failed to load Gems project: %s.\n", loadProjectOutcome.GetError().c_str());
                return false;
            }

            // Populating the gem info list
            for (const auto& pair : projectSettings->GetGems())
            {
                Gems::IGemDescriptionConstPtr desc = registry->GetGemDescription(pair.second);

                if (!desc)
                {
                    Gems::ProjectGemSpecifier gemSpecifier = pair.second;
                    AZStd::string errorStr = AZStd::string::format("Failed to load Gem with ID %s and Version %s (from path %s).\n",
                        gemSpecifier.m_id.ToString<AZStd::string>().c_str(), gemSpecifier.m_version.ToString().c_str(), gemSpecifier.m_path.c_str());

                    if (Gems::IGemDescriptionConstPtr latestVersion = registry->GetLatestGem(pair.first))
                    {
                        errorStr += AZStd::string::format(" Found version %s, you may want to use that instead.\n", latestVersion->GetVersion().ToString().c_str());
                    }

                    AZ_Error("AzToolsFramework::AssetUtils", false, errorStr.c_str());
                    return false;
                }

                // Note: the two 'false' parameters in the ToString call below ToString(false, false)
                // eliminates brackets and dashes in the formatting of the UUID.
                // this keeps it compatible with legacy formatting which also omitted the curly braces and the dashes in the UUID.
                AZStd::string gemId = desc->GetID().ToString<AZStd::string>(false, false).c_str();
                AZStd::to_lower(gemId.begin(), gemId.end());

                bool assetOnlyGem = true;

                Gems::ModuleDefinitionVector moduleList = desc->GetModules();

                for (Gems::ModuleDefinitionConstPtr moduleDef : moduleList)
                {
                    if (moduleDef->m_linkType != Gems::LinkType::NoCode)
                    {
                        assetOnlyGem = false;
                        break;
                    }
                }

                gemInfoList.emplace_back(GemInfo(desc->GetName(), desc->GetPath(), desc->GetAbsolutePath(), gemId, desc->IsGameGem(), assetOnlyGem));
            }

            registry->DestroyProjectSettings(projectSettings);
            registryDestroyerFunc(registry);

            return true;
        }

        bool UpdateFilePathToCorrectCase(const QString& root, QString& relativePathFromRoot)
        {
            AZStd::string rootPath(root.toUtf8().data());
            AZStd::string relPathFromRoot(relativePathFromRoot.toUtf8().data());
            AZ::StringFunc::Path::Normalize(relPathFromRoot);
            AZStd::vector<AZStd::string> tokens;
            AZ::StringFunc::Tokenize(relPathFromRoot.c_str(), tokens, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);

            AZStd::string validatedPath;
            if (rootPath.empty())
            {
                const char* appRoot = nullptr;
                AzFramework::ApplicationRequests::Bus::BroadcastResult(appRoot, &AzFramework::ApplicationRequests::GetAppRoot);
                validatedPath = AZStd::string(appRoot);
            }
            else
            {
                validatedPath = rootPath;
            }

            bool success = true;

            for (int idx = 0; idx < tokens.size(); idx++)
            {
                AZStd::string element = tokens[idx];
                bool foundAMatch = false;
                AZ::IO::FileIOBase::GetInstance()->FindFiles(validatedPath.c_str(), "", [&](const char* file)
                    {
                        if ( idx != tokens.size() - 1 && !AZ::IO::FileIOBase::GetInstance()->IsDirectory(file))
                        {
                            // only the last token is supposed to be a filename, we can skip filenames before that
                            return true;
                        }

                        AZStd::string absFilePath(file);
                        AZ::StringFunc::Path::Normalize(absFilePath);
                        auto found = absFilePath.rfind(AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
                        size_t startingPos = found + 1;
                        if (found != AZStd::string::npos && absFilePath.size() > startingPos)
                        {
                            AZStd::string componentName = AZStd::string(absFilePath.begin() + startingPos, absFilePath.end());
                            if (AZ::StringFunc::Equal(componentName.c_str(), tokens[idx].c_str()))
                            {
                                tokens[idx] = componentName;
                                foundAMatch = true;
                                return false;
                            }
                        }

                        return true;
                    });

                if (!foundAMatch)
                {
                    success = false;
                    break;
                }

                AZStd::string absoluteFilePath;
                AZ::StringFunc::Path::ConstructFull(validatedPath.c_str(), element.c_str(), absoluteFilePath);

                validatedPath = absoluteFilePath; // go one step deeper.
            }

            if (success)
            {
                relPathFromRoot.clear();
                AZ::StringFunc::Join(relPathFromRoot, tokens.begin(), tokens.end(), AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
                relativePathFromRoot = relPathFromRoot.c_str();
            }

            return success;
        }
    } //namespace AssetUtils
} //namespace AzToolsFramework
