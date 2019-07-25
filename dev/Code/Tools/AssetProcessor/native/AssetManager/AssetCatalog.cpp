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

#include "native/AssetManager/AssetCatalog.h"
#include "native/AssetManager/assetProcessorManager.h"
#include "native/utilities/assetUtils.h"
#include "native/AssetDatabase/AssetDatabase.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/tokenize.h>
#include <AzCore/std/string/wildcard.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/AssetRegistry.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <QElapsedTimer>
#include <QMutexLocker>
#include <AzCore/Math/Crc.h>

namespace AssetProcessor
{
    AssetCatalog::AssetCatalog(QObject* parent, AssetProcessor::PlatformConfiguration* platformConfiguration)
        : QObject(parent)
        , m_platformConfig(platformConfiguration)
        , m_registryBuiltOnce(false)
        , m_registriesMutex(QMutex::Recursive)
    {
        
        for (const AssetBuilderSDK::PlatformInfo& info : m_platformConfig->GetEnabledPlatforms())
        {
            m_platforms.push_back(QString::fromUtf8(info.m_identifier.c_str()));
        }

        bool computedCacheRoot = AssetUtilities::ComputeProjectCacheRoot(m_cacheRoot);
        AZ_Assert(computedCacheRoot, "Could not compute cache root for AssetCatalog");

        // save 30mb for this.  Really large projects do get this big (and bigger)
        // if you don't do this, things get fragmented very fast.
        m_saveBuffer.reserve(1024 * 1024 * 30); 
        
        m_absoluteDevFolderPath[0] = 0;
        m_absoluteDevGameFolderPath[0] = 0;

        AZStd::string appRoot;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(appRoot, &AzFramework::ApplicationRequests::GetAppRoot);
        azstrcpy(m_absoluteDevFolderPath, AZ_MAX_PATH_LEN, appRoot.c_str());
        
        AZStd::string gameFolderPath;
        AzFramework::StringFunc::Path::Join(appRoot.c_str(), AssetUtilities::ComputeGameName().toUtf8().constData(), gameFolderPath);
        azstrcpy(m_absoluteDevGameFolderPath, AZ_MAX_PATH_LEN, gameFolderPath.c_str());


        AssetUtilities::ComputeProjectCacheRoot(m_cacheRootDir);

        if (!ConnectToDatabase())
        {
            AZ_Error("AssetCatalog", false, "Failed to connect to sqlite database");
        }

        AssetRegistryRequestBus::Handler::BusConnect();
        AzToolsFramework::AssetSystemRequestBus::Handler::BusConnect();
        AzToolsFramework::ToolsAssetSystemBus::Handler::BusConnect();
        AZ::Data::AssetCatalogRequestBus::Handler::BusConnect();
    }

    AssetCatalog::~AssetCatalog()
    {
        AzToolsFramework::ToolsAssetSystemBus::Handler::BusDisconnect();
        AzToolsFramework::AssetSystemRequestBus::Handler::BusDisconnect();
        AssetRegistryRequestBus::Handler::BusDisconnect();
        AZ::Data::AssetCatalogRequestBus::Handler::BusDisconnect();
        SaveRegistry_Impl();
    }

    void AssetCatalog::OnAssetMessage(QString platform, AzFramework::AssetSystem::AssetNotificationMessage message)
    {
        using namespace AzFramework::AssetSystem;
        if (message.m_type == AssetNotificationMessage::AssetChanged)
        {
            //get the full product path to determine file size
            AZ::Data::AssetInfo assetInfo;
            assetInfo.m_assetId = message.m_assetId;
            assetInfo.m_assetType = message.m_assetType;
            assetInfo.m_relativePath = message.m_data.c_str();
            assetInfo.m_sizeBytes = message.m_sizeBytes;

            AZ_Assert(assetInfo.m_assetId.IsValid(), "AssetID is not valid!!!");
            AZ_Assert(!assetInfo.m_relativePath.empty(), "Product path is empty");

            m_catalogIsDirty = true;
            {
                QMutexLocker locker(&m_registriesMutex);
                m_registries[platform].RegisterAsset(assetInfo.m_assetId, assetInfo);
                for (const AZ::Data::AssetId& mapping : message.m_legacyAssetIds)
                {
                    if (mapping != assetInfo.m_assetId)
                    {
                        m_registries[platform].RegisterLegacyAssetMapping(mapping, assetInfo.m_assetId);
                    }
                }

                m_registries[platform].SetAssetDependencies(message.m_assetId, message.m_dependencies);
            }

            if (m_registryBuiltOnce)
            {
                Q_EMIT SendAssetMessage(platform, message);
            }
        }
        else if (message.m_type == AssetNotificationMessage::AssetRemoved)
        {
            QMutexLocker locker(&m_registriesMutex);
            auto found = m_registries[platform].m_assetIdToInfo.find(message.m_assetId);

            if (found != m_registries[platform].m_assetIdToInfo.end())
            {
                m_catalogIsDirty = true;

                m_registries[platform].UnregisterAsset(message.m_assetId);

                for (const AZ::Data::AssetId& mapping : message.m_legacyAssetIds)
                {
                    if (mapping != message.m_assetId)
                    {
                        m_registries[platform].UnregisterLegacyAssetMapping(mapping);
                    }
                }

                if (m_registryBuiltOnce)
                {
                    Q_EMIT SendAssetMessage(platform, message);
                }
            }
        }
    }

