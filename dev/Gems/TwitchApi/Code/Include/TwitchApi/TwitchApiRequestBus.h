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
#include <TwitchApi/RestTypes.h>

namespace TwitchApi
{
    class TwitchApiRequests
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        // Twitch Auth

        //! Sets the Twitch Application ID for use by the Twitch API.
        //! Can only be set once per execution.
        //! @param twitchApplicationID The Twitch Application ID to be used with requests.
        virtual void SetApplicationId(const AZStd::string& twitchApplicationId) = 0;

        //! Sets the Twitch User ID to be used for Twitch Api API requests.
        //! @param userID The Twitch User ID to be used for user data related requests.
        virtual void SetUserId(const AZStd::string& userId) = 0;

        //! Sets the Twitch User Access OAuth token for Twitch API requests.
        //! The token cannot be provisioned through the Twitch Api Gem and must be supplied.
        //! See the Twitch API documentation for details on how to acquire a token.
        //! @param token An OAuth token generated through Twitch API web requests.
        virtual void SetUserAccessToken(const AZStd::string& token) = 0;

        //! Sets the Twitch App Access OAuth token for Twitch API requests.
        //! The token cannot be provisioned through the Twitch Api Gem and must be supplied.
        //! See the Twitch API documentation for details on how to acquire a token.
        //! @param token An OAuth token generated through Twitch API web requests.
        virtual void SetAppAccessToken(const AZStd::string& token) = 0;

        //! Gets the Twitch Application ID if one was set
        //! @return The Twitch Application ID if one was set
        virtual AZStd::string_view GetApplicationId() const = 0;

        //! Gets the Twitch User ID if one was set
        //! @return The Twitch User ID if one was set
        virtual AZStd::string_view GetUserId() const = 0;

        //! Gets the Twitch User Access Token if one was set
        //! @return The Twitch User Access Token if one was set
        virtual AZStd::string_view GetUserAccessToken() const = 0;

        //! Gets the Twitch App Access Token if one was set
        //! @return The Twitch App Access Token if one was set
        virtual AZStd::string_view GetAppAccessToken() const = 0;

        ////////////////////////////////////////////////////////////////////////
        //! Twitch Request API Implementation, up to date documentation available at https://dev.twitch.tv/docs/api/reference
        //! Parameters to these functions map to the Required and Optional Query String parameters as specified in the API reference page

        //! For API functions defining a limit for total results returned, 20 is often the listed default
        static constexpr AZ::u8 DEFAULT_MAX_RESULTS_LIMIT = 20; 
        static constexpr AZ::u8 BITS_LB_MAX_RESULTS_LIMIT = 10; //!< GetBitsLeaderboard however, defines its limit at 10 per the API

        // Ads
        virtual void StartCommercial(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZ::u64 length) = 0;

        // Analytics
        virtual void GetExtensionAnalytics(ReceiptId& receipt, const AZStd::string& started_at, const AZStd::string& ended_at,
            const AZStd::string& extension_id, const AZStd::string& after, const AZStd::string& type,
            AZ::u8 max_results = DEFAULT_MAX_RESULTS_LIMIT) = 0;
        virtual void GetGameAnalytics(ReceiptId& receipt, const AZStd::string& started_at, const AZStd::string& ended_at,
            const AZStd::string& game_id, const AZStd::string& after, const AZStd::string& type,
            AZ::u8 max_results = DEFAULT_MAX_RESULTS_LIMIT) = 0;

        // Bits
        virtual void GetBitsLeaderboard(ReceiptId& receipt, const AZStd::string& started_at, const AZStd::string& period,
            const AZStd::string& user_id, AZ::u8 max_results = BITS_LB_MAX_RESULTS_LIMIT) = 0;
        virtual void GetCheermotes(ReceiptId& receipt, const AZStd::string& broadcaster_id) = 0;
        virtual void GetExtensionTransactions(ReceiptId& receipt, const AZStd::string& extension_id, const AZStd::string& transaction_id,
            const AZStd::string& after, AZ::u8 max_results = DEFAULT_MAX_RESULTS_LIMIT) = 0;

        // Channels
        virtual void GetChannelInformation(ReceiptId& receipt, const AZStd::string& broadcaster_id) = 0;
        virtual void ModifyChannelInformation(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::string& game_id,
            const AZStd::string& broadcaster_language, const AZStd::string& title) = 0;
        virtual void GetChannelEditors(ReceiptId& receipt, const AZStd::string& broadcaster_id) = 0;

