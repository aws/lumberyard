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

#include "TwitchApiRestUtil.h"

#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/Array.h>
#include <aws/core/utils/memory/stl/AWSString.h>

#include <AzCore/std/string/conversions.h>

namespace TwitchApi
{
    namespace RestUtil
    {
        static void PopulatePagination(const Aws::Utils::Json::JsonView& jsonDoc, PaginationResponse& pageResponse)
        {
            if (jsonDoc.KeyExists("pagination"))
            {
                Aws::Utils::Json::JsonView pagination = jsonDoc.GetObject("pagination");
                pageResponse.m_pagination = pagination.GetString("cursor").c_str();
            }
        }

        // Ads
        void PopulateCommercialResponse(const Aws::Utils::Json::JsonView& jsonDoc, CommercialResponse& comResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                CommercialDatum datum;

                datum.length = item.GetInt64("length");
                datum.message = item.GetString("message").c_str();
                datum.retry_after = item.GetInt64("retry_after");

                comResponse.m_data.push_back(datum);
            }
        }

        // Analytics
        void PopulateExtensionAnalyticsResponse(const Aws::Utils::Json::JsonView& jsonDoc, ExtensionAnalyticsResponse& analyticsResponse)
        {
            // Populate response object from JSON
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                ExtensionAnalyticsDatum datum;
                datum.extension_id = item.GetString("extension_id").c_str();
                datum.url = item.GetString("URL").c_str();
                datum.type = item.GetString("type").c_str();

                Aws::Utils::Json::JsonView dateRange = item.GetObject("date_range");
                datum.started_at = dateRange.GetString("started_at").c_str();
                datum.ended_at = dateRange.GetString("ended_at").c_str();
                analyticsResponse.m_data.push_back(datum);
            }