    void AssetCatalog::SaveRegistry_Impl()
    {
        bool allCatalogsSaved = true;
        // note that its safe not to save the catalog if the catalog is not dirty
        // because the engine will be accepting updates as long as the update has a higher or equal
        // number to the saveId, not just equal.
        if (m_catalogIsDirty)
        {
            m_catalogIsDirty = false;
            // Reflect registry for serialization.
            AZ::SerializeContext* serializeContext = nullptr;
            EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
            AZ_Assert(serializeContext, "Unable to retrieve serialize context.");
            if (nullptr == serializeContext->FindClassData(AZ::AzTypeInfo<AzFramework::AssetRegistry>::Uuid()))
            {
                AzFramework::AssetRegistry::ReflectSerialize(serializeContext);
            }

            // save out a catalog for each platform
            for (const QString& platform : m_platforms)
            {
                // Serialize out the catalog to a memory buffer, and then dump that memory buffer to stream.
                QElapsedTimer timer;
                timer.start();
                m_saveBuffer.clear();
                // allow this to grow by up to 20mb at a time so as not to fragment.
                // we re-use the save buffer each time to further reduce memory load.
                AZ::IO::ByteContainerStream<AZStd::vector<char>> catalogFileStream(&m_saveBuffer, 1024 * 1024 * 20);

                // these 3 lines are what writes the entire registry to the memory stream
                AZ::ObjectStream* objStream = AZ::ObjectStream::Create(&catalogFileStream, *serializeContext, AZ::ObjectStream::ST_BINARY);
                {
                    QMutexLocker locker(&m_registriesMutex);
                    objStream->WriteClass(&m_registries[platform]);
                }
                objStream->Finalize();

                // now write the memory stream out to the temp folder
                QString workSpace;
                if (!AssetUtilities::CreateTempWorkspace(workSpace))
                {
                    AZ_Warning(AssetProcessor::ConsoleChannel, false, "Failed to create a temp workspace for catalog writing\n");
                }
                else
                {
                    QString tempRegistryFile = QString("%1/%2").arg(workSpace).arg("assetcatalog.xml.tmp");
                    QString platformGameDir = QString("%1/%2/").arg(m_cacheRoot.absoluteFilePath(platform)).arg(AssetUtilities::ComputeGameName().toLower());
                    QString actualRegistryFile = QString("%1%2").arg(platformGameDir).arg("assetcatalog.xml");

                    AZ_TracePrintf(AssetProcessor::DebugChannel, "Creating asset catalog: %s --> %s\n", tempRegistryFile.toUtf8().constData(), actualRegistryFile.toUtf8().constData());
                    AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
                    if (AZ::IO::FileIOBase::GetInstance()->Open(tempRegistryFile.toUtf8().data(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary, fileHandle))
                    {
                        AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, m_saveBuffer.data(), m_saveBuffer.size());
                        AZ::IO::FileIOBase::GetInstance()->Close(fileHandle);

                        // Make sure that the destination folder of the registry file exists
                        QDir registryDir(platformGameDir);
                        if (!registryDir.exists())
                        {
                            QString absPath = registryDir.absolutePath();
                            bool makeDirResult = AZ::IO::SystemFile::CreateDir(absPath.toUtf8().constData());
                            AZ_Warning(AssetProcessor::ConsoleChannel, makeDirResult, "Failed create folder %s", platformGameDir.toUtf8().constData());
                        }
                        
                        // if we succeeded in doing this, then use "rename" to move the file over the previous copy.
                        bool moved = AssetUtilities::MoveFileWithTimeout(tempRegistryFile, actualRegistryFile, 3);
                        allCatalogsSaved = allCatalogsSaved && moved;

                        // warn if it failed
                        AZ_Warning(AssetProcessor::ConsoleChannel, moved, "Failed to move %s to %s", tempRegistryFile.toUtf8().constData(), actualRegistryFile.toUtf8().constData());

                        if (moved)
                        {
                            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Saved %s catalog containing %u assets in %fs\n", platform.toUtf8().constData(), m_registries[platform].m_assetIdToInfo.size(), timer.elapsed() / 1000.0f);
                        }
                    }
                    else
                    {
                        AZ_Warning(AssetProcessor::ConsoleChannel, false, "Failed to create catalog file %s", tempRegistryFile.toUtf8().constData());
                        allCatalogsSaved = false;
                    }

                    AZ::IO::FileIOBase::GetInstance()->DestroyPath(workSpace.toUtf8().data());
                }
            }
        }
        
        {
            // scoped to minimize the duration of this mutex lock
            QMutexLocker locker(&m_savingRegistryMutex);
            m_currentlySavingCatalog = false;
            RegistrySaveComplete(m_currentRegistrySaveVersion, allCatalogsSaved);
            AssetRegistryNotificationBus::Broadcast(&AssetRegistryNotifications::OnRegistrySaveComplete, m_currentRegistrySaveVersion, allCatalogsSaved);
        }
    }

    void AssetCatalog::RequestReady(NetworkRequestID requestId, BaseAssetProcessorMessage* message, QString platform, bool fencingFailed)
    {
        AZ_UNUSED(message);
        AZ_UNUSED(platform);
        AZ_UNUSED(fencingFailed);
        int registrySaveVersion = SaveRegistry();
        m_queuedSaveCatalogRequest.insert(registrySaveVersion, requestId);
    }

