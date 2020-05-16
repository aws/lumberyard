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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string_view.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Network/SocketConnection.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

namespace AzFramework
{
    namespace AssetSystem
    {
        constexpr char BranchToken[] = "assetProcessor_branch_token";
        constexpr char ProjectName[] = "sys_game_folder";
        constexpr char Assets[] = "assets";
        constexpr char AssetProcessorRemoteIp[] = "remote_ip";
        constexpr char AssetProcessorRemotePort[] = "remote_port";

        /**
        * A game level component for interacting with the asset processor
        *
        * Currently used to request synchronous asset compilation, provide notifications
        * when assets are updated, and to query asset status
        */
        class AssetSystemComponent
            : public AZ::Component
            , private AssetSystemRequestBus::Handler
            , private AZ::SystemTickBus::Handler
        {
        public:
            AZ_COMPONENT(AssetSystemComponent, "{42C58BBF-0C15-4DF9-9351-4639B36F122A}")

            AssetSystemComponent() = default;
            virtual ~AssetSystemComponent() = default;

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component overrides
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // SystemTickBus overrides
            void OnSystemTick() override;
            //////////////////////////////////////////////////////////////////////////

            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            
            AZStd::string AssetProcessorConnectionAddress();
            AZStd::string AssetProcessorBranchToken();
            AZStd::string AssetProcessorPlatform();
            AZ::u16 AssetProcessorPort();

        private:
            AssetSystemComponent(const AssetSystemComponent&) = delete;
            void EnableSocketConnection();
            void DisableSocketConnection();

            //////////////////////////////////////////////////////////////////////////
            // AssetSystemRequestBus::Handler overrides
            bool ConfigureSocketConnection(const AZStd::string& branch, const AZStd::string& platform, const AZStd::string& identifier, const AZStd::string& projectName) override;
            bool Connect(const char* identifier) override;            
            bool ConnectWithTimeout(const char* identifier, AZStd::chrono::duration<float> timeout) override;
            bool Disconnect() override;
            
            AssetStatus CompileAssetSync(const AZStd::string& assetPath) override;
            AssetStatus CompileAssetSync_FlushIO(const AZStd::string& assetPath) override;

            AssetStatus CompileAssetSyncById(const AZ::Data::AssetId& assetId) override;
            AssetStatus CompileAssetSyncById_FlushIO(const AZ::Data::AssetId& assetId) override;
            
            AssetStatus GetAssetStatus(const AZStd::string& assetPath) override;
            AssetStatus GetAssetStatus_FlushIO(const AZStd::string& assetPath) override;
            
            AssetStatus GetAssetStatusById(const AZ::Data::AssetId& assetId) override;
            AssetStatus GetAssetStatusById_FlushIO(const AZ::Data::AssetId& assetId) override;

            void GetUnresolvedProductReferences(AZ::Data::AssetId assetId, AZ::u32& unresolvedAssetIdReferences, AZ::u32& unresolvedPathReferences) override;
        
            void DetermineAssetsPlatform();

            bool EscalateAssetByUuid(const AZ::Uuid& assetUuid) override;
            bool EscalateAssetBySearchTerm(AZStd::string_view searchTerm) override;

            void ShowAssetProcessor() override;
            void ShowInAssetProcessor(const AZStd::string& assetPath) override;
            float GetAssetProcessorPingTimeMilliseconds() override;
            void SetAssetProcessorPort(AZ::u16 port) override;
            void SetBranchToken(const AZStd::string& branchtoken) override;
            void SetProjectName(const AZStd::string& projectName) override;
            bool SaveCatalog() override;
            void SetAssetProcessorIP(const AZStd::string& ip) override;
            //////////////////////////////////////////////////////////////////////////

            AssetStatus SendAssetStatusRequest(const RequestAssetStatus& request);

            bool RequestFromBootstrapReader(const char* key, AZStd::string& value, bool checkPlatform);

            AZStd::unique_ptr<SocketConnection> m_socketConn = nullptr;
            SocketConnection::TMessageCallbackHandle m_cbHandle = 0;
            AZStd::string m_branchToken;
            AZStd::string m_projectName;
            AZStd::string m_platform;
            AZStd::string m_assetProcessorIP = AZStd::string("127.0.0.1");
            AZ::u16 m_assetProcessorPort = 45643;
        };
    } // namespace AssetSystem
} // namespace AzFramework

