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

#include <AzCore/Script/ScriptContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <TwitchApi/TwitchApiRequestBus.h>
#include "TwitchApiReflection.h"

namespace TwitchApi
{
#define ENUMCASE(code) case code: txtCode=AZStd::string::format(#code"(%llu)", aznumeric_cast<AZ::u64>(code)); break;

    AZStd::string ResultCodeToString(ResultCode code)
    {
        AZStd::string txtCode;
        switch (code)
        {
            ENUMCASE(ResultCode::Success)
                ENUMCASE(ResultCode::InvalidParam)
                ENUMCASE(ResultCode::TwitchRestError)
                ENUMCASE(ResultCode::TwitchChannelNoUpdatesToMake)
                ENUMCASE(ResultCode::Unknown)
        default:
            txtCode = AZStd::string::format("Undefined ResultCode (%llu)", aznumeric_cast<AZ::u64>(code));
        }

        return txtCode;
    }

    AZStd::string ReturnValueToString(const ReturnValue& info)
    {
        return AZStd::string::format("ReceiptId:%llu Result: %s", info.GetId(), ResultCodeToString(info.m_result).c_str());
    }

    namespace Internal
    {
        //! Reflect a Response Class
        #define TwitchApi_ReflectResponseTypeSerialization(context, _type)                                      \
            context.Class<_type>()->                                                                            \
                Version(0)->                                                                                    \
                Field("Data", &_type::m_data);                      

        //! Reflect a Pagination Response Class
        #define TwitchApi_ReflectPaginationResponseTypeSerialization(context, _type)                            \
            context.Class<_type>()->                                                                            \
                Version(0)->                                                                                    \
                Field("Data", &_type::m_data)

        //! Reflect a Result class     
        #define TwitchApi_ReflectReturnTypeSerialization(context, _type)                                        \
            context.Class<_type>()->                                                                            \
                Version(0)->                                                                                    \
                Field("Value", &_type::m_value)->                                                               \
                FieldFromBase<_type>("Result", &_type::m_result);

