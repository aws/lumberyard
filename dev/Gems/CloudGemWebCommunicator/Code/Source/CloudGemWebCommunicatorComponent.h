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
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <NetworkConnection.hpp>
#include <Mqtt/Client.hpp>
#include <CloudGemWebCommunicator/CloudGemWebCommunicatorBus.h>
#include <CloudCanvasCommon/CloudCanvasCommonBus.h>

namespace AZ
{
    class JobManager;
    class JobContext;
}

namespace CloudGemWebCommunicator
{
    class CloudGemWebCommunicatorComponent
        : public AZ::Component
        , protected CloudGemWebCommunicatorRequestBus::Handler
        , protected CloudGemWebCommunicatorLibraryResponseBus::Handler
        , protected AZ::SystemTickBus::Handler
        , protected CloudCanvasCommon::CloudCanvasCommonNotificationsBus::Handler
    {
    public:
        AZ_COMPONENT(CloudGemWebCommunicatorComponent, "{F4D41321-598B-4D90-8499-6EA0450F06B0}");

        CloudGemWebCommunicatorComponent();
        virtual ~CloudGemWebCommunicatorComponent();

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // CloudGemWebCommunicatorRequestBus implementation
        virtual bool RequestRegistration(const AZStd::string& connectionType) override;
        virtual bool RequestConnection(const AZStd::string& connectionType) override;
        virtual bool RequestChannelList() override;
        virtual bool RequestSubscribeChannel(const AZStd::string&  channelName) override;
        virtual bool RequestSubscribeChannelList(const AZStd::vector<AZStd::string>&  channelList) override;
        virtual bool RequestUnsubscribeChannel(const AZStd::string&  channelName) override;
        virtual bool RequestDisconnect() override;
        virtual bool RequestRefreshDeviceInfo() override;
        virtual bool PostClientMessage(const AZStd::string& channelName, const AZStd::string& message) override;
        virtual bool RequestSendMessageDirect(const AZStd::string& channelName, const AZStd::string& message) override;
        virtual AZStd::string GetRegistrationStatus() override;
        virtual AZStd::string GetConnectionStatus() override;
        virtual AZStd::string GetSubscriptionStatus(const AZStd::string& channelName) override;
        virtual AZStd::string GetEndpointPortString() override;
        virtual AZStd::vector<AZStd::string> GetSubscriptionList() override;


    protected:

        // VS 2013 Fix - std::atomic is not copy constructible
        CloudGemWebCommunicatorComponent(const CloudGemWebCommunicatorComponent&) = delete;

        // Job management
        void SetupJobManager();
        void EndJobManager();

        ////////////////////////////////////////////////////////////////////////
        // SystemTickBus handlers
        void OnSystemTick() override;

        ////////////////////////////////////////////////////////////////////////
        // CloudCanvasCommonNotificationBus handlers
        void OnPostInitialization() override;

        ////////////////////////////////////////////////////////////////////////
        // CloudGemWebCommunicatorLibraryResponseBus interface implementation
        virtual void GotMessage(const AZStd::string channelName, const AZStd::string message) override;
        virtual void GotConnectionResponse(ResponseCodeType responseCode, std::shared_ptr<awsiotsdk::MqttClient> connectionClient) override;
        virtual void GotSubscribeResponse(ResponseCodeType responseCode, AZStd::vector<AZStd::string> channelName, std::shared_ptr<awsiotsdk::MqttClient> connectionClient) override;
        virtual void GotUnsubscribeResponse(ResponseCodeType responseCode, AZStd::string channelName, std::shared_ptr<awsiotsdk::MqttClient> connectionClient) override;

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        
        ////////////////////////////////////////////////////////////////////////

        enum class ConnectionStatus
        {
            NOT_CONNECTED,
            CONNECTING,
            CONNECTED
        };

        bool Connect(const AZStd::string& connectionType, const AZStd::string& endPoint, int endpointPort);
        static std::shared_ptr<awsiotsdk::NetworkConnection> ConnectWebsocket(const AZStd::string& endPoint, int endpointPort);
        static std::shared_ptr<awsiotsdk::NetworkConnection> ConnectSSL(const AZStd::string& endPoint, int endpointPort);
        static const AZStd::string GetEndpointRegion(const AZStd::string& endpoint);
        awsiotsdk::ResponseCode Disconnect();
        void SetConnectionStatus(ConnectionStatus newStatus);
        bool IsConnected() const;
 
        static AZStd::string GetCAPath();
        static AZStd::string GetDeviceCertPath();
        static AZStd::string GetKeyPath();
        AZStd::string GetEndpoint() const;
        int GetEndpointPort() const;
        AZStd::string GetClientId();
        AZStd::string GetCognitoClientId();
        AZStd::string GetCertificateClientId();

        static AZStd::string GetResolvedPath(const char* filePath);

        AZStd::string GetDeviceInfoFilePath() const;
        bool WriteDeviceInfo() const;
        bool ReadDeviceInfo();
        bool ReadCertificate(const AZStd::string& filePath);

        bool WriteDeviceFile(const AZStd::string& filePath, const AZStd::string& fileData) const;

        static std::shared_ptr<awsiotsdk::MqttClient> CreateClient(std::shared_ptr<awsiotsdk::NetworkConnection>& connectionPtr, awsiotsdk::ResponseCode& responseCode, const AZStd::string& clientId);

        awsiotsdk::ResponseCode Subscribe(const AZStd::vector<AZStd::string>& topicList);
        awsiotsdk::ResponseCode Unsubscribe(const AZStd::string& topicName);

        AZ::JobManager* m_jobManager{ nullptr };
        AZ::JobContext* m_jobContext{ nullptr };

        std::shared_ptr<awsiotsdk::NetworkConnection> m_connectionPtr;
        std::shared_ptr<awsiotsdk::MqttClient> m_iotClient;

        AZStd::string m_connectionType;
        AZStd::string m_endpoint;
        int m_endpointPort{ 0 };
        AZStd::mutex m_channelMutex;
        AZStd::vector<AZStd::string> m_channelList;
        AZStd::string m_certificateSN;

        static AZ::EntityId m_moduleEntity;
        static AZStd::string m_region;

        AZStd::atomic<ConnectionStatus> m_connectionStatus{ ConnectionStatus::NOT_CONNECTED };
        AZStd::mutex m_subscriptionMutex;
    };
}
