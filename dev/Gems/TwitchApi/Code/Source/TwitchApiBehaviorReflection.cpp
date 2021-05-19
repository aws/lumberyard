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
#include <TwitchApi/TwitchApiNotifyBus.h>
#include "TwitchApiReflection.h"

namespace TwitchApi
{
    namespace Internal
    {
        class BehaviorTwitchApiNotifyBus
            : public TwitchApiNotifyBus::Handler
            , public AZ::BehaviorEBusHandler
        {
        public:
            AZ_EBUS_BEHAVIOR_BINDER(BehaviorTwitchApiNotifyBus, "{1239A193-2120-423A-82CF-B04FDEF9AD5F}", AZ::SystemAllocator,
                UserIdNotify,
                UserAccessTokenNotify,
                AppAccessTokenNotify,
                OnStartCommercial,
                OnGetExtensionAnalytics,
                OnGetGameAnalytics,
                OnGetBitsLeaderboard,
                OnGetCheermotes,
                OnGetExtensionTransactions,
                OnGetChannelInformation,
                OnModifyChannelInformation,
                OnGetChannelEditors,
                OnCreateCustomRewards,
                OnDeleteCustomReward,
                OnGetCustomReward,
                OnGetCustomRewardRedemption,
                OnUpdateCustomReward,
                OnUpdateRedemptionStatus,
                OnCreateClip,
                OnGetClips,
                OnCreateEntitlementGrantsUploadURL,
                OnGetCodeStatus,
                OnGetDropsEntitlements,
                OnRedeemCode,
                OnGetTopGames,
                OnGetGames,
                OnGetHypeTrainEvents,
                OnCheckAutoModStatus,
                OnGetBannedEvents,
                OnGetBannedUsers,
                OnGetModerators,
                OnGetModeratorEvents,
                OnSearchCategories,
                OnSearchChannels,
                OnGetStreamKey,
                OnGetStreams,
                OnCreateStreamMarker,
                OnGetStreamMarkers,
                OnGetBroadcasterSubscriptions,
                OnGetAllStreamTags,
                OnGetStreamTags,
                OnReplaceStreamTags,
                OnGetUsers,
                OnUpdateUser,
                OnGetUsersFollows,
                OnCreateUserFollows,
                OnDeleteUserFollows,
                OnGetUserBlockList,
                OnBlockUser,
                OnUnblockUser,
                OnGetUserExtensions,
                OnGetUserActiveExtensions,
                OnUpdateUserExtensions,
                OnGetVideos,
                OnDeleteVideos,
                OnGetWebhookSubscriptions
            );

            void UserIdNotify(const AZStd::string& userId)
            {
                Call(FN_UserIdNotify, userId);
            }

            void UserAccessTokenNotify(const AZStd::string& token)
            {
                Call(FN_UserAccessTokenNotify, token);
            }

            void AppAccessTokenNotify(const AZStd::string& token)
            {
                Call(FN_AppAccessTokenNotify, token);
            }

            //! Creates function for calling respective Lua function using resultType
            #define TwitchApi_LuaFnCallResultHelper(functionName, resultType) \
                void functionName(const resultType& result) { Call(FN_ ##functionName, result); }

            //! Creates function for calling respective Lua function using receipt and ResultCode
            #define TwitchApi_LuaFnCallReceiptHelper(functionName) \
                void functionName(const ReceiptId& receipt, const ResultCode& rc) { Call(FN_ ##functionName, receipt, rc); }


            TwitchApi_LuaFnCallResultHelper(OnStartCommercial, CommercialResult)
            TwitchApi_LuaFnCallResultHelper(OnGetExtensionAnalytics, ExtensionAnalyticsResult)
            TwitchApi_LuaFnCallResultHelper(OnGetGameAnalytics, GameAnalyticsResult)
            TwitchApi_LuaFnCallResultHelper(OnGetBitsLeaderboard, BitsLeaderboardResult)
            TwitchApi_LuaFnCallResultHelper(OnGetCheermotes, CheermotesResult)
            TwitchApi_LuaFnCallResultHelper(OnGetExtensionTransactions, ExtensionTransactionsResult)
            TwitchApi_LuaFnCallResultHelper(OnGetChannelInformation, ChannelInformationResult)
            TwitchApi_LuaFnCallReceiptHelper(OnModifyChannelInformation)
            TwitchApi_LuaFnCallResultHelper(OnGetChannelEditors, ChannelEditorsResult)
            TwitchApi_LuaFnCallResultHelper(OnCreateCustomRewards, CustomRewardsResult)
            TwitchApi_LuaFnCallReceiptHelper(OnDeleteCustomReward)
            TwitchApi_LuaFnCallResultHelper(OnGetCustomReward, CustomRewardsResult)
            TwitchApi_LuaFnCallResultHelper(OnGetCustomRewardRedemption, CustomRewardsRedemptionResult)
            TwitchApi_LuaFnCallResultHelper(OnUpdateCustomReward, CustomRewardsResult)
            TwitchApi_LuaFnCallResultHelper(OnUpdateRedemptionStatus, CustomRewardsRedemptionResult)
            TwitchApi_LuaFnCallResultHelper(OnCreateClip, CreateClipResult)
            TwitchApi_LuaFnCallResultHelper(OnGetClips, GetClipsResult)
            TwitchApi_LuaFnCallResultHelper(OnCreateEntitlementGrantsUploadURL, EntitlementsGrantUploadResult)
            TwitchApi_LuaFnCallResultHelper(OnGetCodeStatus, CodeStatusResult)
            TwitchApi_LuaFnCallResultHelper(OnGetDropsEntitlements, DropsEntitlementsResult)
            TwitchApi_LuaFnCallResultHelper(OnRedeemCode, CodeStatusResult)
            TwitchApi_LuaFnCallResultHelper(OnGetTopGames, GetTopGamesResult)
            TwitchApi_LuaFnCallResultHelper(OnGetGames, GetGamesResult)
            TwitchApi_LuaFnCallResultHelper(OnGetHypeTrainEvents, HypeTrainEventsResult)
            TwitchApi_LuaFnCallResultHelper(OnCheckAutoModStatus, CheckAutoModResult)
            TwitchApi_LuaFnCallResultHelper(OnGetBannedEvents, GetBannedEventsResult)
            TwitchApi_LuaFnCallResultHelper(OnGetBannedUsers, GetBannedUsersResult)
            TwitchApi_LuaFnCallResultHelper(OnGetModerators, GetModeratorsResult)
            TwitchApi_LuaFnCallResultHelper(OnGetModeratorEvents, GetModeratorEventsResult)
            TwitchApi_LuaFnCallResultHelper(OnSearchCategories, SearchCategoriesResult)
            TwitchApi_LuaFnCallResultHelper(OnSearchChannels, SearchChannelsResult)
            TwitchApi_LuaFnCallResultHelper(OnGetStreamKey, StreamKeyResult)
            TwitchApi_LuaFnCallResultHelper(OnGetStreams, GetStreamsResult)
            TwitchApi_LuaFnCallResultHelper(OnCreateStreamMarker, CreateStreamMarkerResult)
            TwitchApi_LuaFnCallResultHelper(OnGetStreamMarkers, GetStreamMarkersResult)
            TwitchApi_LuaFnCallResultHelper(OnGetBroadcasterSubscriptions, GetBroadcasterSubscriptionsResult)
            TwitchApi_LuaFnCallResultHelper(OnGetAllStreamTags, GetAllStreamTagsResult);
            TwitchApi_LuaFnCallResultHelper(OnGetStreamTags, GetStreamTagsResult)
            TwitchApi_LuaFnCallReceiptHelper(OnReplaceStreamTags)
            TwitchApi_LuaFnCallResultHelper(OnGetUsers, GetUsersResult)
            TwitchApi_LuaFnCallResultHelper(OnUpdateUser, GetUsersResult)
            TwitchApi_LuaFnCallResultHelper(OnGetUsersFollows, GetUsersFollowsResult)
            TwitchApi_LuaFnCallReceiptHelper(OnCreateUserFollows)
            TwitchApi_LuaFnCallReceiptHelper(OnDeleteUserFollows)
            TwitchApi_LuaFnCallResultHelper(OnGetUserBlockList, UserBlockListResult)
            TwitchApi_LuaFnCallReceiptHelper(OnBlockUser)
            TwitchApi_LuaFnCallReceiptHelper(OnUnblockUser)
            TwitchApi_LuaFnCallResultHelper(OnGetUserExtensions, GetUserExtensionsResult)
            TwitchApi_LuaFnCallResultHelper(OnGetUserActiveExtensions, GetUserActiveExtensionsResult)
            TwitchApi_LuaFnCallResultHelper(OnUpdateUserExtensions, GetUserActiveExtensionsResult)
            TwitchApi_LuaFnCallResultHelper(OnGetVideos, GetVideosResult)
            TwitchApi_LuaFnCallResultHelper(OnDeleteVideos, DeleteVideosResult)
            TwitchApi_LuaFnCallResultHelper(OnGetWebhookSubscriptions, GetWebhookSubscriptionsResult)
        };

        /*
        ** helper macro for reflecting the enums
        */

        #define TwitchApi_EnumClassHelper(className, enumName) Enum<(int)className::enumName>(#enumName)

        //! Reflect a Response Class
        #define TwitchApi_ReflectResponseTypeClass(context, _type)                                              \
            context.Class<_type>()->                                                                            \
                Property("Data", [](const _type& value) { return value.m_data; }, nullptr);                       

        //! Reflect a Pagination Response Class
        #define TwitchApi_ReflectPaginationResponseTypeClass(context, _type)                                    \
            context.Class<_type>()->                                                                            \
                Property("Data", [](const _type& value) { return value.m_data; }, nullptr);

        //! Reflect a Result class     
        #define TwitchApi_ReflectReturnTypeClass(context, _type)                                                \
            context.Class<_type>()->                                                                            \
                Property("Value", [](const _type& value) { return value.m_value; }, nullptr)->                  \
                Property("Result", [](const _type& value) { return value.m_result; }, nullptr);                   

        void ReflectBehaviors(AZ::BehaviorContext& context)
        {
            context.Class<ResultCode>("TwitchApiResultCode")->
                TwitchApi_EnumClassHelper(ResultCode, Success)->
                TwitchApi_EnumClassHelper(ResultCode, InvalidParam)->
                TwitchApi_EnumClassHelper(ResultCode, TwitchRestError)->
                TwitchApi_EnumClassHelper(ResultCode, TwitchChannelNoUpdatesToMake)->
                TwitchApi_EnumClassHelper(ResultCode, Unknown)
                ;

            context.Class<ReceiptId>("TwitchApiReceiptId")->
                Method("Equal", &ReceiptId::operator==)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)->
                Property("Id", &ReceiptId::GetId, &ReceiptId::SetId)
                ;

            context.Class<PaginationResponse>()->
                Property("Pagination", [](const PaginationResponse& value) { return value.m_pagination; }, nullptr);

            context.Class<CustomRewardSettings>()->
                Constructor()->
                Constructor<const AZStd::string&, const AZStd::string&, const AZ::u64, const bool, const AZStd::string&, const bool,
                const bool, const AZ::u64, const bool, const AZ::u64, const bool, const AZ::u64, const bool>()->
                Property("Title", [](const CustomRewardSettings& value) { return value.m_title; }, nullptr)->
                Property("Prompt", [](const CustomRewardSettings& value) { return value.m_prompt; }, nullptr)->
                Property("Cost", [](const CustomRewardSettings& value) { return value.m_cost; }, nullptr)->
                Property("BackgroundColor", [](const CustomRewardSettings& value) { return value.m_background_color; }, nullptr)->
                Property("IsEnabled", [](const CustomRewardSettings& value) { return value.m_is_enabled; }, nullptr)->
                Property("IsUserInputRequired", [](const CustomRewardSettings& value) { return value.m_is_user_input_required; }, nullptr)->
                Property("IsMaxPerStreamEnabled", [](const CustomRewardSettings& value)
                    { return value.m_is_max_per_stream_enabled; }, nullptr)->
                Property("MaxPerStream", [](const CustomRewardSettings& value) { return value.m_max_per_stream; }, nullptr)->
                Property("IsMaxPerUserPerStreamEnabled", [](const CustomRewardSettings& value)
                    { return value.m_is_max_per_user_per_stream_enabled; }, nullptr)->
                Property("MaxPerUserPerStream", [](const CustomRewardSettings& value)
                    { return value.m_max_per_user_per_stream; }, nullptr)->
                Property("IsGlobalCooldownEnabled", [](const CustomRewardSettings& value)
                    { return value.m_is_global_cooldown_enabled; }, nullptr)->
                Property("GlobalCooldownSeconds", [](const CustomRewardSettings& value)
                    { return value.m_global_cooldown_seconds; }, nullptr)->
                Property("ShouldRedemptionsSkipRequestQueue", [](const CustomRewardSettings& value)
                    { return value.m_should_redemptions_skip_request_queue; }, nullptr);

            context.Class<CommercialDatum>()->
                Property("Length", [](const CommercialDatum& value) { return value.length; }, nullptr)->
                Property("Message", [](const CommercialDatum& value) { return value.message; }, nullptr)->
                Property("RetryAfter", [](const CommercialDatum& value) { return value.retry_after; }, nullptr)
                ;
            TwitchApi_ReflectResponseTypeClass(context, CommercialResponse);
            TwitchApi_ReflectReturnTypeClass(context, CommercialResult);

            context.Class<ExtensionAnalyticsDatum>()->
                Property("ExtensionId", [](const ExtensionAnalyticsDatum& value) { return value.extension_id; }, nullptr)->
                Property("Type", [](const ExtensionAnalyticsDatum& value) { return value.type; }, nullptr)->
                Property("StartedAt", [](const ExtensionAnalyticsDatum& value) { return value.started_at; }, nullptr)->
                Property("EndedAt", [](const ExtensionAnalyticsDatum& value) { return value.ended_at; }, nullptr)->
                Property("URL", [](const ExtensionAnalyticsDatum& value) { return value.url; }, nullptr)
                ;

            TwitchApi_ReflectPaginationResponseTypeClass(context, ExtensionAnalyticsResponse);
            TwitchApi_ReflectReturnTypeClass(context, ExtensionAnalyticsResult);

            context.Class<GameAnalyticsDatum>()->
                Property("GameId", [](const GameAnalyticsDatum& value) { return value.game_id; }, nullptr)->
                Property("Type", [](const GameAnalyticsDatum& value) { return value.type; }, nullptr)->
                Property("StartedAt", [](const GameAnalyticsDatum& value) { return value.started_at; }, nullptr)->
                Property("EndedAt", [](const GameAnalyticsDatum& value) { return value.ended_at; }, nullptr)->
                Property("URL", [](const GameAnalyticsDatum& value) { return value.url; }, nullptr)
                ;

            TwitchApi_ReflectPaginationResponseTypeClass(context, GameAnalyticsResponse);
            TwitchApi_ReflectReturnTypeClass(context, GameAnalyticsResult);

            context.Class<BitsLeaderboardDatum>()->
                Property("Rank", [](const BitsLeaderboardDatum& value) { return value.rank; }, nullptr)->
                Property("Score", [](const BitsLeaderboardDatum& value) { return value.score; }, nullptr)->
                Property("UserId", [](const BitsLeaderboardDatum& value) { return value.user_id; }, nullptr)->
                Property("UserName", [](const BitsLeaderboardDatum& value) { return value.user_name; }, nullptr)
                ;

            context.Class<BitsLeaderboardResponse>()->
                Property("Total", [](const BitsLeaderboardResponse& value) { return value.total; }, nullptr)->
                Property("StartedAt", [](const BitsLeaderboardResponse& value) { return value.started_at; }, nullptr)->
                Property("EndedAt", [](const BitsLeaderboardResponse& value) { return value.ended_at; }, nullptr)->
                Property("Data", [](const BitsLeaderboardResponse& value) { return value.data; }, nullptr)
                ;

            TwitchApi_ReflectReturnTypeClass(context, BitsLeaderboardResult);

            context.Class<CheermotesImageSetDatum>()->
                Property("URL1x", [](const CheermotesImageSetDatum& value) { return value.url_1x; }, nullptr)->
                Property("URL1.5x", [](const CheermotesImageSetDatum& value) { return value.url_15x; }, nullptr)->
                Property("URL2x", [](const CheermotesImageSetDatum& value) { return value.url_2x; }, nullptr)->
                Property("URL3x", [](const CheermotesImageSetDatum& value) { return value.url_3x; }, nullptr)->
                Property("URL4x", [](const CheermotesImageSetDatum& value) { return value.url_4x; }, nullptr)
                ;

            context.Class<CheermotesImagesDatum>()->
                Property("DarkAnimated", [](const CheermotesImagesDatum& value) { return value.dark_animated; }, nullptr)->
                Property("DarkStatic", [](const CheermotesImagesDatum& value) { return value.dark_static; }, nullptr)->
                Property("LightAnimated", [](const CheermotesImagesDatum& value) { return value.light_animated; }, nullptr)->
                Property("LightStatic", [](const CheermotesImagesDatum& value) { return value.light_static; }, nullptr)
                ;

            context.Class<CheermotesTiersDatum>()->
                Property("MinBits", [](const CheermotesTiersDatum& value) { return value.min_bits; }, nullptr)->
                Property("Id", [](const CheermotesTiersDatum& value) { return value.id; }, nullptr)->
                Property("Color", [](const CheermotesTiersDatum& value) { return value.color; }, nullptr)->
                Property("Images", [](const CheermotesTiersDatum& value) { return value.images; }, nullptr)->
                Property("CanCheer", [](const CheermotesTiersDatum& value) { return value.can_cheer; }, nullptr)->
                Property("ShowInBitsCard", [](const CheermotesTiersDatum& value) { return value.show_in_bits_card; }, nullptr)
                ;

            context.Class<CheermotesDatum>()->
                Property("Prefix", [](const CheermotesDatum& value) { return value.prefix; }, nullptr)->
                Property("Type", [](const CheermotesDatum& value) { return value.type; }, nullptr)->
                Property("Order", [](const CheermotesDatum& value) { return value.order; }, nullptr)->
                Property("LastUpdated", [](const CheermotesDatum& value) { return value.last_updated; }, nullptr)->
                Property("IsCharitable", [](const CheermotesDatum& value) { return value.is_charitable; }, nullptr)->
                Property("Tiers", [](const CheermotesDatum& value) { return value.tiers; }, nullptr)
                ;

            TwitchApi_ReflectPaginationResponseTypeClass(context, CheermotesResponse);
            TwitchApi_ReflectReturnTypeClass(context, CheermotesResult);

            context.Class<ExtensionTransactionProductDatum>()->
                Property("SKU", [](const ExtensionTransactionProductDatum& value) { return value.sku; }, nullptr)->
                Property("Cost", [](const ExtensionTransactionProductDatum& value) { return value.cost_amount; }, nullptr)->
                Property("CostType", [](const ExtensionTransactionProductDatum& value) { return value.cost_type; }, nullptr)->
                Property("Domain", [](const ExtensionTransactionProductDatum& value) { return value.domain; }, nullptr)->
                Property("DisplayName", [](const ExtensionTransactionProductDatum& value) { return value.displayName; }, nullptr)->
                Property("IsInDevelopment", [](const ExtensionTransactionProductDatum& value) { return value.inDevelopment; }, nullptr)->
                Property("WasBroadcasted", [](const ExtensionTransactionProductDatum& value) { return value.broadcast; }, nullptr)
                ;

            context.Class<ExtensionTransactionDatum>()->
                Property("Id", [](const ExtensionTransactionDatum& value) { return value.id; }, nullptr)->
                Property("Timestamp", [](const ExtensionTransactionDatum& value) { return value.timestamp; }, nullptr)->
                Property("BroadcasterId", [](const ExtensionTransactionDatum& value) { return value.broadcaster_id; }, nullptr)->
                Property("BroadcasterName", [](const ExtensionTransactionDatum& value) { return value.broadcaster_name; }, nullptr)->
                Property("UserId", [](const ExtensionTransactionDatum& value) { return value.user_id; }, nullptr)->
                Property("UserName", [](const ExtensionTransactionDatum& value) { return value.user_name; }, nullptr)->
                Property("ProductType", [](const ExtensionTransactionDatum& value) { return value.product_type; }, nullptr)->
                Property("ProductData", [](const ExtensionTransactionDatum& value) { return value.product_data; }, nullptr)
                ;

            TwitchApi_ReflectPaginationResponseTypeClass(context, ExtensionTransactionsResponse);
            TwitchApi_ReflectReturnTypeClass(context, ExtensionTransactionsResult);

            context.Class<ChannelInformationDatum>()->
                Property("BroadcasterId", [](const ChannelInformationDatum& value) { return value.broadcaster_id; }, nullptr)->
                Property("BroadcasterName", [](const ChannelInformationDatum& value) { return value.broadcaster_name; }, nullptr)->
                Property("BroadcasterLanguage", [](const ChannelInformationDatum& value) { return value.broadcaster_language; }, nullptr)->
                Property("GameId", [](const ChannelInformationDatum& value) { return value.game_id; }, nullptr)->
                Property("GameName", [](const ChannelInformationDatum& value) { return value.game_name; }, nullptr)->
                Property("Title", [](const ChannelInformationDatum& value) { return value.title; }, nullptr)
                ;

            TwitchApi_ReflectResponseTypeClass(context, ChannelInformationResponse);
            TwitchApi_ReflectReturnTypeClass(context, ChannelInformationResult);

            context.Class<ChannelEditorsDatum>()->
                Property("UserId", [](const ChannelEditorsDatum& value) { return value.user_id; }, nullptr)->
                Property("UserName", [](const ChannelEditorsDatum& value) { return value.user_name; }, nullptr)->
                Property("CreatedAt", [](const ChannelEditorsDatum& value) { return value.created_at; }, nullptr)
                ;

            TwitchApi_ReflectResponseTypeClass(context, ChannelEditorsResponse);
            TwitchApi_ReflectReturnTypeClass(context, ChannelEditorsResult);

            context.Class<CustomRewardsImageDatum>()->
                Property("Url1x", [](const CustomRewardsImageDatum& value) { return value.url_1x; }, nullptr)->
                Property("Url2x", [](const CustomRewardsImageDatum& value) { return value.url_2x; }, nullptr)->
                Property("Url4x", [](const CustomRewardsImageDatum& value) { return value.url_4x; }, nullptr)
                ;

            context.Class<CustomRewardsDatum>()->
                Property("BroadcasterId", [](const CustomRewardsDatum& value) { return value.broadcaster_id; }, nullptr)->
                Property("BroadcasterName", [](const CustomRewardsDatum& value) { return value.broadcaster_name; }, nullptr)->
                Property("Id", [](const CustomRewardsDatum& value) { return value.id; }, nullptr)->
                Property("Image", [](const CustomRewardsDatum& value) { return value.image; }, nullptr)->
                Property("BackgroundColor", [](const CustomRewardsDatum& value) { return value.background_color; }, nullptr)->
                Property("IsEnabled", [](const CustomRewardsDatum& value) { return value.is_enabled; }, nullptr)->
                Property("Cost", [](const CustomRewardsDatum& value) { return value.cost; }, nullptr)->
                Property("Title", [](const CustomRewardsDatum& value) { return value.title; }, nullptr)->
                Property("Prompt", [](const CustomRewardsDatum& value) { return value.prompt; }, nullptr)->
                Property("IsUserInputRequired", [](const CustomRewardsDatum& value) { return value.is_user_input_required; }, nullptr)->
                Property("IsMaxPerStreamEnabled", [](const CustomRewardsDatum& value)
                    { return value.is_max_per_stream_enabled; }, nullptr)->
                Property("MaxPerStream", [](const CustomRewardsDatum& value) { return value.max_per_stream; }, nullptr)->
                Property("IsMaxPerUserPerStreamEnabled", [](const CustomRewardsDatum& value)
                    { return value.is_max_per_user_per_stream_enabled; }, nullptr)->
                Property("MaxPerUserPerStream", [](const CustomRewardsDatum& value) { return value.max_per_user_per_stream; }, nullptr)->
                Property("IsGlobalCooldownEnabled", [](const CustomRewardsDatum& value)
                    { return value.is_global_cooldown_enabled; }, nullptr)->
                Property("GlobalCooldownSeconds", [](const CustomRewardsDatum& value) { return value.global_cooldown_seconds; }, nullptr)->
                Property("IsPaused", [](const CustomRewardsDatum& value) { return value.is_paused; }, nullptr)->
                Property("IsInStock", [](const CustomRewardsDatum& value) { return value.is_in_stock; }, nullptr)->
                Property("DefaultImage", [](const CustomRewardsDatum& value) { return value.default_image; }, nullptr)->
                Property("ShouldRedemptionsSkipRequestQueue", [](const CustomRewardsDatum& value)
                    { return value.should_redemptions_skip_request_queue; }, nullptr)->
                Property("RedemptionsRedeemedCurrentStream", [](const CustomRewardsDatum& value)
                    { return value.redemptions_redeemed_current_stream; }, nullptr)->
                Property("CooldownExpiresAt", [](const CustomRewardsDatum& value) { return value.cooldown_expires_at; }, nullptr)
                ;
            TwitchApi_ReflectResponseTypeClass(context, CustomRewardsResponse);
            TwitchApi_ReflectReturnTypeClass(context, CustomRewardsResult);

            context.Class<CustomRewardsRedemptionRewardDatum>()->
                Property("Id", [](const CustomRewardsRedemptionRewardDatum& value) { return value.id; }, nullptr)->
                Property("Title", [](const CustomRewardsRedemptionRewardDatum& value) { return value.title; }, nullptr)->
                Property("Prompt", [](const CustomRewardsRedemptionRewardDatum& value) { return value.prompt; }, nullptr)->
                Property("Cost", [](const CustomRewardsRedemptionRewardDatum& value) { return value.cost; }, nullptr)
                ;

            context.Class<CustomRewardsRedemptionDatum>()->
                Property("BroadcasterName", [](const CustomRewardsRedemptionDatum& value) { return value.broadcaster_name; }, nullptr)->
                Property("BroadcasterId", [](const CustomRewardsRedemptionDatum& value) { return value.broadcaster_id; }, nullptr)->
                Property("Id", [](const CustomRewardsRedemptionDatum& value) { return value.id; }, nullptr)->
                Property("UserId", [](const CustomRewardsRedemptionDatum& value) { return value.user_id; }, nullptr)->
                Property("UserName", [](const CustomRewardsRedemptionDatum& value) { return value.user_name; }, nullptr)->
                Property("UserInput", [](const CustomRewardsRedemptionDatum& value) { return value.user_input; }, nullptr)->
                Property("Status", [](const CustomRewardsRedemptionDatum& value) { return value.status; }, nullptr)->
                Property("RedeemedAt", [](const CustomRewardsRedemptionDatum& value) { return value.redeemed_at; }, nullptr)->
                Property("Reward", [](const CustomRewardsRedemptionDatum& value) { return value.reward; }, nullptr)
                ;
            TwitchApi_ReflectPaginationResponseTypeClass(context, CustomRewardsRedemptionResponse);
            TwitchApi_ReflectReturnTypeClass(context, CustomRewardsRedemptionResult);

            context.Class<CreateClipDatum>()->
                Property("Id", [](const CreateClipDatum& value) { return value.id; }, nullptr)->
                Property("EditURL", [](const CreateClipDatum& value) { return value.edit_url; }, nullptr)
                ;
            TwitchApi_ReflectResponseTypeClass(context, CreateClipResponse);
            TwitchApi_ReflectReturnTypeClass(context, CreateClipResult);

            context.Class<GetClipsDatum>()->
                Property("BroadcasterId", [](const GetClipsDatum& value) { return value.broadcaster_id; }, nullptr)->
                Property("BroadcasterName", [](const GetClipsDatum& value) { return value.broadcaster_name; }, nullptr)->
                Property("CreatedAt", [](const GetClipsDatum& value) { return value.created_at; }, nullptr)->
                Property("CreatorId", [](const GetClipsDatum& value) { return value.creator_id; }, nullptr)->
                Property("CreatorName", [](const GetClipsDatum& value) { return value.creator_name; }, nullptr)->
                Property("EmbedURL", [](const GetClipsDatum& value) { return value.embed_url; }, nullptr)->
                Property("GameId", [](const GetClipsDatum& value) { return value.game_id; }, nullptr)->
                Property("Id", [](const GetClipsDatum& value) { return value.id; }, nullptr)->
                Property("Language", [](const GetClipsDatum& value) { return value.language; }, nullptr)->
                Property("ThumbnailURL", [](const GetClipsDatum& value) { return value.thumbnail_url; }, nullptr)->
                Property("Title", [](const GetClipsDatum& value) { return value.title; }, nullptr)->
                Property("URL", [](const GetClipsDatum& value) { return value.url; }, nullptr)->
                Property("VideoId", [](const GetClipsDatum& value) { return value.video_id; }, nullptr)->
                Property("ViewCount", [](const GetClipsDatum& value) { return value.view_count; }, nullptr)
                ;
            TwitchApi_ReflectPaginationResponseTypeClass(context, GetClipsResponse);
            TwitchApi_ReflectReturnTypeClass(context, GetClipsResult);

            context.Class<EntitlementsGrantUploadResponse>()->
                Property("URLs", [](const EntitlementsGrantUploadResponse& value) { return value.urls; }, nullptr)
                ;
            TwitchApi_ReflectReturnTypeClass(context, EntitlementsGrantUploadResult);

            context.Class<CodeStatusResponse>()->
                Property("Statuses", [](const CodeStatusResponse& value) { return value.statuses; }, nullptr)
                ;
            TwitchApi_ReflectReturnTypeClass(context, CodeStatusResult);

            context.Class<DropsEntitlementsDatum>()->
                Property("Id", [](const DropsEntitlementsDatum& value) { return value.id; }, nullptr)->
                Property("BenefitId", [](const DropsEntitlementsDatum& value) { return value.benefit_id; }, nullptr)->
                Property("Timestamp", [](const DropsEntitlementsDatum& value) { return value.timestamp; }, nullptr)->
                Property("UserId", [](const DropsEntitlementsDatum& value) { return value.user_id; }, nullptr)->
                Property("GameId", [](const DropsEntitlementsDatum& value) { return value.game_id; }, nullptr)
                ;
            TwitchApi_ReflectPaginationResponseTypeClass(context, DropsEntitlementsResponse);
            TwitchApi_ReflectReturnTypeClass(context, DropsEntitlementsResult);

            context.Class<GetGamesDatum>()->
                Property("BoxArtURL", [](const GetGamesDatum& value) { return value.box_art_url; }, nullptr)->
                Property("Id", [](const GetGamesDatum& value) { return value.id; }, nullptr)->
                Property("Name", [](const GetGamesDatum& value) { return value.name; }, nullptr)
                ;
            TwitchApi_ReflectPaginationResponseTypeClass(context, GetTopGamesResponse);
            TwitchApi_ReflectReturnTypeClass(context, GetTopGamesResult);
            TwitchApi_ReflectResponseTypeClass(context, GetGamesResponse);
            TwitchApi_ReflectReturnTypeClass(context, GetGamesResult);

            context.Class<HypeTrainContributionDatum>()->
                Property("Total", [](const HypeTrainContributionDatum& value) { return value.total; }, nullptr)->
                Property("Type", [](const HypeTrainContributionDatum& value) { return value.type; }, nullptr)->
                Property("User", [](const HypeTrainContributionDatum& value) { return value.user; }, nullptr)
                ;

            context.Class<HypeTrainEventDatum>()->
                Property("BroadcasterId", [](const HypeTrainEventDatum& value) { return value.broadcaster_id; }, nullptr)->
                Property("CooldownEndTime", [](const HypeTrainEventDatum& value) { return value.cooldown_end_time; }, nullptr)->
                Property("ExpiresAt", [](const HypeTrainEventDatum& value) { return value.expires_at; }, nullptr)->
                Property("Goal", [](const HypeTrainEventDatum& value) { return value.goal; }, nullptr)->
                Property("Id", [](const HypeTrainEventDatum& value) { return value.id; }, nullptr)->
                Property("LastContribution", [](const HypeTrainEventDatum& value) { return value.last_contribution; }, nullptr)->
                Property("Level", [](const HypeTrainEventDatum& value) { return value.level; }, nullptr)->
                Property("StartedAt", [](const HypeTrainEventDatum& value) { return value.started_at; }, nullptr)->
                Property("TopContributions", [](const HypeTrainEventDatum& value) { return value.top_contributions; }, nullptr)->
                Property("Total", [](const HypeTrainEventDatum& value) { return value.total; }, nullptr)
                ;

            context.Class<HypeTrainEventsDatum>()->
                Property("Id", [](const HypeTrainEventsDatum& value) { return value.id; }, nullptr)->
                Property("EventType", [](const HypeTrainEventsDatum& value) { return value.event_type; }, nullptr)->
                Property("EventTimestamp", [](const HypeTrainEventsDatum& value) { return value.event_timestamp; }, nullptr)->
                Property("Version", [](const HypeTrainEventsDatum& value) { return value.version; }, nullptr)->
                Property("EventData", [](const HypeTrainEventsDatum& value) { return value.event_data; }, nullptr)
                ;
            TwitchApi_ReflectPaginationResponseTypeClass(context, HypeTrainEventsResponse);
            TwitchApi_ReflectReturnTypeClass(context, HypeTrainEventsResult);

            context.Class<CheckAutoModInputDatum>()->
                Property("MsgId", BehaviorValueProperty(&CheckAutoModInputDatum::msg_id))->
                Property("MsgText", BehaviorValueProperty(&CheckAutoModInputDatum::msg_text))->
                Property("UserId", BehaviorValueProperty(&CheckAutoModInputDatum::user_id))
                ;

            context.Class<CheckAutoModResponseDatum>()->
                Property("MsgId", [](const CheckAutoModResponseDatum& value) { return value.msg_id; }, nullptr)->
                Property("IsPermitted", [](const CheckAutoModResponseDatum& value) { return value.is_permitted; }, nullptr)
                ;
            TwitchApi_ReflectResponseTypeClass(context, CheckAutoModResponse);
            TwitchApi_ReflectReturnTypeClass(context, CheckAutoModResult);

            context.Class<GetBannedEventsDatum>()->
                Property("Id", [](const GetBannedEventsDatum& value) { return value.id; }, nullptr)->
                Property("EventType", [](const GetBannedEventsDatum& value) { return value.event_type; }, nullptr)->
                Property("EventTimestamp", [](const GetBannedEventsDatum& value) { return value.event_timestamp; }, nullptr)->
                Property("BroadcasterId", [](const GetBannedEventsDatum& value) { return value.broadcaster_id; }, nullptr)->
                Property("BroadcasterName", [](const GetBannedEventsDatum& value) { return value.broadcaster_name; }, nullptr)->
                Property("UserId", [](const GetBannedEventsDatum& value) { return value.user_id; }, nullptr)->
                Property("UserName", [](const GetBannedEventsDatum& value) { return value.user_name; }, nullptr)->
                Property("ExpiresAt", [](const GetBannedEventsDatum& value) { return value.expires_at; }, nullptr)->
                Property("Version", [](const GetBannedEventsDatum& value) { return value.version; }, nullptr)
                ;
            TwitchApi_ReflectPaginationResponseTypeClass(context, GetBannedEventsResponse);
            TwitchApi_ReflectReturnTypeClass(context, GetBannedEventsResult);

            context.Class<GetBannedUsersDatum>()->
                Property("UserId", [](const GetBannedUsersDatum& value) { return value.user_id; }, nullptr)->
                Property("UserName", [](const GetBannedUsersDatum& value) { return value.user_name; }, nullptr)->
                Property("ExpiresAt", [](const GetBannedUsersDatum& value) { return value.expires_at; }, nullptr)
                ;
            TwitchApi_ReflectPaginationResponseTypeClass(context, GetBannedUsersResponse);
            TwitchApi_ReflectReturnTypeClass(context, GetBannedUsersResult);

            context.Class<GetModeratorsDatum>()->
                Property("UserId", [](const GetModeratorsDatum& value) { return value.user_id; }, nullptr)->
                Property("UserName", [](const GetModeratorsDatum& value) { return value.user_name; }, nullptr)
                ;
            TwitchApi_ReflectPaginationResponseTypeClass(context, GetModeratorsResponse);
            TwitchApi_ReflectReturnTypeClass(context, GetModeratorsResult);

            context.Class<GetModeratorEventsDatum>()->
                Property("Id", [](const GetModeratorEventsDatum& value) { return value.id; }, nullptr)->
                Property("EventType", [](const GetModeratorEventsDatum& value) { return value.event_type; }, nullptr)->
                Property("EventTimestamp", [](const GetModeratorEventsDatum& value) { return value.event_timestamp; }, nullptr)->
                Property("Version", [](const GetModeratorEventsDatum& value) { return value.version; }, nullptr)
                ;
            TwitchApi_ReflectPaginationResponseTypeClass(context, GetModeratorEventsResponse);
            TwitchApi_ReflectReturnTypeClass(context, GetModeratorEventsResult);

            context.Class<SearchCategoriesDatum>()->
                Property("Id", [](const SearchCategoriesDatum& value) { return value.id; }, nullptr)->
                Property("Name", [](const SearchCategoriesDatum& value) { return value.name; }, nullptr)->
                Property("BoxArtUrl", [](const SearchCategoriesDatum& value) { return value.box_art_url; }, nullptr)
                ;
            TwitchApi_ReflectPaginationResponseTypeClass(context, SearchCategoriesResponse);
            TwitchApi_ReflectReturnTypeClass(context, SearchCategoriesResult);

            context.Class<SearchChannelsDatum>()->
                Property("BroadcasterLanguage", [](const SearchChannelsDatum& value) { return value.broadcaster_language; }, nullptr)->
                Property("DisplayName", [](const SearchChannelsDatum& value) { return value.display_name; }, nullptr)->
                Property("GameId", [](const SearchChannelsDatum& value) { return value.game_id; }, nullptr)->
                Property("Id", [](const SearchChannelsDatum& value) { return value.id; }, nullptr)->
                Property("IsLive", [](const SearchChannelsDatum& value) { return value.is_live; }, nullptr)->
                Property("TagIds", [](const SearchChannelsDatum& value) { return value.tag_ids; }, nullptr)->
                Property("ThumbnailUrl", [](const SearchChannelsDatum& value) { return value.thumbnail_url; }, nullptr)->
                Property("Title", [](const SearchChannelsDatum& value) { return value.title; }, nullptr)->
                Property("StartedAt", [](const SearchChannelsDatum& value) { return value.started_at; }, nullptr)
                ;
            TwitchApi_ReflectPaginationResponseTypeClass(context, SearchChannelsResponse);
            TwitchApi_ReflectReturnTypeClass(context, SearchChannelsResult);

            context.Class<StreamKeyResponse>()->
                Property("StreamKey", [](const StreamKeyResponse& value) { return value.stream_key; }, nullptr)
                ;
            TwitchApi_ReflectReturnTypeClass(context, StreamKeyResult);

            context.Class<GetStreamsDatum>()->
                Property("GameId", [](const GetStreamsDatum& value) { return value.game_id; }, nullptr)->
                Property("Type", [](const GetStreamsDatum& value) { return value.type; }, nullptr)->
                Property("StartedAt", [](const GetStreamsDatum& value) { return value.started_at; }, nullptr)->
                Property("TagIds", [](const GetStreamsDatum& value) { return value.tag_ids; }, nullptr)->
                Property("UserId", [](const GetStreamsDatum& value) { return value.user_id; }, nullptr)->
                Property("UserName", [](const GetStreamsDatum& value) { return value.user_name; }, nullptr)->
                Property("Id", [](const GetStreamsDatum& value) { return value.id; }, nullptr)->
                Property("Language", [](const GetStreamsDatum& value) { return value.language; }, nullptr)->
                Property("ThumbnailURL", [](const GetStreamsDatum& value) { return value.thumbnail_url; }, nullptr)->
                Property("Title", [](const GetStreamsDatum& value) { return value.title; }, nullptr)->
                Property("ViewerCount", [](const GetStreamsDatum& value) { return value.viewer_count; }, nullptr)
                ;
            TwitchApi_ReflectPaginationResponseTypeClass(context, GetStreamsResponse);
            TwitchApi_ReflectReturnTypeClass(context, GetStreamsResult);

            context.Class<CreateStreamMarkerDatum>()->
                Property("CreatedAt", [](const CreateStreamMarkerDatum& value) { return value.created_at; }, nullptr)->
                Property("Description", [](const CreateStreamMarkerDatum& value) { return value.description; }, nullptr)->
                Property("Id", [](const CreateStreamMarkerDatum& value) { return value.id; }, nullptr)->
                Property("PositionSeconds", [](const CreateStreamMarkerDatum& value) { return value.position_seconds; }, nullptr)
                ;
            TwitchApi_ReflectResponseTypeClass(context, CreateStreamMarkerResponse);
            TwitchApi_ReflectReturnTypeClass(context, CreateStreamMarkerResult);

            context.Class<GetStreamMarkersMarkerDatum>()->
                Property("CreatedAt", [](const GetStreamMarkersMarkerDatum& value) { return value.created_at; }, nullptr)->
                Property("Id", [](const GetStreamMarkersMarkerDatum& value) { return value.id; }, nullptr)->
                Property("Description", [](const GetStreamMarkersMarkerDatum& value) { return value.description; }, nullptr)->
                Property("URL", [](const GetStreamMarkersMarkerDatum& value) { return value.URL; }, nullptr)->
                Property("PositionSeconds", [](const GetStreamMarkersMarkerDatum& value) { return value.position_seconds; }, nullptr)
                ;

            context.Class<GetStreamMarkersVideoDatum>()->
                Property("VideoId", [](const GetStreamMarkersVideoDatum& value) { return value.video_id; }, nullptr)->
                Property("Markers", [](const GetStreamMarkersVideoDatum& value) { return value.markers; }, nullptr)
                ;

            context.Class<GetStreamMarkersDatum>()->
                Property("UserId", [](const GetStreamMarkersDatum& value) { return value.user_id; }, nullptr)->
                Property("UserName", [](const GetStreamMarkersDatum& value) { return value.user_name; }, nullptr)->
                Property("Videos", [](const GetStreamMarkersDatum& value) { return value.videos; }, nullptr)
                ;
            TwitchApi_ReflectPaginationResponseTypeClass(context, GetStreamMarkersResponse);
            TwitchApi_ReflectReturnTypeClass(context, GetStreamMarkersResult);

            context.Class<GetBroadcasterSubscriptionsDatum>()->
                Property("BroadcasterId", [](const GetBroadcasterSubscriptionsDatum& value) { return value.broadcaster_id; }, nullptr)->
                Property("BroadcasterName", [](const GetBroadcasterSubscriptionsDatum& value) { return value.broadcaster_name; }, nullptr)->
                Property("UserId", [](const GetBroadcasterSubscriptionsDatum& value) { return value.user_id; }, nullptr)->
                Property("UserName", [](const GetBroadcasterSubscriptionsDatum& value) { return value.user_name; }, nullptr)->
                Property("Tier", [](const GetBroadcasterSubscriptionsDatum& value) { return value.tier; }, nullptr)->
                Property("PlanName", [](const GetBroadcasterSubscriptionsDatum& value) { return value.plan_name; }, nullptr)->
                Property("IsGift", [](const GetBroadcasterSubscriptionsDatum& value) { return value.is_gift; }, nullptr)
                ;
            TwitchApi_ReflectPaginationResponseTypeClass(context, GetBroadcasterSubscriptionsResponse);
            TwitchApi_ReflectReturnTypeClass(context, GetBroadcasterSubscriptionsResult);

            context.Class<GetStreamTagsDatum>()->
                Property("IsAuto", [](const GetStreamTagsDatum& value) { return value.is_auto; }, nullptr)->
                Property("LocalizationNames", [](const GetStreamTagsDatum& value) { return value.localization_names; }, nullptr)->
                Property("LocalizationDescriptions", [](const GetStreamTagsDatum& value)
                    { return value.localization_descriptions; }, nullptr)->
                Property("TagId", [](const GetStreamTagsDatum& value) { return value.tag_id; }, nullptr)
                ;
            TwitchApi_ReflectResponseTypeClass(context, GetStreamTagsResponse);
            TwitchApi_ReflectReturnTypeClass(context, GetStreamTagsResult);
            TwitchApi_ReflectPaginationResponseTypeClass(context, GetAllStreamTagsResponse);
            TwitchApi_ReflectReturnTypeClass(context, GetAllStreamTagsResult);

            context.Class<GetUsersDatum>()->
                Property("BroadcasterType", [](const GetUsersDatum& value) { return value.broadcaster_type; }, nullptr)->
                Property("Description", [](const GetUsersDatum& value) { return value.description; }, nullptr)->
                Property("DisplayName", [](const GetUsersDatum& value) { return value.display_name; }, nullptr)->
                Property("Email", [](const GetUsersDatum& value) { return value.email; }, nullptr)->
                Property("Id", [](const GetUsersDatum& value) { return value.id; }, nullptr)->
                Property("Login", [](const GetUsersDatum& value) { return value.login; }, nullptr)->
                Property("OfflineImageURL", [](const GetUsersDatum& value) { return value.offline_image_url; }, nullptr)->
                Property("ProfileImageURL", [](const GetUsersDatum& value) { return value.profile_image_url; }, nullptr)->
                Property("Type", [](const GetUsersDatum& value) { return value.type; }, nullptr)->
                Property("ViewCount", [](const GetUsersDatum& value) { return value.view_count; }, nullptr)
                ;
            TwitchApi_ReflectResponseTypeClass(context, GetUsersResponse);
            TwitchApi_ReflectReturnTypeClass(context, GetUsersResult);

            context.Class<GetUsersFollowsDatum>()->
                Property("FollowedAt", [](const GetUsersFollowsDatum& value) { return value.followed_at; }, nullptr)->
                Property("FromId", [](const GetUsersFollowsDatum& value) { return value.from_id; }, nullptr)->
                Property("FromName", [](const GetUsersFollowsDatum& value) { return value.from_name; }, nullptr)->
                Property("ToId", [](const GetUsersFollowsDatum& value) { return value.to_id; }, nullptr)->
                Property("ToName", [](const GetUsersFollowsDatum& value) { return value.to_name; }, nullptr)->
                Property("Total", [](const GetUsersFollowsDatum& value) { return value.total; }, nullptr)
                ;
            TwitchApi_ReflectPaginationResponseTypeClass(context, GetUsersFollowsResponse);
            TwitchApi_ReflectReturnTypeClass(context, GetUsersFollowsResult);

            context.Class<UserBlockListDatum>()->
                Property("Id", [](const UserBlockListDatum& value) { return value.id; }, nullptr)->
                Property("UserLogin", [](const UserBlockListDatum& value) { return value.user_login; }, nullptr)->
                Property("DisplayName", [](const UserBlockListDatum& value) { return value.display_name; }, nullptr)
                ;
            TwitchApi_ReflectPaginationResponseTypeClass(context, UserBlockListResponse);
            TwitchApi_ReflectReturnTypeClass(context, UserBlockListResult);

            context.Class<GetUserExtensionsDatum>()->
                Property("Id", [](const GetUserExtensionsDatum& value) { return value.id; }, nullptr)->
                Property("Type", [](const GetUserExtensionsDatum& value) { return value.type; }, nullptr)->
                Property("Name", [](const GetUserExtensionsDatum& value) { return value.name; }, nullptr)->
                Property("Version", [](const GetUserExtensionsDatum& value) { return value.version; }, nullptr)->
                Property("CanActivate", [](const GetUserExtensionsDatum& value) { return value.can_activate; }, nullptr)
                ;
            TwitchApi_ReflectResponseTypeClass(context, GetUserExtensionsResponse);
            TwitchApi_ReflectReturnTypeClass(context, GetUserExtensionsResult);

            context.Class<GetUserActiveExtensionsComponent>()->
                Property("Active", BehaviorValueProperty(&GetUserActiveExtensionsComponent::active))->
                Property("Id", BehaviorValueProperty(&GetUserActiveExtensionsComponent::id))->
                Property("Version", BehaviorValueProperty(&GetUserActiveExtensionsComponent::version))->
                Property("Name", BehaviorValueProperty(&GetUserActiveExtensionsComponent::name))->
                Property("x", BehaviorValueProperty(&GetUserActiveExtensionsComponent::x))->
                Property("y", BehaviorValueProperty(&GetUserActiveExtensionsComponent::y))
                ;

            context.Class<GetUserActiveExtensionsResponse>()->
                Property("Panel", [](const GetUserActiveExtensionsResponse& value) { return value.panel; }, nullptr)->
                Property("Overlay", [](const GetUserActiveExtensionsResponse& value) { return value.overlay; }, nullptr)->
                Property("Component", [](const GetUserActiveExtensionsResponse& value) { return value.component; }, nullptr)
                ;
            TwitchApi_ReflectReturnTypeClass(context, GetUserActiveExtensionsResult);

            context.Class<GetVideosDatum>()->
                Property("CreatedAt", [](const GetVideosDatum& value) { return value.created_at; }, nullptr)->
                Property("Description", [](const GetVideosDatum& value) { return value.description; }, nullptr)->
                Property("Duration", [](const GetVideosDatum& value) { return value.duration; }, nullptr)->
                Property("Id", [](const GetVideosDatum& value) { return value.id; }, nullptr)->
                Property("Language", [](const GetVideosDatum& value) { return value.language; }, nullptr)->
                Property("PublishedAt", [](const GetVideosDatum& value) { return value.published_at; }, nullptr)->
                Property("ThumbnailURL", [](const GetVideosDatum& value) { return value.thumbnail_url; }, nullptr)->
                Property("Title", [](const GetVideosDatum& value) { return value.title; }, nullptr)->
                Property("Type", [](const GetVideosDatum& value) { return value.type; }, nullptr)->
                Property("URL", [](const GetVideosDatum& value) { return value.url; }, nullptr)->
                Property("UserId", [](const GetVideosDatum& value) { return value.user_id; }, nullptr)->
                Property("UserName", [](const GetVideosDatum& value) { return value.user_name; }, nullptr)->
                Property("ViewCount", [](const GetVideosDatum& value) { return value.view_count; }, nullptr)->
                Property("Viewable", [](const GetVideosDatum& value) { return value.viewable; }, nullptr)
                ;
            TwitchApi_ReflectPaginationResponseTypeClass(context, GetVideosResponse);
            TwitchApi_ReflectReturnTypeClass(context, GetVideosResult);

            context.Class<DeleteVideosResponse>()->
                Property("VideoKeys", [](const DeleteVideosResponse& value) { return value.video_keys; }, nullptr)
                ;
            TwitchApi_ReflectReturnTypeClass(context, DeleteVideosResult);

            context.Class<GetWebhookSubscriptionsDatum>()->
                Property("Callback", [](const GetWebhookSubscriptionsDatum& value) { return value.callback; }, nullptr)->
                Property("ExpiresAt", [](const GetWebhookSubscriptionsDatum& value) { return value.expires_at; }, nullptr)->
                Property("Topic", [](const GetWebhookSubscriptionsDatum& value) { return value.topic; }, nullptr)
                ;
            context.Class<GetWebhookSubscriptionsResponse>()->
                Property("Data", [](const GetWebhookSubscriptionsResponse& value) { return value.data; }, nullptr)->
                Property("Pagination", [](const GetWebhookSubscriptionsResponse& value) { return value.m_pagination; }, nullptr)->
                Property("Total", [](const GetWebhookSubscriptionsResponse& value) { return value.total; }, nullptr);
            TwitchApi_ReflectReturnTypeClass(context, GetWebhookSubscriptionsResult);

            context.EBus<TwitchApiRequestBus>("TwitchApiRequestBus")
                ->Event("SetApplicationId", &TwitchApiRequestBus::Events::SetApplicationId)
                ->Event("GetApplicationId", &TwitchApiRequestBus::Events::GetApplicationId)
                ->Event("GetUserId", &TwitchApiRequestBus::Events::GetUserId)
                ->Event("GetUserAccessToken", &TwitchApiRequestBus::Events::GetUserAccessToken)
                ->Event("GetAppAccessToken", &TwitchApiRequestBus::Events::GetAppAccessToken)
                ->Event("SetUserId", &TwitchApiRequestBus::Events::SetUserId)
                ->Event("SetUserAccessToken", &TwitchApiRequestBus::Events::SetUserAccessToken)
                ->Event("SetAppAccessToken", &TwitchApiRequestBus::Events::SetAppAccessToken)
                ->Event("StartCommercial", &TwitchApiRequestBus::Events::StartCommercial)
                ->Event("GetExtensionAnalytics", &TwitchApiRequestBus::Events::GetExtensionAnalytics)
                ->Event("GetGameAnalytics", &TwitchApiRequestBus::Events::GetGameAnalytics)
                ->Event("GetBitsLeaderboard", &TwitchApiRequestBus::Events::GetBitsLeaderboard)
                ->Event("GetCheermotes", &TwitchApiRequestBus::Events::GetCheermotes)
                ->Event("GetExtensionTransactions", &TwitchApiRequestBus::Events::GetExtensionTransactions)
                ->Event("GetChannelInformation", &TwitchApiRequestBus::Events::GetChannelInformation)
                ->Event("ModifyChannelInformation", &TwitchApiRequestBus::Events::ModifyChannelInformation)
                ->Event("GetChannelEditors", &TwitchApiRequestBus::Events::GetChannelEditors)
                ->Event("CreateCustomRewards", &TwitchApiRequestBus::Events::CreateCustomRewards)
                ->Event("DeleteCustomReward", &TwitchApiRequestBus::Events::DeleteCustomReward)
                ->Event("GetCustomReward", &TwitchApiRequestBus::Events::GetCustomReward)
                ->Event("GetCustomRewardRedemption", &TwitchApiRequestBus::Events::GetCustomRewardRedemption)
                ->Event("UpdateCustomReward", &TwitchApiRequestBus::Events::UpdateCustomReward)
                ->Event("UpdateRedemptionStatus", &TwitchApiRequestBus::Events::UpdateRedemptionStatus)
                ->Event("CreateClip", &TwitchApiRequestBus::Events::CreateClip)
                ->Event("GetClips", &TwitchApiRequestBus::Events::GetClips)
                ->Event("CreateEntitlementGrantsUploadURL", &TwitchApiRequestBus::Events::CreateEntitlementGrantsUploadURL)
                ->Event("GetCodeStatus", &TwitchApiRequestBus::Events::GetCodeStatus)
                ->Event("GetDropsEntitlements", &TwitchApiRequestBus::Events::GetDropsEntitlements)
                ->Event("RedeemCode", &TwitchApiRequestBus::Events::RedeemCode)
                ->Event("GetTopGames", &TwitchApiRequestBus::Events::GetTopGames)
                ->Event("GetGames", &TwitchApiRequestBus::Events::GetGames)
                ->Event("GetHypeTrainEvents", &TwitchApiRequestBus::Events::GetHypeTrainEvents)
                ->Event("CheckAutoModStatus", &TwitchApiRequestBus::Events::CheckAutoModStatus)
                ->Event("GetBannedEvents", &TwitchApiRequestBus::Events::GetBannedEvents)
                ->Event("GetBannedUsers", &TwitchApiRequestBus::Events::GetBannedUsers)
                ->Event("GetModerators", &TwitchApiRequestBus::Events::GetModerators)
                ->Event("GetModeratorEvents", &TwitchApiRequestBus::Events::GetModeratorEvents)
                ->Event("SearchCategories", &TwitchApiRequestBus::Events::SearchCategories)
                ->Event("SearchChannels", &TwitchApiRequestBus::Events::SearchChannels)
                ->Event("GetStreamKey", &TwitchApiRequestBus::Events::GetStreamKey)
                ->Event("GetStreams", &TwitchApiRequestBus::Events::GetStreams)
                ->Event("CreateStreamMarker", &TwitchApiRequestBus::Events::CreateStreamMarker)
                ->Event("GetStreamMarkers", &TwitchApiRequestBus::Events::GetStreamMarkers)
                ->Event("GetBroadcasterSubscriptions", &TwitchApiRequestBus::Events::GetBroadcasterSubscriptions)
                ->Event("GetAllStreamTags", &TwitchApiRequestBus::Events::GetAllStreamTags)
                ->Event("GetStreamTags", &TwitchApiRequestBus::Events::GetStreamTags)
                ->Event("ReplaceStreamTags", &TwitchApiRequestBus::Events::ReplaceStreamTags)
                ->Event("GetUsers", &TwitchApiRequestBus::Events::GetUsers)
                ->Event("UpdateUser", &TwitchApiRequestBus::Events::UpdateUser)
                ->Event("GetUserFollows", &TwitchApiRequestBus::Events::GetUserFollows)
                ->Event("CreateUserFollows", &TwitchApiRequestBus::Events::CreateUserFollows)
                ->Event("DeleteUserFollows", &TwitchApiRequestBus::Events::DeleteUserFollows)
                ->Event("GetUserBlockList", &TwitchApiRequestBus::Events::GetUserBlockList)
                ->Event("BlockUser", &TwitchApiRequestBus::Events::BlockUser)
                ->Event("UnblockUser", &TwitchApiRequestBus::Events::UnblockUser)
                ->Event("GetUserExtensions", &TwitchApiRequestBus::Events::GetUserExtensions)
                ->Event("GetUserActiveExtensions", &TwitchApiRequestBus::Events::GetUserActiveExtensions)
                ->Event("UpdateUserExtensions", &TwitchApiRequestBus::Events::UpdateUserExtensions)
                ->Event("GetVideos", &TwitchApiRequestBus::Events::GetVideos)
                ->Event("DeleteVideos", &TwitchApiRequestBus::Events::DeleteVideos)
                ->Event("GetWebhookSubscriptions", &TwitchApiRequestBus::Events::GetWebhookSubscriptions)
                ;

            context.EBus<TwitchApiNotifyBus>("TwitchApiNotifyBus")
                ->Handler<BehaviorTwitchApiNotifyBus>();
        }
    }
}
