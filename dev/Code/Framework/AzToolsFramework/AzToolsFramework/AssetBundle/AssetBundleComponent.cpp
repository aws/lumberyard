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

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzFramework/Asset/AssetBundleManifest.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/AssetBundle/AssetBundleComponent.h>
#include <AzToolsFramework/Archive/ArchiveAPI.h>
#include <AzToolsFramework/Asset/AssetSeedManager.h>
#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalog.h>
#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalogBus.h>


namespace AzToolsFramework 
{
    const char* logWindowName = "AssetBundle";
    const int NumOfBytesInMB = 1024 * 1024;
    const int ManifestFileSizeBufferInBytes = 10 * 1024; // 10 KB
    const float AssetCatalogFileSizeBufferPercentage = 1.0f;
    using ArchiveCommandsBus = AzToolsFramework::ArchiveCommands::Bus;
    using AssetCatalogRequestBus = AZ::Data::AssetCatalogRequestBus;

    const char AssetBundleComponent::DeltaCatalogName[] = "DeltaCatalog.xml";

    bool MaxSizeExceeded(AZ::u64 totalFileSize, AZ::u64 bundleSize, AZ::u64 assetCatalogFileSizeBuffer, AZ::u64 maxSizeInBytes)
    {
        return ((totalFileSize + bundleSize + assetCatalogFileSizeBuffer + ManifestFileSizeBufferInBytes) > maxSizeInBytes);
    }

    bool MakePath(const AZStd::string& directory)
    {
        if (!AZ::IO::FileIOBase::GetInstance()->Exists(directory.c_str()))
        {
            auto result = AZ::IO::FileIOBase::GetInstance()->CreatePath(directory.c_str());
            if (!result)
            {
                AZ_Error(logWindowName, false, "Path creation failed. Input path: %s \n", directory.c_str());
                return false;
            }
        }
        return true;
    }

    bool MakeTempFolderFromFilePath(AZStd::string filePath, AZStd::string& tempFolderPathOutput)
    {
        AzFramework::StringFunc::Path::StripExtension(filePath);
        filePath += "_temp";

        if (!MakePath(filePath))
        {
            return false;
        }

        tempFolderPathOutput = filePath;
        return true;
    }
    
