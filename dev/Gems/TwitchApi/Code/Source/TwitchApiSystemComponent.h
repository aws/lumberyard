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
#include <TwitchApi/TwitchApiNotifyBus.h>
#include <ITwitchApiRest.h>

namespace TwitchApi
{
    class TwitchApiSystemComponent
        : public AZ::Component
        , protected TwitchApiRequestBus::Handler
        , public AZ::SystemTickBus::Handler
    {
    public:
        AZ_COMPONENT(TwitchApiSystemComponent, "{9F0AB019-CB03-46B3-8BC0-30EE1964847F}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        void OnSystemTick() override;

    protected:
        ////////////////////////////////////////////////////////////////////////
        // TwitchApiRequestBus interface implementation
        ////////////////////////////////////////////////////////////////////////

        //! ApplicationId can only be set once during runtime, to avoid using tokens from the wrong App
        void SetApplicationId(const AZStd::string& twitchApplicationId) override;
        void SetUserId(const AZStd::string& userId) override;
        void SetUserAccessToken(const AZStd::string& token) override;
        void SetAppAccessToken(const AZStd::string& token) override;
        AZStd::string_view GetApplicationId() const override;
        AZStd::string_view GetUserId() const override;
        AZStd::string_view GetUserAccessToken() const override;
        AZStd::string_view GetAppAccessToken() const override;

        ////////////////////////////////////////////////////////////////////////
        // Twitch API Implementation, current documentation available at https://dev.twitch.tv/docs/api/reference
        //! Parameters to these functions map to the Response Fields as specified in the API reference page
        //! JsonView parameters map to complex JSON Objects passed via HTTP body as specified in the API reference page

        // Ads
        void StartCommercial(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZ::u64 length) override;

        // Analytics
        void GetExtensionAnalytics(ReceiptId& receipt, const AZStd::string& started_at, const AZStd::string& ended_at,
            const AZStd::string& extension_id, const AZStd::string& after, const AZStd::string& type,
            AZ::u8 max_results = TwitchApiRequests::DEFAULT_MAX_RESULTS_LIMIT) override;
        void GetGameAnalytics(ReceiptId& receipt, const AZStd::string& started_at, const AZStd::string& ended_at,
            const AZStd::string& game_id, const AZStd::string& after, const AZStd::string& type,
            AZ::u8 max_results = TwitchApiRequests::DEFAULT_MAX_RESULTS_LIMIT) override;

        // Bits
        void GetBitsLeaderboard(ReceiptId& receipt, const AZStd::string& started_at, const AZStd::string& period,
            const AZStd::string& user_id, AZ::u8 max_results = TwitchApiRequests::BITS_LB_MAX_RESULTS_LIMIT) override;
        void GetCheermotes(ReceiptId& receipt, const AZStd::string& broadcaster_id) override;
        void GetExtensionTransactions(ReceiptId& receipt, const AZStd::string& extension_id, const AZStd::string& transaction_id,
            const AZStd::string& after, AZ::u8 max_results = TwitchApiRequests::DEFAULT_MAX_RESULTS_LIMIT) override;

        // Channels
        void GetChannelInformation(ReceiptId& receipt, const AZStd::string& broadcaster_id) override;
        void ModifyChannelInformation(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::string& game_id,
            const AZStd::string& broadcaster_language, const AZStd::string& title) override;
        void GetChannelEditors(ReceiptId& receipt, const AZStd::string& broadcaster_id) override;

        // Channel Points
        void CreateCustomRewards(ReceiptId& receipt, const AZStd::string& broadcaster_id,
            const CustomRewardSettings& rewardSettings) override;
        void DeleteCustomReward(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::string& id) override;
        void GetCustomReward(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::string& id,
            const bool only_manageable_rewards) override;
        void GetCustomRewardRedemption(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::string& reward_id,
            const AZStd::string& id, const AZStd::string& status, const AZStd::string& sort, const AZStd::string& after,
            AZ::u8 max_results = DEFAULT_MAX_RESULTS_LIMIT) override;
        void UpdateCustomReward(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::string& id,
            const CustomRewardSettings& rewardSettings) override;
        void UpdateRedemptionStatus(ReceiptId& receipt, const AZStd::string& id, const AZStd::string& broadcaster_id,
            const AZStd::string& reward_id, const AZStd::string& status) override;

        // Clips
        void CreateClip(ReceiptId& receipt, const AZStd::string& broadcaster_id, const bool has_delay = false) override;
        void GetClips(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::string& game_id, const AZStd::string& clip_id,
            const AZStd::string& started_at, const AZStd::string& ended_at, const AZStd::string& before,
            const AZStd::string& after, AZ::u8 max_results = TwitchApiRequests::DEFAULT_MAX_RESULTS_LIMIT) override;

        // Entitlements
        void CreateEntitlementGrantsUploadURL(ReceiptId& receipt, const AZStd::string& manifest_id,
            const AZStd::string& entitlement_type) override;
        void GetCodeStatus(ReceiptId& receipt, const AZStd::vector<AZStd::string>& codes, const AZ::u64 user_id) override;
        void GetDropsEntitlements(ReceiptId& receipt, const AZStd::string& id, const AZStd::string& user_id, const AZStd::string& game_id,
            const AZStd::string& after, AZ::u8 max_results = TwitchApiRequests::DEFAULT_MAX_RESULTS_LIMIT) override;
        void RedeemCode(ReceiptId& receipt, const AZStd::vector<AZStd::string>& codes, const AZ::u64 user_id) override;

        // Games
        void GetTopGames(ReceiptId& receipt, const AZStd::string& before, const AZStd::string& after,
            AZ::u8 max_results = TwitchApiRequests::DEFAULT_MAX_RESULTS_LIMIT) override;
        void GetGames(ReceiptId& receipt, const AZStd::string& game_id, const AZStd::string& game_name) override;

        // Hype Train
        void GetHypeTrainEvents(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::string& id,
            const AZStd::string& cursor, AZ::u8 max_results = TwitchApiRequests::DEFAULT_MAX_RESULTS_LIMIT) override;

        // Moderation
        void CheckAutoModStatus(ReceiptId& receipt, const AZStd::string& broadcaster_id,
            const AZStd::vector<CheckAutoModInputDatum>& filterTargets) override;
        void GetBannedEvents(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::vector<AZStd::string>& user_ids,
            const AZStd::string& after, AZ::u8 max_results = TwitchApiRequests::DEFAULT_MAX_RESULTS_LIMIT) override;
        void GetBannedUsers(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::vector<AZStd::string>& user_ids,
            const AZStd::string& before, const AZStd::string& after) override;
        void GetModerators(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::vector<AZStd::string>& user_ids,
            const AZStd::string& after) override;
        void GetModeratorEvents(ReceiptId& receipt, const AZStd::string& broadcaster_id,
            const AZStd::vector<AZStd::string>& user_ids) override;

        // Search
        void SearchCategories(ReceiptId& receipt, const AZStd::string& query, const AZStd::string& after,
            AZ::u8 max_results = TwitchApiRequests::DEFAULT_MAX_RESULTS_LIMIT) override;
        void SearchChannels(ReceiptId& receipt, const AZStd::string& query, const AZStd::string& after,
            const bool live_only, AZ::u8 max_results = TwitchApiRequests::DEFAULT_MAX_RESULTS_LIMIT) override;

        // Streams
        void GetStreamKey(ReceiptId& receipt, const AZStd::string& broadcaster_id) override;
        void GetStreams(ReceiptId& receipt, const AZStd::vector<AZStd::string>& game_ids, const AZStd::vector<AZStd::string>& user_ids,
            const AZStd::vector<AZStd::string>& user_logins, const AZStd::vector<AZStd::string>& languages, const AZStd::string& before,
            const AZStd::string& after, AZ::u8 max_results = TwitchApiRequests::DEFAULT_MAX_RESULTS_LIMIT) override;
        void CreateStreamMarker(ReceiptId& receipt, const AZStd::string& user_id, const AZStd::string& description) override;
        void GetStreamMarkers(ReceiptId& receipt, const AZStd::string& user_id, const AZStd::string& video_id, const AZStd::string& before,
            const AZStd::string& after, AZ::u8 max_results = TwitchApiRequests::DEFAULT_MAX_RESULTS_LIMIT) override;

        // Subscriptions
        void GetBroadcasterSubscriptions(ReceiptId& receipt, const AZStd::string& broadcaster_id,
            const AZStd::vector<AZStd::string>& user_ids) override;

        // Tags
        void GetAllStreamTags(ReceiptId& receipt, const AZStd::vector<AZStd::string>& tag_ids, const AZStd::string& after,
            AZ::u8 max_results = TwitchApiRequests::DEFAULT_MAX_RESULTS_LIMIT) override;
        void GetStreamTags(ReceiptId& receipt, const AZStd::string& broadcaster_id) override;
        void ReplaceStreamTags(ReceiptId& receipt, const AZStd::string& broadcaster_id,
            const AZStd::vector<AZStd::string>& tag_ids) override;

        // Users
        void GetUsers(ReceiptId& receipt, const AZStd::vector<AZStd::string>& user_ids,
            const AZStd::vector<AZStd::string>& user_logins) override;
        void UpdateUser(ReceiptId& receipt, const AZStd::string& description) override;
        void GetUserFollows(ReceiptId& receipt, AZStd::string from_id, const AZStd::string& to_id, const AZStd::string& after,
            AZ::u8 max_results = TwitchApiRequests::DEFAULT_MAX_RESULTS_LIMIT) override;
        void CreateUserFollows(ReceiptId& receipt, const AZStd::string& from_id, const AZStd::string& to_id,
            const bool allow_notifications) override;
        void DeleteUserFollows(ReceiptId& receipt, const AZStd::string& from_id, const AZStd::string& to_id) override;
        void GetUserBlockList(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::string& after,
            AZ::u8 max_results = TwitchApiRequests::DEFAULT_MAX_RESULTS_LIMIT) override;
        void BlockUser(ReceiptId& receipt, const AZStd::string& target_user_id, const AZStd::string& source_context,
            const AZStd::string& reason) override;
        void UnblockUser(ReceiptId& receipt, const AZStd::string& target_user_id) override;
        void GetUserExtensions(ReceiptId& receipt) override;
        void GetUserActiveExtensions(ReceiptId& receipt, const AZStd::string& user_id) override;
        void UpdateUserExtensions(ReceiptId& receipt, const AZStd::vector<GetUserActiveExtensionsComponent>& panel,
            const AZStd::vector<GetUserActiveExtensionsComponent>& overlay,
            const AZStd::vector<GetUserActiveExtensionsComponent>& component) override;

        // Videos
        void GetVideos(ReceiptId& receipt, const AZStd::vector<AZStd::string>& video_ids, const AZStd::string& user_id,
            const AZStd::string& game_id, const AZStd::string& video_type, const AZStd::string& sort_order, const AZStd::string& period,
            const AZStd::string& language, const AZStd::string& before, const AZStd::string& after,
            AZ::u8 max_results = TwitchApiRequests::DEFAULT_MAX_RESULTS_LIMIT) override;
        void DeleteVideos(ReceiptId& receipt, const AZStd::vector<AZStd::string>& video_ids) override;

        // Webhooks
        void GetWebhookSubscriptions(ReceiptId& receipt, const AZStd::string& after,
            AZ::u8 max_results = TwitchApiRequests::DEFAULT_MAX_RESULTS_LIMIT) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        //! Increments and returns on an atomic counter used to uniquely identify each made request.
        //! Primarily for tracking purposes and matching requests to responses
        AZ::u64 GetReceipt();

    private:
        //! Checks if OAuth has been provided and the Rest handler is available and returns a corresponding code.
        ResultCode GetUserAuthResultStatus() const;
        ResultCode GetAppAuthResultStatus() const;
        bool IsValidString(AZStd::string_view str, AZStd::size_t minLength, AZStd::size_t maxLength) const;
        bool IsValidFriendId(AZStd::string_view friendId) const;
        bool IsValidOAuthToken(AZStd::string_view oAuthToken) const;
        bool IsValidSyncToken(AZStd::string_view syncToken) const;
        bool IsValidChannelId(AZStd::string_view channelId) const { return IsValidFriendId(channelId); }
        bool IsValidCommunityId(AZStd::string_view communityId) const { return IsValidFriendId(communityId); }
        bool IsValidTwitchAppId(AZStd::string_view twitchAppId) const;

        void AddStringParam(AZStd::string& URI, AZStd::string_view key, AZStd::string_view value) const;
        void AddIntParam(AZStd::string& URI, AZStd::string_view key, const AZ::u64 value) const;
        void AddBoolParam(AZStd::string& URI, AZStd::string_view key, const bool value) const;

    private:
        AZStd::string           m_applicationId;        //!< Twitch Application ID, must be provided to the Gem.
        AZStd::string           m_cachedClientId;       //!< Twitch Client/User ID, must be provided to the Gem.
        AZStd::string           m_cachedUserAccessToken;//!< Twitch OAuth Token for client side API calls, must be provided to the Gem.
        AZStd::string           m_cachedAppAccessToken; //!< Twitch OAuth Token for server to server API calls, must be provided to the Gem.

        ITwitchApiRestPtr       m_restUtil;             //!< Handler to make REST calls and related utils        
        AZStd::atomic<AZ::u64>  m_receiptCounter;       //!< Atomic counter/identifier of Twitch API requests
    };
}