        // Channel Points
        virtual void CreateCustomRewards(ReceiptId& receipt, const AZStd::string& broadcaster_id,
            const CustomRewardSettings& rewardSettings) = 0;
        virtual void DeleteCustomReward(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::string& id) = 0;
        virtual void GetCustomReward(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::string& id,
            const bool only_manageable_rewards) = 0;
        virtual void GetCustomRewardRedemption(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::string& reward_id,
            const AZStd::string& id, const AZStd::string& status, const AZStd::string& sort, const AZStd::string& after,
            AZ::u8 max_results = DEFAULT_MAX_RESULTS_LIMIT) = 0;
        virtual void UpdateCustomReward(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::string& id,
            const CustomRewardSettings& rewardSettings) = 0;
        virtual void UpdateRedemptionStatus(ReceiptId& receipt, const AZStd::string& id, const AZStd::string& broadcaster_id,
            const AZStd::string& reward_id, const AZStd::string& status) = 0;

        // Clips
        virtual void CreateClip(ReceiptId& receipt, const AZStd::string& broadcaster_id, const bool has_delay = false) = 0;
        virtual void GetClips(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::string& game_id,
            const AZStd::string& clip_id, const AZStd::string& started_at, const AZStd::string& ended_at, const AZStd::string& before,
            const AZStd::string& after, AZ::u8 max_results = DEFAULT_MAX_RESULTS_LIMIT) = 0;

        // Entitlements
        virtual void CreateEntitlementGrantsUploadURL(ReceiptId& receipt, const AZStd::string& manifest_id,
            const AZStd::string& entitlement_type) = 0;
        virtual void GetCodeStatus(ReceiptId& receipt, const AZStd::vector<AZStd::string>& codes, const AZ::u64 user_id) = 0;
        virtual void GetDropsEntitlements(ReceiptId& receipt, const AZStd::string& id, const AZStd::string& user_id,
            const AZStd::string& game_id, const AZStd::string& after,
            AZ::u8 max_results = TwitchApiRequests::DEFAULT_MAX_RESULTS_LIMIT) = 0;
        virtual void RedeemCode(ReceiptId& receipt, const AZStd::vector<AZStd::string>& codes, const AZ::u64 user_id) = 0;

        // Games
        virtual void GetTopGames(ReceiptId& receipt, const AZStd::string& before, const AZStd::string& after,
            AZ::u8 max_results = DEFAULT_MAX_RESULTS_LIMIT) = 0;
        virtual void GetGames(ReceiptId& receipt, const AZStd::string& game_id, const AZStd::string& game_name) = 0;

        // Hype Train
        virtual void GetHypeTrainEvents(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::string& id,
            const AZStd::string& cursor, AZ::u8 max_results = TwitchApiRequests::DEFAULT_MAX_RESULTS_LIMIT) = 0;

        // Moderation
        virtual void CheckAutoModStatus(ReceiptId& receipt, const AZStd::string& broadcaster_id,
            const AZStd::vector<CheckAutoModInputDatum>& filterTargets) = 0;
        virtual void GetBannedEvents(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::vector<AZStd::string>& user_ids,
            const AZStd::string& after, AZ::u8 max_results = DEFAULT_MAX_RESULTS_LIMIT) = 0;
        virtual void GetBannedUsers(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::vector<AZStd::string>& user_ids,
            const AZStd::string& before, const AZStd::string& after) = 0;
        virtual void GetModerators(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::vector<AZStd::string>& user_ids,
            const AZStd::string& after) = 0;
        virtual void GetModeratorEvents(ReceiptId& receipt, const AZStd::string& broadcaster_id,
            const AZStd::vector<AZStd::string>& user_ids) = 0;

        // Search
        virtual void SearchCategories(ReceiptId& receipt, const AZStd::string& query, const AZStd::string& after,
            AZ::u8 max_results = TwitchApiRequests::DEFAULT_MAX_RESULTS_LIMIT) = 0;
        virtual void SearchChannels(ReceiptId& receipt, const AZStd::string& query, const AZStd::string& after, const bool live_only,
            AZ::u8 max_results = TwitchApiRequests::DEFAULT_MAX_RESULTS_LIMIT) = 0;