    void AssetCatalog::RegistrySaveComplete(int assetCatalogVersion, bool allCatalogsSaved)
    {
        for (auto iter = m_queuedSaveCatalogRequest.begin(); iter != m_queuedSaveCatalogRequest.end();)
        {
            if (iter.key() <= assetCatalogVersion)
            {
                AssetProcessor::NetworkRequestID& requestId = iter.value();
                AzFramework::AssetSystem::SaveAssetCatalogResponse saveCatalogResponse;
                saveCatalogResponse.m_saved = allCatalogsSaved;
                AssetProcessor::ConnectionBus::Event(requestId.first, &AssetProcessor::ConnectionBus::Events::SendResponse, requestId.second, saveCatalogResponse);
                iter = m_queuedSaveCatalogRequest.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }

    int AssetCatalog::SaveRegistry()
    {
        QMutexLocker locker(&m_savingRegistryMutex);

        if (!m_currentlySavingCatalog)
        {
            m_currentlySavingCatalog = true;
            QMetaObject::invokeMethod(this, "SaveRegistry_Impl", Qt::QueuedConnection);
            return ++m_currentRegistrySaveVersion;
        }

        return m_currentRegistrySaveVersion;
    }

    void AssetCatalog::BuildRegistry()
    {
        m_catalogIsDirty = true;
        m_registryBuiltOnce = true;

        for (QString platform : m_platforms)
        {
            auto inserted = m_registries.insert(platform, AzFramework::AssetRegistry());
            AzFramework::AssetRegistry& currentRegistry = inserted.value();

            QElapsedTimer timer;
            timer.start();
            auto databaseQueryCallback = [&](AzToolsFramework::AssetDatabase::CombinedDatabaseEntry& combined)
                {
                    AZ::Data::AssetId assetId(combined.m_sourceGuid, combined.m_subID);

                    // relative file path is gotten by removing the platform and game from the product name
                    QString relativeProductPath = combined.m_productName.c_str();
                    relativeProductPath = relativeProductPath.right(relativeProductPath.length() - relativeProductPath.indexOf('/') - 1); // remove PLATFORM and an extra slash
                    relativeProductPath = relativeProductPath.right(relativeProductPath.length() - relativeProductPath.indexOf('/') - 1); // remove GAMENAME and an extra slash
                    QString fullProductPath = m_cacheRoot.absoluteFilePath(combined.m_productName.c_str());

                    AZ::Data::AssetInfo info;
                    info.m_assetType = combined.m_assetType;
                    info.m_relativePath = relativeProductPath.toUtf8().data();
                    info.m_assetId = assetId;
                    info.m_sizeBytes = AZ::IO::SystemFile::Length(fullProductPath.toUtf8().constData());

                    // also register it at the legacy id(s) if its different:
                    AZ::Data::AssetId legacyAssetId(combined.m_legacyGuid, 0);
                    AZ::Uuid  legacySourceUuid = AssetUtilities::CreateSafeSourceUUIDFromName(combined.m_sourceName.c_str(), false);
                    AZ::Data::AssetId legacySourceAssetId(legacySourceUuid, combined.m_subID);

                    currentRegistry.RegisterAsset(assetId, info);

                    if (legacyAssetId != assetId)
                    {
                        currentRegistry.RegisterLegacyAssetMapping(legacyAssetId, assetId);
                    }

                    if (legacySourceAssetId != assetId)
                    {
                        currentRegistry.RegisterLegacyAssetMapping(legacySourceAssetId, assetId);
                    }

                    // now include the additional legacies based on the SubIDs by which this asset was previously referred to.
                    for (const auto& entry : combined.m_legacySubIDs)
                    {
                        AZ::Data::AssetId legacySubID(combined.m_sourceGuid, entry.m_subID);
                        if ((legacySubID != assetId) && (legacySubID != legacyAssetId) && (legacySubID != legacySourceAssetId))
                        {
                            currentRegistry.RegisterLegacyAssetMapping(legacySubID, assetId);
                        }

                    }

                    return true;//see them all
                };

            AZStd::lock_guard<AZStd::mutex> lock(m_databaseMutex);

            m_db->QueryCombined(
                databaseQueryCallback, AZ::Uuid::CreateNull(),
                nullptr,
                platform.toUtf8().constData(),
                AzToolsFramework::AssetSystem::JobStatus::Any,
                true); /*we still need legacy IDs - hardly anyone else does*/

            m_db->QueryProductDependenciesTable([this, &platform](AZ::Data::AssetId& assetId, AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& entry)
            {
                m_registries[platform].RegisterAssetDependency(assetId, AZ::Data::ProductDependency{ AZ::Data::AssetId(entry.m_dependencySourceGuid, entry.m_dependencySubID), entry.m_dependencyFlags });

                return true;
            });

            AZ_TracePrintf("Catalog", "Read %u assets from database for %s in %fs\n", currentRegistry.m_assetIdToInfo.size(), platform.toUtf8().constData(), timer.elapsed() / 1000.0f);
        }
    }

    void AssetCatalog::OnSourceQueued(AZ::Uuid sourceUuid, AZ::Uuid legacyUuid, QString rootPath, QString relativeFilePath)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_sourceUUIDToSourceNameMapMutex);

        SourceInfo sourceInfo = { rootPath, relativeFilePath };
        m_sourceUUIDToSourceNameMap.insert({ sourceUuid, sourceInfo });

        //adding legacy source uuid as well
        m_sourceUUIDToSourceNameMap.insert({ legacyUuid, sourceInfo });

        AZStd::string nameForMap(relativeFilePath.toUtf8().constData());
        AZStd::to_lower(nameForMap.begin(), nameForMap.end());
       
        m_sourceNameToSourceUUIDMap.insert({ nameForMap, sourceUuid });
    }

