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

#include <TwitchApi/TwitchApiRequestBus.h>

namespace TwitchApi
{
    using TwitchApiRequestBus = AZ::EBus<TwitchApiRequests>;

    class TwitchApiNotifications
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const bool EnableEventQueue = true;
        static const bool EnableQueuedReferences = true;

        // Twitch Auth notifications

        //! Notification event for when the Twitch User ID is set
        //! @param userID The Twitch User ID
        virtual void UserIdNotify(const AZStd::string& userId) {}

        //! Notification event for when the Twitch User Access Token is set
        //! @param userID The Twitch User Access Token
        virtual void UserAccessTokenNotify(const AZStd::string& token) {}

        //! Notification event for when the Twitch App Access Token is set
        //! @param userID The Twitch App Access Token
        virtual void AppAccessTokenNotify(const AZStd::string& token) {}

        ////////////////////////////////////////////////////////////////////////
        //! Twitch Response API Implementation, up to date documentation available at https://dev.twitch.tv/docs/api/reference
        //! Parameters to these functions map to the Response Fields as specified in the API reference page

        // Ads
        virtual void OnStartCommercial(const CommercialResult& result) {}

        // Analytics
        virtual void OnGetExtensionAnalytics(const ExtensionAnalyticsResult& result) {}
        virtual void OnGetGameAnalytics(const GameAnalyticsResult& result) {}

        // Bits
        virtual void OnGetBitsLeaderboard(const BitsLeaderboardResult& result) {}
        virtual void OnGetCheermotes(const CheermotesResult& result) {}
        virtual void OnGetExtensionTransactions(const ExtensionTransactionsResult& result) {}

        // Channels
        virtual void OnGetChannelInformation(const ChannelInformationResult& result) {}
        virtual void OnModifyChannelInformation(const ReceiptId& receipt, const ResultCode& rc) {}
        virtual void OnGetChannelEditors(const ChannelEditorsResult& result) {}

        // Channel Points
        virtual void OnCreateCustomRewards(const CustomRewardsResult& result) {}
        virtual void OnDeleteCustomReward(const ReceiptId& receipt, const ResultCode& rc) {}
        virtual void OnGetCustomReward(const CustomRewardsResult& result) {}
        virtual void OnGetCustomRewardRedemption(const CustomRewardsRedemptionResult& result) {}
        virtual void OnUpdateCustomReward(const CustomRewardsResult& result) {}
        virtual void OnUpdateRedemptionStatus(const CustomRewardsRedemptionResult& result) {}

        // Clips
        virtual void OnCreateClip(const CreateClipResult& result) {}
        virtual void OnGetClips(const GetClipsResult& result) {}

        // Entitlements
        virtual void OnCreateEntitlementGrantsUploadURL(const EntitlementsGrantUploadResult& result) {}
        virtual void OnGetCodeStatus(const CodeStatusResult& result) {}
        virtual void OnGetDropsEntitlements(const DropsEntitlementsResult& result) {}
        virtual void OnRedeemCode(const CodeStatusResult& result) {}

        // Games
        virtual void OnGetTopGames(const GetTopGamesResult& result) {}
        virtual void OnGetGames(const GetGamesResult& result) {}

        // Hype Train
        virtual void OnGetHypeTrainEvents(const HypeTrainEventsResult& result) {}

        // Moderation
        virtual void OnCheckAutoModStatus(const CheckAutoModResult& result) {}
        virtual void OnGetBannedEvents(const GetBannedEventsResult& result) {}
        virtual void OnGetBannedUsers(const GetBannedUsersResult& result) {}
        virtual void OnGetModerators(const GetModeratorsResult& result) {}
        virtual void OnGetModeratorEvents(const GetModeratorEventsResult& result) {}

        // Search
        virtual void OnSearchCategories(const SearchCategoriesResult& result) {}
        virtual void OnSearchChannels(const SearchChannelsResult& result) {}

        // Streams
        virtual void OnGetStreamKey(const StreamKeyResult& result) {}
        virtual void OnGetStreams(const GetStreamsResult& result) {}
        virtual void OnCreateStreamMarker(const CreateStreamMarkerResult& result) {}
        virtual void OnGetStreamMarkers(const GetStreamMarkersResult& result) {}

        // Subscriptions
        virtual void OnGetBroadcasterSubscriptions(const GetBroadcasterSubscriptionsResult& result) {}

        // Tags
        virtual void OnGetAllStreamTags(const GetAllStreamTagsResult& result) {}
        virtual void OnGetStreamTags(const GetStreamTagsResult& result) {}
        virtual void OnReplaceStreamTags(const ReceiptId& receipt, const ResultCode& rc) {}

        // Users
        virtual void OnGetUsers(const GetUsersResult& result) {}
        virtual void OnUpdateUser(const GetUsersResult& result) {}
        virtual void OnGetUsersFollows(const GetUsersFollowsResult& result) {}
        virtual void OnCreateUserFollows(const ReceiptId& receipt, const ResultCode& rc) {}
        virtual void OnDeleteUserFollows(const ReceiptId& receipt, const ResultCode& rc) {}
        virtual void OnGetUserBlockList(const UserBlockListResult& result) {}
        virtual void OnBlockUser(const ReceiptId& receipt, const ResultCode& rc) {}
        virtual void OnUnblockUser(const ReceiptId& receipt, const ResultCode& rc) {}
        virtual void OnGetUserExtensions(const GetUserExtensionsResult& result) {}
        virtual void OnGetUserActiveExtensions(const GetUserActiveExtensionsResult& result) {}
        virtual void OnUpdateUserExtensions(const GetUserActiveExtensionsResult& result) {}

        // Videos
        virtual void OnGetVideos(const GetVideosResult& result) {}
        virtual void OnDeleteVideos(const DeleteVideosResult& result) {}

        // Webhooks
        virtual void OnGetWebhookSubscriptions(const GetWebhookSubscriptionsResult& result) {}
        ////////////////////////////////////////////////////////////////////////
    };

    using TwitchApiNotifyBus = AZ::EBus<TwitchApiNotifications>;
} // namespace TwitchApi
