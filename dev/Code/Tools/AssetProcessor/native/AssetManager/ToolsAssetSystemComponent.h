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
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <native/AssetDatabase/AssetDatabase.h>
#include <QDir>

namespace AssetProcessor
{
    //! AssetProcessor's replacement for the AssetCatalogComponent
    //! Services the AssetCatalogRequestBus by interfacing with the AssetProcessorManager
    class ToolsAssetSystemComponent :
        public AZ::Component,
        public AssetBuilderSDK::ToolsAssetSystemBus::Handler,
        public AZ::Data::AssetCatalogRequestBus::Handler,
        public AZ::Data::AssetCatalog
    {
    public:
        
        AZ_COMPONENT(ToolsAssetSystemComponent, "{AE68E46B-0E21-499A-8309-41408BCBE4BF}");

        ToolsAssetSystemComponent() = default;
        ~ToolsAssetSystemComponent() override = default;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services);

        void Activate() override;
        void Deactivate() override;

        // AssetCatalog overrides
        AZ::Data::AssetStreamInfo GetStreamInfoForLoad(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType) override;
        ////////////////////////////////////////////////////////////////////////////////

        // AssetCatalogRequestBus overrides
        void EnableCatalogForAsset(const AZ::Data::AssetType& assetType) override;
        void DisableCatalog() override;
        AZ::Data::AssetInfo GetAssetInfoById(const AZ::Data::AssetId& id) override;
        ////////////////////////////////////////////////////////////////////////////////

        // ToolsAssetSystemBus overrides
        void RegisterSourceAssetType(const AZ::Data::AssetType& assetType, const char* assetFileFilter) override;
        void UnregisterSourceAssetType(const AZ::Data::AssetType& assetType) override;
        ////////////////////////////////////////////////////////////////////////////////

    private:
#if defined(AZ_COMPILER_MSVC) && AZ_COMPILER_MSVC <= 1800
        // Workaround for VS2013 - Delete the copy constructor and make it private
        // https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
        ToolsAssetSystemComponent(const ToolsAssetSystemComponent&) = delete;
#endif

        //! Internal GetAssetInfo that optionally takes an asset type
        AZ::Data::AssetInfo GetAssetInfoByIdInternal(const AZ::Data::AssetId &id, const AZ::Data::AssetType& assetType, AZStd::string& rootFilePath);

        //! GetAssetInfo that tries to figure out if the asset is a product or source so it can return info about the product or source respectively
        AZ::Data::AssetInfo GetAssetInfoByIdOnly(const AZ::Data::AssetId& id, AZStd::string& rootFilePath);

        //! Gets the product AssetInfo
        AZ::Data::AssetInfo GetProductAssetInfo(const AZ::Data::AssetId &id, AZStd::string &rootFilePath);

        //! Gets the source file info for an Asset by checking the DB first and the APM queue second
        bool GetSourceFileInfoFromAssetId(const AZ::Data::AssetId &assetId, AZStd::string& watchFolder, AZStd::string& relativePath);

        //! Make sure we're connected to the DB, since we do a lazy connection
        bool CheckDatabaseConnection();

        //! List of AssetTypes that should return info for the source instead of the product
        AZStd::unordered_map<AZ::Data::AssetType, AZStd::string> m_sourceAssetTypes;

        //! Database connection, lazily connected since we don't know when the APM is ready
        AZStd::unique_ptr<AssetProcessor::AssetDatabaseConnection> m_assetDatabaseConnection;

        //! Used to protect access to the database connection, only one thread can use it at a time
        AZStd::mutex m_databaseMutex;

        //! Asset cache root directory
        QDir m_cacheRootDir;
    };
}