    void AssetCatalog::OnSourceFinished(AZ::Uuid sourceUuid, AZ::Uuid legacyUuid)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_sourceUUIDToSourceNameMapMutex);

        auto found = m_sourceUUIDToSourceNameMap.find(sourceUuid);
        if (found != m_sourceUUIDToSourceNameMap.end())
        {
            AZStd::string nameForMap = found->second.m_sourceName.toUtf8().constData();
            AZStd::to_lower(nameForMap.begin(), nameForMap.end());
            m_sourceNameToSourceUUIDMap.erase(nameForMap);
        }

        m_sourceUUIDToSourceNameMap.erase(sourceUuid);
        m_sourceUUIDToSourceNameMap.erase(legacyUuid);
    }

    //////////////////////////////////////////////////////////////////////////

    const char* AssetCatalog::GetAbsoluteDevGameFolderPath()
    {
        return m_absoluteDevGameFolderPath;
    }

    const char* AssetCatalog::GetAbsoluteDevRootFolderPath()
    {
        return m_absoluteDevFolderPath;
    }

    bool AssetCatalog::GetRelativeProductPathFromFullSourceOrProductPath(const AZStd::string& fullSourceOrProductPath, AZStd::string& relativeProductPath)
    {
        ProcessGetRelativeProductPathFromFullSourceOrProductPathRequest(fullSourceOrProductPath, relativeProductPath);

        if (!relativeProductPath.length())
        {
            // if we are here it means we have failed to determine the assetId we will send back the original path
            AZ_TracePrintf(AssetProcessor::DebugChannel, "GetRelativeProductPath no result, returning original %s...\n", fullSourceOrProductPath.c_str());
            relativeProductPath = fullSourceOrProductPath;
            return false;
        }

        return true;
    }

    bool AssetCatalog::GetFullSourcePathFromRelativeProductPath(const AZStd::string& relPath, AZStd::string& fullSourcePath)
    {
        ProcessGetFullSourcePathFromRelativeProductPathRequest(relPath, fullSourcePath);

        if (!fullSourcePath.length())
        {
            // if we are here it means that we failed to determine the full source path from the relative path and we will send back the original path
            AZ_TracePrintf(AssetProcessor::DebugChannel, "GetFullSourcePath no result, returning original %s...\n", relPath.c_str());
            fullSourcePath = relPath;
            return false;
        }

        return true;
    }

    bool AssetCatalog::GetAssetInfoById(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType, AZ::Data::AssetInfo& assetInfo, AZStd::string& rootFilePath)
    {
        assetInfo.m_assetId.SetInvalid();
        assetInfo.m_relativePath.clear();
        assetInfo.m_assetType = AZ::Data::s_invalidAssetType;
        assetInfo.m_sizeBytes = 0;

        // If the assetType wasn't provided, try to guess it
        if (assetType.IsNull())
        {
            return GetAssetInfoByIdOnly(assetId, assetInfo, rootFilePath);
        }

        bool isSourceType;

        {
            AZStd::lock_guard<AZStd::mutex> lock(m_sourceAssetTypesMutex);
            isSourceType = m_sourceAssetTypes.find(assetType) != m_sourceAssetTypes.end();
        }

        // If the assetType is registered as a source type, look up the source info
        if (isSourceType)
        {
            AZStd::string relativePath;

            if (GetSourceFileInfoFromAssetId(assetId, rootFilePath, relativePath))
            {
                AZStd::string sourceFileFullPath;
                AzFramework::StringFunc::Path::Join(rootFilePath.c_str(), relativePath.c_str(), sourceFileFullPath);

                assetInfo.m_assetId = assetId;
                assetInfo.m_assetType = assetType;
                assetInfo.m_relativePath = relativePath;
                assetInfo.m_sizeBytes = AZ::IO::SystemFile::Length(sourceFileFullPath.c_str());

                return true;
            }

            return false;
        }

        // Return the product file info
        rootFilePath.clear(); // products don't have root file paths.
        assetInfo = GetProductAssetInfo(nullptr, assetId);

        return !assetInfo.m_relativePath.empty();
    }

    bool AssetCatalog::GetSourceInfoBySourcePath(const char* sourcePath, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder)
    {
        if (!sourcePath)
        {
            assetInfo.m_assetId.SetInvalid();
            return false;
        }

        // regardless of which way we come into this function we must always use ConvertToRelativePath
        // to convert from whatever the input format is to a database path (which may include output prefix)
        QString databaseName; 
        QString scanFolder;
        if (!AzFramework::StringFunc::Path::IsRelative(sourcePath))
        {
            // absolute paths just get converted directly.
            m_platformConfig->ConvertToRelativePath(QString::fromUtf8(sourcePath), databaseName, scanFolder);
        }
        else
        {
            // relative paths get the first matching asset, and then they get the usual call.
            QString absolutePath = m_platformConfig->FindFirstMatchingFile(QString::fromUtf8(sourcePath));
            if (!absolutePath.isEmpty())
            {
                m_platformConfig->ConvertToRelativePath(absolutePath, databaseName, scanFolder);
            }
        }

        if ((databaseName.isEmpty()) || (scanFolder.isEmpty()))
        {
            assetInfo.m_assetId.SetInvalid();
            return false;
        }

        // now that we have a database path, we can at least return something.
        // but source info also includes UUID, which we need to hit the database for (or the in-memory map).

        // Check the database first for the UUID now that we have the "database name" (which includes output prefix)

        {
            AZStd::lock_guard<AZStd::mutex> lock(m_databaseMutex);
            AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer returnedSources;

            if (m_db->GetSourcesBySourceName(databaseName, returnedSources))
            {
                if (!returnedSources.empty())
                {
                    AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry = returnedSources.front();

                    AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry scanEntry;
                    if (m_db->GetScanFolderByScanFolderID(entry.m_scanFolderPK, scanEntry))
                    {
                        watchFolder = scanEntry.m_scanFolder;
                        // since we are returning the UUID of a source file, as opposed to the full assetId of a product file produced by that source file,
                        // the subId part of the assetId will always be set to zero.
                        assetInfo.m_assetId = AZ::Data::AssetId(entry.m_sourceGuid, 0);

                        // the Scan Folder may have an output prefix, so we must remove it from the relpath.
                        // this involves deleting the output prefix and the first slash ('editor/') which is why
                        // we remove the length of outputPrefix and one extra character.
                        if (!scanEntry.m_outputPrefix.empty())
                        {
                            assetInfo.m_relativePath = entry.m_sourceName.substr(static_cast<int>(scanEntry.m_outputPrefix.size()) + 1);
                        }
                        else
                        {
                            assetInfo.m_relativePath = entry.m_sourceName;
                        }
                        AZStd::string absolutePath;
                        AzFramework::StringFunc::Path::Join(scanEntry.m_scanFolder.c_str(), assetInfo.m_relativePath.c_str(), absolutePath);
                        assetInfo.m_sizeBytes = AZ::IO::SystemFile::Length(absolutePath.c_str());

                        assetInfo.m_assetType = AZ::Uuid::CreateNull(); // most source files don't have a type!

                        // Go through the list of source assets and see if this asset's file path matches any of the filters
                        for (const auto& pair : m_sourceAssetTypeFilters)
                        {
                            if (AZStd::wildcard_match(pair.first, assetInfo.m_relativePath))
                            {
                                assetInfo.m_assetType = pair.second;
                                break;
                            }
                        }

                        return true;
                    }
                }
            }
        }

        // Source file isn't in the database yet, see if its in the job queue
        if (GetQueuedAssetInfoByRelativeSourceName(databaseName.toUtf8().data(), assetInfo, watchFolder))
        {
            return true;
        }

        // Source file isn't in the job queue yet, source UUID needs to be created
        watchFolder = scanFolder.toUtf8().data();
        return GetUncachedSourceInfoFromDatabaseNameAndWatchFolder(databaseName.toUtf8().data(), watchFolder.c_str(), assetInfo);
    }

    bool AssetCatalog::GetSourceInfoBySourceUUID(const AZ::Uuid& sourceUuid, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder)
    {
        AZ::Data::AssetId partialId(sourceUuid, 0);
        AZStd::string relativePath;

        if (GetSourceFileInfoFromAssetId(partialId, watchFolder, relativePath))
        {
            AZStd::string sourceFileFullPath;
            AzFramework::StringFunc::Path::Join(watchFolder.c_str(), relativePath.c_str(), sourceFileFullPath);
            assetInfo.m_assetId = partialId;
            assetInfo.m_assetType = AZ::Uuid::CreateNull(); // most source files don't have a type!
            assetInfo.m_relativePath = relativePath;
            assetInfo.m_sizeBytes = AZ::IO::SystemFile::Length(sourceFileFullPath.c_str());

            // if the type has registered with a typeid, then supply it here
            AZStd::lock_guard<AZStd::mutex> lock(m_sourceAssetTypesMutex);

            // Go through the list of source assets and see if this asset's file path matches any of the filters
            // if it does, we know what type it is (if not, the above call to CreateNull ensures it is null).
            for (const auto& pair : m_sourceAssetTypeFilters)
            {
                if (AZStd::wildcard_match(pair.first, relativePath))
                {
                    assetInfo.m_assetType = pair.second;
                    break;
                }
            }

            return true;
        }
        // failed!
        return false;
    }

    bool AssetCatalog::GetScanFolders(AZStd::vector<AZStd::string>& scanFolders)
    {
        int scanFolderCount = m_platformConfig->GetScanFolderCount();
        for (int i = 0; i < scanFolderCount; ++i)
        {
            scanFolders.push_back(m_platformConfig->GetScanFolderAt(i).ScanPath().toUtf8().constData());
        }
        return true;
    }

    bool AssetCatalog::GetAssetSafeFolders(AZStd::vector<AZStd::string>& assetSafeFolders)
    {
        int scanFolderCount = m_platformConfig->GetScanFolderCount();
        for (int scanFolderIndex = 0; scanFolderIndex < scanFolderCount; ++scanFolderIndex)
        {
            AssetProcessor::ScanFolderInfo& scanFolder = m_platformConfig->GetScanFolderAt(scanFolderIndex);
            if (scanFolder.CanSaveNewAssets())
            {
                assetSafeFolders.push_back(scanFolder.ScanPath().toUtf8().constData());
            }
        }
        return true;
    }

    bool AssetCatalog::IsAssetPlatformEnabled(const char* platform)
    {
        const AZStd::vector<AssetBuilderSDK::PlatformInfo>& enabledPlatforms = m_platformConfig->GetEnabledPlatforms();
        for (const AssetBuilderSDK::PlatformInfo& platformInfo : enabledPlatforms)
        {
            if (platformInfo.m_identifier == platform)
            {
                return true;
            }
        }
        return false;
    }

    int AssetCatalog::GetPendingAssetsForPlatform(const char* platform)
    {
        AZ_Assert(false, "Call to unsupported Asset Processor function GetPendingAssetsForPlatform on AssetCatalog");
        return -1;
    }

    AZStd::string AssetCatalog::GetAssetPathById(const AZ::Data::AssetId& id)
    {
        return GetAssetInfoById(id).m_relativePath;
        
    }

    AZ::Data::AssetId AssetCatalog::GetAssetIdByPath(const char* path, const AZ::Data::AssetType& typeToRegister, bool autoRegisterIfNotFound)
    {
        AZ_UNUSED(autoRegisterIfNotFound);
        AZ_Assert(autoRegisterIfNotFound == false, "Auto registration is invalid during asset processing.");
        AZ_UNUSED(typeToRegister);
        AZ_Assert(typeToRegister == AZ::Data::s_invalidAssetType, "Can not register types during asset processing.");
        AZStd::string relProductPath;
        GetRelativeProductPathFromFullSourceOrProductPath(path, relProductPath);
        QString tempPlatformName;
        
        // Search the current platform registry first if enabled otherwise search the first registry
        if (m_platforms.contains(AzToolsFramework::AssetSystem::GetHostAssetPlatform()))
        {
            tempPlatformName = QString::fromUtf8(AzToolsFramework::AssetSystem::GetHostAssetPlatform());
        }
        else
        {
            tempPlatformName = m_platforms[0];
        }
        AZ::Data::AssetId assetId;
        {
            QMutexLocker locker(&m_registriesMutex);
            assetId = m_registries[tempPlatformName].GetAssetIdByPath(relProductPath.c_str());
        }
        return assetId;
    }

    AZ::Data::AssetInfo AssetCatalog::GetAssetInfoById(const AZ::Data::AssetId& id)
    {
        AZ::Data::AssetType assetType;
        AZ::Data::AssetInfo assetInfo;
        AZStd::string rootFilePath;
        GetAssetInfoById(id, assetType, assetInfo, rootFilePath);
        return assetInfo;
    }

    bool ConvertDatabaseProductPathToProductFileName(QString dbPath, QString& productFileName)
    {
        QString gameName = AssetUtilities::ComputeGameName();
        bool result = false;
        int gameNameIndex = dbPath.indexOf(gameName, 0, Qt::CaseInsensitive);
        if (gameNameIndex != -1)
        {
            //we will now remove the gameName and the native separator to get the assetId
            dbPath.remove(0, gameNameIndex + gameName.length() + 1);   // adding one for the native separator also
            result = true;
        }
        else
        {
            //we will just remove the platform and the native separator to get the assetId
            int separatorIndex = dbPath.indexOf("/");
            if (separatorIndex != -1)
            {
                dbPath.remove(0, separatorIndex + 1);   // adding one for the native separator
                result = true;
            }
        }

        productFileName = dbPath;

        return result;
    }

    void AssetCatalog::ProcessGetRelativeProductPathFromFullSourceOrProductPathRequest(const AZStd::string& fullPath, AZStd::string& relativeProductPath)
    {
        QString sourceOrProductPath = fullPath.c_str();
        QString normalizedSourceOrProductPath = AssetUtilities::NormalizeFilePath(sourceOrProductPath);

        QString productFileName;
        int resultCode = 0;
        QDir inputPath(normalizedSourceOrProductPath);

        AZ_TracePrintf(AssetProcessor::DebugChannel, "ProcessGetRelativeProductPath: %s...\n", sourceOrProductPath.toUtf8().constData());

        if (inputPath.isRelative())
        {
            //if the path coming in is already a relative path,we just send it back
            productFileName = sourceOrProductPath;
            resultCode = 1;
        }
        else
        {
            QDir cacheRoot;
            AssetUtilities::ComputeProjectCacheRoot(cacheRoot);
            QString normalizedCacheRoot = AssetUtilities::NormalizeFilePath(cacheRoot.path());

            bool inCacheFolder = normalizedSourceOrProductPath.startsWith(normalizedCacheRoot, Qt::CaseInsensitive);
            if (inCacheFolder)
            {
                // The path send by the game/editor contains the cache root so we try to find the asset id
                // from the asset database
                normalizedSourceOrProductPath.remove(0, normalizedCacheRoot.length() + 1); // adding 1 for the native separator

                // If we are here it means that the asset database does not have any knowledge about this file,
                // most probably because AP has not processed the file yet
                // In this case we will try to compute the asset id from the product path
                // Now after removing the cache root,normalizedInputAssetPath can either be $Platform/$Game/xxx/yyy or something like $Platform/zzz
                // and the corresponding assetId have to be either xxx/yyy or zzz

                resultCode = ConvertDatabaseProductPathToProductFileName(normalizedSourceOrProductPath, productFileName);
            }
            else
            {
                // If we are here it means its a source file, first see whether there is any overriding file and than try to find products
                QString scanFolder;
                QString relativeName;
                if (m_platformConfig->ConvertToRelativePath(normalizedSourceOrProductPath, relativeName, scanFolder))
                {
                    QString overridingFile = m_platformConfig->GetOverridingFile(relativeName, scanFolder);

                    if (overridingFile.isEmpty())
                    {
                        // no overriding file found
                        overridingFile = normalizedSourceOrProductPath;
                    }
                    else
                    {
                        overridingFile = AssetUtilities::NormalizeFilePath(overridingFile);
                    }

                    if (m_platformConfig->ConvertToRelativePath(overridingFile, relativeName, scanFolder))
                    {
                        AZStd::lock_guard<AZStd::mutex> lock(m_databaseMutex);
                        AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer products;

                        if (m_db->GetProductsBySourceName(relativeName, products))
                        {
                            resultCode = ConvertDatabaseProductPathToProductFileName(products[0].m_productName.c_str(), productFileName);
                        }
                        else
                        {
                            productFileName = relativeName;
                            resultCode = 1;
                        }
                    }
                }
            }
        }

        if (!resultCode)
        {
            productFileName = "";
        }

        relativeProductPath = productFileName.toUtf8().data();

    }

    void AssetCatalog::ProcessGetFullSourcePathFromRelativeProductPathRequest(const AZStd::string& relPath, AZStd::string& fullSourcePath)
    {
        QString assetPath = relPath.c_str();
        QString normalisedAssetPath = AssetUtilities::NormalizeFilePath(assetPath);
        int resultCode = 0;
        QString fullAssetPath;

        if (normalisedAssetPath.isEmpty())
        {
            fullSourcePath = "";
            return;
        }

        QDir inputPath(normalisedAssetPath);

        if (inputPath.isAbsolute())
        {
            bool inCacheFolder = false;
            QDir cacheRoot;
            AssetUtilities::ComputeProjectCacheRoot(cacheRoot);
            QString normalizedCacheRoot = AssetUtilities::NormalizeFilePath(cacheRoot.path());
            //Check to see whether the path contains the cache root
            inCacheFolder = normalisedAssetPath.startsWith(normalizedCacheRoot, Qt::CaseInsensitive);

            if (!inCacheFolder)
            {
                // Attempt to convert to relative path
                QString dummy, convertedRelPath;
                if (m_platformConfig->ConvertToRelativePath(assetPath, convertedRelPath, dummy))
                {
                    // then find the first matching file to get correct casing
                    fullAssetPath = m_platformConfig->FindFirstMatchingFile(convertedRelPath);
                }

                if (fullAssetPath.isEmpty())
                {
                    // if we couldn't find it, just return the passed in path
                    fullAssetPath = assetPath;
                }

                resultCode = 1;
            }
            else
            {
                // The path send by the game/editor contains the cache root ,try to find the productName from it
                normalisedAssetPath.remove(0, normalizedCacheRoot.length() + 1); // adding 1 for the native separator
            }
        }

        if (!resultCode)
        {
            //remove aliases if present
            normalisedAssetPath = AssetUtilities::NormalizeAndRemoveAlias(normalisedAssetPath);

            if (!normalisedAssetPath.isEmpty()) // this happens if it comes in as just for example "@assets@/"
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_databaseMutex);

                //We should have the asset now, we can now find the full asset path
                // we have to check each platform individually until we get a hit.
                const auto& platforms = m_platformConfig->GetEnabledPlatforms();
                QString productName;
                for (const AssetBuilderSDK::PlatformInfo& platformInfo : platforms)
                {
                    QString platformName = QString::fromUtf8(platformInfo.m_identifier.c_str());
                    productName = AssetUtilities::GuessProductNameInDatabase(normalisedAssetPath, platformName, m_db.get());
                    if (!productName.isEmpty())
                    {
                        break;
                    }
                }

                if (!productName.isEmpty())
                {
                    //Now find the input name for the path,if we are here this should always return true since we were able to find the productName before
                    AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer sources;
                    if (m_db->GetSourcesByProductName(productName, sources))
                    {
                        //Once we have found the inputname we will try finding the full path
                        fullAssetPath = m_platformConfig->FindFirstMatchingFile(sources[0].m_sourceName.c_str());
                        if (!fullAssetPath.isEmpty())
                        {
                            resultCode = 1;
                        }
                    }
                }
                else
                {
                    // if we are not able to guess the product name than maybe the asset path is an input name
                    fullAssetPath = m_platformConfig->FindFirstMatchingFile(normalisedAssetPath);
                    if (!fullAssetPath.isEmpty())
                    {
                        resultCode = 1;
                    }
                }
            }
        }

        if (!resultCode)
        {
            fullSourcePath = "";
        }
        else
        {
            fullSourcePath = fullAssetPath.toUtf8().data();
        }
    }

    //////////////////////////////////////////////////////////////////////////

    void AssetCatalog::RegisterSourceAssetType(const AZ::Data::AssetType& assetType, const char* assetFileFilter)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_sourceAssetTypesMutex);
        m_sourceAssetTypes.insert(assetType);
        AZStd::vector<AZStd::string> tokens;
        AZStd::string semicolonSeperated(assetFileFilter);
        AZStd::tokenize(semicolonSeperated, AZStd::string(";"), tokens);

        for (const auto& pattern : tokens)
        {
            m_sourceAssetTypeFilters[pattern] = assetType;
        }
    }

    void AssetCatalog::UnregisterSourceAssetType(const AZ::Data::AssetType& assetType)
    {
        // For now, this does nothing, because it would just needlessly complicate things for no gain.
        // Unregister is only called when a builder is shut down, which really is only supposed to happen when AssetCatalog is being shutdown
        // Without a way of tracking how many builders have registered the same assetType and being able to perfectly keep track of every builder shutdown, even in the event of a crash,
        // the map would either be cleared prematurely or never get cleared at all
    }

    //////////////////////////////////////////////////////////////////////////

    bool AssetCatalog::GetSourceFileInfoFromAssetId(const AZ::Data::AssetId &assetId, AZStd::string& watchFolder, AZStd::string& relativePath)
    {
        // Check the database first
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_databaseMutex);
            AzToolsFramework::AssetDatabase::SourceDatabaseEntry entry;

            if (m_db->GetSourceBySourceGuid(assetId.m_guid, entry))
            {
                AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry scanEntry;
                if (m_db->GetScanFolderByScanFolderID(entry.m_scanFolderPK, scanEntry))
                {
                    // the Scan Folder may have an output prefix, so we must remove it from the relpath.
                    // this involves deleting the output prefix and the first slash ('editor/') which is why
                    // we remove the length of outputPrefix and one extra character.
                    if (!scanEntry.m_outputPrefix.empty())
                    {
                        relativePath = entry.m_sourceName.substr(static_cast<int>(scanEntry.m_outputPrefix.size()) + 1);
                    }
                    else
                    {
                        relativePath = entry.m_sourceName;
                    }

                    watchFolder = scanEntry.m_scanFolder;
                    

                    return true;
                }
            }
        }

        // Source file isn't in the database yet, see if its in the job queue
        return GetQueuedAssetInfoById(assetId.m_guid, watchFolder, relativePath);
    }

    AZ::Data::AssetInfo AssetCatalog::GetProductAssetInfo(const char* platformName, const AZ::Data::AssetId& assetId)
    {
        // this more or less follows the same algorithm that the game uses to look up products.
        using namespace AZ::Data;
        using namespace AzFramework;

        if ((!assetId.IsValid()) || (m_platforms.isEmpty()))
        {
            return AssetInfo();
        }

        // in case no platform name has been given, we are prepared to compute one.
        QString tempPlatformName;

        // if no platform specified, we'll use the current platform.
        if ((!platformName) || (platformName[0] == 0))
        {
            // get the first available platform, preferring the host platform.
            if (m_platforms.contains(AzToolsFramework::AssetSystem::GetHostAssetPlatform()))
            {
                tempPlatformName = QString::fromUtf8(AzToolsFramework::AssetSystem::GetHostAssetPlatform());
            }
            else
            {
                // the GetHostAssetPlatform() "pc" or "osx" is not actually enabled for this compilation (maybe "server" or similar is in a build job).
                // in that case, we'll use the first we find!
                tempPlatformName = m_platforms[0];
            }
        }
        else
        {
            tempPlatformName = QString::fromUtf8(platformName);
        }

        // note that m_platforms is not mutated at all during runtime, so we ignore it in the lock
        if (!m_platforms.contains(tempPlatformName))
        {
            return AssetInfo();
        }

        QMutexLocker locker(&m_registriesMutex);

        const AssetRegistry& registryToUse = m_registries[tempPlatformName];

        auto foundIter = registryToUse.m_assetIdToInfo.find(assetId);
        if (foundIter != registryToUse.m_assetIdToInfo.end())
        {
            return foundIter->second;
        }
         
        // we did not find it - try the backup mapping!
        AssetId legacyMapping = registryToUse.GetAssetIdByLegacyAssetId(assetId);
        if (legacyMapping.IsValid())
        {
            return GetProductAssetInfo(platformName, legacyMapping);
        }

        return AssetInfo(); // not found!
    }

    bool AssetCatalog::GetAssetInfoByIdOnly(const AZ::Data::AssetId& id, AZ::Data::AssetInfo& assetInfo, AZStd::string& rootFilePath)
    {
        AZStd::string relativePath;

        if (GetSourceFileInfoFromAssetId(id, rootFilePath, relativePath))
        {
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_sourceAssetTypesMutex);

                // Go through the list of source assets and see if this asset's file path matches any of the filters
                for (const auto& pair : m_sourceAssetTypeFilters)
                {
                    if (AZStd::wildcard_match(pair.first, relativePath))
                    {
                        AZStd::string sourceFileFullPath;
                        AzFramework::StringFunc::Path::Join(rootFilePath.c_str(), relativePath.c_str(), sourceFileFullPath);

                        assetInfo.m_assetId = id;
                        assetInfo.m_assetType = pair.second;
                        assetInfo.m_relativePath = relativePath;
                        assetInfo.m_sizeBytes = AZ::IO::SystemFile::Length(sourceFileFullPath.c_str());

                        return true;
                    }
                }
            }

            // If we get to here, we're going to assume it's a product type
            rootFilePath.clear();
            assetInfo = GetProductAssetInfo(nullptr, id);

            return !assetInfo.m_relativePath.empty();
        }
        
        // Asset isn't in the DB or in the APM queue, we don't know what this asset ID is
        return false;
    }

    bool AssetCatalog::GetQueuedAssetInfoById(const AZ::Uuid& guid, AZStd::string& watchFolder, AZStd::string& relativePath)
    {
        if (!guid.IsNull())
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_sourceUUIDToSourceNameMapMutex);

            auto foundSource = m_sourceUUIDToSourceNameMap.find(guid);
            if (foundSource != m_sourceUUIDToSourceNameMap.end())
            {
                const SourceInfo& sourceInfo = foundSource->second;

                watchFolder = sourceInfo.m_watchFolder.toStdString().c_str();
                relativePath = sourceInfo.m_sourceName.toStdString().c_str();

                return true;
            }

            AZ_TracePrintf(AssetProcessor::DebugChannel, "GetQueuedAssetInfoById: AssetCatalog unable to find the requested source asset having uuid (%s).\n", guid.ToString<AZStd::string>().c_str());
        }

        return false;
    }


    bool AssetCatalog::GetQueuedAssetInfoByRelativeSourceName(const char* sourceName, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder)
    {
        if (sourceName)
        {
            AZStd::string sourceNameForMap = sourceName;
            AZStd::to_lower(sourceNameForMap.begin(), sourceNameForMap.end());
            AZStd::lock_guard<AZStd::mutex> lock(m_sourceUUIDToSourceNameMapMutex);

            auto foundSourceUUID = m_sourceNameToSourceUUIDMap.find(sourceNameForMap);
            if (foundSourceUUID != m_sourceNameToSourceUUIDMap.end())
            {
                auto foundSource = m_sourceUUIDToSourceNameMap.find(foundSourceUUID->second);
                if (foundSource != m_sourceUUIDToSourceNameMap.end())
                {
                    const SourceInfo& sourceInfo = foundSource->second;

                    watchFolder = sourceInfo.m_watchFolder.toStdString().c_str();

                    AZStd::string sourceNameStr(sourceInfo.m_sourceName.toStdString().c_str());
                    
                    // the Scan Folder may have an output prefix, so we must remove it from the relpath.
                    // this involves deleting the output prefix and the first slash ('editor/') which is why
                    // we remove the length of outputPrefix and one extra character.
                    const AssetProcessor::ScanFolderInfo* info = m_platformConfig->GetScanFolderByPath(watchFolder.c_str());
                    if ((info)&&(!info->GetOutputPrefix().isEmpty()))
                    {
                        assetInfo.m_relativePath = sourceNameStr.substr(static_cast<int>(info->GetOutputPrefix().length()) + 1);
                    }
                    else
                    {
                        assetInfo.m_relativePath.swap(sourceNameStr);
                    }

                    assetInfo.m_assetId = foundSource->first;

                    AZStd::string sourceFileFullPath;
                    AzFramework::StringFunc::Path::Join(watchFolder.c_str(), assetInfo.m_relativePath.c_str(), sourceFileFullPath);
                    assetInfo.m_sizeBytes = AZ::IO::SystemFile::Length(sourceFileFullPath.c_str());
                    
                    assetInfo.m_assetType = AZ::Uuid::CreateNull(); // most source files don't have a type!

                    // Go through the list of source assets and see if this asset's file path matches any of the filters
                    for (const auto& pair : m_sourceAssetTypeFilters)
                    {
                        if (AZStd::wildcard_match(pair.first, assetInfo.m_relativePath))
                        {
                            assetInfo.m_assetType = pair.second;
                            break;
                        }
                    }

                    return true;
                }
            }
        }
        assetInfo.m_assetId.SetInvalid();

        return false;
    }

    bool AssetCatalog::GetUncachedSourceInfoFromDatabaseNameAndWatchFolder(const char* sourceDatabaseName, const char* watchFolder, AZ::Data::AssetInfo& assetInfo)
    {
        AZ::Uuid sourceUUID = AssetUtilities::CreateSafeSourceUUIDFromName(sourceDatabaseName);
        if (sourceUUID.IsNull())
        {
            return false;
        }

        AZ::Data::AssetId sourceAssetId(sourceUUID, 0);
        assetInfo.m_assetId = sourceAssetId;

        // If relative path starts with the output prefix then remove it first
        const ScanFolderInfo* scanFolderInfo = m_platformConfig->GetScanFolderForFile(watchFolder);
        if (!scanFolderInfo)
        {
            return false;
        }
        QString databasePath = QString::fromUtf8(sourceDatabaseName);
        if (!scanFolderInfo->GetOutputPrefix().isEmpty() && databasePath.startsWith(scanFolderInfo->GetOutputPrefix(), Qt::CaseInsensitive))
        {
            QString relativePath = databasePath.right(databasePath.length() - scanFolderInfo->GetOutputPrefix().length() - 1); // also eat the slash, hence -1
            assetInfo.m_relativePath = relativePath.toUtf8().data();
        }
        else
        {
            assetInfo.m_relativePath = sourceDatabaseName;
        }

        AZStd::string absolutePath;
        AzFramework::StringFunc::Path::Join(watchFolder, assetInfo.m_relativePath.c_str(), absolutePath);
        assetInfo.m_sizeBytes = AZ::IO::SystemFile::Length(absolutePath.c_str());
        // Make sure the source file exists
        if (assetInfo.m_sizeBytes == 0 && !AZ::IO::SystemFile::Exists(absolutePath.c_str()))
        {
            return false;
        }

        assetInfo.m_assetType = AZ::Uuid::CreateNull();

        // Go through the list of source assets and see if this asset's file path matches any of the filters
        for (const auto& pair : m_sourceAssetTypeFilters)
        {
            if (AZStd::wildcard_match(pair.first, assetInfo.m_relativePath))
            {
                assetInfo.m_assetType = pair.second;
                break;
            }
        }

        return true;
    }

    bool AssetCatalog::ConnectToDatabase()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_databaseMutex);

        if (!m_db)
        {
            AZStd::string databaseLocation;
            AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Broadcast(&AzToolsFramework::AssetDatabase::AssetDatabaseRequests::GetAssetDatabaseLocation, databaseLocation);

            if (!databaseLocation.empty())
            {
                m_db = AZStd::make_unique<AssetProcessor::AssetDatabaseConnection>();
                m_db->OpenDatabase();

                return true;
            }

            return false;
        }

        return true;
    }

    void AssetCatalog::AsyncAssetCatalogStatusRequest()
    {
        if (m_catalogIsDirty)
        {
            Q_EMIT AsyncAssetCatalogStatusResponse(AssetCatalogStatus::RequiresSaving);
        }
        else
        {
            Q_EMIT AsyncAssetCatalogStatusResponse(AssetCatalogStatus::UpToDate);
        }
    }

}//namespace AssetProcessor

#include <native/AssetManager/AssetCatalog.moc>
