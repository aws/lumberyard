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

#include <AzFramework/Asset/AssetSystemComponent.h>

#include <AzCore/std/string/conversions.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Debug/EventTrace.h>
#include <AzFramework/API/BootstrapReaderBus.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/Network/AssetProcessorConnection.h>

namespace AzFramework
{
    namespace AssetSystem
    {
        const char* const BOOTSTRAP_FILE = "bootstrap.cfg";
        const char* const BRANCH_TOKEN = "assetProcessor_branch_token";
        const char* const ASSETS = "assets";
        const char* const ASSET_PROCESSOR_REMOTE_IP = "remote_ip";
        const char* const ASSET_PROCESSOR_REMOTE_PORT = "remote_port";

        void OnAssetSystemMessage(unsigned int /*typeId*/, const void* buffer, unsigned int bufferSize)
        {
            AssetNotificationMessage message;
            // note that we forbid asset loading and we set STRICT mode.  These messages are all the kind of message that is supposed to be transmitted between the
            // same version of software, and are created at runtime, not loaded from disk, so they should not contain errors - if they do, it requires investigation.
            if (!AZ::Utils::LoadObjectFromBufferInPlace(buffer, bufferSize, message, nullptr, AZ::ObjectStream::FilterDescriptor(&AZ::Data::AssetFilterNoAssetLoading, AZ::ObjectStream::FILTERFLAG_STRICT)))
            {
                AZ_WarningOnce("AssetSystem", false, "AssetNotificationMessage received but unable to deserialize it.  Is AssetProcessor.exe up to date?");
                return;
            }

            if (message.m_data.length() > AZ_MAX_PATH_LEN)
            {
                auto maxPath = message.m_data.substr(0, AZ_MAX_PATH_LEN - 1);
                AZ_Warning("AssetSystem", false, "HotUpdate: filename too long(%zd) : %s...", bufferSize, maxPath.c_str());
                return;
            }

            switch (message.m_type)
            {
            case AssetNotificationMessage::AssetChanged:
                AssetSystemBus::QueueBroadcast(&AssetSystemBus::Events::AssetChanged, message);
                break;
            case AssetNotificationMessage::AssetRemoved:
                AssetSystemBus::QueueBroadcast(&AssetSystemBus::Events::AssetRemoved, message);
                break;
            case AssetNotificationMessage::JobStarted:
                AssetSystemInfoBus::Broadcast(&AssetSystemInfoBus::Events::AssetCompilationStarted, message.m_data);
                break;
            case AssetNotificationMessage::JobCompleted:
                AssetSystemInfoBus::Broadcast(&AssetSystemInfoBus::Events::AssetCompilationSuccess, message.m_data);
                break;
            case AssetNotificationMessage::JobFailed:
                AssetSystemInfoBus::Broadcast(&AssetSystemInfoBus::Events::AssetCompilationFailed, message.m_data);
                break;
            case AssetNotificationMessage::JobCount:
            {
                int numberOfAssets = atoi(message.m_data.c_str());
                AssetSystemInfoBus::Broadcast(&AssetSystemInfoBus::Events::CountOfAssetsInQueue, numberOfAssets);
            }
            break;
            default:
                AZ_WarningOnce("AssetSystem", false, "Unknown AssetNotificationMessage type received from network.  Is AssetProcessor.exe up to date?");
                break;
            }
        }

        void AssetSystemComponent::Init()
        {
            m_socketConn.reset(new AssetProcessorConnection());
        }