            PopulatePagination(jsonDoc, analyticsResponse);
        }

        void PopulateGameAnalyticsResponse(const Aws::Utils::Json::JsonView& jsonDoc, GameAnalyticsResponse& analyticsResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                GameAnalyticsDatum datum;
                datum.game_id = item.GetString("game_id").c_str();
                datum.url = item.GetString("URL").c_str();
                datum.type = item.GetString("type").c_str();

                Aws::Utils::Json::JsonView dateRange = item.GetObject("date_range");
                datum.started_at = dateRange.GetString("started_at").c_str();
                datum.ended_at = dateRange.GetString("ended_at").c_str();
                analyticsResponse.m_data.push_back(datum);
            }

            PopulatePagination(jsonDoc, analyticsResponse);
        }

        // Bits
        void PopulateBitsLeaderboardResponse(const Aws::Utils::Json::JsonView& jsonDoc, BitsLeaderboardResponse& bitsResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                BitsLeaderboardDatum datum;
                datum.user_id = item.GetString("user_id").c_str();
                datum.user_name = item.GetString("user_name").c_str();
                datum.rank = item.GetInt64("rank");
                datum.score = item.GetInt64("score");

                bitsResponse.data.push_back(datum);
            }

            Aws::Utils::Json::JsonView dateRange = jsonDoc.GetObject("date_range");
            bitsResponse.started_at = dateRange.GetString("started_at").c_str();
            bitsResponse.ended_at = dateRange.GetString("ended_at").c_str();
            bitsResponse.total = jsonDoc.GetInt64("total");
        }

        void static PopulateCheermotesImageSetDatum(const Aws::Utils::Json::JsonView& imageSet, CheermotesImageSetDatum& imageDatum)
        {
            imageDatum.url_1x = imageSet.GetString("1").c_str();
            imageDatum.url_15x = imageSet.GetString("1.5").c_str();
            imageDatum.url_2x = imageSet.GetString("2").c_str();
            imageDatum.url_3x = imageSet.GetString("3").c_str();
            imageDatum.url_4x = imageSet.GetString("4").c_str();
        }

        void PopulateCheermotesResponse(const Aws::Utils::Json::JsonView& jsonDoc, CheermotesResponse& cheersResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                CheermotesDatum datum;

                datum.prefix = item.GetString("prefix").c_str();
                datum.type = item.GetString("type").c_str();
                datum.order = item.GetInt64("order");
                datum.last_updated = item.GetString("last_updated").c_str();
                datum.is_charitable = item.GetBool("is_charitable");

                Aws::Utils::Array<Aws::Utils::Json::JsonView> tierArray = item.GetArray("tiers");
                for (size_t tierIndex = 0; tierIndex < tierArray.GetLength(); tierIndex++)
                {
                    Aws::Utils::Json::JsonView tierItem = tierArray.GetItem(tierIndex);

                    CheermotesTiersDatum tierDatum;

                    tierDatum.min_bits = tierItem.GetInt64("min_bits");
                    tierDatum.id = tierItem.GetString("id").c_str();
                    tierDatum.color = tierItem.GetString("color").c_str();

                    Aws::Utils::Json::JsonView images = tierItem.GetObject("images");
                    Aws::Utils::Json::JsonView darkImages = images.GetObject("dark");
                    Aws::Utils::Json::JsonView lightImages = images.GetObject("light");
                    PopulateCheermotesImageSetDatum(darkImages.GetObject("animated"), tierDatum.images.dark_animated);
                    PopulateCheermotesImageSetDatum(darkImages.GetObject("static"), tierDatum.images.dark_static);
                    PopulateCheermotesImageSetDatum(lightImages.GetObject("animated"), tierDatum.images.light_animated);
                    PopulateCheermotesImageSetDatum(lightImages.GetObject("static"), tierDatum.images.light_static);

                    tierDatum.can_cheer = tierItem.GetBool("can_cheer");
                    tierDatum.show_in_bits_card = tierItem.GetBool("show_in_bits_card");

                    datum.tiers.push_back(tierDatum);
                }

                cheersResponse.m_data.push_back(datum);
            }
        }

        void PopulateExtensionTransactionsResponse(const Aws::Utils::Json::JsonView& jsonDoc, ExtensionTransactionsResponse& xactResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                ExtensionTransactionDatum datum;
                datum.id = item.GetString("id").c_str();
                datum.timestamp = item.GetString("timestamp").c_str();
                datum.broadcaster_id = item.GetString("broadcaster_id").c_str();
                datum.broadcaster_name = item.GetString("broadcaster_name").c_str();
                datum.user_id = item.GetString("user_id").c_str();
                datum.product_type = item.GetString("product_type").c_str();

                Aws::Utils::Json::JsonView productData = item.GetObject("product_data");
                datum.product_data.inDevelopment = productData.GetBool("inDevelopment");
                datum.product_data.sku = productData.GetString("sku").c_str();
                datum.product_data.displayName = productData.GetString("displayName").c_str();

                if (productData.KeyExists("broadcast"))
                {
                    datum.product_data.broadcast = productData.GetBool("broadcast");
                }
                if (productData.KeyExists("domain"))
                {
                    datum.product_data.domain = productData.GetString("domain").c_str();
                }

                Aws::Utils::Json::JsonView costData = productData.GetObject("cost");
                datum.product_data.cost_amount = costData.GetInt64("amount");
                datum.product_data.cost_type = costData.GetString("type").c_str();
                xactResponse.m_data.push_back(datum);
            }

            PopulatePagination(jsonDoc, xactResponse);
        }

        // Channels
        void PopulateChannelInformationResponse(const Aws::Utils::Json::JsonView& jsonDoc, ChannelInformationResponse& channelResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);
                ChannelInformationDatum datum;

                datum.broadcaster_id = item.GetString("broadcaster_id").c_str();
                datum.broadcaster_name = item.GetString("broadcaster_name").c_str();
                datum.broadcaster_language = item.GetString("broadcaster_language").c_str();
                datum.game_id = item.GetString("game_id").c_str();
                datum.game_name = item.GetString("game_name").c_str();
                datum.title = item.GetString("title").c_str();

                channelResponse.m_data.push_back(datum);
            }
        }

        void PopulateChannelEditorsResponse(const Aws::Utils::Json::JsonView& jsonDoc, ChannelEditorsResponse& channelResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);
                ChannelEditorsDatum datum;

                datum.user_id = item.GetString("user_id").c_str();
                datum.user_name = item.GetString("user_name").c_str();
                datum.created_at = item.GetString("created_at").c_str();

                channelResponse.m_data.push_back(datum);
            }
        }

        // Channel Rewards
        static void PopulateCustomRewardsImageDatum(const Aws::Utils::Json::JsonView& imageObj, CustomRewardsImageDatum& imagedatum)
        {
            if (!imageObj.IsNull())
            {
                imagedatum.url_1x = imageObj.GetString("url_1x").c_str();
                imagedatum.url_2x = imageObj.GetString("url_2x").c_str();
                imagedatum.url_4x = imageObj.GetString("url_4x").c_str();
            }
            else
            {
                imagedatum.url_1x.clear();
                imagedatum.url_2x.clear();
                imagedatum.url_4x.clear();
            }
        }

        void PopulateCustomRewardsResponse(const Aws::Utils::Json::JsonView& jsonDoc, CustomRewardsResponse& rewardsResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                CustomRewardsDatum datum;

                datum.broadcaster_id = item.GetString("broadcaster_id").c_str();
                datum.broadcaster_name = item.GetString("broadcaster_name").c_str();
                datum.id = item.GetString("id").c_str();
                PopulateCustomRewardsImageDatum(item.GetObject("image"), datum.image);
                datum.background_color = item.GetString("background_color").c_str();
                datum.is_enabled = item.GetBool("is_enabled");
                datum.cost = item.GetInt64("cost");
                datum.title = item.GetString("title").c_str();
                datum.prompt = item.GetString("prompt").c_str();
                datum.is_user_input_required = item.GetBool("is_user_input_required");

                Aws::Utils::Json::JsonView max_per_stream = item.GetObject("max_per_stream_setting");
                datum.is_max_per_stream_enabled = max_per_stream.GetBool("is_enabled");
                datum.max_per_stream = max_per_stream.GetInt64("max_per_stream");

                Aws::Utils::Json::JsonView max_per_user_per_stream = item.GetObject("max_per_user_per_stream_setting");
                datum.is_max_per_user_per_stream_enabled = max_per_user_per_stream.GetBool("is_enabled");
                datum.max_per_user_per_stream = max_per_user_per_stream.GetInt64("max_per_user_per_stream");

                Aws::Utils::Json::JsonView global_cooldown = item.GetObject("global_cooldown_setting");
                datum.is_global_cooldown_enabled = global_cooldown.GetBool("is_enabled");
                datum.global_cooldown_seconds = global_cooldown.GetInt64("global_cooldown_seconds");

                datum.is_paused = item.GetBool("is_paused");
                datum.is_in_stock = item.GetBool("is_in_stock");
                PopulateCustomRewardsImageDatum(item.GetObject("default_image"), datum.default_image);
                datum.should_redemptions_skip_request_queue = item.GetBool("should_redemptions_skip_request_queue");
                datum.redemptions_redeemed_current_stream = item.GetInt64("redemptions_redeemed_current_stream");
                datum.cooldown_expires_at = item.GetString("cooldown_expires_at").c_str();

                rewardsResponse.m_data.push_back(datum);
            }
        }

        void PopulateCustomRewardsRedemptionResponse(const Aws::Utils::Json::JsonView& jsonDoc,
            CustomRewardsRedemptionResponse& rewardsResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                CustomRewardsRedemptionDatum datum;

                datum.broadcaster_name = item.GetString("broadcaster_name").c_str();
                datum.broadcaster_id = item.GetString("broadcaster_id").c_str();
                datum.id = item.GetString("id").c_str();
                datum.user_id = item.GetString("user_id").c_str();
                datum.user_name = item.GetString("user_name").c_str();
                datum.user_input = item.GetString("user_input").c_str();
                datum.status = item.GetString("status").c_str();
                datum.redeemed_at = item.GetString("redeemed_at").c_str();

                Aws::Utils::Json::JsonView reward = item.GetObject("reward");
                datum.reward.id = reward.GetString("id").c_str();
                datum.reward.title = reward.GetString("title").c_str();
                datum.reward.prompt = reward.GetString("prompt").c_str();
                datum.reward.cost = reward.GetInt64("cost");

                rewardsResponse.m_data.push_back(datum);
            }

            PopulatePagination(jsonDoc, rewardsResponse);
        }

        // Clips
        void PopulateCreateClipResponse(const Aws::Utils::Json::JsonView& jsonDoc, CreateClipResponse& clipResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);
                CreateClipDatum datum;
                datum.id = item.GetString("id").c_str();
                datum.edit_url = item.GetString("edit_url").c_str();
                clipResponse.m_data.push_back(datum);
            }
        }

        void PopulateGetClipsResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetClipsResponse& clipResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                GetClipsDatum datum;

                datum.broadcaster_id = item.GetString("broadcaster_id").c_str();
                datum.broadcaster_name = item.GetString("broadcaster_name").c_str();
                datum.created_at = item.GetString("created_at").c_str();
                datum.creator_id = item.GetString("creator_id").c_str();
                datum.creator_name = item.GetString("creator_name").c_str();
                datum.embed_url = item.GetString("embed_url").c_str();
                datum.game_id = item.GetString("game_id").c_str();
                datum.id = item.GetString("id").c_str();
                datum.language = item.GetString("language").c_str();
                datum.thumbnail_url = item.GetString("thumbnail_url").c_str();
                datum.title = item.GetString("title").c_str();
                datum.url = item.GetString("url").c_str();
                datum.video_id = item.GetString("video_id").c_str();
                datum.view_count = item.GetInt64("view_count");

                clipResponse.m_data.push_back(datum);
            }

            PopulatePagination(jsonDoc, clipResponse);
        }

        // Entitlements
        void PopulateEntitlementGrantsUploadURLResponse(const Aws::Utils::Json::JsonView& jsonDoc,
            EntitlementsGrantUploadResponse& entitlementResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);
                entitlementResponse.urls.push_back(item.GetString("url").c_str());
            }
        }

        void PopulateCodeStatusResponse(const Aws::Utils::Json::JsonView& jsonDoc, CodeStatusResponse& codeResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                codeResponse.statuses.insert(
                    AZStd::pair<AZStd::string, AZStd::string>(item.GetString("code").c_str(), item.GetString("status").c_str()));
            }
        }

        void PopulateDropsEntitlementsResponse(const Aws::Utils::Json::JsonView& jsonDoc, DropsEntitlementsResponse& dropResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                DropsEntitlementsDatum datum;

                datum.id = item.GetString("id").c_str();
                datum.benefit_id = item.GetString("benefit_id").c_str();
                datum.timestamp = item.GetString("timestamp").c_str();
                datum.user_id = item.GetString("user_id").c_str();
                datum.game_id = item.GetString("game_id").c_str();

                dropResponse.m_data.push_back(datum);
            }

            PopulatePagination(jsonDoc, dropResponse);
        }

        // Games
        void PopulateGetTopGamesResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetTopGamesResponse& gamesResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                GetGamesDatum datum;

                datum.id = item.GetString("id").c_str();
                datum.box_art_url = item.GetString("box_art_url").c_str();
                datum.name = item.GetString("name").c_str();

                gamesResponse.m_data.push_back(datum);
            }

            PopulatePagination(jsonDoc, gamesResponse);
        }

        void PopulateGetGamesResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetGamesResponse& gamesResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                GetGamesDatum datum;

                datum.id = item.GetString("id").c_str();
                datum.box_art_url = item.GetString("box_art_url").c_str();
                datum.name = item.GetString("name").c_str();

                gamesResponse.m_data.push_back(datum);
            }
        }

        void static PopulateHypeTrainContributionDatum(const Aws::Utils::Json::JsonView& contribution,
            HypeTrainContributionDatum& hypeResponse)
        {
            hypeResponse.total = contribution.GetInt64("total");
            hypeResponse.type = contribution.GetString("type").c_str();
            hypeResponse.user = contribution.GetString("user").c_str();
        }

        // Hype Train
        void PopulateHypeTrainEventsResponse(const Aws::Utils::Json::JsonView& jsonDoc, HypeTrainEventsResponse& hypeResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                HypeTrainEventsDatum datum;

                datum.id = item.GetString("id").c_str();
                datum.event_type = item.GetString("event_type").c_str();
                datum.event_timestamp = item.GetString("event_timestamp").c_str();
                datum.version = item.GetString("version").c_str();

                Aws::Utils::Json::JsonView event = item.GetObject("event_data");

                datum.event_data.broadcaster_id = event.GetString("broadcaster_id").c_str();
                datum.event_data.cooldown_end_time = event.GetString("cooldown_end_time").c_str();
                datum.event_data.expires_at = event.GetString("expires_at").c_str();
                datum.event_data.goal = event.GetInt64("goal");
                datum.event_data.id = event.GetString("id").c_str();
                PopulateHypeTrainContributionDatum(event.GetObject("last_contribution"), datum.event_data.last_contribution);
                datum.event_data.level = event.GetInt64("level");
                datum.event_data.started_at = event.GetString("started_at").c_str();

                Aws::Utils::Array<Aws::Utils::Json::JsonView> contributionsArray = event.GetArray("top_contributions");
                for (size_t contIndex = 0; contIndex < contributionsArray.GetLength(); contIndex++)
                {
                    Aws::Utils::Json::JsonView contribution = contributionsArray.GetItem(contIndex);

                    HypeTrainContributionDatum contDatum;
                    PopulateHypeTrainContributionDatum(contribution, contDatum);

                    datum.event_data.top_contributions.push_back(contDatum);
                }

                datum.event_data.total = event.GetInt64("total");

                hypeResponse.m_data.push_back(datum);
            }

            PopulatePagination(jsonDoc, hypeResponse);
        }

        // Moderation
        void PopulateCheckAutoModResponse(const Aws::Utils::Json::JsonView& jsonDoc, CheckAutoModResponse& modResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                CheckAutoModResponseDatum datum;

                datum.msg_id = item.GetString("msg_id").c_str();
                datum.is_permitted = item.GetBool("is_permitted");

                modResponse.m_data.push_back(datum);
            }
        }

        void PopulateGetBannedEventsResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetBannedEventsResponse& bannedResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                GetBannedEventsDatum datum;
                datum.id = item.GetString("id").c_str();
                datum.event_type = item.GetString("event_type").c_str();
                datum.event_timestamp = item.GetString("event_timestamp").c_str();
                datum.version = item.GetString("version").c_str();

                Aws::Utils::Json::JsonView eventData = item.GetObject("event_data");
                datum.broadcaster_id = eventData.GetString("broadcaster_id").c_str();
                datum.broadcaster_name = eventData.GetString("broadcaster_name").c_str();
                datum.expires_at = eventData.GetString("expires_at").c_str();
                datum.user_id = eventData.GetString("user_id").c_str();
                datum.user_name = eventData.GetString("user_name").c_str();

                bannedResponse.m_data.push_back(datum);
            }

            PopulatePagination(jsonDoc, bannedResponse);
        }

        void PopulateGetBannedUsersResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetBannedUsersResponse& bannedResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                GetBannedUsersDatum datum;
                datum.user_id = item.GetString("user_id").c_str();
                datum.user_name = item.GetString("user_name").c_str();
                datum.expires_at = item.GetString("expires_at").c_str();

                bannedResponse.m_data.push_back(datum);
            }

            PopulatePagination(jsonDoc, bannedResponse);
        }

        void PopulateGetModeratorsResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetModeratorsResponse& modResponse)
        {
            if (jsonDoc.KeyExists("data") && !jsonDoc.GetObject("data").IsNull())
            {
                Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
                for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
                {
                    Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                    GetModeratorsDatum datum;
                    datum.user_id = item.GetString("user_id").c_str();
                    datum.user_name = item.GetString("user_name").c_str();

                    modResponse.m_data.push_back(datum);
                }
            }

            PopulatePagination(jsonDoc, modResponse);
        }

        void PopulateGetModeratorEventsResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetModeratorEventsResponse& modResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                GetModeratorEventsDatum datum;
                datum.id = item.GetString("id").c_str();
                datum.event_timestamp = item.GetString("event_timestamp").c_str();
                datum.event_type = item.GetString("event_type").c_str();
                datum.version = item.GetString("version").c_str();

                Aws::Utils::Json::JsonView eventData = item.GetObject("event_data");
                datum.broadcaster_id = eventData.GetString("broadcaster_id").c_str();
                datum.broadcaster_name = eventData.GetString("broadcaster_name").c_str();
                datum.user_id = eventData.GetString("user_id").c_str();
                datum.user_name = eventData.GetString("user_name").c_str();

                modResponse.m_data.push_back(datum);
            }

            PopulatePagination(jsonDoc, modResponse);
        }

        // Search
        void PopulateSearchCategoriesResponse(const Aws::Utils::Json::JsonView& jsonDoc, SearchCategoriesResponse& searchResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                SearchCategoriesDatum datum;
                datum.id = item.GetString("id").c_str();
                datum.name = item.GetString("name").c_str();
                datum.box_art_url = item.GetString("box_art_url").c_str();

                searchResponse.m_data.push_back(datum);
            }

            PopulatePagination(jsonDoc, searchResponse);
        }

        void PopulateSearchChannelsResponse(const Aws::Utils::Json::JsonView& jsonDoc, SearchChannelsResponse& searchResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                SearchChannelsDatum datum;
                datum.broadcaster_language = item.GetString("broadcaster_language").c_str();
                datum.display_name = item.GetString("display_name").c_str();
                datum.game_id = item.GetString("game_id").c_str();
                datum.id = item.GetString("id").c_str();
                datum.is_live = item.GetBool("is_live");

                if (item.KeyExists("tag_ids"))
                {
                    Aws::Utils::Array<Aws::Utils::Json::JsonView> tagArray = item.GetArray("tag_ids");
                    for (size_t tagIndex = 0; tagIndex < tagArray.GetLength(); tagIndex++)
                    {
                        Aws::Utils::Json::JsonView tag = tagArray.GetItem(tagIndex);

                        datum.tag_ids.push_back(tag.AsString().c_str());
                    }
                }

                datum.thumbnail_url = item.GetString("thumbnail_url").c_str();
                datum.title = item.GetString("id").c_str();
                datum.started_at = item.GetString("started_at").c_str();

                searchResponse.m_data.push_back(datum);
            }

            PopulatePagination(jsonDoc, searchResponse);
        }

        // Streams
        void PopulateStreamKeyResponse(const Aws::Utils::Json::JsonView& jsonDoc, StreamKeyResponse& keyResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                keyResponse.stream_key = item.GetString("stream_key").c_str();

                break;
            }
        }

        void PopulateGetStreamsResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetStreamsResponse& streamsResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                GetStreamsDatum datum;
                datum.game_id = item.GetString("game_id").c_str();
                datum.id = item.GetString("id").c_str();
                datum.language = item.GetString("language").c_str();
                datum.started_at = item.GetString("started_at").c_str();
                datum.thumbnail_url = item.GetString("thumbnail_url").c_str();
                datum.title = item.GetString("title").c_str();
                datum.type = item.GetString("type").c_str();
                datum.user_id = item.GetString("user_id").c_str();
                datum.user_name = item.GetString("user_name").c_str();
                datum.viewer_count = item.GetInt64("viewer_count");

                if (item.KeyExists("tag_ids"))
                {
                    Aws::Utils::Array<Aws::Utils::Json::JsonView> tagsArray = item.GetArray("tag_ids");
                    for (size_t tagIndex = 0; tagIndex < tagsArray.GetLength(); tagIndex++)
                    {
                        datum.tag_ids.push_back(tagsArray.GetItem(tagIndex).AsString().c_str());
                    }
                }

                streamsResponse.m_data.push_back(datum);
            }

            PopulatePagination(jsonDoc, streamsResponse);
        }

        void PopulateCreateStreamMarkerResponse(const Aws::Utils::Json::JsonView& jsonDoc, CreateStreamMarkerResponse& streamsResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                CreateStreamMarkerDatum datum;
                datum.created_at = item.GetString("created_at").c_str();
                datum.description = item.GetString("description").c_str();
                datum.id = item.GetString("id").c_str();
                datum.position_seconds = item.GetString("position_seconds").c_str();

                streamsResponse.m_data.push_back(datum);
            }
        }

        void PopulateGetStreamMarkersResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetStreamMarkersResponse& streamsResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                GetStreamMarkersDatum datum;

                datum.user_id = item.GetString("user_id").c_str();
                datum.user_name = item.GetString("user_name").c_str();

                Aws::Utils::Array<Aws::Utils::Json::JsonView> videoArray = item.GetArray("videos");
                for (size_t videoIndex = 0; videoIndex < videoArray.GetLength(); videoIndex++)
                {
                    Aws::Utils::Json::JsonView videoItem = videoArray.GetItem(videoIndex);

                    GetStreamMarkersVideoDatum videoDatum;
                    videoDatum.video_id = videoItem.GetString("video_id").c_str();

                    Aws::Utils::Array<Aws::Utils::Json::JsonView> markerArray = videoItem.GetArray("markers");
                    for (size_t markerIndex = 0; markerIndex < markerArray.GetLength(); markerIndex++)
                    {
                        Aws::Utils::Json::JsonView markerItem = markerArray.GetItem(markerIndex);

                        GetStreamMarkersMarkerDatum markerDatum;
                        markerDatum.id = markerItem.GetString("id").c_str();
                        markerDatum.created_at = markerItem.GetString("created_at").c_str();
                        markerDatum.description = markerItem.GetString("description").c_str();
                        markerDatum.position_seconds = markerItem.GetInt64("position_seconds");
                        markerDatum.URL = markerItem.GetString("URL").c_str();

                        videoDatum.markers.push_back(markerDatum);
                    }
                    datum.videos.push_back(videoDatum);
                }

                streamsResponse.m_data.push_back(datum);
            }

            PopulatePagination(jsonDoc, streamsResponse);
        }

        // Subscriptions
        void PopulateGetBroadcasterSubscriptionsResponse(const Aws::Utils::Json::JsonView& jsonDoc,
            GetBroadcasterSubscriptionsResponse& subResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                GetBroadcasterSubscriptionsDatum datum;
                datum.broadcaster_id = item.GetString("broadcaster_id").c_str();
                datum.broadcaster_name = item.GetString("broadcaster_name").c_str();
                datum.is_gift = item.GetBool("is_gift");
                datum.tier = item.GetString("tier").c_str();
                datum.plan_name = item.GetString("plan_name").c_str();
                datum.user_id = item.GetString("user_id").c_str();
                datum.user_name = item.GetString("user_name").c_str();

                subResponse.m_data.push_back(datum);
            }

            PopulatePagination(jsonDoc, subResponse);
        }

        // Tags
        void PopulateGetAllStreamTagsResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetAllStreamTagsResponse& tagsResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                GetStreamTagsDatum datum;
                datum.tag_id = item.GetString("tag_id").c_str();
                datum.is_auto = item.GetBool("is_auto");

                Aws::Utils::Json::JsonView locNamesData = item.GetObject("localization_names");
                Aws::Map<Aws::String, Aws::Utils::Json::JsonView> locNamesMap = locNamesData.GetAllObjects();
                for (const std::pair<const Aws::String, Aws::Utils::Json::JsonView>& mapping : locNamesMap)
                {
                    datum.localization_names[mapping.first.c_str()] = mapping.second.AsString().c_str();
                }

                Aws::Utils::Json::JsonView locDescData = item.GetObject("localization_descriptions");
                Aws::Map<Aws::String, Aws::Utils::Json::JsonView> locDescMap = locDescData.GetAllObjects();
                for (const std::pair<const Aws::String, Aws::Utils::Json::JsonView>& mapping : locDescMap)
                {
                    datum.localization_descriptions[mapping.first.c_str()] = mapping.second.AsString().c_str();
                }

                tagsResponse.m_data.push_back(datum);
            }

            PopulatePagination(jsonDoc, tagsResponse);
        }

        void PopulateGetStreamTagsResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetStreamTagsResponse& tagsResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                GetStreamTagsDatum datum;
                datum.tag_id = item.GetString("tag_id").c_str();
                datum.is_auto = item.GetBool("is_auto");

                Aws::Utils::Json::JsonView locNamesData = item.GetObject("localization_names");
                Aws::Map<Aws::String, Aws::Utils::Json::JsonView> locNamesMap = locNamesData.GetAllObjects();
                for (const std::pair<const Aws::String, Aws::Utils::Json::JsonView>& mapping : locNamesMap)
                {
                    datum.localization_names[mapping.first.c_str()] = mapping.second.AsString().c_str();
                }

                Aws::Utils::Json::JsonView locDescData = item.GetObject("localization_descriptions");
                Aws::Map<Aws::String, Aws::Utils::Json::JsonView> locDescMap = locDescData.GetAllObjects();
                for (const std::pair<const Aws::String, Aws::Utils::Json::JsonView>& mapping : locDescMap)
                {
                    datum.localization_descriptions[mapping.first.c_str()] = mapping.second.AsString().c_str();
                }

                tagsResponse.m_data.push_back(datum);
            }
        }

        // Users
        void PopulateGetUsersResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetUsersResponse& usersResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                GetUsersDatum datum;
                datum.broadcaster_type = item.GetString("broadcaster_type").c_str();
                datum.description = item.GetString("description").c_str();
                datum.display_name = item.GetString("display_name").c_str();
                datum.email = item.GetString("email").c_str();
                datum.id = item.GetString("id").c_str();
                datum.login = item.GetString("login").c_str();
                datum.offline_image_url = item.GetString("offline_image_url").c_str();
                datum.profile_image_url = item.GetString("profile_image_url").c_str();
                datum.type = item.GetString("type").c_str();
                datum.view_count = item.GetInt64("view_count");

                usersResponse.m_data.push_back(datum);
            }
        }

        void PopulateGetUsersFollowsResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetUsersFollowsResponse& usersResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                GetUsersFollowsDatum datum;
                datum.from_id = item.GetString("from_id").c_str();
                datum.from_name = item.GetString("from_name").c_str();
                datum.to_id = item.GetBool("to_id");
                datum.to_name = item.GetString("to_name").c_str();
                datum.followed_at = item.GetString("followed_at").c_str();

                usersResponse.m_data.push_back(datum);
            }

            PopulatePagination(jsonDoc, usersResponse);
        }

        void PopulateUserBlockListResponse(const Aws::Utils::Json::JsonView& jsonDoc, UserBlockListResponse& blockResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                UserBlockListDatum datum;
                datum.id = item.GetString("id").c_str();
                datum.user_login = item.GetString("user_login").c_str();
                datum.display_name = item.GetString("display_name").c_str();

                blockResponse.m_data.push_back(datum);
            }

            PopulatePagination(jsonDoc, blockResponse);
        }

        void PopulateGetUserExtensionsResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetUserExtensionsResponse& usersResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                GetUserExtensionsDatum datum;
                datum.can_activate = item.GetBool("can_activate");
                datum.id = item.GetString("id").c_str();
                datum.name = item.GetString("name").c_str();
                datum.version = item.GetString("version").c_str();

                Aws::Utils::Array<Aws::Utils::Json::JsonView> typeDataArray = item.GetArray("type");
                for (size_t typeIndex = 0; typeIndex < typeDataArray.GetLength(); typeIndex++)
                {
                    Aws::Utils::Json::JsonView typeData = typeDataArray.GetItem(typeIndex);
                    datum.type.push_back(typeData.AsString().c_str());
                }

                usersResponse.m_data.push_back(datum);
            }
        }

        // Helper util to handle populating an element of the response for GetUserActiveExtensions
        GetUserActiveExtensionsComponent PopulateGetUserActiveExtensionsComponent(const Aws::Utils::Json::JsonView& data)
        {
            GetUserActiveExtensionsComponent component;
            component.active = data.GetBool("active");
            if (data.KeyExists("active"))
            {
                component.id = data.GetString("id").c_str();
            }
            if (data.KeyExists("version"))
            {
                component.version = data.GetString("version").c_str();
            }
            if (data.KeyExists("name"))
            {
                component.name = data.GetString("name").c_str();
            }
            if (data.KeyExists("x"))
            {
                component.x = data.GetInt64("x");
            }
            if (data.KeyExists("y"))
            {
                component.y = data.GetInt64("y");
            }

            return component;
        }

        // Helper util to handle parsing and populating a JSON map for GetUserActiveExtensions
        void PopulateGetUserActiveExtensionsMap(const Aws::Utils::Json::JsonView& source, AZStd::string map_name,
            AZStd::unordered_map<AZStd::string, GetUserActiveExtensionsComponent>& out_map)
        {
            Aws::Utils::Json::JsonView jsonData = source.GetObject(map_name.c_str());
            Aws::Map<Aws::String, Aws::Utils::Json::JsonView> jsonMap = jsonData.GetAllObjects();
            for (const std::pair<const Aws::String, Aws::Utils::Json::JsonView>& mapping : jsonMap)
            {
                out_map[mapping.first.c_str()] = PopulateGetUserActiveExtensionsComponent(mapping.second);
            }
        }

        void PopulateGetUserActiveExtensionsResponse(const Aws::Utils::Json::JsonView& jsonDoc,
            GetUserActiveExtensionsResponse& usersResponse)
        {
            Aws::Utils::Json::JsonView jsonDataObject = jsonDoc.GetObject("data");
            PopulateGetUserActiveExtensionsMap(jsonDataObject, "panel", usersResponse.panel);
            PopulateGetUserActiveExtensionsMap(jsonDataObject, "overlay", usersResponse.overlay);
            PopulateGetUserActiveExtensionsMap(jsonDataObject, "component", usersResponse.component);
        }

        // Videos
        void PopulateGetVideosResponse(const Aws::Utils::Json::JsonView& jsonDoc, GetVideosResponse& videosResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                GetVideosDatum datum;
                datum.created_at = item.GetString("created_at").c_str();
                datum.description = item.GetString("description").c_str();
                datum.duration = item.GetBool("duration");
                datum.id = item.GetString("id").c_str();
                datum.language = item.GetString("language").c_str();
                datum.published_at = item.GetString("published_at").c_str();
                datum.thumbnail_url = item.GetString("thumbnail_url").c_str();
                datum.title = item.GetString("title").c_str();
                datum.url = item.GetString("url").c_str();
                datum.user_id = item.GetString("user_id").c_str();
                datum.user_name = item.GetString("user_name").c_str();
                datum.view_count = item.GetInt64("view_count");
                datum.viewable = item.GetString("viewable").c_str();

                videosResponse.m_data.push_back(datum);
            }

            PopulatePagination(jsonDoc, videosResponse);
        }

        void PopulateDeleteVideosResponse(const Aws::Utils::Json::JsonView& jsonDoc, DeleteVideosResponse& videosResponse)
        {
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);
                videosResponse.video_keys.push_back(item.AsString().c_str());
            }
        }

        // Webhooks
        void PopulateGetWebhookSubscriptionsResponse(const Aws::Utils::Json::JsonView& jsonDoc,
            GetWebhookSubscriptionsResponse& webhooksResponse)
        {
            webhooksResponse.total = jsonDoc.GetInt64("total");
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonDataArray = jsonDoc.GetArray("data");
            for (size_t index = 0; index < jsonDataArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item = jsonDataArray.GetItem(index);

                GetWebhookSubscriptionsDatum datum;
                datum.topic = item.GetString("topic").c_str();
                datum.callback = item.GetString("callback").c_str();
                datum.expires_at = item.GetBool("expires_at");

                webhooksResponse.data.push_back(datum);
            }

            PopulatePagination(jsonDoc, webhooksResponse);
        }
    }
}
