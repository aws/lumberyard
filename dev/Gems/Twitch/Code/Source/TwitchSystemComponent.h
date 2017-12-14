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
#include <AzCore/std/parallel/atomic.h>
#include <Twitch/TwitchBus.h>
#include <IFuelInterface.h>
#include <ITwitchREST.h>

namespace Twitch
{
    class TwitchSystemComponent
        : public AZ::Component
        , protected TwitchRequestBus::Handler
        , public AZ::SystemTickBus::Handler
    {
    public:
        AZ_COMPONENT(TwitchSystemComponent, "{8AC76E51-CE55-4D67-90DE-41D1A7756E32}");

        TwitchSystemComponent();
        virtual ~TwitchSystemComponent() {}

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        void OnSystemTick() override;

    protected:
        ////////////////////////////////////////////////////////////////////////
        // TwitchRequestBus interface implementation
        ////////////////////////////////////////////////////////////////////////
        void SetApplicationID(const AZStd::string& twitchApplicationID) override;
        void RequestUserID(ReceiptID& receipt) override;
        void RequestOAuthToken(ReceiptID& receipt) override;
        void RequestEntitlement(ReceiptID& receipt) override;
        void RequestProductCatalog(ReceiptID& receipt) override;
        void PurchaseProduct(ReceiptID& receipt, const Twitch::FuelSku& sku) override;
        void GetPurchaseUpdates(ReceiptID& receipt, const AZStd::string& syncToken) override;

        void GetUser(ReceiptID& receipt) override;
        void ResetFriendsNotificationCount(ReceiptID & receipt, const AZStd::string& friendID) override;
        void GetFriendNotificationCount(ReceiptID & receipt, const AZStd::string& friendID) override;
        void GetFriendRecommendations(ReceiptID & receipt, const AZStd::string& friendID) override;
        void GetFriends(ReceiptID & receipt, const AZStd::string& friendID, const AZStd::string& cursor) override;
        void GetFriendStatus(ReceiptID & receipt, const AZStd::string& sourceFriendID, const AZStd::string& targetFriendID) override;
        void AcceptFriendRequest(ReceiptID & receipt, const AZStd::string& friendID) override;
        void GetFriendRequests(ReceiptID & receipt, const AZStd::string& cursor) override;
        void CreateFriendRequest(ReceiptID & receipt, const AZStd::string& friendID) override;
        void DeclineFriendRequest(ReceiptID & receipt, const AZStd::string& friendID) override;
        
        // presence
        void UpdatePresenceStatus(ReceiptID & receipt, PresenceAvailability availability, PresenceActivityType activityType, const AZStd::string& gameContext) override;
        void GetPresenceStatusofFriends(ReceiptID & receipt) override;
        void GetPresenceSettings(ReceiptID & receipt) override;
        void UpdatePresenceSettings(ReceiptID & receipt, bool isInvisible, bool shareActivity) override;
        
        // channel
        void GetChannel(ReceiptID& receipt) override;
        void GetChannelbyID(ReceiptID& receipt, const AZStd::string& channelID) override;
        void UpdateChannel(ReceiptID& receipt, const ChannelUpdateInfo& channelUpdateInfo) override;
        void GetChannelEditors(ReceiptID& receipt, const AZStd::string& channelID) override;
        void GetChannelFollowers(ReceiptID& receipt, const AZStd::string& channelID, const AZStd::string& cursor, AZ::u64 offset) override;
        void GetChannelTeams(ReceiptID& receipt, const AZStd::string& channelID) override;
        void GetChannelSubscribers(ReceiptID& receipt, const AZStd::string& channelID, AZ::u64 offset) override;
        void CheckChannelSubscriptionbyUser(ReceiptID& receipt, const AZStd::string& channelID, const AZStd::string& userID) override;
        void GetChannelVideos(ReceiptID& receipt, const AZStd::string& channelID, BroadCastType boradcastType, const AZStd::string& language, AZ::u64 offset) override;
        void StartChannelCommercial(ReceiptID& receipt, const AZStd::string& channelID, CommercialLength length) override;
        void ResetChannelStreamKey(ReceiptID& receipt, const AZStd::string& channelID) override;
        void GetChannelCommunity(ReceiptID& receipt, const AZStd::string& channelID) override;
        void SetChannelCommunity(ReceiptID& receipt, const AZStd::string& channelID, const AZStd::string& communityID) override;
        void DeleteChannelfromCommunity(ReceiptID& receipt, const AZStd::string& channelID) override;

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        AZ::u64 GetReceipt();

    private:
        TwitchSystemComponent(const TwitchSystemComponent&) = delete;
        bool IsValidString(const AZStd::string& str, AZStd::size_t minLength, AZStd::size_t maxLength) const;
        bool IsValidFriendID(const AZStd::string& friendID) const;
        bool IsValidSyncToken(const AZStd::string& syncToken) const;
        bool IsValidChannelID(const AZStd::string& channelID) const { return IsValidFriendID(channelID); }
        bool IsValidCommunityID(const AZStd::string& communityID) const { return IsValidFriendID(communityID); }
        bool IsValidSku(const Twitch::FuelSku& sku) const { return IsValidFriendID(sku); }

     private:
        IFuelInterfacePtr           m_fuelInterface;
        ITwitchRESTPtr              m_twitchREST;
        AZStd::atomic<AZ::u64>      m_receiptCounter;
    };
}