    void AssetBundleComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AssetBundleComponent, AZ::Component>();
        }
    }

    void AssetBundleComponent::Activate()
    {
        AssetBundleCommands::Bus::Handler::BusConnect();
    }

    void AssetBundleComponent::Deactivate()
    {
        AssetBundleCommands::Bus::Handler::BusDisconnect();
    }

    bool AssetBundleComponent::CreateDeltaCatalog(const AZStd::string& sourcePak, bool regenerate)
    {
        AZStd::string normalizedSourcePakPath = sourcePak;
        AzFramework::StringFunc::Path::Normalize(normalizedSourcePakPath);

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
        {
            AZ_Error("AssetBundle", false, "No FileIOBase Instance");
            return false;
        }
        if (!fileIO->Exists(normalizedSourcePakPath.c_str()))
        {
            AZ_Error("AssetBundle", false, "Invalid Arg: Source Pak does not exist at \"%s\".", normalizedSourcePakPath.c_str());
            return false;
        }

        AZStd::string outCatalogPath = DeltaCatalogName;

        AZ_TracePrintf(logWindowName, "Gathering file entries in source pak file \"%s\".\n", sourcePak.c_str());
        bool result = false;
        AZStd::vector<AZStd::string> fileEntries;
        ArchiveCommandsBus::BroadcastResult(result, &AzToolsFramework::ArchiveCommands::ListFilesInArchiveBlocking, normalizedSourcePakPath, fileEntries);
        // This ebus currently always returns false as the result, as it is believed that the 7z process is 
        //  being terminated by the user instead of ending gracefully. Check against an empty fileList instead
        //  as a result.
        if (fileEntries.empty())
        {
            AZ_Error(logWindowName, false, "Failed to read archive \"%s\" for file entries.", sourcePak.c_str());
            return false;
        }

        // if this bundle already contains a manifest and should not regenerate it, then this bundle is good to go.
        bool manifestExists = HasManifest(fileEntries);
        if (manifestExists && !regenerate)
        {
            AZ_TracePrintf(logWindowName, "Skipping delta asset catalog creation for \"%s\", as it already contains a delta catalog.\n", normalizedSourcePakPath.c_str());
            return true;
        }

        // Load the manifest if it exists, and set the output path to the catalog in the manifest
        AzFramework::AssetBundleManifest* manifest = nullptr;
        if (manifestExists)
        {
            manifest = GetManifestFromBundle(normalizedSourcePakPath);
            if (!manifest)
            {
                return false;
            }
            outCatalogPath = manifest->GetCatalogName();
        }

        // remove non-asset entries from this file list (including the source pak itself)
        AZ_TracePrintf(logWindowName, "Removing known non-assets from the gathered list of file entries.\n");        
        if (!RemoveNonAssetFileEntries(fileEntries, normalizedSourcePakPath, manifest))
        {
            return false;
        }

        // create the new asset registry containing these files and save the catalog to a file here.
        AZ_TracePrintf(logWindowName, "Creating new asset catalog file \"%s\" containing information for assets in source pak.\n", outCatalogPath.c_str());
        bool catalogSaved = false;
        AssetCatalogRequestBus::BroadcastResult(catalogSaved, &AssetCatalogRequestBus::Events::CreateDeltaCatalog, fileEntries, outCatalogPath);
        if (!catalogSaved)
        {
            AZ_Error(logWindowName, false, "Failed to create new asset catalog \"%s\".", outCatalogPath.c_str());
            return false;
        }

        if (!InjectFile(outCatalogPath, normalizedSourcePakPath))
        {
            return false;
        }        

        // clean up the file that was created.
        if (!fileIO->Remove(outCatalogPath.c_str()))
        {
            AZ_Warning(logWindowName, false, "Failed to clean up catalog artifact at %s.", outCatalogPath.c_str());
        }

        // if the manifest already exists, we don't need to modify it, so this bundle is set.
        if (manifest)
        {
            return true;
        }

        // create the new bundle manifest and save it to a file here.
        AZ_TracePrintf(logWindowName, "Creating new asset bundle manifest file \"%s\" for source pak \"%s\".\n", AzFramework::AssetBundleManifest::s_manifestFileName, sourcePak.c_str());
        bool manifestSaved = false;
        AZStd::string manifestDirectory;
        AzFramework::StringFunc::Path::GetFullPath(sourcePak.c_str(), manifestDirectory);
        AssetCatalogRequestBus::BroadcastResult(manifestSaved, &AssetCatalogRequestBus::Events::CreateBundleManifest, outCatalogPath, AZStd::vector<AZStd::string>(), manifestDirectory, AzFramework::AssetBundleManifest::CurrentBundleVersion);

        AZStd::string manifestPath;
        AzFramework::StringFunc::Path::Join(manifestDirectory.c_str(), AzFramework::AssetBundleManifest::s_manifestFileName, manifestPath);
        if (!manifestSaved)
        {
            AZ_Error(logWindowName, false, "Failed to create new manifest file \"%s\" for source pak \"%s\".\n", manifestPath.c_str(), sourcePak.c_str());
            return false;
        }

        if (!InjectFile(manifestPath, sourcePak))
        {
            return false;
        }

        // clean up the file that was created.
        if (!fileIO->Remove(manifestPath.c_str()))
        {
            AZ_Warning(logWindowName, false, "Failed to clean up manifest artifact.");
        }

        return true;
    }

    AZStd::string AssetBundleComponent::CreateAssetBundleFileName(const AZStd::string& assetBundleFilePath, int bundleIndex)
    {
        AZStd::string fileName;
        AZStd::string extension;
        AzFramework::StringFunc::AssetDatabasePath::Split(assetBundleFilePath.c_str(), nullptr, nullptr, nullptr, &fileName, &extension);
        if (!bundleIndex)
        {
            return fileName;
        }
        return AZStd::string::format("%s__%d%s", fileName.c_str(), bundleIndex, extension.c_str());
    }

    bool AssetBundleComponent::CreateAssetBundleFromList(const AssetBundleSettings& assetBundleSettings, const AssetFileInfoList& assetFileInfoList)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO != nullptr, "AZ::IO::FileIOBase must be ready for use.\n");

        AZStd::string bundleFilePath = assetBundleSettings.m_bundleFilePath;

        AzFramework::PlatformId platformId = static_cast<AzFramework::PlatformId>(AzFramework::PlatformHelper::GetPlatformIndexFromName(assetBundleSettings.m_platform.c_str()));

        if (platformId == AzFramework::PlatformId::Invalid)
        {
            return false;
        }

        const char* appRoot = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(appRoot, &AzFramework::ApplicationRequests::GetAppRoot);

        if (AzFramework::StringFunc::Path::IsRelative(bundleFilePath.c_str()))
        {
            AzFramework::StringFunc::Path::ConstructFull(appRoot, bundleFilePath.c_str(), bundleFilePath, true);
        }

        AZ::u64 maxSizeInBytes = static_cast<AZ::u64>(assetBundleSettings.m_maxBundleSizeInMB * NumOfBytesInMB);
        AZ::u64 assetCatalogFileSizeBuffer = static_cast<AZ::u64>(AssetCatalogFileSizeBufferPercentage * assetBundleSettings.m_maxBundleSizeInMB * NumOfBytesInMB) / 100;
        AZ::u64 bundleSize = 0;
        AZ::u64 totalFileSize = 0;
        int bundleIndex = 0;

        AZStd::string bundleFullPath = bundleFilePath;

        AZStd::vector<AZStd::string> dependentBundleNames;
        AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>> bundlePathDeltaCatalogPair;
        bundlePathDeltaCatalogPair.emplace_back(AZStd::make_pair(bundleFullPath, DeltaCatalogName));

        AZStd::vector<AZStd::string> fileEntries; // this is used to add files to the archive
        AZStd::vector<AZStd::string> deltaCatalogEntries; // this is used to create the delta catalog

        AZStd::string bundleFolder;
        AzFramework::StringFunc::Path::GetFullPath(bundleFilePath.c_str(), bundleFolder);

        // Create the archive directory if it does not exist
        if (!MakePath(bundleFolder))
        {
            return  false;
        }

        AZStd::string deltaCatalogFilePath;
        AzFramework::StringFunc::Path::ConstructFull(bundleFolder.c_str(), bundlePathDeltaCatalogPair.back().second.c_str(), deltaCatalogFilePath, true);

        AZStd::string assetAlias = PlatformAddressedAssetCatalog::GetAssetRootForPlatform(platformId);

        if (fileIO->Exists(bundleFilePath.c_str()))
        {
            // This will delete both the parent bundle as well as all the dependent bundles mentioned in the manifest file of the parent bundle.
            if (!DeleteBundleFiles(bundleFilePath))
            {
                return false;
            }
        }

        for (const AzToolsFramework::AssetFileInfo& assetFileInfo : assetFileInfoList.m_fileInfoList)
        {
            AZ::u64 fileSize = 0;
            if (!fileIO->Size(assetFileInfo.m_assetRelativePath.c_str(), fileSize))
            {
                AZ_Error(logWindowName, false, "Unable to find size of file (%s).\n", assetFileInfo.m_assetRelativePath.c_str());
                return false;
            }

            if (fileSize > maxSizeInBytes)
            {
                AZ_Warning(logWindowName, false, "File (%s) size (%d) is bigger than the max bundle size (%d).\n", assetFileInfo.m_assetRelativePath.c_str(), fileSize, maxSizeInBytes);
            }

            totalFileSize += fileSize;

            if (!MaxSizeExceeded(totalFileSize, bundleSize, assetCatalogFileSizeBuffer, maxSizeInBytes))
            {
                // if we are here it implies that the total file size on disk is less than the max bundle size 
                // and therefore these files can be added together into the bundle.
                fileEntries.emplace_back(assetFileInfo.m_assetRelativePath);
                deltaCatalogEntries.emplace_back(assetFileInfo.m_assetRelativePath);
                continue;
            }
            else
            {
                // add all files to the archive as a batch and update the bundle size
                if (!InjectFiles(fileEntries, bundleFullPath, assetAlias.c_str()))
                {
                    return false;
                }

                fileEntries.clear();

                if (fileIO->Exists(bundleFullPath.c_str()))
                {
                    if (!fileIO->Size(bundleFullPath.c_str(), bundleSize))
                    {
                        AZ_Error(logWindowName, false, "Unable to find size of archive file (%s).\n", bundleFilePath.c_str());
                        return false;
                    }
                }
            }

            totalFileSize = fileSize;

            if (MaxSizeExceeded(totalFileSize, bundleSize, assetCatalogFileSizeBuffer, maxSizeInBytes))
            {
                // if we are here it implies that adding file size to the remaining increases the size over the max size 
                // and therefore we can add the pending files and the delta catalog to the bundle
                if (!AddCatalogAndFilesToBundle(deltaCatalogEntries, fileEntries, bundleFullPath, deltaCatalogFilePath, assetAlias.c_str(), platformId))
                {
                    return false;
                }

                fileEntries.clear();
                deltaCatalogEntries.clear();
                bundleSize = 0;
                int numOfTries = 20;
                AZStd::string dependentBundleFileName;
                do
                {
                    // we need to find a bundle which does not exist on disk;
                    bundleIndex++;
                    numOfTries--;
                    dependentBundleFileName = CreateAssetBundleFileName(bundleFilePath, bundleIndex);
                    AzFramework::StringFunc::Path::ReplaceFullName(bundleFullPath, dependentBundleFileName.c_str());
                } while (numOfTries && fileIO->Exists(bundleFullPath.c_str()));

                if (!numOfTries)
                {
                    AZ_Error(logWindowName, false, "Unable to find a unique name for the archive in the directory (%s).\n", bundleFolder.c_str());
                    return false;
                }

                dependentBundleNames.emplace_back(dependentBundleFileName);

                AZStd::string currentDeltaCatalogName = DeltaCatalogName;
                AzFramework::StringFunc::Path::ConstructFull(bundleFolder.c_str(), currentDeltaCatalogName.c_str(), deltaCatalogFilePath, true);
                bundlePathDeltaCatalogPair.emplace_back(AZStd::make_pair(bundleFullPath, currentDeltaCatalogName));
            }

            fileEntries.emplace_back(assetFileInfo.m_assetRelativePath);
            deltaCatalogEntries.emplace_back(assetFileInfo.m_assetRelativePath);
        }


        if (!AddCatalogAndFilesToBundle(deltaCatalogEntries, fileEntries, bundleFullPath, deltaCatalogFilePath, assetAlias.c_str(), platformId))
        {
            return false;
        }

        // Create and add manifest files for all the bundles
        if (!AddManifestFileToBundles(bundlePathDeltaCatalogPair, dependentBundleNames, bundleFolder, assetBundleSettings))
        {
            return false;
        }

        return true;
    }

    bool AssetBundleComponent::CreateAssetBundle(const AssetBundleSettings& assetBundleSettings)
    {

        if (assetBundleSettings.m_assetFileInfoListPath.empty())
        {
            AZ_Error(logWindowName, false, "AssetFileInfo List path is empty. \n");
            return false;
        }

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();

        AZStd::string assetFileInfoListPath = assetBundleSettings.m_assetFileInfoListPath;

        const char* appRoot = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(appRoot, &AzFramework::ApplicationRequests::GetAppRoot);

        if (AzFramework::StringFunc::Path::IsRelative(assetFileInfoListPath.c_str()))
        {
            AzFramework::StringFunc::Path::ConstructFull(appRoot, assetFileInfoListPath.c_str(), assetFileInfoListPath, true);
        }

        if (!fileIO->Exists(assetFileInfoListPath.c_str()))
        {
            AZ_Error(logWindowName, false, "AssetFileInfoList file (%s) does not exist. \n", assetFileInfoListPath.c_str());
            return false;
        }

        AssetFileInfoList assetFileInfoList;

        if (!AZ::Utils::LoadObjectFromFileInPlace(assetFileInfoListPath.c_str(), assetFileInfoList))
        {
            AZ_Error(logWindowName, false, "Failed to deserialize file (%s).\n", assetFileInfoListPath.c_str());
            return false;
        }

        return CreateAssetBundleFromList(assetBundleSettings, assetFileInfoList);
    }

    bool AssetBundleComponent::AddCatalogAndFilesToBundle(const AZStd::vector<AZStd::string>& deltaCatalogEntries, const AZStd::vector<AZStd::string>& fileEntries, const AZStd::string& bundleFilePath, const AZStd::string& deltaCatalogFilePath, const char* assetAlias, const AzFramework::PlatformId& platformId)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZStd::string bundleFolder;
        AzFramework::StringFunc::Path::GetFullPath(bundleFilePath.c_str(), bundleFolder);

        if (!MakePath(bundleFolder))
        {
            return  false;
        }

        if (fileEntries.size())
        {
            if (!InjectFiles(fileEntries, bundleFilePath, assetAlias))
            {
                return false;
            }
        }

        if(deltaCatalogEntries.size())
        {
            // add the delta catalog to the bundle
            bool success = false;
            AssetCatalog::PlatformAddressedAssetCatalogRequestBus::EventResult(success, platformId, &AssetCatalog::PlatformAddressedAssetCatalogRequestBus::Events::CreateDeltaCatalog, fileEntries, deltaCatalogFilePath.c_str());
            if (!success)
            {
                AZ_Error(logWindowName, false, "Failed to create the delta catalog file (%s).\n", deltaCatalogFilePath.c_str());
                return false;
            }
            AZStd::string deltaCatalogFileName;
            AzFramework::StringFunc::Path::GetFullFileName(deltaCatalogFilePath.c_str(), deltaCatalogFileName);
            if (!InjectFile(deltaCatalogFileName, bundleFilePath, bundleFolder.c_str()))
            {
                AZ_Error(logWindowName, false, "Failed to add the delta catalog file (%s) in the bundle (%s).\n", deltaCatalogFilePath.c_str(), bundleFilePath.c_str());
                return false;
            }
            fileIO->Remove(deltaCatalogFilePath.c_str());
        }

        return true;
    }

    bool AssetBundleComponent::AddManifestFileToBundles(const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& bundlePathDeltaCatalogPair, const AZStd::vector<AZStd::string>& dependentBundleNames, const AZStd::string& bundleFolder, const AzToolsFramework::AssetBundleSettings& assetBundleSettings)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();

        if (!MakePath(bundleFolder))
        {
            return false;
        }

        if (bundlePathDeltaCatalogPair.empty())
        {
            AZ_Warning(logWindowName, false, "AddManifestFilesToBundle called with no bundle paths provided.  Cannot add manifest file.");

            return false;
        }

        AZStd::string bundleTempFolder = bundlePathDeltaCatalogPair[0].first;

        if (!MakeTempFolderFromFilePath(bundlePathDeltaCatalogPair[0].first, bundleTempFolder))
        {
            return false;
        }

        AZStd::string bundleManifestPath;
        AzFramework::StringFunc::Path::ConstructFull(bundleTempFolder.c_str(), AzFramework::AssetBundleManifest::s_manifestFileName, bundleManifestPath, true);

        for (int idx = 0; idx < bundlePathDeltaCatalogPair.size(); idx++)
        {
            AZStd::vector<AZStd::string> bundleNameList;
            if (!idx)
            {
                // The manifest file of the base bundle is special and contains the names of all the rollover bundle in it.
                bundleNameList = dependentBundleNames;
            }

            bool manifestSaved = false;
            AssetCatalogRequestBus::BroadcastResult(manifestSaved, &AssetCatalogRequestBus::Events::CreateBundleManifest, bundlePathDeltaCatalogPair[idx].second, bundleNameList, bundleTempFolder, assetBundleSettings.m_bundleVersion);
            if (!manifestSaved)
            {
                AZ_Error(logWindowName, false, "Failed to create manifest file (%s) for the bundle (%s).\n", AzFramework::AssetBundleManifest::s_manifestFileName, bundlePathDeltaCatalogPair[idx].first.c_str());
                return false;
            }

            AZStd::string bundleManifestFileName;
            AzFramework::StringFunc::Path::GetFullFileName(bundleManifestPath.c_str(), bundleManifestFileName);

            if (!InjectFile(bundleManifestFileName, bundlePathDeltaCatalogPair[idx].first, bundleTempFolder.c_str()))
            {
                AZ_Error(logWindowName, false, "Failed to add manifest file (%s) in the bundle (%s).\n", bundleManifestPath.c_str(), bundlePathDeltaCatalogPair[idx].first.c_str());
                return false;
            }

            fileIO->DestroyPath(bundleTempFolder.c_str());
        }

        return true;
    }

    bool AssetBundleComponent::DeleteBundleFiles(const AZStd::string& assetBundleFilePath)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZStd::string bundleFolder;
        AzFramework::StringFunc::Path::GetFullPath(assetBundleFilePath.c_str(), bundleFolder);
        AZStd::unique_ptr<AzFramework::AssetBundleManifest> manifest(GetManifestFromBundle(assetBundleFilePath));
        if (manifest)
        {
            for (const AZStd::string& filename : manifest->GetDependentBundleNames())
            {
                AZStd::string dependentBundlesFilePath;
                AzFramework::StringFunc::Path::ConstructFull(bundleFolder.c_str(), filename.c_str(), dependentBundlesFilePath, true);
                if (!fileIO->Remove(dependentBundlesFilePath.c_str()))
                {
                    AZ_Warning(logWindowName, false, "Failed to delete dependent bundle file (%s)", dependentBundlesFilePath.c_str());
                }
            }

        }

        // remove the parent bundle
        if (!fileIO->Remove(assetBundleFilePath.c_str()))
        {
            AZ_Error(logWindowName, false, "Failed to delete bundle file (%s)", assetBundleFilePath.c_str());
            return false;
        }

        return true;
    }

    bool AssetBundleComponent::InjectFile(const AZStd::string& filePath, const AZStd::string& archiveFilePath, const char* workingDirectory)
    {
        AZ_TracePrintf(logWindowName, "Injecting file (%s) into bundle (%s).\n", filePath.c_str(), archiveFilePath.c_str());
        bool fileAddedToArchive = false;
        ArchiveCommandsBus::BroadcastResult(fileAddedToArchive, &AzToolsFramework::ArchiveCommands::AddFileToArchiveBlocking, archiveFilePath, workingDirectory, filePath);
        if (!fileAddedToArchive)
        {
            AZ_Error(logWindowName, false, "Failed to insert file (%s) into bundle (%s).", filePath.c_str(), archiveFilePath.c_str());
        }
        return fileAddedToArchive;
    }
    
    bool AssetBundleComponent::InjectFile(const AZStd::string& filePath, const AZStd::string& sourcePak)
    {
        return InjectFile(filePath, sourcePak, "");
    }

    bool AssetBundleComponent::InjectFiles(const AZStd::vector<AZStd::string>& fileEntries, const AZStd::string& sourcePak, const char* workingDirectory)
    {
        if (!fileEntries.size())
        {
            return true;
        }
        AZStd::string filesStr;

        for (const AZStd::string& file : fileEntries)
        {
            filesStr.append(AZStd::string::format("%s\n", file.c_str()));
        }

        AZStd::string bundleFolder;

        if (!MakeTempFolderFromFilePath(sourcePak, bundleFolder))
        {
            return false;
        }

        // Creating a list file
        AZStd::string listFilePath;
        const char listFileName[] = "ListFile.txt";
        AzFramework::StringFunc::Path::ConstructFull(bundleFolder.c_str(), listFileName, listFilePath, true);

        {
            AZ::IO::FileIOStream fileStream(listFilePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText);
            if (fileStream.IsOpen())
            {
                fileStream.Write(filesStr.size(), filesStr.data());
            }
            else
            {
                AZ_Error(logWindowName, false, "Failed to create list file (%s) for adding files in the bundle (%s).\n", listFilePath.c_str(), sourcePak.c_str());
                AZ::IO::FileIOBase::GetInstance()->DestroyPath(bundleFolder.c_str());
                return false;
            }
        }

        bool filesAddedToArchive = false;
        AzToolsFramework::ArchiveCommandsBus::BroadcastResult(filesAddedToArchive, &AzToolsFramework::ArchiveCommands::AddFilesToArchiveBlocking, sourcePak, workingDirectory, listFilePath);
        if (!filesAddedToArchive)
        {
            AZ_Error(logWindowName, false, "Failed to insert files into bundle (%s).\n", sourcePak.c_str());
        }

        AZ::IO::FileIOBase::GetInstance()->DestroyPath(bundleFolder.c_str());
        return filesAddedToArchive;
    }

    bool AssetBundleComponent::HasManifest(const AZStd::vector<AZStd::string>& fileEntries)
    {
        auto itr = AZStd::find(fileEntries.begin(), fileEntries.end(), AZStd::string(AzFramework::AssetBundleManifest::s_manifestFileName));
        return itr != fileEntries.end();
    }

    AzFramework::AssetBundleManifest* AssetBundleComponent::GetManifestFromBundle(const AZStd::string& sourcePak)
    {
        // open the manifest and deserialize it
        bool manifestExtracted = false;
        const bool overwriteExisting = true;
        AZStd::string archiveFolder;

        if (!MakeTempFolderFromFilePath(sourcePak, archiveFolder))
        {
            return nullptr;
        }

        AZStd::string manifestFilePath;
        AzFramework::StringFunc::Path::ConstructFull(archiveFolder.c_str(), AzFramework::AssetBundleManifest::s_manifestFileName, manifestFilePath, true);
        ArchiveCommandsBus::BroadcastResult(manifestExtracted, &ArchiveCommandsBus::Events::ExtractFileBlocking, sourcePak, AzFramework::AssetBundleManifest::s_manifestFileName, archiveFolder, overwriteExisting);
        if (!manifestExtracted)
        {
            AZ_Error(logWindowName, false, "Failed to extract existing manifest from archive \"%s\".", sourcePak.c_str());
            AZ::IO::FileIOBase::GetInstance()->DestroyPath(archiveFolder.c_str());
            return nullptr;
        }

        // deserialize the manifest
        AZ::SerializeContext* serializeContext = nullptr;
        EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(serializeContext, "Unable to retrieve serialize context.");
        if (nullptr == serializeContext->FindClassData(AZ::AzTypeInfo<AzFramework::AssetBundleManifest>::Uuid()))
        {
            AzFramework::AssetBundleManifest::ReflectSerialize(serializeContext);
        }
        AzFramework::AssetBundleManifest* manifest = AZ::Utils::LoadObjectFromFile<AzFramework::AssetBundleManifest>(manifestFilePath, serializeContext);
        if (manifest == nullptr)
        {
            AZ_Error(logWindowName, false, "Failed to deserialize existing manifest from archive \"%s\".", sourcePak.c_str());
        }

        AZ::IO::FileIOBase::GetInstance()->DestroyPath(archiveFolder.c_str());
        return manifest;
    }

    bool AssetBundleComponent::RemoveNonAssetFileEntries(AZStd::vector<AZStd::string>& fileEntries, const AZStd::string& normalizedSourcePakPath, const AzFramework::AssetBundleManifest* manifest)
    {
        auto sourcePakItr = AZStd::find(fileEntries.begin(), fileEntries.end(), normalizedSourcePakPath);
        if (sourcePakItr != fileEntries.end())
        {
            fileEntries.erase(sourcePakItr);
        }

        if (manifest)
        {
            sourcePakItr = AZStd::find(fileEntries.begin(), fileEntries.end(), AZStd::string(AzFramework::AssetBundleManifest::s_manifestFileName));
            if (sourcePakItr != fileEntries.end())
            {
                fileEntries.erase(sourcePakItr);
            }

            // use the catalog name stored in the manifest to determine what to exclude and overwrite
            sourcePakItr = AZStd::find(fileEntries.begin(), fileEntries.end(), manifest->GetCatalogName());
            if (sourcePakItr != fileEntries.end())
            {
                fileEntries.erase(sourcePakItr);
            }
            else
            {
                AZ_Error(logWindowName, false, "Asset catalog \"%s\" referenced in manifest doesn't exist in bundle \"%s\".", manifest->GetCatalogName().c_str(), normalizedSourcePakPath.c_str());
                return false;
            }
        }
        return true;
    }
}

