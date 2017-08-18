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

    /*
     * Implements an asset catalog that populates data by scanning an asset root.
     */
    class AssetCatalog final
        : public AZ::Data::AssetCatalog
        , public AZ::Data::AssetCatalogRequestBus::Handler
        , private AssetSystemBus::Handler
    {
    public:

        AZ_TYPE_INFO(AssetCatalog, "{D80BAFE6-0391-4D40-9C76-1E63D2D7C64F}");
        AZ_CLASS_ALLOCATOR(AssetCatalog, AZ::SystemAllocator, 0)

        AssetCatalog();
        ~AssetCatalog() override;

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
        void AddExtension(const char* extension) override;
        
        void RegisterAsset(const AZ::Data::AssetId& id, AZ::Data::AssetInfo& info) override;
        void UnregisterAsset(const AZ::Data::AssetId& id) override;

        AZStd::string GetAssetPathById(const AZ::Data::AssetId& id) override;
        AZ::Data::AssetInfo GetAssetInfoById(const AZ::Data::AssetId& id) override;
        AZ::Data::AssetId GetAssetIdByPath(const char* path, const AZ::Data::AssetType& typeToRegister, bool autoRegisterIfNotFound) override;
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

    protected:

        /// \return true if the specified filename's extension matches those handled
        /// by the catalog.
        bool IsTrackedAssetType(const char* assetFilename) const;

    private:

        AZStd::atomic_bool m_shutdownThreadSignal;                  ///< Signals the monitoring thread to stop.
        AZStd::thread m_thread;                                     ///< Monitoring thread
        AZStd::string m_assetRoot;                                  ///< Asset root the catalog is bound to.
        AZStd::unordered_set<AZStd::string> m_extensions;           ///< Valid asset extensions.
        mutable AZStd::recursive_mutex m_registryMutex;

        AZStd::unique_ptr<AssetRegistry> m_registry;
        AZStd::string m_pathBuffer;
    };
} // namespace AzFramework