        // Streams
        virtual void GetStreamKey(ReceiptId& receipt, const AZStd::string& broadcaster_id) = 0;
        virtual void GetStreams(ReceiptId& receipt, const AZStd::vector<AZStd::string>& game_ids,
            const AZStd::vector<AZStd::string>& user_ids, const AZStd::vector<AZStd::string>& user_logins,
            const AZStd::vector<AZStd::string>& languages, const AZStd::string& before, const AZStd::string& after,
            AZ::u8 max_results = 20) = 0;
        virtual void CreateStreamMarker(ReceiptId& receipt, const AZStd::string& user_id, const AZStd::string& description) = 0;
        virtual void GetStreamMarkers(ReceiptId& receipt, const AZStd::string& user_id, const AZStd::string& video_id,
            const AZStd::string& before, const AZStd::string& after, AZ::u8 max_results = DEFAULT_MAX_RESULTS_LIMIT) = 0;

        // Subscriptions
        virtual void GetBroadcasterSubscriptions(ReceiptId& receipt, const AZStd::string& broadcaster_id,
            const AZStd::vector<AZStd::string>& user_ids) = 0;

        // Tags
        virtual void GetAllStreamTags(ReceiptId& receipt, const AZStd::vector<AZStd::string>& tag_ids,
            const AZStd::string& after, AZ::u8 max_results = DEFAULT_MAX_RESULTS_LIMIT) = 0;
        virtual void GetStreamTags(ReceiptId& receipt, const AZStd::string& broadcaster_id) = 0;
        virtual void ReplaceStreamTags(ReceiptId& receipt, const AZStd::string& broadcaster_id,
            const AZStd::vector<AZStd::string>& tag_ids) = 0;

        // Users
        virtual void GetUsers(ReceiptId& receipt, const AZStd::vector<AZStd::string>& user_ids,
            const AZStd::vector<AZStd::string>& user_logins) = 0;
        virtual void UpdateUser(ReceiptId& receipt, const AZStd::string& description) = 0;
        virtual void GetUserFollows(ReceiptId& receipt, AZStd::string from_id, const AZStd::string& to_id, const AZStd::string& after,
            AZ::u8 max_results = TwitchApiRequests::DEFAULT_MAX_RESULTS_LIMIT) = 0;
        virtual void CreateUserFollows(ReceiptId& receipt, const AZStd::string& from_id, const AZStd::string& to_id,
            const bool allow_notifications) = 0;
        virtual void DeleteUserFollows(ReceiptId& receipt, const AZStd::string& from_id, const AZStd::string& to_id) = 0;
        virtual void GetUserBlockList(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::string& after,
            AZ::u8 max_results = TwitchApiRequests::DEFAULT_MAX_RESULTS_LIMIT) = 0;
        virtual void BlockUser(ReceiptId& receipt, const AZStd::string& target_user_id, const AZStd::string& source_context,
            const AZStd::string& reason) = 0;
        virtual void UnblockUser(ReceiptId& receipt, const AZStd::string& target_user_id) = 0;
        virtual void GetUserExtensions(ReceiptId& receipt) = 0;
        virtual void GetUserActiveExtensions(ReceiptId& receipt, const AZStd::string& user_id) = 0;
        virtual void UpdateUserExtensions(ReceiptId& receipt, const AZStd::vector<GetUserActiveExtensionsComponent>& panel,
            const AZStd::vector<GetUserActiveExtensionsComponent>& overlay,
            const AZStd::vector<GetUserActiveExtensionsComponent>& component) = 0;

        // Videos
        virtual void GetVideos(ReceiptId& receipt, const AZStd::vector<AZStd::string>& video_ids, const AZStd::string& user_id,
            const AZStd::string& game_id, const AZStd::string& video_type, const AZStd::string& sort_order, const AZStd::string& period,
            const AZStd::string& language, const AZStd::string& before, const AZStd::string& after,
            AZ::u8 max_results = DEFAULT_MAX_RESULTS_LIMIT) = 0;
        virtual void DeleteVideos(ReceiptId& receipt, const AZStd::vector<AZStd::string>& video_ids) = 0;

        // Webhooks
        virtual void GetWebhookSubscriptions(ReceiptId& receipt, const AZStd::string& after,
            AZ::u8 max_results = DEFAULT_MAX_RESULTS_LIMIT) = 0;
        ////////////////////////////////////////////////////////////////////////
    };

    using TwitchApiRequestBus = AZ::EBus<TwitchApiRequests>;
} // namespace TwitchApi
