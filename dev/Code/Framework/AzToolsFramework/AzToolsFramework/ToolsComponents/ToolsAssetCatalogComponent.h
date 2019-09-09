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

namespace AssetProcessor
{
    //! Tools replacement for the AssetCatalogComponent
    //! Services the AssetCatalogRequestBus by interfacing with the AssetProcessor over a network connection
    class ToolsAssetCatalogComponent :
        public AZ::Component,
        public AZ::Data::AssetCatalogRequestBus::Handler,
        public AZ::Data::AssetCatalog
    {
    public:
        
        AZ_COMPONENT(ToolsAssetCatalogComponent, "{AE68E46B-0E21-499A-8309-41408BCBE4BF}");

        ToolsAssetCatalogComponent() = default;
        ~ToolsAssetCatalogComponent() override = default;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services);

        void Activate() override;
        void Deactivate() override;

        // AssetCatalog overrides
        AZ::Data::AssetStreamInfo GetStreamInfoForLoad(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType) override;
        ////////////////////////////////////////////////////////////////////////////////

        // AssetCatalogRequestBus overrides
        AZStd::string GetAssetPathById(const AZ::Data::AssetId& id) override;
        AZ::Data::AssetId GetAssetIdByPath(const char* path, const AZ::Data::AssetType& typeToRegister, bool autoRegisterIfNotFound) override;
        void EnableCatalogForAsset(const AZ::Data::AssetType& assetType) override;
        void DisableCatalog() override;
        AZ::Data::AssetInfo GetAssetInfoById(const AZ::Data::AssetId& id) override;
        ////////////////////////////////////////////////////////////////////////////////

    private:
#if defined(AZ_COMPILER_MSVC) && AZ_COMPILER_MSVC <= 1800
        // Workaround for VS2013 - Delete the copy constructor and make it private
        // https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
        ToolsAssetCatalogComponent(const ToolsAssetCatalogComponent&) = delete;
#endif
    };
}
