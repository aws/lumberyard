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
#include <AzFramework/Asset/AssetRegistry.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>

#include <QElapsedTimer>
#include <QMutexLocker>
#include <AzCore/Math/Crc.h>

namespace AssetProcessor
{
    AssetCatalog::AssetCatalog(QObject* parent, QStringList platforms, AZStd::shared_ptr<AssetDatabaseConnection> db)
        : QObject(parent)
        , m_platforms(platforms)
        , m_db(db)
        , m_registryBuiltOnce(false)
    {
        bool computedCacheRoot = AssetUtilities::ComputeProjectCacheRoot(m_cacheRoot);
        AZ_Assert(computedCacheRoot, "Could not compute cache root for AssetCatalog");

        // save 30mb for this.  Really large projects do get this big (and bigger)
        // if you don't do this, things get fragmented very fast.
        m_saveBuffer.reserve(1024 * 1024 * 30); 
        
        BuildRegistry();
        m_registryBuiltOnce = true;
        AssetRegistryRequestBus::Handler::BusConnect();
    }

    AssetCatalog::~AssetCatalog()
    {
        AssetRegistryRequestBus::Handler::BusDisconnect();
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
            m_registries[platform].RegisterAsset(assetInfo.m_assetId, assetInfo);
            for (const AZ::Data::AssetId& mapping : message.m_legacyAssetIds)
            {
                if (mapping != assetInfo.m_assetId)
                {
                    m_registries[platform].RegisterLegacyAssetMapping(mapping, assetInfo.m_assetId);
                }
            }
            
            if (m_registryBuiltOnce)
            {
                Q_EMIT SendAssetMessage(platform, message);
            }
        }
        else if (message.m_type == AssetNotificationMessage::AssetRemoved)
        {
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
        QMutexLocker locker(&m_savingRegistryMutex);
        
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
                objStream->WriteClass(&m_registries[platform]);
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
                    QString actualRegistryFile = QString("%1/%2/%3").arg(m_cacheRoot.absoluteFilePath(platform)).arg(AssetUtilities::ComputeGameName().toLower()).arg("assetcatalog.xml");

                    AZ_TracePrintf(AssetProcessor::DebugChannel, "Creating asset catalog: %s --> %s\n", tempRegistryFile.toUtf8().constData(), actualRegistryFile.toUtf8().constData());
                    AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
                    if (AZ::IO::FileIOBase::GetInstance()->Open(tempRegistryFile.toUtf8().data(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary, fileHandle))
                    {
                        AZ::IO::FileIOBase::GetInstance()->Write(fileHandle, m_saveBuffer.data(), m_saveBuffer.size());
                        AZ::IO::FileIOBase::GetInstance()->Close(fileHandle);

                        // if we succeeded in doing this, then use "rename" to move the file over the previous copy.
                        bool moved = AZ::IO::SystemFile::Rename(tempRegistryFile.toUtf8().constData(), actualRegistryFile.toUtf8().constData(), true);

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
                    }

                    AZ::IO::FileIOBase::GetInstance()->DestroyPath(workSpace.toUtf8().data());
                }
            }
        }

        m_currentlySavingCatalog = false;
        AssetRegistryNotificationBus::Broadcast(&AssetRegistryNotifications::OnRegistrySaveComplete, m_currentRegistrySaveVersion);
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
                    info.m_assetType = combined.m_assetType;
                    info.m_sizeBytes = AZ::IO::SystemFile::Length(fullProductPath.toUtf8().constData());

                    // also register it at the legacy id(s) if its different:
                    AZ::Data::AssetId legacyAssetId(combined.m_legacyGuid, 0);
                    AZ::Data::AssetId legacySourceAssetId(AssetUtilities::CreateSafeSourceUUIDFromName(combined.m_sourceName.c_str(), false), combined.m_subID);

                    m_registries[platform].RegisterAsset(assetId, info);

                    if (legacyAssetId != assetId)
                    {
                        m_registries[platform].RegisterLegacyAssetMapping(legacyAssetId, assetId);
                    }

                    if (legacySourceAssetId != assetId)
                    {
                        m_registries[platform].RegisterLegacyAssetMapping(legacySourceAssetId, assetId);
                    }

                    return true;//see them all
                };

            m_db->QueryCombined(
                databaseQueryCallback, AZ::Uuid::CreateNull(),
                nullptr,
                platform.toUtf8().constData(),
                AzToolsFramework::AssetSystem::JobStatus::Any);

            AZ_TracePrintf("Catalog", "Read %u assets from database for %s in %fs\n", currentRegistry.m_assetIdToInfo.size(), platform.toUtf8().constData(), timer.elapsed() / 1000.0f);
        }
    }
}//namespace AssetProcessor

#include <native/AssetManager/AssetCatalog.moc>
