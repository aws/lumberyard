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

#include "AssetCatalog.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/Streamer.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/AssetRegistry.h>

// uncomment to have the catalog be dumped to stdout:
//#define DEBUG_DUMP_CATALOG

namespace AzFramework
{
    //=========================================================================
    // AssetCatalog ctor
    //=========================================================================
    AssetCatalog::AssetCatalog()
        : m_shutdownThreadSignal(false)
        , m_registry(aznew AssetRegistry())
    {
        AZ::Data::AssetCatalogRequestBus::Handler::BusConnect();
    }

    //=========================================================================
    // AssetCatalog dtor
    //=========================================================================
    AssetCatalog::~AssetCatalog()
    {
        DisableCatalog();
        Reset();
    }

    //=========================================================================
    // Reset
    //=========================================================================
    void AssetCatalog::Reset()
    {
        StopMonitoringAssets();

        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);
        m_registry->Clear();
    }

    //=========================================================================
    // EnableCatalog
    //=========================================================================
    void AssetCatalog::EnableCatalogForAsset(const AZ::Data::AssetType& assetType)
    {
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager is not ready.");
        AZ::Data::AssetManager::Instance().RegisterCatalog(this, assetType);
    }

    //=========================================================================
    // GetHandledAssetTypes
    //=========================================================================
    void AssetCatalog::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager is not ready.");
        AZ::Data::AssetManager::Instance().GetHandledAssetTypes(this, assetTypes);
    }

    //=========================================================================
    // DisableCatalog
    //=========================================================================
    void AssetCatalog::DisableCatalog()
    {
        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterCatalog(this);
        }
    }

    //=========================================================================
    // AddExtension
    //=========================================================================
    void AssetCatalog::AddExtension(const char* extension)
    {
        AZStd::string extensionFixed = extension;
        EBUS_EVENT(ApplicationRequests::Bus, NormalizePath, extensionFixed);

        if (extensionFixed.empty())
        {
            AZ_Error("AssetCatalog", false, "Invalid extension provided: %s", extension);
            return;
        }

        if (extensionFixed[0] == '.')
        {
            extensionFixed = extensionFixed.substr(1);
        }

        EBUS_EVENT(ApplicationRequests::Bus, NormalizePath, extensionFixed);

        if (m_extensions.end() == AZStd::find(m_extensions.begin(), m_extensions.end(), extensionFixed))
        {
            m_extensions.insert(AZStd::move(extensionFixed));
        }
    }

    //=========================================================================
    // GetAssetPathById
    //=========================================================================
    AZStd::string AssetCatalog::GetAssetPathById(const AZ::Data::AssetId& id)
    {
        if (!id.IsValid())
        {
            return AZStd::string();
        }

        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);

        auto foundIter = m_registry->m_assetIdToInfo.find(id);
        if (foundIter != m_registry->m_assetIdToInfo.end())
        {
            return foundIter->second.m_relativePath;
        }

        // we did not find it - try the backup mapping!
        AZ::Data::AssetId legacyMapping = m_registry->GetAssetIdByLegacyAssetId(id);
        if (legacyMapping.IsValid())
        {
            return GetAssetPathById(legacyMapping);
        }

        return AZStd::string();
    }

    //=========================================================================
    // GetAssetInfoById
    //=========================================================================
    AZ::Data::AssetInfo AssetCatalog::GetAssetInfoById(const AZ::Data::AssetId& id)
    {
        if (!id.IsValid())
        {
            return AZ::Data::AssetInfo();
        }

        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);

        auto foundIter = m_registry->m_assetIdToInfo.find(id);
        if (foundIter != m_registry->m_assetIdToInfo.end())
        {
            return foundIter->second;
        }

        // we did not find it - try the backup mapping!
        AZ::Data::AssetId legacyMapping = m_registry->GetAssetIdByLegacyAssetId(id);
        if (legacyMapping.IsValid())
        {
            return GetAssetInfoById(legacyMapping);
        }

        return AZ::Data::AssetInfo();
    }

    //=========================================================================
    // GetAssetIdByPath
    //=========================================================================
    AZ::Data::AssetId AssetCatalog::GetAssetIdByPath(const char* path, const AZ::Data::AssetType& typeToRegister, bool autoRegisterIfNotFound)
    {
        if (!path || !path[0])
        {
            return AZ::Data::AssetId();
        }
        m_pathBuffer = path;
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, MakePathAssetRootRelative, m_pathBuffer);
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);

            AZ::Data::AssetId foundId = m_registry->GetAssetIdByPath(m_pathBuffer.c_str());
            if (foundId.IsValid())
            {
                const AZ::Data::AssetInfo& assetInfo = m_registry->m_assetIdToInfo.find(foundId)->second;

                // If the type is already registered, but with no valid type, allow it to be re-registered.
                // Otherwise, return the Id.
                if (!autoRegisterIfNotFound || !assetInfo.m_assetType.IsNull())
                {
                    return foundId;
                }
            }
        }

        if (autoRegisterIfNotFound)
        {
            AZ_Error("AssetCatalog", typeToRegister != AZ::Data::s_invalidAssetType,
                "Invalid asset type provided for registration of asset \"%s\".", m_pathBuffer.c_str());

            // note, we are intentionally allowing missing files, since this is an explicit ask to add.
            AZ::u64 fileSize = 0;
            auto* fileIo = AZ::IO::FileIOBase::GetInstance();
            if (fileIo->Exists(m_pathBuffer.c_str()))
            {
                fileIo->Size(m_pathBuffer.c_str(), fileSize);
            }

            AZ::Data::AssetInfo newInfo;
            newInfo.m_relativePath = m_pathBuffer;
            newInfo.m_assetType = typeToRegister;
            newInfo.m_sizeBytes = fileSize;
            AZ::Data::AssetId generatedID = GenerateAssetIdTEMP(newInfo.m_relativePath.c_str());
            newInfo.m_assetId = generatedID;

            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);
                m_registry->RegisterAsset(generatedID, newInfo);
            }

            EBUS_EVENT(AzFramework::AssetCatalogEventBus, OnCatalogAssetAdded, generatedID);

            return generatedID;
        }

        return AZ::Data::AssetId();
    }

    AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> AssetCatalog::GetDirectProductDependencies(const AZ::Data::AssetId& id)
    {
        auto itr = m_registry->m_assetDependencies.find(id);

        if (itr == m_registry->m_assetDependencies.end())
        {
            return AZ::Failure<AZStd::string>("Failed to find asset in dependency map");
        }

        return AZ::Success(itr->second);
    }
    
    AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> AssetCatalog::GetAllProductDependencies(const AZ::Data::AssetId& id)
    {
        AZStd::vector<AZ::Data::ProductDependency> dependencyList;
        AZStd::unordered_set<AZ::Data::AssetId> assetSet;

        AddAssetDependencies(id, assetSet, dependencyList);

        // dependencyList will be appended to while looping, so use a traditional loop
        for(size_t i = 0; i < dependencyList.size(); ++i)
        {
            AddAssetDependencies(dependencyList[i].m_assetId, assetSet, dependencyList);
        }

        return AZ::Success(AZStd::move(dependencyList));
    }

    void AssetCatalog::AddAssetDependencies(const AZ::Data::AssetId& searchAssetId, AZStd::unordered_set<AZ::Data::AssetId>& assetSet, AZStd::vector<AZ::Data::ProductDependency>& dependencyList)
    {
        using namespace AZ::Data;
        auto itr = m_registry->m_assetDependencies.find(searchAssetId);

        if (itr != m_registry->m_assetDependencies.end())
        {
            AZStd::vector<ProductDependency>& assetDependencyList = itr->second;

            for (const ProductDependency& dependency : assetDependencyList)
            {
                // Only proceed if we haven't encountered this assetId before
                if (assetSet.find(dependency.m_assetId) == assetSet.end())
                {
                    assetSet.insert(dependency.m_assetId); // add to the set of already-encountered assets
                    dependencyList.push_back(dependency); // put it in the flat list of dependencies we've found
                }
            }
        }
    }

    //=========================================================================
    // GenerateAssetIdTEMP
    //=========================================================================
    AZ::Data::AssetId AssetCatalog::GenerateAssetIdTEMP(const char* path)
    {
        if ((!path) || (path[0] == 0))
        {
            return AZ::Data::AssetId();
        }
        AZStd::string relativePath = path;
        EBUS_EVENT(ApplicationRequests::Bus, MakePathAssetRootRelative, relativePath);
        return AZ::Data::AssetId(AZ::Uuid::CreateName(relativePath.c_str()));
    }

    //=========================================================================
    // EnumerateAssets
    //=========================================================================
    void AssetCatalog::EnumerateAssets(BeginAssetEnumerationCB beginCB, AssetEnumerationCB enumerateCB, EndAssetEnumerationCB endCB)
    {
        if (beginCB)
        {
            beginCB();
        }

        if (enumerateCB)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);

            for (auto& it : m_registry->m_assetIdToInfo)
            {
                enumerateCB(it.first, it.second);
            }
        }

        if (endCB)
        {
            endCB();
        }
    }

    //=========================================================================
    // InitializeCatalog
    //=========================================================================
    void AssetCatalog::InitializeCatalog(const char* catalogRegistryFile /*= nullptr*/)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);

        AZ_Warning("AssetCatalog", m_registry->m_assetIdToInfo.size() == 0, "Catalog reset will erase %u assets", m_registry->m_assetIdToInfo.size());

        // Get asset root from application.
        EBUS_EVENT_RESULT(m_assetRoot, AzFramework::ApplicationRequests::Bus, GetAssetRoot);

        // Reflect registry for serialization.
        AZ::SerializeContext* serializeContext = nullptr;
        EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(serializeContext, "Unable to retrieve serialize context.");
        if (nullptr == serializeContext->FindClassData(AZ::AzTypeInfo<AssetRegistry>::Uuid()))
        {
            AssetRegistry::ReflectSerialize(serializeContext);
        }

        AZ_TracePrintf("AssetCatalog",
            "\n========================================================\n"
            "Initializing asset catalog with root \"%s\"."
            "\n========================================================\n",
            m_assetRoot.c_str());

        // even though this could be a chunk of memory to allocate and deallocate, this is many times faster and more efficient
        // in terms of memory AND fragmentation than allowing it to perform thousands of reads on physical media.
        AZStd::vector<char> bytes;
        if (catalogRegistryFile && AZ::IO::FileIOBase::GetInstance())
        {
            AZ::IO::HandleType handle = AZ::IO::InvalidHandle;
            AZ::u64 size = 0;
            AZ::IO::FileIOBase::GetInstance()->Size(catalogRegistryFile, size);

            if (size)
            {
                if (AZ::IO::FileIOBase::GetInstance()->Open(catalogRegistryFile, AZ::IO::OpenMode::ModeRead, handle))
                {
                    bytes.resize_no_construct(size);
                    // this call will fail on purpose if bytes.size() != size successfully actually read from disk.
                    if (!AZ::IO::FileIOBase::GetInstance()->Read(handle, bytes.data(), bytes.size(), true))
                    {
                        AZ_Error("AssetCatalog", false, "File %s failed read - read was truncated!", catalogRegistryFile);
                        bytes.set_capacity(0);
                    }
                    AZ::IO::FileIOBase::GetInstance()->Close(handle);
                }
            }
        }

        if (!bytes.empty())
        {
            AZ::IO::MemoryStream catalogStream(bytes.data(), bytes.size());
            AZ::Utils::LoadObjectFromStreamInPlace<AzFramework::AssetRegistry>(catalogStream, *m_registry.get(), serializeContext, AZ::ObjectStream::FilterDescriptor(&AZ::Data::AssetFilterNoAssetLoading));

            AZ_TracePrintf("AssetCatalog",
                "\n========================================================\n"
                "Loaded registry containing %u assets.\n"
                "========================================================\n",
                m_registry->m_assetIdToInfo.size());

            AssetCatalogEventBus::Broadcast(&AssetCatalogEventBus::Events::OnCatalogLoaded, catalogRegistryFile);
        }
        else
        {
            AZ_ErrorOnce("AssetCatalog", false, "Unable to load the asset catalog from %s!", catalogRegistryFile);
        }
    }

    //=========================================================================
    // IsTrackedAssetType
    //=========================================================================
    bool AssetCatalog::IsTrackedAssetType(const char* assetFilename) const
    {
        AZStd::string assetExtension;
        AzFramework::StringFunc::Path::GetExtension(assetFilename, assetExtension, false);

        if (assetExtension.empty())
        {
            return false;
        }

        return (m_extensions.find(assetExtension) != m_extensions.end());
    }

    //=========================================================================
    // RegisterAsset
    //=========================================================================
    void AssetCatalog::RegisterAsset(const AZ::Data::AssetId& id, AZ::Data::AssetInfo& info)
    {
        AZ_Error("AssetCatalog", id != AZ::Data::s_invalidAssetType, "Registering asset \"%s\" with invalid type.", info.m_relativePath.c_str());

        info.m_assetId = id;
        if (info.m_sizeBytes == 0)
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            fileIO->Size(info.m_relativePath.c_str(), info.m_sizeBytes);
        }
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);
            m_registry->RegisterAsset(id, info);
        }
        EBUS_EVENT(AzFramework::AssetCatalogEventBus, OnCatalogAssetAdded, id);
    }

    //=========================================================================
    // UnregisterAsset
    //=========================================================================
    void AssetCatalog::UnregisterAsset(const AZ::Data::AssetId& assetId)
    {
        if (assetId.IsValid())
        {
            // Must notify listeners first, in case they need lookups based on this assetId
            EBUS_EVENT(AzFramework::AssetCatalogEventBus, OnCatalogAssetRemoved, assetId);

            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);
            m_registry->UnregisterAsset(assetId);
        }
    }

    //=========================================================================
    // GetStreamInfoForLoad
    //=========================================================================
    AZ::Data::AssetStreamInfo AssetCatalog::GetStreamInfoForLoad(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType)
    {
        (void)assetType;

        AZ::Data::AssetInfo info;
        {
            // narrow-scoped lock.
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);
            info = GetAssetInfoById(assetId);
        }

        if (!info.m_relativePath.empty())
        {
            AZ::Data::AssetStreamInfo streamInfo;
            streamInfo.m_dataLen = info.m_sizeBytes;

            // All asset handlers are presently required to use the LoadAssetData override
            // that takes a path, in order to pipe the IO through FileIO / VFS.
            streamInfo.m_isCustomStreamType = true;
            streamInfo.m_streamName = info.m_relativePath;

            return streamInfo;
        }

        return AZ::Data::AssetStreamInfo();
    }

    //=========================================================================
    // GetStreamInfoForSave
    //=========================================================================
    AZ::Data::AssetStreamInfo AssetCatalog::GetStreamInfoForSave(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType)
    {
        (void)assetType;

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (fileIO)
        {
            AZ::Data::AssetInfo info;
            {
                // narrow-scoped lock.
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);
                info = GetAssetInfoById(assetId);
            }


            if (!info.m_relativePath.empty())
            {
                const char* devAssetRoot = fileIO->GetAlias("@devassets@");
                if (devAssetRoot)
                {
                    AZ::Data::AssetStreamInfo streamInfo;
                    streamInfo.m_dataLen = info.m_sizeBytes;
                    streamInfo.m_streamFlags = AZ::IO::OpenMode::ModeWrite;
                    streamInfo.m_isCustomStreamType = true;
                    streamInfo.m_streamName = AZStd::string::format("%s/%s", devAssetRoot, info.m_relativePath.c_str());
                    return streamInfo;
                }
            }
        }

        return AZ::Data::AssetStreamInfo();
    }

    //=========================================================================
    // StartMonitoringAssetRoot
    //=========================================================================
    void AssetCatalog::StartMonitoringAssets()
    {
        AssetSystemBus::Handler::BusConnect();
    }

    //=========================================================================
    // StopMonitoringAssetRoot
    //=========================================================================
    void AssetCatalog::StopMonitoringAssets()
    {
        AssetSystemBus::Handler::BusDisconnect();
    }

    //=========================================================================
    // AssetSystemBus::AssetChanged
    //=========================================================================
    void AssetCatalog::AssetChanged(AzFramework::AssetSystem::AssetNotificationMessage message)
    {
        AZStd::string relativePath = message.m_data;
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, MakePathAssetRootRelative, relativePath);

        AZ::Data::AssetId assetId = message.m_assetId;

        // Uncomment this when debugging change notifications
        //AZ_TracePrintf("AssetCatalog", "AssetChanged: path=%s id=%s\n", relativePath.c_str(), assetId.ToString<AZStd::string>().c_str());
        AZStd::string extension;
        AzFramework::StringFunc::Path::GetExtension(relativePath.c_str(), extension, false);
        if (assetId.IsValid())
        {
            // is it an add or a change?
            auto assetInfoPair = m_registry->m_assetIdToInfo.find(assetId);
            const bool isNewAsset = (assetInfoPair == m_registry->m_assetIdToInfo.end());

#if defined(AZ_ENABLE_TRACING)
            if (message.m_assetType == AZ::Data::s_invalidAssetType)
            {
                AZ_TracePrintf("AssetCatalog", "Registering asset \"%s\" via AssetSystem message, but type is not set.", relativePath.c_str());
            }
#endif

            const AZ::Data::AssetType& assetType = isNewAsset ? message.m_assetType : assetInfoPair->second.m_assetType;

            AZ::Data::AssetInfo newData;
            newData.m_assetId = assetId;
            newData.m_assetType = assetType;
            newData.m_relativePath = message.m_data;
            newData.m_sizeBytes = message.m_sizeBytes;

            m_registry->RegisterAsset(assetId, newData);
            m_registry->SetAssetDependencies(assetId, message.m_dependencies);

            for (const auto& mapping : message.m_legacyAssetIds)
            {
                m_registry->RegisterLegacyAssetMapping(mapping, assetId);
            }

            if (!isNewAsset)
            {
                EBUS_EVENT(AzFramework::AssetCatalogEventBus, OnCatalogAssetChanged, assetId);

                // in case someone has an ancient reference, notify on that too.
                for (const auto& mapping : message.m_legacyAssetIds)
                {
                    EBUS_EVENT(AzFramework::AssetCatalogEventBus, OnCatalogAssetChanged, mapping);
                }
            }
            else
            {
                EBUS_EVENT(AzFramework::AssetCatalogEventBus, OnCatalogAssetAdded, assetId);
                for (const auto& mapping : message.m_legacyAssetIds)
                {
                    EBUS_EVENT(AzFramework::AssetCatalogEventBus, OnCatalogAssetAdded, mapping);
                }
            }

            EBUS_QUEUE_EVENT_ID(AZ::Crc32(extension.c_str()), AzFramework::LegacyAssetEventBus, OnFileChanged, relativePath);
            AZ::Data::AssetManager::Instance().ReloadAsset(assetId);
        }
        else
        {
            AZ_TracePrintf("AssetCatalog", "AssetChanged: invalid asset id: %s", assetId.ToString<AZStd::string>().c_str());
        }
    }

    //=========================================================================
    // AssetSystemBus::AssetRemoved
    //=========================================================================
    void AssetCatalog::AssetRemoved(AzFramework::AssetSystem::AssetNotificationMessage message)
    {
        AZStd::string relativePath = message.m_data;
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, MakePathAssetRootRelative, relativePath);

        const AZ::Data::AssetId assetId = message.m_assetId;
        if (assetId.IsValid())
        {
            AZ_TracePrintf("AssetCatalog", "AssetRemoved: path=%s id=%s\n", relativePath.c_str(), assetId.ToString<AZStd::string>().c_str());
            AZStd::string extension;
            AzFramework::StringFunc::Path::GetExtension(relativePath.c_str(), extension, false);
            EBUS_QUEUE_EVENT_ID(AZ::Crc32(extension.c_str()), AzFramework::LegacyAssetEventBus, OnFileRemoved, relativePath);
            UnregisterAsset(assetId);
            for (const auto& mapping : message.m_legacyAssetIds)
            {
                m_registry->UnregisterLegacyAssetMapping(mapping);
            }
        }
        else
        {
            AZ_TracePrintf("AssetCatalog", "AssetRemoved: invalid asset id: %s", assetId.ToString<AZStd::string>().c_str());
        }
    }

    //=========================================================================
    // LoadCatalog
    //=========================================================================
    bool AssetCatalog::LoadCatalog(const char* catalogRegistryFile)
    {
        // right before we load the catalog, make sure you are listening for update events, so that you don't miss any in the gap 
        // that happens AFTER the catalog is saved but BEFORE you start monitoring them:
        StartMonitoringAssets();

        // it does not matter if this call fails - it will succeed if it can.  In release builds or builds where the user has turned off the
        // asset processing system because they have precompiled all assets, the call will fail but there will still be a valid catalog.
        // for this reason, we use AZ_Warning, as that is removed automatically from release builds.  We still show it as it may aid debugging
        // in the general case.
        bool catalogSavedOK = false;
        AzFramework::AssetSystemRequestBus::BroadcastResult(catalogSavedOK, &AzFramework::AssetSystem::AssetSystemRequests::SaveCatalog);
        AZ_WarningOnce("AssetCatalog", catalogSavedOK, "Asset catalog was not saved.  This is only okay if running with pre-built assets.");

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (fileIO && fileIO->Exists(catalogRegistryFile))
        {
            InitializeCatalog(catalogRegistryFile);

#if defined(DEBUG_DUMP_CATALOG)
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);

            for (auto& it : m_registry->m_assetIdToInfo)
            {
                AZ_TracePrintf("Asset Registry: AssetID->Info", "%s --> %s %llu bytes\n", it.first.ToString<AZStd::string>().c_str(), it.second.m_relativePath.c_str(), it.second.m_sizeBytes);
            }

#endif
            return true;
        }

        return false;
    }
} // namespace AzFramework
