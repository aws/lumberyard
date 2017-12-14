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
#include <Twitch/TwitchTypes.h>
#include <Twitch/FuelTypes.h>

namespace Twitch
{
    class TwitchRequests
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        // Twitch Commerce
        virtual void SetApplicationID(const AZStd::string& twitchApplicationID) = 0;
        virtual void RequestUserID(ReceiptID& receipt) = 0;
        virtual void RequestOAuthToken(ReceiptID& receipt) = 0;
        virtual void RequestEntitlement(ReceiptID& receipt) = 0;
        virtual void RequestProductCatalog(ReceiptID& receipt) = 0;
        virtual void PurchaseProduct(ReceiptID& receipt, const Twitch::FuelSku& sku) = 0;
        virtual void GetPurchaseUpdates(ReceiptID& receipt, const AZStd::string& syncToken) = 0;

        // User
        virtual void GetUser(ReceiptID& receipt) = 0;

        // Friends
        virtual void ResetFriendsNotificationCount(ReceiptID& receipt, const AZStd::string& friendID) = 0;
        virtual void GetFriendNotificationCount(ReceiptID& receipt, const AZStd::string& friendID) = 0;
        virtual void GetFriendRecommendations(ReceiptID& receipt, const AZStd::string& friendID) = 0;
        virtual void GetFriends(ReceiptID& receipt, const AZStd::string& friendID, const AZStd::string& cursor) = 0;
        virtual void GetFriendStatus(ReceiptID& receipt, const AZStd::string& sourceFriendID, const AZStd::string& targetFriendID) = 0;
        virtual void AcceptFriendRequest(ReceiptID& receipt, const AZStd::string& friendID) = 0;
        virtual void GetFriendRequests(ReceiptID& receipt, const AZStd::string& cursor) = 0;
        virtual void CreateFriendRequest(ReceiptID& receipt, const AZStd::string& friendID) = 0;
        virtual void DeclineFriendRequest(ReceiptID& receipt, const AZStd::string& friendID) = 0;

        // Rich Presence
        virtual void UpdatePresenceStatus(ReceiptID& receipt, PresenceAvailability availability, PresenceActivityType activityType, const AZStd::string& gameContext) = 0;
        virtual void GetPresenceStatusofFriends(ReceiptID& receipt) = 0;
        virtual void GetPresenceSettings(ReceiptID& receipt) = 0;
        virtual void UpdatePresenceSettings(ReceiptID& receipt, bool isInvisible, bool shareActivity) = 0;

        // Channels
        virtual void GetChannel(ReceiptID& receipt) = 0;
        virtual void GetChannelbyID(ReceiptID& receipt, const AZStd::string& channelID) = 0;
        virtual void UpdateChannel(ReceiptID& receipt, const ChannelUpdateInfo & channelUpdateInfo) = 0;
        virtual void GetChannelEditors(ReceiptID& receipt, const AZStd::string& channelID) = 0;
        virtual void GetChannelFollowers(ReceiptID& receipt, const AZStd::string& channelID, const AZStd::string& cursor, AZ::u64 offset) = 0;
        virtual void GetChannelTeams(ReceiptID& receipt, const AZStd::string& channelID) = 0;
        virtual void GetChannelSubscribers(ReceiptID& receipt, const AZStd::string& channelID, AZ::u64 offset) = 0;
        virtual void CheckChannelSubscriptionbyUser(ReceiptID& receipt, const AZStd::string& channelID, const AZStd::string& userID) = 0;
        virtual void GetChannelVideos(ReceiptID& receipt, const AZStd::string& channelID, BroadCastType boradcastType, const AZStd::string& language, AZ::u64 offset) = 0;
        virtual void StartChannelCommercial(ReceiptID& receipt, const AZStd::string& channelID, CommercialLength length) = 0;
        virtual void ResetChannelStreamKey(ReceiptID& receipt, const AZStd::string& channelID) = 0;
        virtual void GetChannelCommunity(ReceiptID& receipt, const AZStd::string& channelID) = 0;
        virtual void SetChannelCommunity(ReceiptID& receipt, const AZStd::string& channelID, const AZStd::string& communityID) = 0;
        virtual void DeleteChannelfromCommunity(ReceiptID& receipt, const AZStd::string& channelID) = 0;
    };

    using TwitchRequestBus = AZ::EBus<TwitchRequests>;

    class TwitchNotifications
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const bool EnableEventQueue = true;
        static const bool EnableQueuedReferences = true;

        // Twitch Commerce notifications
        virtual void UserIDNotify(const StringValue& userID) { (void) userID; }
        virtual void OAuthTokenNotify(const StringValue& token) { (void) token; } 
        virtual void EntitlementNotify(const StringValue& entitlement) { (void)entitlement; }
        virtual void RequestProductCatalog(const ProductDataReturnValue& result) { (void) result; }
        virtual void PurchaseProduct(const PurchaseReceiptReturnValue& result) { (void)result; }
        virtual void GetPurchaseUpdates(const PurchaseUpdateReturnValue& result) { (void)result; }

        // Users notifications
        virtual void GetUser(const UserInfoValue& result) { (void)result; }

        // Friend notifications
        virtual void ResetFriendsNotificationCountNotify(const Int64Value& result) { (void) result; }
        virtual void GetFriendNotificationCount(const Int64Value& result) { (void) result; }
        virtual void GetFriendRecommendations(const FriendRecommendationValue& result) { (void) result; }
        virtual void GetFriends(const GetFriendValue& result) {(void) result; }
        virtual void GetFriendStatus(const FriendStatusValue& result) { (void) result; };
        virtual void AcceptFriendRequest(const Int64Value& result) { (void)result; };
        virtual void GetFriendRequests(const FriendRequestValue& result) { (void) result; }
        virtual void CreateFriendRequest(const Int64Value& result) { (void)result; }
        virtual void DeclineFriendRequest(const Int64Value& result) { (void)result; }

        // Rich Presence
        virtual void UpdatePresenceStatus(const Int64Value& result) { (void)result; }
        virtual void GetPresenceStatusofFriends(const PresenceStatusValue& result) { (void) result; }
        virtual void GetPresenceSettings(const PresenceSettingsValue& result) { (void) result; }
        virtual void UpdatePresenceSettings(const PresenceSettingsValue& result) { (void) result; }

        // Channels
        virtual void GetChannel(const ChannelInfoValue& result) { (void) result; }
        virtual void GetChannelbyID(const ChannelInfoValue& result) { (void) result; }
        virtual void UpdateChannel(const ChannelInfoValue& result) { (void) result; }
        virtual void GetChannelEditors(const UserInfoListValue& result) { (void) result; }
        virtual void GetChannelFollowers(const FollowerResultValue& result) { (void) result; }
        virtual void GetChannelTeams(const ChannelTeamValue& result) { (void)result; }
        virtual void GetChannelSubscribers(const SubscriberValue& result) { (void)result; }
        virtual void CheckChannelSubscriptionbyUser(const SubscriberbyUserValue& result) { (void)result; }
        virtual void GetChannelVideos(const VideoReturnValue& result) { (void)result; }
        virtual void StartChannelCommercial(const StartChannelCommercialValue& result) { (void)result; }
        virtual void ResetChannelStreamKey(const ChannelInfoValue& result) { (void)result; }
        virtual void GetChannelCommunity(const CommunityInfoValue& result) { (void)result; }
        virtual void SetChannelCommunity(const Int64Value& result) { (void)result; }
        virtual void DeleteChannelfromCommunity(const Int64Value& result) { (void)result; }
    };

    using TwitchNotifyBus = AZ::EBus<TwitchNotifications>;
} // namespace Twitch