        void AssetSystemComponent::Activate()
        {
            EnableSocketConnection();

            m_cbHandle = m_socketConn->AddMessageHandler(AZ_CRC("AssetProcessorManager::AssetNotification", 0xd6191df5),
                    [](unsigned int typeId, unsigned int /*serial*/, const void* data, unsigned int dataLength)
                    {
                        if (dataLength)
                        {
                            OnAssetSystemMessage(typeId, data, dataLength);
                        }
                    });

            AZStd::string value;
            if (RequestFromBootstrapReader(ASSET_PROCESSOR_REMOTE_PORT, value, true))
            {
                m_assetProcessorPort = static_cast<AZ::u16>(AZStd::stoi(value));
            }

            if (RequestFromBootstrapReader(BRANCH_TOKEN, value, false))
            {
                m_branchToken = value;
            }

            if (RequestFromBootstrapReader(ASSETS, value, true))
            {
                m_platform = value;
            }

            if (RequestFromBootstrapReader(ASSET_PROCESSOR_REMOTE_IP, value, true))
            {
                m_assetProcessorIP = value;
            }

            if (auto apConnection = azrtti_cast<AssetProcessorConnection*>(m_socketConn.get()))
            {
                apConnection->Configure(m_branchToken.c_str(), m_platform.c_str(), "");
            }

            AzFramework::AssetSystemBus::AllowFunctionQueuing(true);
            AssetSystemRequestBus::Handler::BusConnect();
            AZ::SystemTickBus::Handler::BusConnect();
        }

        void AssetSystemComponent::Deactivate()
        {
            AZ::SystemTickBus::Handler::BusDisconnect();
            AssetSystemRequestBus::Handler::BusDisconnect();
            
            m_socketConn->RemoveMessageHandler(AZ_CRC("AssetProcessorManager::AssetNotification", 0xd6191df5), m_cbHandle);
            m_socketConn->Disconnect(true);
            
            DisableSocketConnection();

            // clear the queue out early
            AzFramework::AssetSystemBus::AllowFunctionQueuing(false);
            AzFramework::AssetSystemBus::ClearQueuedEvents();
        }

        void AssetSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            NegotiationMessage::Reflect(context);
            BaseAssetProcessorMessage::Reflect(context);
            RequestAssetStatus::Reflect(context);
            ResponseAssetProcessorStatus::Reflect(context);
            RequestAssetProcessorStatus::Reflect(context);
            ResponseAssetStatus::Reflect(context);

            RequestPing::Reflect(context);
            ResponsePing::Reflect(context);

            // Requests
            GetRelativeProductPathFromFullSourceOrProductPathRequest::Reflect(context);
            GetFullSourcePathFromRelativeProductPathRequest::Reflect(context);
            SourceAssetInfoRequest::Reflect(context);
            AssetInfoRequest::Reflect(context);
            RegisterSourceAssetRequest::Reflect(context);
            UnregisterSourceAssetRequest::Reflect(context);
            ShowAssetProcessorRequest::Reflect(context);
            FileOpenRequest::Reflect(context);
            FileCloseRequest::Reflect(context);
            FileReadRequest::Reflect(context);
            FileWriteRequest::Reflect(context);
            FileTellRequest::Reflect(context);
            FileSeekRequest::Reflect(context);
            FileIsReadOnlyRequest::Reflect(context);
            PathIsDirectoryRequest::Reflect(context);
            FileSizeRequest::Reflect(context);
            FileModTimeRequest::Reflect(context);
            FileExistsRequest::Reflect(context);
            FileFlushRequest::Reflect(context);
            PathCreateRequest::Reflect(context);
            PathDestroyRequest::Reflect(context);
            FileRemoveRequest::Reflect(context);
            FileCopyRequest::Reflect(context);
            FileRenameRequest::Reflect(context);
            FindFilesRequest::Reflect(context);

            // Responses
            GetRelativeProductPathFromFullSourceOrProductPathResponse::Reflect(context);
            GetFullSourcePathFromRelativeProductPathResponse::Reflect(context);
            SourceAssetInfoResponse::Reflect(context);
            AssetInfoResponse::Reflect(context);
            FileOpenResponse::Reflect(context);
            FileReadResponse::Reflect(context);
            FileWriteResponse::Reflect(context);
            FileTellResponse::Reflect(context);
            FileSeekResponse::Reflect(context);
            FileIsReadOnlyResponse::Reflect(context);
            PathIsDirectoryResponse::Reflect(context);
            FileSizeResponse::Reflect(context);
            FileModTimeResponse::Reflect(context);
            FileExistsResponse::Reflect(context);
            FileFlushResponse::Reflect(context);
            PathCreateResponse::Reflect(context);
            PathDestroyResponse::Reflect(context);
            FileRemoveResponse::Reflect(context);
            FileCopyResponse::Reflect(context);
            FileRenameResponse::Reflect(context);
            FindFilesResponse::Reflect(context);
            SaveAssetCatalogRequest::Reflect(context);
            SaveAssetCatalogResponse::Reflect(context);

