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

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/Asset/AssetSystemBus.h>

#include "AssetCatalogBus.h"

namespace AzFramework
{
    class AssetRegistry;
    class AssetBundleManifest;

    /*
     * Implements an asset catalog that populates data by scanning an asset root.
     */
    class AssetCatalog 
        : public AZ::Data::AssetCatalog
        , public AZ::Data::AssetCatalogRequestBus::Handler
        , private AssetSystemBus::Handler
    {
    public:

        AZ_TYPE_INFO(AssetCatalog, "{D80BAFE6-0391-4D40-9C76-1E63D2D7C64F}");
        AZ_CLASS_ALLOCATOR(AssetCatalog, AZ::SystemAllocator, 0)

        AssetCatalog();
        ~AssetCatalog() override;

        explicit AssetCatalog(bool useRequestBus);

        /// Wipe and reset the catalog.
        void Reset();

        /// Initialize the catalog for the current asset root.
        /// \param catalogRegistryFile - Optionally a previously saved catalog from which to load, rather than scanning.
        void InitializeCatalog(const char* catalogRegistryFile = nullptr);

        //////////////////////////////////////////////////////////////////////////
        // AssetCatalogRequestBus
        void EnableCatalogForAsset(const AZ::Data::AssetType& assetType) override;
        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override;
        void DisableCatalog() override;
        void StartMonitoringAssets() override;
        void StopMonitoringAssets() override;
        bool LoadCatalog(const char* catalogRegistryFile) override;
        void ClearCatalog() override;

        bool SaveCatalog(const char* catalogRegistryFile) override;
        static bool SaveCatalog(const char* catalogRegistryFile, AzFramework::AssetRegistry* catalogRegistry);
        bool AddDeltaCatalog(AZStd::shared_ptr<AzFramework::AssetRegistry> deltaCatalog) override;
        bool InsertDeltaCatalog(AZStd::shared_ptr<AzFramework::AssetRegistry> deltaCatalog, size_t slotNum) override;
        bool InsertDeltaCatalogBefore(AZStd::shared_ptr<AzFramework::AssetRegistry> deltaCatalog, AZStd::shared_ptr<AzFramework::AssetRegistry> afterDeltaCatalog) override;
        bool RemoveDeltaCatalog(AZStd::shared_ptr<AzFramework::AssetRegistry> deltaCatalog) override;
        static bool SaveAssetBundleManifest(const char* assetBundleManifestFile, AzFramework::AssetBundleManifest* bundleManifest);
        bool CreateBundleManifest(const AZStd::string& deltaCatalogPath, const AZStd::vector<AZStd::string>& dependentBundleNames, const AZStd::string& fileDirectory, int bundleVersion) override;
        bool CreateDeltaCatalog(const AZStd::vector<AZStd::string>& files, const AZStd::string& filePath) override;
        void AddExtension(const char* extension) override;

        void RegisterAsset(const AZ::Data::AssetId& id, AZ::Data::AssetInfo& info) override;
        void UnregisterAsset(const AZ::Data::AssetId& id) override;

        AZStd::string GetAssetPathById(const AZ::Data::AssetId& id) override;
        AZ::Data::AssetInfo GetAssetInfoById(const AZ::Data::AssetId& id) override;
        AZ::Data::AssetId GetAssetIdByPath(const char* path, const AZ::Data::AssetType& typeToRegister, bool autoRegisterIfNotFound) override;
        AZStd::vector<AZStd::string> GetRegisteredAssetPaths() override;
        AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> GetDirectProductDependencies(const AZ::Data::AssetId& asset) override;
        AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> GetAllProductDependencies(const AZ::Data::AssetId& asset) override;

        AZ::Data::AssetId GenerateAssetIdTEMP(const char* /*path*/) override;
        void EnumerateAssets(BeginAssetEnumerationCB beginCB, AssetEnumerationCB enumerateCB, EndAssetEnumerationCB endCB) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetCatalog
        AZ::Data::AssetStreamInfo GetStreamInfoForLoad(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType) override;
        AZ::Data::AssetStreamInfo GetStreamInfoForSave(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AssetSystemBus
        void AssetChanged(AzFramework::AssetSystem::AssetNotificationMessage message) override;
        void AssetRemoved(AzFramework::AssetSystem::AssetNotificationMessage message) override;
        //////////////////////////////////////////////////////////////////////////

        static AZStd::shared_ptr<AzFramework::AssetRegistry> LoadCatalogFromFile(const char* catalogFile);
    protected:

        /// \return true if the specified filename's extension matches those handled
        /// by the catalog.
        bool IsTrackedAssetType(const char* assetFilename) const;

        /// Helper function that adds all of searchAssetId's dependencies to the depedencySet/List (leaving out ones that are already in the list)
        void AddAssetDependencies(const AZ::Data::AssetId& searchAssetId, AZStd::unordered_set<AZ::Data::AssetId>& assetSet, AZStd::vector<AZ::Data::ProductDependency>& dependencyList);
        // Called by LoadCatalog to load the base
        bool LoadBaseCatalogInternal();
        // Called by RemoveDeltaCatalog - reassemble our registry from loaded catalog files
        bool ReloadCatalogs();
        // Add a specific name to the list and check for duplicates
        void AddCatalogEntry(AZStd::shared_ptr<AzFramework::AssetRegistry> deltaCatalog);
        // Apply specific catalog to registry by name
        bool ApplyDeltaCatalog(AZStd::shared_ptr<AzFramework::AssetRegistry> deltaCatalog);
        // Insert new catalog by position
        void InsertCatalogEntry(AZStd::shared_ptr<AzFramework::AssetRegistry> deltaCatalog, size_t catalogIndex);
        // Clear just the registry
        void ResetRegistry();

    private:

        AZStd::atomic_bool m_shutdownThreadSignal;                  ///< Signals the monitoring thread to stop.
        AZStd::thread m_thread;                                     ///< Monitoring thread
        AZStd::string m_assetRoot;                                  ///< Asset root the catalog is bound to.
        AZStd::unordered_set<AZStd::string> m_extensions;           ///< Valid asset extensions.
        mutable AZStd::recursive_mutex m_registryMutex;
        AZStd::unique_ptr<AssetRegistry> m_registry;
        AZStd::string m_pathBuffer;
        mutable AZStd::recursive_mutex m_baseCatalogNameMutex;
        AZStd::string m_baseCatalogName;
        mutable AZStd::recursive_mutex m_deltaCatalogMutex;
        AZStd::vector<AZStd::shared_ptr<AzFramework::AssetRegistry>> m_deltaCatalogList;
        bool m_useRequestBus{ true };
    };

} // namespace AzFramework
