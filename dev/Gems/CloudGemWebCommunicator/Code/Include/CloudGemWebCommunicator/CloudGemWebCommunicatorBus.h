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

#include <AzCore/EBus/EBus.h>

#include <ResponseCode.hpp>

namespace CloudGemWebCommunicator
{
    class CloudGemWebCommunicatorRequests
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        // Public functions

        virtual bool RequestRegistration(const AZStd::string& connectionType) = 0;
        virtual bool RequestConnection(const AZStd::string& connectionType) = 0;
        virtual bool RequestChannelList() = 0;
        virtual bool RequestSubscribeChannel(const AZStd::string&  channelName) = 0;
        virtual bool RequestSubscribeChannelList(const AZStd::vector<AZStd::string>&  channelName) = 0;
        virtual bool RequestUnsubscribeChannel(const AZStd::string&  channelName) = 0;
        virtual bool RequestDisconnect() = 0;
        virtual bool RequestRefreshDeviceInfo() = 0;
        virtual bool PostClientMessage(const AZStd::string& channelName, const AZStd::string& message) = 0;
        virtual bool RequestSendMessageDirect(const AZStd::string& channelName, const AZStd::string& message) = 0;

        virtual AZStd::string GetRegistrationStatus() = 0;
        virtual AZStd::string GetConnectionStatus() = 0;
        virtual AZStd::string GetSubscriptionStatus(const AZStd::string& channelName) = 0;
        virtual AZStd::vector<AZStd::string> GetSubscriptionList() = 0;
        virtual AZStd::string GetEndpointPortString() = 0;
    };
    using CloudGemWebCommunicatorRequestBus = AZ::EBus<CloudGemWebCommunicatorRequests>;

    class CloudGemWebCommunicatorUpdates
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        // Public functions

        virtual void ConnectionStatusChanged(const AZStd::string& connection) = 0;
        virtual void MessageReceived(const AZStd::string& channelName, const AZStd::string& channelMessage) = 0;
        virtual void RegistrationStatusChanged(const AZStd::string& registrationStatus) = 0;
        virtual void SubscriptionStatusChanged(const AZStd::string& channelName, const AZStd::string& subscriptionStatus) = 0;
    };
    using CloudGemWebCommunicatorUpdateBus = AZ::EBus<CloudGemWebCommunicatorUpdates>;

    // Handle callbacks for job requests to the socket library
    class CloudGemWebCommunicatorLibraryResponse
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const bool EnableEventQueue = true;

        using BusIdType = AZ::EntityId;
        using ResponseCodeType = awsiotsdk::ResponseCode;
        // Public functions

        virtual void GotConnectionResponse(ResponseCodeType responseCode, std::shared_ptr<awsiotsdk::MqttClient> connectionClient) = 0;
        virtual void GotSubscribeResponse(ResponseCodeType responseCode, AZStd::vector<AZStd::string> channelName, std::shared_ptr<awsiotsdk::MqttClient> connectionClient) = 0;
        virtual void GotMessage(const AZStd::string channelName, const AZStd::string channelMessage) = 0;
        virtual void GotUnsubscribeResponse(ResponseCodeType responseCode, AZStd::string channelName, std::shared_ptr<awsiotsdk::MqttClient> connectionClient) = 0;
    };
    using CloudGemWebCommunicatorLibraryResponseBus = AZ::EBus<CloudGemWebCommunicatorLibraryResponse>;
} // namespace CloudGemWebCommunicator