            AssetNotificationMessage::Reflect(context);

            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetSystemComponent, AZ::Component>()
                    ;
            }
        }

        void AssetSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("AssetProcessorConnection", 0xf0cd75cd));
        }

        void AssetSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("BootstrapService", 0x90c22e09));
        }

        void AssetSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("AssetProcessorConnection", 0xf0cd75cd));
        }

        void AssetSystemComponent::EnableSocketConnection()
        {
            AZ_Assert(!SocketConnection::GetInstance(), "You can only have one AZ::SocketConnection");
            if (!SocketConnection::GetInstance())
            {
                SocketConnection::SetInstance(m_socketConn.get());
            }
        }

        void AssetSystemComponent::DisableSocketConnection()
        {
            SocketConnection* socketConnection = SocketConnection::GetInstance();
            if (socketConnection && socketConnection == m_socketConn.get())
            {
                SocketConnection::SetInstance(nullptr);
            }
        }

        bool AssetSystemComponent::ConfigureSocketConnection(const AZStd::string& branch, const AZStd::string& platform, const AZStd::string& identifier)
        {
            AZ_Assert(m_socketConn.get(), "SocketConnection doesn't exist!  Ensure AssetSystemComponent::Init was called");
            auto apConnection = azrtti_cast<AssetProcessorConnection*>(m_socketConn.get());

            if (apConnection)
            {
                m_branchToken = branch;
                m_platform = platform;
                apConnection->Configure(branch.c_str(), platform.c_str(), identifier.c_str());
                return true;
            }

            return false;
        }

        bool AssetSystemComponent::Connect(const char* identifier)
        {
            auto apConnection = azrtti_cast<AssetProcessorConnection*>(m_socketConn.get());
            if (!apConnection)
            {
                return false;
            }

            // We are doing this configure only to set the identifier
            apConnection->Configure(m_branchToken.c_str(), m_platform.c_str(), identifier);

            //connect is async
            AZ_TracePrintf("Asset System Connection", "Asset Processor Connection IP: %s, port: %hu, branch token %s\n", m_assetProcessorIP.c_str(), m_assetProcessorPort, m_branchToken.c_str());
            apConnection->Connect(m_assetProcessorIP.c_str(), m_assetProcessorPort);

            //this should be pretty much immediate, but allow up to 40 seconds in case the CPU is heavily overloaded and thread starting takes a while.
            AZStd::chrono::system_clock::time_point start = AZStd::chrono::system_clock::now();
            while (!apConnection->IsConnected() && AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(AZStd::chrono::system_clock::now() - start) < AZStd::chrono::seconds(40))
            {
                //yield
                AZStd::this_thread::yield();
            }

            //if we are not connected at this point stop trying and fail
            if (!apConnection->IsConnected())
            {
                return false;
            }

            return true;
        }

        void AssetSystemComponent::SetBranchToken(const AZStd::string branchtoken)
        {
            m_branchToken = branchtoken;
        }

        bool AssetSystemComponent::Disconnect()
        {
            auto apConnection = azrtti_cast<AssetProcessorConnection*>(m_socketConn.get());
            if (!apConnection)
            {
                return false;
            }

            bool result = true;

            if (apConnection->IsConnected())
            {
                result = apConnection->Disconnect(true);
            }

            AzFramework::AssetSystemBus::ClearQueuedEvents();

            return result;
        }

        bool AssetSystemComponent::SaveCatalog()
        {
            auto apConnection = azrtti_cast<AssetProcessorConnection*>(m_socketConn.get());
            if (!apConnection)
            {
                return false;
            }

            SaveAssetCatalogRequest saveCatalogRequest;
            SaveAssetCatalogResponse saveCatalogRespose;
            if (SendRequest(saveCatalogRequest, saveCatalogRespose))
            {
                return saveCatalogRespose.m_saved;
            }

            return false;
        }

        AssetStatus AssetSystemComponent::CompileAssetSync(const AZStd::string& assetPath)
        {
            return SendAssetStatusRequest(assetPath, false, false);
        }
        AssetStatus AssetSystemComponent::GetAssetStatus(const AZStd::string& assetPath)
        {
            return SendAssetStatusRequest(assetPath, true, false);
        }

        AssetStatus AssetSystemComponent::CompileAssetSync_FlushIO(const AZStd::string& assetPath)
        {
            return SendAssetStatusRequest(assetPath, false, true);
        }

        AssetStatus AssetSystemComponent::GetAssetStatus_FlushIO(const AZStd::string& assetPath)
        {
            return SendAssetStatusRequest(assetPath, true, true);
        }

        void AssetSystemComponent::OnSystemTick()
        {
            AZ_TRACE_METHOD();
            AssetSystemBus::ExecuteQueuedEvents();
            LegacyAssetEventBus::ExecuteQueuedEvents();
        }

        AssetStatus AssetSystemComponent::SendAssetStatusRequest(const AZStd::string& assetPath, bool statusOnly, bool requireFencing)
        {
            AssetStatus eStatus = AssetStatus_Unknown;

            if (m_socketConn && m_socketConn->IsConnected())
            {
                RequestAssetStatus request(assetPath.c_str(), statusOnly, requireFencing);
                ResponseAssetStatus response;
                SendRequest(request, response);

                eStatus = static_cast<AssetStatus>(response.m_assetStatus);
            }

            return eStatus;
        }

        float AssetSystemComponent::GetAssetProcessorPingTimeMilliseconds()
        {
            if ((!m_socketConn) || (!m_socketConn->IsConnected()))
            {
                return 0.0f;
            }

            AZStd::chrono::system_clock::time_point beforePing = AZStd::chrono::system_clock::now();
            RequestPing pinger;
            ResponsePing pingRespose;
            if (SendRequest(pinger, pingRespose))
            {
                AZStd::chrono::duration<float, AZStd::milli> difference = AZStd::chrono::duration_cast<AZStd::chrono::duration<float, AZStd::milli> >(AZStd::chrono::system_clock::now() - beforePing);
                return difference.count();
            }

            return 0.0f;
        }

        void AssetSystemComponent::SetAssetProcessorPort(AZ::u16 port)
        {
            if (!m_socketConn || !m_socketConn->IsConnected()) // Don't allow changing the port while connected
            {
                m_assetProcessorPort = port;
            }
            else
            {
                AZ_Warning("AssetSystem", false, "Cannot change port while already connected");
            }
        }

        void AssetSystemComponent::SetAssetProcessorIP(const AZStd::string& ip)
        {
            if (!m_socketConn || !m_socketConn->IsConnected()) // Don't allow changing the IP while connected
            {
                m_assetProcessorIP = ip;
            }
            else
            {
                AZ_Warning("AssetSystem", false, "Cannot change IP while already connected");
            }
        }

        AZStd::string AssetSystemComponent::AssetProcessorConnectionAddress()
        {
            return m_assetProcessorIP;
        }

        AZStd::string AssetSystemComponent::AssetProcessorBranchToken()
        {
            return m_branchToken;
        }

        AZStd::string AssetSystemComponent::AssetProcessorPlatform()
        {
            return m_platform;
        }

        AZ::u16 AssetSystemComponent::AssetProcessorPort()
        {
            return m_assetProcessorPort;
        }

        bool AssetSystemComponent::RequestFromBootstrapReader(const char* key, AZStd::string& value, bool checkPlatform)
        {
            bool success = false;
            BootstrapReaderRequestBus::BroadcastResult(success, &BootstrapReaderRequestBus::Events::SearchConfigurationForKey, key, checkPlatform, value);
            return success;
        }
    } // namespace AssetSystem
} // namespace AzFramework