        void ReflectSerialization(AZ::SerializeContext& context)
        {
            context.Class<ReceiptId>()->
                Version(0)
                ;

            context.Class<PaginationResponse>()->
                Field("Pagination", &PaginationResponse::m_pagination);

            context.Class<CustomRewardSettings>()->
                Field("IsEnabled", &CustomRewardSettings::m_is_enabled)->
                Field("Cost", &CustomRewardSettings::m_cost)->
                Field("Title", &CustomRewardSettings::m_title)->
                Field("Prompt", &CustomRewardSettings::m_prompt)->
                Field("BackgroundColor", &CustomRewardSettings::m_background_color)->
                Field("IsUserInputRequired", &CustomRewardSettings::m_is_user_input_required)->
                Field("IsMaxPerStreamEnabled", &CustomRewardSettings::m_is_max_per_stream_enabled)->
                Field("MaxPerStream", &CustomRewardSettings::m_max_per_stream)->
                Field("IsMaxPerUserPerStreamEnabled", &CustomRewardSettings::m_is_max_per_user_per_stream_enabled)->
                Field("MaxPerUserPerStream", &CustomRewardSettings::m_max_per_user_per_stream)->
                Field("IsGlobalCooldownEnabled", &CustomRewardSettings::m_is_global_cooldown_enabled)->
                Field("GlobalCooldownSeconds", &CustomRewardSettings::m_global_cooldown_seconds)->
                Field("ShouldRedemptionsSkipRequestQueue", &CustomRewardSettings::m_should_redemptions_skip_request_queue);

            context.Class<CommercialDatum>()->
                Field("Length", &CommercialDatum::length)->
                Field("Message", &CommercialDatum::message)->
                Field("RetryAfter", &CommercialDatum::retry_after)
                ;
            TwitchApi_ReflectResponseTypeSerialization(context, CommercialResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, CommercialResult);

            context.Class<ExtensionAnalyticsDatum>()->
                Version(0)->
                Field("ExtensionId", &ExtensionAnalyticsDatum::extension_id)->
                Field("Type", &ExtensionAnalyticsDatum::type)->
                Field("StartedAt", &ExtensionAnalyticsDatum::started_at)->
                Field("EndedAt", &ExtensionAnalyticsDatum::ended_at)->
                Field("URL", &ExtensionAnalyticsDatum::url)
                ;

            TwitchApi_ReflectPaginationResponseTypeSerialization(context, ExtensionAnalyticsResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, ExtensionAnalyticsResult);

            context.Class<GameAnalyticsDatum>()->
                Version(0)->
                Field("GameId", &GameAnalyticsDatum::game_id)->
                Field("Type", &GameAnalyticsDatum::type)->
                Field("StartedAt", &GameAnalyticsDatum::started_at)->
                Field("EndedAt", &GameAnalyticsDatum::ended_at)->
                Field("URL", &GameAnalyticsDatum::url)
                ;

            TwitchApi_ReflectPaginationResponseTypeSerialization(context, GameAnalyticsResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, GameAnalyticsResult);

            context.Class<BitsLeaderboardDatum>()->
                Version(0)->
                Field("Rank", &BitsLeaderboardDatum::rank)->
                Field("Score", &BitsLeaderboardDatum::score)->
                Field("UserId", &BitsLeaderboardDatum::user_id)->
                Field("UserName", &BitsLeaderboardDatum::user_name)
                ;

            context.Class<BitsLeaderboardResponse>()->
                Field("Total", &BitsLeaderboardResponse::total)->
                Field("StartedAt", &BitsLeaderboardResponse::started_at)->
                Field("EndedAt", &BitsLeaderboardResponse::ended_at)->
                Field("Data", &BitsLeaderboardResponse::data)
                ;

            TwitchApi_ReflectReturnTypeSerialization(context, BitsLeaderboardResult);

            context.Class<CheermotesImageSetDatum>()->
                Field("URL1x", &CheermotesImageSetDatum::url_1x)->
                Field("URL1.5x", &CheermotesImageSetDatum::url_15x)->
                Field("URL2x", &CheermotesImageSetDatum::url_2x)->
                Field("URL3x", &CheermotesImageSetDatum::url_3x)->
                Field("URL4x", &CheermotesImageSetDatum::url_4x)
                ;

            context.Class<CheermotesImagesDatum>()->
                Field("DarkAnimated", &CheermotesImagesDatum::dark_animated)->
                Field("DarkStatic", &CheermotesImagesDatum::dark_static)->
                Field("LightAnimated", &CheermotesImagesDatum::light_animated)->
                Field("LightStatic", &CheermotesImagesDatum::light_static)
                ;

            context.Class<CheermotesTiersDatum>()->
                Field("MinBits", &CheermotesTiersDatum::min_bits)->
                Field("Id", &CheermotesTiersDatum::id)->
                Field("Color", &CheermotesTiersDatum::color)->
                Field("Images", &CheermotesTiersDatum::images)->
                Field("CanCheer", &CheermotesTiersDatum::can_cheer)->
                Field("ShowInBitsCard", &CheermotesTiersDatum::show_in_bits_card)
                ;

            context.Class<CheermotesDatum>()->
                Field("Prefix", &CheermotesDatum::prefix)->
                Field("Type", &CheermotesDatum::type)->
                Field("Order", &CheermotesDatum::order)->
                Field("LastUpdated", &CheermotesDatum::last_updated)->
                Field("IsCharitable", &CheermotesDatum::is_charitable)->
                Field("Tiers", &CheermotesDatum::tiers)
                ;
            TwitchApi_ReflectResponseTypeSerialization(context, CheermotesResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, CheermotesResult);

            context.Class<ExtensionTransactionProductDatum>()->
                Field("SKU", &ExtensionTransactionProductDatum::sku)->
                Field("Cost", &ExtensionTransactionProductDatum::cost_amount)->
                Field("CostType", &ExtensionTransactionProductDatum::cost_type)->
                Field("Domain", &ExtensionTransactionProductDatum::domain)->
                Field("DisplayName", &ExtensionTransactionProductDatum::displayName)->
                Field("IsInDevelopment", &ExtensionTransactionProductDatum::inDevelopment)->
                Field("WasBroadcasted", &ExtensionTransactionProductDatum::broadcast)
                ;

            context.Class<ExtensionTransactionDatum>()->
                Field("Id", &ExtensionTransactionDatum::id)->
                Field("Timestamp", &ExtensionTransactionDatum::timestamp)->
                Field("BroadcasterId", &ExtensionTransactionDatum::broadcaster_id)->
                Field("BroadcasterName", &ExtensionTransactionDatum::broadcaster_name)->
                Field("UserId", &ExtensionTransactionDatum::user_id)->
                Field("UserName", &ExtensionTransactionDatum::user_name)->
                Field("ProductType", &ExtensionTransactionDatum::product_type)->
                Field("ProductData", &ExtensionTransactionDatum::product_data)
                ;

            TwitchApi_ReflectResponseTypeSerialization(context, ExtensionTransactionsResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, ExtensionTransactionsResult);

            context.Class<ChannelInformationDatum>()->
                Field("BroadcasterId", &ChannelInformationDatum::broadcaster_id)->
                Field("BroadcasterName", &ChannelInformationDatum::broadcaster_name)->
                Field("BroadcasterLanguage", &ChannelInformationDatum::broadcaster_language)->
                Field("GameId", &ChannelInformationDatum::game_id)->
                Field("GameName", &ChannelInformationDatum::game_name)->
                Field("Title", &ChannelInformationDatum::title)
                ;

            TwitchApi_ReflectResponseTypeSerialization(context, ChannelInformationResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, ChannelInformationResult);

            context.Class<ChannelEditorsDatum>()->
                Field("UserId", &ChannelEditorsDatum::user_id)->
                Field("UserName", &ChannelEditorsDatum::user_name)->
                Field("CreatedAt", &ChannelEditorsDatum::created_at)
                ;

            TwitchApi_ReflectResponseTypeSerialization(context, ChannelEditorsResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, ChannelEditorsResult);

            context.Class<CustomRewardsImageDatum>()->
                Field("Url1x", &CustomRewardsImageDatum::url_1x)->
                Field("Url2x", &CustomRewardsImageDatum::url_2x)->
                Field("Url4x", &CustomRewardsImageDatum::url_4x)
                ;

            context.Class<CustomRewardsDatum>()->
                Field("BroadcasterId", &CustomRewardsDatum::broadcaster_id)->
                Field("BroadcasterName", &CustomRewardsDatum::broadcaster_name)->
                Field("Id", &CustomRewardsDatum::id)->
                Field("Image", &CustomRewardsDatum::image)->
                Field("BackgroundColor", &CustomRewardsDatum::background_color)->
                Field("IsEnabled", &CustomRewardsDatum::is_enabled)->
                Field("Cost", &CustomRewardsDatum::cost)->
                Field("Title", &CustomRewardsDatum::title)->
                Field("Prompt", &CustomRewardsDatum::prompt)->
                Field("IsUserInputRequired", &CustomRewardsDatum::is_user_input_required)->
                Field("IsMaxPerStreamEnabled", &CustomRewardsDatum::is_max_per_stream_enabled)->
                Field("MaxPerStream", &CustomRewardsDatum::max_per_stream)->
                Field("IsMaxPerUserPerStreamEnabled", &CustomRewardsDatum::is_max_per_user_per_stream_enabled)->
                Field("MaxPerUserPerStream", &CustomRewardsDatum::max_per_user_per_stream)->
                Field("IsGlobalCooldownEnabled", &CustomRewardsDatum::is_global_cooldown_enabled)->
                Field("GlobalCooldownSeconds", &CustomRewardsDatum::global_cooldown_seconds)->
                Field("IsPaused", &CustomRewardsDatum::is_paused)->
                Field("IsInStock", &CustomRewardsDatum::is_in_stock)->
                Field("DefaultImage", &CustomRewardsDatum::default_image)->
                Field("ShouldRedemptionsSkipRequestQueue", &CustomRewardsDatum::should_redemptions_skip_request_queue)->
                Field("RedemptionsRedeemedCurrentStream", &CustomRewardsDatum::redemptions_redeemed_current_stream)->
                Field("CooldownExpiresAt", &CustomRewardsDatum::cooldown_expires_at)
                ;

            TwitchApi_ReflectResponseTypeSerialization(context, CustomRewardsResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, CustomRewardsResult);

            context.Class<CustomRewardsRedemptionRewardDatum>()->
                Field("Id", &CustomRewardsRedemptionRewardDatum::id)->
                Field("Title", &CustomRewardsRedemptionRewardDatum::title)->
                Field("Prompt", &CustomRewardsRedemptionRewardDatum::prompt)->
                Field("Cost", &CustomRewardsRedemptionRewardDatum::cost)
                ;

            context.Class<CustomRewardsRedemptionDatum>()->
                Field("BroadcasterName", &CustomRewardsRedemptionDatum::broadcaster_name)->
                Field("BroadcasterId", &CustomRewardsRedemptionDatum::broadcaster_id)->
                Field("Id", &CustomRewardsRedemptionDatum::id)->
                Field("UserId", &CustomRewardsRedemptionDatum::user_id)->
                Field("UserName", &CustomRewardsRedemptionDatum::user_name)->
                Field("UserInput", &CustomRewardsRedemptionDatum::user_input)->
                Field("Status", &CustomRewardsRedemptionDatum::status)->
                Field("RedeemedAt", &CustomRewardsRedemptionDatum::redeemed_at)->
                Field("Reward", &CustomRewardsRedemptionDatum::reward)
                ;

            TwitchApi_ReflectPaginationResponseTypeSerialization(context, CustomRewardsRedemptionResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, CustomRewardsRedemptionResult);

            context.Class<CreateClipDatum>()->
                Field("Id", &CreateClipDatum::id)->
                Field("EditURL", &CreateClipDatum::edit_url)
                ;
            TwitchApi_ReflectResponseTypeSerialization(context, CreateClipResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, CreateClipResult);

            context.Class<GetClipsDatum>()->
                Field("BroadcasterId", &GetClipsDatum::broadcaster_id)->
                Field("BroadcasterName", &GetClipsDatum::broadcaster_name)->
                Field("CreatedAt", &GetClipsDatum::created_at)->
                Field("CreatorId", &GetClipsDatum::creator_id)->
                Field("CreatorName", &GetClipsDatum::creator_name)->
                Field("EmbedURL", &GetClipsDatum::embed_url)->
                Field("GameId", &GetClipsDatum::game_id)->
                Field("Id", &GetClipsDatum::id)->
                Field("Language", &GetClipsDatum::language)->
                Field("ThumbnailURL", &GetClipsDatum::thumbnail_url)->
                Field("Title", &GetClipsDatum::title)->
                Field("URL", &GetClipsDatum::url)->
                Field("VideoId", &GetClipsDatum::video_id)->
                Field("ViewCount", &GetClipsDatum::view_count)
                ;
            TwitchApi_ReflectPaginationResponseTypeSerialization(context, GetClipsResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, GetClipsResult);

            context.Class<EntitlementsGrantUploadResponse>()->
                Field("URLs", &EntitlementsGrantUploadResponse::urls)
                ;
            TwitchApi_ReflectReturnTypeSerialization(context, EntitlementsGrantUploadResult);

            context.Class<CodeStatusResponse>()->
                Field("Statuses", &CodeStatusResponse::statuses)
                ;
            TwitchApi_ReflectReturnTypeSerialization(context, CodeStatusResult);

            context.Class<DropsEntitlementsDatum>()->
                Field("Id", &DropsEntitlementsDatum::id)->
                Field("BenefitId", &DropsEntitlementsDatum::benefit_id)->
                Field("Timestamp", &DropsEntitlementsDatum::timestamp)->
                Field("UserId", &DropsEntitlementsDatum::user_id)->
                Field("GameId", &DropsEntitlementsDatum::game_id)
                ;
            TwitchApi_ReflectPaginationResponseTypeSerialization(context, DropsEntitlementsResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, DropsEntitlementsResult);

            context.Class<GetGamesDatum>()->
                Field("BoxArtURL", &GetGamesDatum::box_art_url)->
                Field("Id", &GetGamesDatum::id)->
                Field("Name", &GetGamesDatum::name)
                ;
            TwitchApi_ReflectPaginationResponseTypeSerialization(context, GetTopGamesResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, GetTopGamesResult);
            TwitchApi_ReflectResponseTypeSerialization(context, GetGamesResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, GetGamesResult);

            context.Class<HypeTrainContributionDatum>()->
                Field("Total", &HypeTrainContributionDatum::total)->
                Field("Type", &HypeTrainContributionDatum::type)->
                Field("User", &HypeTrainContributionDatum::user)
                ;

            context.Class<HypeTrainEventDatum>()->
                Field("BroadcasterId", &HypeTrainEventDatum::broadcaster_id)->
                Field("CooldownEndTime", &HypeTrainEventDatum::cooldown_end_time)->
                Field("ExpiresAt", &HypeTrainEventDatum::expires_at)->
                Field("Goal", &HypeTrainEventDatum::goal)->
                Field("Id", &HypeTrainEventDatum::id)->
                Field("LastContribution", &HypeTrainEventDatum::last_contribution)->
                Field("Level", &HypeTrainEventDatum::level)->
                Field("StartedAt", &HypeTrainEventDatum::started_at)->
                Field("TopContributions", &HypeTrainEventDatum::top_contributions)->
                Field("Total", &HypeTrainEventDatum::total)
                ;

            context.Class<HypeTrainEventsDatum>()->
                Field("Id", &HypeTrainEventsDatum::id)->
                Field("EventType", &HypeTrainEventsDatum::event_type)->
                Field("EventTimestamp", &HypeTrainEventsDatum::event_timestamp)->
                Field("Version", &HypeTrainEventsDatum::version)->
                Field("EventData", &HypeTrainEventsDatum::event_data)
                ;
            TwitchApi_ReflectPaginationResponseTypeSerialization(context, HypeTrainEventsResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, HypeTrainEventsResult);

            context.Class<CheckAutoModInputDatum>()->
                Field("MsgId", &CheckAutoModInputDatum::msg_id)->
                Field("MsgText", &CheckAutoModInputDatum::msg_text)->
                Field("UserId", &CheckAutoModInputDatum::user_id)
                ;

            context.Class<CheckAutoModResponseDatum>()->
                Field("MsgId", &CheckAutoModResponseDatum::msg_id)->
                Field("IsPermitted", &CheckAutoModResponseDatum::is_permitted)
                ;
            TwitchApi_ReflectResponseTypeSerialization(context, CheckAutoModResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, CheckAutoModResult);

            context.Class<GetBannedEventsDatum>()->
                Field("Id", &GetBannedEventsDatum::id)->
                Field("EventType", &GetBannedEventsDatum::event_type)->
                Field("EventTimestamp", &GetBannedEventsDatum::event_timestamp)->
                Field("BroadcasterId", &GetBannedEventsDatum::broadcaster_id)->
                Field("BroadcasterName", &GetBannedEventsDatum::broadcaster_name)->
                Field("UserId", &GetBannedEventsDatum::user_id)->
                Field("UserName", &GetBannedEventsDatum::user_name)->
                Field("ExpiresAt", &GetBannedEventsDatum::expires_at)->
                Field("Version", &GetBannedEventsDatum::version)
                ;
            TwitchApi_ReflectPaginationResponseTypeSerialization(context, GetBannedEventsResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, GetBannedEventsResult);

            context.Class<GetBannedUsersDatum>()->
                Field("UserId", &GetBannedUsersDatum::user_id)->
                Field("UserName", &GetBannedUsersDatum::user_name)->
                Field("ExpiresAt", &GetBannedUsersDatum::expires_at)
                ;
            TwitchApi_ReflectPaginationResponseTypeSerialization(context, GetBannedUsersResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, GetBannedUsersResult);

            context.Class<GetModeratorsDatum>()->
                Field("UserId", &GetModeratorsDatum::user_id)->
                Field("UserName", &GetModeratorsDatum::user_name)
                ;
            TwitchApi_ReflectPaginationResponseTypeSerialization(context, GetModeratorsResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, GetModeratorsResult);

            context.Class<GetModeratorEventsDatum>()->
                Field("Id", &GetModeratorEventsDatum::id)->
                Field("EventType", &GetModeratorEventsDatum::event_type)->
                Field("EventTimestamp", &GetModeratorEventsDatum::event_timestamp)->
                Field("Version", &GetModeratorEventsDatum::version)
                ;
            TwitchApi_ReflectPaginationResponseTypeSerialization(context, GetModeratorEventsResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, GetModeratorEventsResult);

            context.Class<SearchCategoriesDatum>()->
                Field("Id", &SearchCategoriesDatum::id)->
                Field("Name", &SearchCategoriesDatum::name)->
                Field("BoxArtUrl", &SearchCategoriesDatum::box_art_url)
                ;
            TwitchApi_ReflectPaginationResponseTypeSerialization(context, SearchCategoriesResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, SearchCategoriesResult);

            context.Class<SearchChannelsDatum>()->
                Field("BroadcasterLanguage", &SearchChannelsDatum::broadcaster_language)->
                Field("DisplayName", &SearchChannelsDatum::display_name)->
                Field("GameId", &SearchChannelsDatum::game_id)->
                Field("Id", &SearchChannelsDatum::id)->
                Field("IsLive", &SearchChannelsDatum::is_live)->
                Field("TagIds", &SearchChannelsDatum::tag_ids)->
                Field("ThumbnailUrl", &SearchChannelsDatum::thumbnail_url)->
                Field("Title", &SearchChannelsDatum::title)->
                Field("StartedAt", &SearchChannelsDatum::started_at)
                ;
            TwitchApi_ReflectPaginationResponseTypeSerialization(context, SearchChannelsResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, SearchChannelsResult);

            context.Class<StreamKeyResponse>()->
                Field("StreamKey", &StreamKeyResponse::stream_key)
                ;
            TwitchApi_ReflectReturnTypeSerialization(context, StreamKeyResult);

            context.Class<GetStreamsDatum>()->
                Field("GameId", &GetStreamsDatum::game_id)->
                Field("Type", &GetStreamsDatum::type)->
                Field("StartedAt", &GetStreamsDatum::started_at)->
                Field("TagIds", &GetStreamsDatum::tag_ids)->
                Field("UserId", &GetStreamsDatum::user_id)->
                Field("UserName", &GetStreamsDatum::user_name)->
                Field("Id", &GetStreamsDatum::id)->
                Field("Language", &GetStreamsDatum::language)->
                Field("ThumbnailURL", &GetStreamsDatum::thumbnail_url)->
                Field("Title", &GetStreamsDatum::title)->
                Field("ViewerCount", &GetStreamsDatum::viewer_count)
                ;
            TwitchApi_ReflectPaginationResponseTypeSerialization(context, GetStreamsResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, GetStreamsResult);

            context.Class<CreateStreamMarkerDatum>()->
                Field("CreatedAt", &CreateStreamMarkerDatum::created_at)->
                Field("Description", &CreateStreamMarkerDatum::description)->
                Field("Id", &CreateStreamMarkerDatum::id)->
                Field("PositionSeconds", &CreateStreamMarkerDatum::position_seconds)
                ;
            TwitchApi_ReflectResponseTypeSerialization(context, CreateStreamMarkerResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, CreateStreamMarkerResult);

            context.Class<GetStreamMarkersMarkerDatum>()->
                Field("CreatedAt", &GetStreamMarkersMarkerDatum::created_at)->
                Field("Id", &GetStreamMarkersMarkerDatum::id)->
                Field("Description", &GetStreamMarkersMarkerDatum::description)->
                Field("URL", &GetStreamMarkersMarkerDatum::URL)->
                Field("PositionSeconds", &GetStreamMarkersMarkerDatum::position_seconds)
                ;

            context.Class<GetStreamMarkersVideoDatum>()->
                Field("VideoId", &GetStreamMarkersVideoDatum::video_id)->
                Field("Markers", &GetStreamMarkersVideoDatum::markers)
                ;

            context.Class<GetStreamMarkersDatum>()->
                Field("UserId", &GetStreamMarkersDatum::user_id)->
                Field("UserName", &GetStreamMarkersDatum::user_name)->
                Field("Videos", &GetStreamMarkersDatum::videos)
                ;
            TwitchApi_ReflectPaginationResponseTypeSerialization(context, GetStreamMarkersResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, GetStreamMarkersResult);

            context.Class<GetBroadcasterSubscriptionsDatum>()->
                Field("BroadcasterId", &GetBroadcasterSubscriptionsDatum::broadcaster_id)->
                Field("BroadcasterName", &GetBroadcasterSubscriptionsDatum::broadcaster_name)->
                Field("UserId", &GetBroadcasterSubscriptionsDatum::user_id)->
                Field("UserName", &GetBroadcasterSubscriptionsDatum::user_name)->
                Field("Tier", &GetBroadcasterSubscriptionsDatum::tier)->
                Field("PlanName", &GetBroadcasterSubscriptionsDatum::plan_name)->
                Field("IsGift", &GetBroadcasterSubscriptionsDatum::is_gift)
                ;
            TwitchApi_ReflectPaginationResponseTypeSerialization(context, GetBroadcasterSubscriptionsResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, GetBroadcasterSubscriptionsResult);

            context.Class<GetStreamTagsDatum>()->
                Field("IsAuto", &GetStreamTagsDatum::is_auto)->
                Field("LocalizationNames", &GetStreamTagsDatum::localization_names)->
                Field("LocalizationDescriptions", &GetStreamTagsDatum::localization_descriptions)->
                Field("TagId", &GetStreamTagsDatum::tag_id)
                ;
            TwitchApi_ReflectResponseTypeSerialization(context, GetStreamTagsResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, GetStreamTagsResult);
            TwitchApi_ReflectPaginationResponseTypeSerialization(context, GetAllStreamTagsResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, GetAllStreamTagsResult);

            context.Class<GetUsersDatum>()->
                Field("BroadcasterType", &GetUsersDatum::broadcaster_type)->
                Field("Description", &GetUsersDatum::description)->
                Field("DisplayName", &GetUsersDatum::display_name)->
                Field("Email", &GetUsersDatum::email)->
                Field("Id", &GetUsersDatum::id)->
                Field("Login", &GetUsersDatum::login)->
                Field("OfflineImageURL", &GetUsersDatum::offline_image_url)->
                Field("ProfileImageURL", &GetUsersDatum::profile_image_url)->
                Field("Type", &GetUsersDatum::type)->
                Field("ViewCount", &GetUsersDatum::view_count)
                ;
            TwitchApi_ReflectResponseTypeSerialization(context, GetUsersResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, GetUsersResult);

            context.Class<GetUsersFollowsDatum>()->
                Field("FollowedAt", &GetUsersFollowsDatum::followed_at)->
                Field("FromId", &GetUsersFollowsDatum::from_id)->
                Field("FromName", &GetUsersFollowsDatum::from_name)->
                Field("ToId", &GetUsersFollowsDatum::to_id)->
                Field("ToName", &GetUsersFollowsDatum::to_name)->
                Field("Total", &GetUsersFollowsDatum::total)
                ;
            TwitchApi_ReflectPaginationResponseTypeSerialization(context, GetUsersFollowsResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, GetUsersFollowsResult);

            context.Class<UserBlockListDatum>()->
                Field("Id", &UserBlockListDatum::id)->
                Field("UserLogin", &UserBlockListDatum::user_login)->
                Field("DisplayName", &UserBlockListDatum::display_name)
                ;
            TwitchApi_ReflectPaginationResponseTypeSerialization(context, UserBlockListResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, UserBlockListResult);

            context.Class<GetUserExtensionsDatum>()->
                Field("Id", &GetUserExtensionsDatum::id)->
                Field("Type", &GetUserExtensionsDatum::type)->
                Field("Name", &GetUserExtensionsDatum::name)->
                Field("Version", &GetUserExtensionsDatum::version)->
                Field("CanActivate", &GetUserExtensionsDatum::can_activate)
                ;
            TwitchApi_ReflectResponseTypeSerialization(context, GetUserExtensionsResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, GetUserExtensionsResult);

            context.Class<GetUserActiveExtensionsComponent>()->
                Field("Active", &GetUserActiveExtensionsComponent::active)->
                Field("Id", &GetUserActiveExtensionsComponent::id)->
                Field("Version", &GetUserActiveExtensionsComponent::version)->
                Field("Name", &GetUserActiveExtensionsComponent::name)->
                Field("x", &GetUserActiveExtensionsComponent::x)->
                Field("y", &GetUserActiveExtensionsComponent::y)
                ;

            context.Class<GetUserActiveExtensionsResponse>()->
                Field("Panel", &GetUserActiveExtensionsResponse::panel)->
                Field("Overlay", &GetUserActiveExtensionsResponse::overlay)->
                Field("Component", &GetUserActiveExtensionsResponse::component)
                ;
            TwitchApi_ReflectReturnTypeSerialization(context, GetUserActiveExtensionsResult);

            context.Class<GetVideosDatum>()->
                Field("CreatedAt", &GetVideosDatum::created_at)->
                Field("Description", &GetVideosDatum::description)->
                Field("Duration", &GetVideosDatum::duration)->
                Field("Id", &GetVideosDatum::id)->
                Field("Language", &GetVideosDatum::language)->
                Field("PublishedAt", &GetVideosDatum::published_at)->
                Field("ThumbnailURL", &GetVideosDatum::thumbnail_url)->
                Field("Title", &GetVideosDatum::title)->
                Field("Type", &GetVideosDatum::type)->
                Field("URL", &GetVideosDatum::url)->
                Field("UserId", &GetVideosDatum::user_id)->
                Field("UserName", &GetVideosDatum::user_name)->
                Field("ViewCount", &GetVideosDatum::view_count)->
                Field("Viewable", &GetVideosDatum::viewable)
                ;
            TwitchApi_ReflectPaginationResponseTypeSerialization(context, GetVideosResponse);
            TwitchApi_ReflectReturnTypeSerialization(context, GetVideosResult);

            context.Class<DeleteVideosResponse>()->
                Field("VideoKeys", &DeleteVideosResponse::video_keys)
                ;
            TwitchApi_ReflectReturnTypeSerialization(context, DeleteVideosResult);

            context.Class<GetWebhookSubscriptionsDatum>()->
                Field("Callback", &GetWebhookSubscriptionsDatum::callback)->
                Field("ExpiresAt", &GetWebhookSubscriptionsDatum::expires_at)->
                Field("Topic", &GetWebhookSubscriptionsDatum::topic)
                ;
            context.Class<GetWebhookSubscriptionsResponse>()->
                Field("Data", &GetWebhookSubscriptionsResponse::data)->
                Field("Total", &GetWebhookSubscriptionsResponse::total);
            TwitchApi_ReflectReturnTypeSerialization(context, GetWebhookSubscriptionsResult);
        }
    }
}
