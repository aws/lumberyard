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

#include <TwitchApi/RestTypes.h>

namespace Aws
{
    namespace Utils
    {
        namespace Json
        {
            class JsonView;
        } // namespace Json
    } // namespace Utils
} // namespace Aws

namespace TwitchApi
{
    namespace RestUtil
    {
        // Ads
        void PopulateCommercialResponse(const Aws::Utils::Json::JsonView& jsonDoc, CommercialResponse& comResponse);

        // Analytics
        void PopulateExtensionAnalyticsResponse(const Aws::Utils::Json::JsonView& jsonDoc, ExtensionAnalyticsResponse& analyticsResponse);
        void PopulateGameAnalyticsResponse(const Aws::Utils::Json::JsonView& jsonDoc, GameAnalyticsResponse& analyticsResponse);

        // Bits
        void PopulateBitsLeaderboardResponse(const Aws::Utils::Json::JsonView& jsonDoc, BitsLeaderboardResponse& bitsResponse);
        void PopulateCheermotesResponse(const Aws::Utils::Json::JsonView& jsonDoc, CheermotesResponse& cheersResponse);
        void PopulateExtensionTransactionsResponse(const Aws::Utils::Json::JsonView& jsonDoc, ExtensionTransactionsResponse& xactResponse);

        // Channels
        void PopulateChannelInformationResponse(const Aws::Utils::Json::JsonView& jsonDoc, ChannelInformationResponse& channelResponse);
        void PopulateChannelEditorsResponse(const Aws::Utils::Json::JsonView& jsonDoc, ChannelEditorsResponse& channelResponse);

        // Channel Rewards
        void PopulateCustomRewardsResponse(const Aws::Utils::Json::JsonView& jsonDoc, CustomRewardsResponse& rewardsResponse);
        void PopulateCustomRewardsRedemptionResponse(const Aws::Utils::Json::JsonView& jsonDoc,
            CustomRewardsRedemptionResponse& rewardsResponse);

        // Clips
        void PopulateCreateClipResponse(const Aws::Utils::Json::JsonView& jsonDoc, CreateClipResponse& clipResponse);
        void PopulateGetClipsResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetClipsResponse& clipResponse);

        // Entitlements
        void PopulateEntitlementGrantsUploadURLResponse(const Aws::Utils::Json::JsonView& jsonDoc,
            EntitlementsGrantUploadResponse& entitlementResponse);
        void PopulateCodeStatusResponse(const Aws::Utils::Json::JsonView& jsonDoc, CodeStatusResponse& codeResponse);
        void PopulateDropsEntitlementsResponse(const Aws::Utils::Json::JsonView& jsonDoc, DropsEntitlementsResponse& dropResponse);

        // Games
        void PopulateGetTopGamesResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetTopGamesResponse& gamesResponse);
        void PopulateGetGamesResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetGamesResponse& gamesResponse);

        // Hype Train
        void PopulateHypeTrainEventsResponse(const Aws::Utils::Json::JsonView& jsonDoc, HypeTrainEventsResponse& hypeResponse);

        // Moderation
        void PopulateCheckAutoModResponse(const Aws::Utils::Json::JsonView& jsonDoc, CheckAutoModResponse& modResponse);
        void PopulateGetBannedEventsResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetBannedEventsResponse& bannedResponse);
        void PopulateGetBannedUsersResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetBannedUsersResponse& bannedResponse);
        void PopulateGetModeratorsResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetModeratorsResponse& modResponse);
        void PopulateGetModeratorEventsResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetModeratorEventsResponse& modResponse);

        // Search
        void PopulateSearchCategoriesResponse(const Aws::Utils::Json::JsonView& jsonDoc, SearchCategoriesResponse& searchResponse);
        void PopulateSearchChannelsResponse(const Aws::Utils::Json::JsonView& jsonDoc, SearchChannelsResponse& searchResponse);

        // Streams
        void PopulateStreamKeyResponse(const Aws::Utils::Json::JsonView& jsonDoc, StreamKeyResponse& keyResponse);
        void PopulateGetStreamsResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetStreamsResponse& streamsResponse);
        void PopulateCreateStreamMarkerResponse(const Aws::Utils::Json::JsonView& jsonDoc, CreateStreamMarkerResponse& streamsResponse);
        void PopulateGetStreamMarkersResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetStreamMarkersResponse& streamsResponse);

        // Subscriptions
        void PopulateGetBroadcasterSubscriptionsResponse(const Aws::Utils::Json::JsonView& jsonDoc,
            GetBroadcasterSubscriptionsResponse& subResponse);

        // Tags
        void PopulateGetAllStreamTagsResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetAllStreamTagsResponse& tagsResponse);
        void PopulateGetStreamTagsResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetStreamTagsResponse& tagsResponse);

        // Users
        void PopulateGetUsersResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetUsersResponse& usersResponse);
        void PopulateGetUsersFollowsResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetUsersFollowsResponse& usersResponse);
        void PopulateUserBlockListResponse(const Aws::Utils::Json::JsonView& jsonDoc, UserBlockListResponse& blockResponse);
        void PopulateGetUserExtensionsResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetUserExtensionsResponse& usersResponse);
        void PopulateGetUserActiveExtensionsResponse(const Aws::Utils::Json::JsonView& jsonDoc,
            GetUserActiveExtensionsResponse& usersResponse);

        // Videos
        void PopulateGetVideosResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetVideosResponse& videosResponse);
        void PopulateDeleteVideosResponse(const Aws::Utils::Json::JsonView& jsonDoc, DeleteVideosResponse& videosResponse);

        // Webhooks
        void PopulateGetWebhookSubscriptionsResponse(const Aws::Utils::Json::JsonView& jsonDoc,
            GetWebhookSubscriptionsResponse& webhooksResponse);
    } // namespace TwitchApi::RestUtil
} // namespace TwitchApi

