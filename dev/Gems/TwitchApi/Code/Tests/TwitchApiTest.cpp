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

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

#include <TwitchApi/RestTypes.h>
#include "TwitchApiRestUtil.h"

#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/Array.h>
#include <aws/core/utils/memory/stl/AWSString.h>

class TwitchApiTest
    : public UnitTest::AllocatorsTestFixture
{
protected:
    void SetUp() override
    {
        AllocatorsTestFixture::SetUp();
    }

    void TearDown() override
    {
        AllocatorsTestFixture::TearDown();
    }
};

TEST_F(TwitchApiTest, TestStartCommercialResponse_FT)
{
    Aws::String rawJson = R"(
        {
            "data": [{
                "length" : 60,
                "message" : "",
                "retry_after" : 480
            }]
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::CommercialResponse response;
    TwitchApi::RestUtil::PopulateCommercialResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 1);
    ASSERT_EQ(response.m_data.front().length, 60);
    ASSERT_EQ(response.m_data.front().message, "");
    ASSERT_EQ(response.m_data.front().retry_after, 480);
}

TEST_F(TwitchApiTest, TestExtensionAnalyticsResponse_FT)
{
    Aws::String rawJson = R"(
        {
           "data": [
              {
                 "extension_id": "abcd",
                 "URL": "https://my.twitch.test/123",
                 "type": "overview_v1",
                 "date_range": {
                    "started_at": "2018-04-30T00:00:00Z",
                    "ended_at": "2018-06-01T00:00:00Z"
                 }
              },
              {
                 "extension_id": "efgh",
                 "URL": "https://my.twitch.test/456",
                 "type": "overview_v2",
                 "date_range": {
                    "started_at": "2018-03-01T00:00:00Z",
                    "ended_at": "2018-06-01T00:00:00Z"
                 }
              }
           ],
           "pagination": {"cursor": "eyJiIjpudWxsLCJhIjp7Ik9mZnNldCI6NX19"}
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::ExtensionAnalyticsResponse response;
    TwitchApi::RestUtil::PopulateExtensionAnalyticsResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 2);
    ASSERT_EQ(response.m_data.front().type, "overview_v1");
    ASSERT_EQ(response.m_data.front().started_at, "2018-04-30T00:00:00Z");
    ASSERT_EQ(response.m_data.back().started_at, "2018-03-01T00:00:00Z");
    ASSERT_EQ(response.m_pagination, "eyJiIjpudWxsLCJhIjp7Ik9mZnNldCI6NX19");
}

TEST_F(TwitchApiTest, TestGameAnalyticsResponse_FT)
{
    Aws::String rawJson = R"(
        {
          "data": [
            {
              "game_id" : "493057",
              "URL" : "https://my.twitch.test/123",
              "type" : "overview_v2",
              "date_range" : {
                "started_at" : "2018-01-01T00:00:00Z",
                "ended_at" : "2018-03-01T00:00:00Z"
              }
            }
          ],
          "pagination": {"cursor": "eyJiIjpudWxsLJxhIjoiIn0gf5"}
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::GameAnalyticsResponse response;
    TwitchApi::RestUtil::PopulateGameAnalyticsResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 1);
    ASSERT_EQ(response.m_data.front().game_id, "493057");
    ASSERT_EQ(response.m_data.front().started_at, "2018-01-01T00:00:00Z");
    ASSERT_EQ(response.m_pagination, "eyJiIjpudWxsLJxhIjoiIn0gf5");
}

TEST_F(TwitchApiTest, TestBitsLeaderboardResponse_FT)
{
    Aws::String rawJson = R"(
        {
          "data": [
            {
              "user_id": "158010205",
              "user_name": "TundraCowboy",
              "rank": 1,
              "score": 12543
            },
            {
              "user_id": "7168163",
              "user_name": "Topramens",
              "rank": 2,
              "score": 6900
            }
          ],
          "date_range": {
            "started_at": "2018-02-05T08:00:00Z",
            "ended_at": "2018-02-12T08:00:00Z"
          },
          "total": 2
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::BitsLeaderboardResponse response;
    TwitchApi::RestUtil::PopulateBitsLeaderboardResponse(jsonVal.View(), response);

    ASSERT_EQ(response.data.size(), 2);
    ASSERT_EQ(response.data.front().score, 12543);
    ASSERT_EQ(response.data.back().rank, 2);
    ASSERT_EQ(response.ended_at, "2018-02-12T08:00:00Z");
}

TEST_F(TwitchApiTest, TestGetCheermotesResponse_FT)
{
    Aws::String rawJson = R"(
        {
            "data": [
                {
                    "prefix": "Cheer",
                    "tiers": [
                        {
                            "min_bits": 1,
                            "id": "1",
                            "color": "#979797",
                            "images": {
                                "dark": {
                                    "animated": {
                                        "1": "https://d3aqoihi2n8ty8.cloudfront.net/actions/cheer/dark/animated/1/1.gif",
                                        "1.5": "https://d3aqoihi2n8ty8.cloudfront.net/actions/cheer/dark/animated/1/1.5.gif",
                                        "2": "https://d3aqoihi2n8ty8.cloudfront.net/actions/cheer/dark/animated/1/2.gif",
                                        "3": "https://d3aqoihi2n8ty8.cloudfront.net/actions/cheer/dark/animated/1/3.gif",
                                        "4": "https://d3aqoihi2n8ty8.cloudfront.net/actions/cheer/dark/animated/1/4.gif"
                                    },
                                    "static": {
                                        "1": "https://d3aqoihi2n8ty8.cloudfront.net/actions/cheer/dark/static/1/1.png",
                                        "1.5": "https://d3aqoihi2n8ty8.cloudfront.net/actions/cheer/dark/static/1/1.5.png",
                                        "2": "https://d3aqoihi2n8ty8.cloudfront.net/actions/cheer/dark/static/1/2.png",
                                        "3": "https://d3aqoihi2n8ty8.cloudfront.net/actions/cheer/dark/static/1/3.png",
                                        "4": "https://d3aqoihi2n8ty8.cloudfront.net/actions/cheer/dark/static/1/4.png"
                                    }
                                },
                                "light": {
                                    "animated": {
                                        "1": "https://d3aqoihi2n8ty8.cloudfront.net/actions/cheer/light/animated/1/1.gif",
                                        "1.5": "https://d3aqoihi2n8ty8.cloudfront.net/actions/cheer/light/animated/1/1.5.gif",
                                        "2": "https://d3aqoihi2n8ty8.cloudfront.net/actions/cheer/light/animated/1/2.gif",
                                        "3": "https://d3aqoihi2n8ty8.cloudfront.net/actions/cheer/light/animated/1/3.gif",
                                        "4": "https://d3aqoihi2n8ty8.cloudfront.net/actions/cheer/light/animated/1/4.gif"
                                    },
                                    "static": {
                                        "1": "https://d3aqoihi2n8ty8.cloudfront.net/actions/cheer/light/static/1/1.png",
                                        "1.5": "https://d3aqoihi2n8ty8.cloudfront.net/actions/cheer/light/static/1/1.5.png",
                                        "2": "https://d3aqoihi2n8ty8.cloudfront.net/actions/cheer/light/static/1/2.png",
                                        "3": "https://d3aqoihi2n8ty8.cloudfront.net/actions/cheer/light/static/1/3.png",
                                        "4": "https://d3aqoihi2n8ty8.cloudfront.net/actions/cheer/light/static/1/4.png"
                                    }
                                }
                            },
                            "can_cheer": true,
                            "show_in_bits_card": true
                        }
                    ],
                    "type": "global_first_party",
                    "order": 1,
                    "last_updated": "2018-05-22T00:06:04Z",
                    "is_charitable": false
                }
            ]
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::CheermotesResponse response;
    TwitchApi::RestUtil::PopulateCheermotesResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 1);
    ASSERT_EQ(response.m_data.front().prefix, "Cheer");
    ASSERT_EQ(response.m_data.front().tiers.size(), 1);
    ASSERT_EQ(response.m_data.front().tiers.front().min_bits, 1);
    ASSERT_EQ(response.m_data.front().tiers.front().can_cheer, true);
    ASSERT_EQ(response.m_data.front().tiers.front().images.light_animated.url_2x,
        "https://d3aqoihi2n8ty8.cloudfront.net/actions/cheer/light/animated/1/2.gif");
    ASSERT_EQ(response.m_data.front().last_updated, "2018-05-22T00:06:04Z");
}

TEST_F(TwitchApiTest, TestExtensionTransactionsResponse_FT)
{
    Aws::String rawJson = R"(
        {
             "data": [
                  {
                       "id": "74c52265-e214-48a6-91b9-23b6014e8041",
                       "timestamp": "2019-01-28T04:15:53.325Z",
                       "broadcaster_id": "439964613",
                       "broadcaster_name": "chikuseuma",
                       "user_id": "424596340",
                       "user_name": "quotrok",
                       "product_type": "BITS_IN_EXTENSION",
                       "product_data": {
                             "sku": "testSku100",
                             "cost": {
                                  "amount": 100,
                                  "type": "bits"
                             },
                             "displayName": "Test Sku",
                             "inDevelopment": false
                        }
                   },
                   {
                        "id": "8d303dc6-a460-4945-9f48-59c31d6735cb",
                        "timestamp": "2019-01-18T09:10:13.397Z",
                        "broadcaster_id": "439964613",
                        "broadcaster_name": "chikuseuma",
                        "user_id": "439966926",
                        "user_name": "liscuit",
                        "product_type": "BITS_IN_EXTENSION",
                        "product_data": {
                             "sku": "testSku100",
                             "cost": {
                                  "amount": 100,
                                  "type": "bits"
                             },
                             "displayName": "Test Sku",
                             "inDevelopment": false
                        }
                  }
             ],
             "pagination": {
                  "cursor": "cursorString"
             }
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::ExtensionTransactionsResponse response;
    TwitchApi::RestUtil::PopulateExtensionTransactionsResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 2);
    ASSERT_EQ(response.m_data.front().id, "74c52265-e214-48a6-91b9-23b6014e8041");
    ASSERT_EQ(response.m_data.front().product_data.displayName, "Test Sku");
    ASSERT_FALSE(response.m_data.back().product_data.inDevelopment);
    ASSERT_EQ(response.m_pagination, "cursorString");
}

TEST_F(TwitchApiTest, TestGetChannelInformationResponse_FT)
{
    Aws::String rawJson = R"(
        {
            "data": [
                {
                    "broadcaster_id": "44445592",
                    "broadcaster_name": "pokimane",
                    "broadcaster_language": "en",
                    "game_id": "21779",
                    "game_name": "League of Legends",
                    "title": "twitch rivals ft yassuo, trick2g, simpchovies & benji :"
                }
            ]
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::ChannelInformationResponse response;
    TwitchApi::RestUtil::PopulateChannelInformationResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 1);
    ASSERT_EQ(response.m_data.front().broadcaster_id, "44445592");
    ASSERT_EQ(response.m_data.front().broadcaster_language, "en");
    ASSERT_EQ(response.m_data.front().game_name, "League of Legends");
}

TEST_F(TwitchApiTest, TestGetChannelEditorsResponse_FT)
{
    Aws::String rawJson = R"(
        {
            "data": [
                {
                    "user_id": "182891647",
                    "user_name": "mauerbac",
                    "created_at": "2019-02-15T21:19:50.380833Z"
                },
                {
                    "user_id": "135093069",
                    "user_name": "BlueLava",
                    "created_at": "2018-03-07T16:28:29.872937Z"
                }
            ]
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::ChannelEditorsResponse response;
    TwitchApi::RestUtil::PopulateChannelEditorsResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 2);
    ASSERT_EQ(response.m_data.front().user_id, "182891647");
    ASSERT_EQ(response.m_data.front().created_at, "2019-02-15T21:19:50.380833Z");
    ASSERT_EQ(response.m_data.at(1).user_name, "BlueLava");
}

TEST_F(TwitchApiTest, TestCustomRewardsResponse_FT)
{
    Aws::String rawJson = R"(
        {
            "data": [
                {
                    "broadcaster_name": "torpedo09",
                    "broadcaster_id": "274637212",
                    "id": "afaa7e34-6b17-49f0-a19a-d1e76eaaf673",
                    "image": null,
                    "background_color": "#00E5CB",
                    "is_enabled": true,
                    "cost": 50000,
                    "title": "game analysis 1v1",
                    "prompt": "",
                    "is_user_input_required": false,
                    "max_per_stream_setting": {
                        "is_enabled": false,
                        "max_per_stream": 0
                    },
                    "max_per_user_per_stream_setting": {
                        "is_enabled": false,
                        "max_per_user_per_stream": 0
                    },
                    "global_cooldown_setting": {
                        "is_enabled": false,
                        "global_cooldown_seconds": 0
                    },
                    "is_paused": false,
                    "is_in_stock": true,
                    "default_image": {
                        "url_1x": "https://static-cdn.jtvnw.net/custom-reward-images/default-1.png",
                        "url_2x": "https://static-cdn.jtvnw.net/custom-reward-images/default-2.png",
                        "url_4x": "https://static-cdn.jtvnw.net/custom-reward-images/default-4.png"
                    },
                    "should_redemptions_skip_request_queue": false,
                    "redemptions_redeemed_current_stream": null,
                    "cooldown_expires_at": null
                }
            ]
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::CustomRewardsResponse response;
    TwitchApi::RestUtil::PopulateCustomRewardsResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 1);
    ASSERT_EQ(response.m_data.front().broadcaster_name, "torpedo09");
    ASSERT_EQ(response.m_data.front().is_max_per_stream_enabled, false);
    ASSERT_EQ(response.m_data.front().max_per_stream, 0);
    ASSERT_EQ(response.m_data.front().is_paused, false);
    ASSERT_EQ(response.m_data.front().default_image.url_1x, "https://static-cdn.jtvnw.net/custom-reward-images/default-1.png");
    ASSERT_EQ(response.m_data.front().redemptions_redeemed_current_stream, 0);
}

TEST_F(TwitchApiTest, TestCustomRewardRedemptionResponse_FT)
{
    Aws::String rawJson = R"(
        {
            "data": [
                {
                    "broadcaster_name": "torpedo09",
                    "broadcaster_id": "274637212",
                    "id": "17fa2df1-ad76-4804-bfa5-a40ef63efe63",
                    "user_id": "274637212",
                    "user_name": "torpedo09",
                    "user_input": "",
                    "status": "CANCELED",
                    "redeemed_at": "2020-07-01T18:37:32Z",
                    "reward": {
                        "id": "92af127c-7326-4483-a52b-b0da0be61c01",
                        "title": "game analysis",
                        "prompt": "",
                        "cost": 50000
                    }
                }
            ],
            "pagination": {
                "cursor": "eyJiIjpudWxsLCJhIjp7"
            }
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::CustomRewardsRedemptionResponse response;
    TwitchApi::RestUtil::PopulateCustomRewardsRedemptionResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 1);
    ASSERT_EQ(response.m_data.front().broadcaster_name, "torpedo09");
    ASSERT_EQ(response.m_data.front().user_input, "");
    ASSERT_EQ(response.m_data.front().status, "CANCELED");
    ASSERT_EQ(response.m_data.front().reward.title, "game analysis");
    ASSERT_EQ(response.m_pagination, "eyJiIjpudWxsLCJhIjp7");
}

TEST_F(TwitchApiTest, TestCreateClipResponse_FT)
{
    Aws::String rawJson = R"(
        {
           "data":
           [{
              "id": "FiveWordsForClipSlug",
              "edit_url": "https://my.twitch.test/123"
           }]
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::CreateClipResponse response;
    TwitchApi::RestUtil::PopulateCreateClipResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 1);
    ASSERT_EQ(response.m_data.front().id, "FiveWordsForClipSlug");
}

TEST_F(TwitchApiTest, TestGetClipsResponse_FT)
{
    Aws::String rawJson = R"(
        {
          "data": [
            {
              "id":"RandomClip1",
              "url":"https://my.twitch.test/123",
              "embed_url":"https://my.twitch.test/456",
              "broadcaster_id":"1234",
              "broadcaster_name": "JJ",
              "creator_id":"123456",
              "creator_name": "MrMarshall",
              "video_id":"1234567",
              "game_id":"33103",
              "language":"en",
              "title":"random1",
              "view_count":10,
              "created_at":"2017-11-30T22:34:18Z",
              "thumbnail_url":"https://my.twitch.test/789"
            }
          ],
          "pagination": {
            "cursor": "eyJiIjpudWxsLCJhIjoiIn0"
          }
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::GetClipsResponse response;
    TwitchApi::RestUtil::PopulateGetClipsResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 1);
    ASSERT_EQ(response.m_data.front().id, "RandomClip1");
    ASSERT_EQ(response.m_data.front().embed_url, "https://my.twitch.test/456");
    ASSERT_EQ(response.m_pagination, "eyJiIjpudWxsLCJhIjoiIn0");
}

TEST_F(TwitchApiTest, TestEntitlementGrantsUploadURLResponse_FT)
{
    Aws::String rawJson = R"(
        {
          "data":[
            {
              "url": "https://my.twitch.test/123"
            },
            {
              "url": "https://my.twitch.test/456"
            }
          ]
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::EntitlementsGrantUploadResponse response;
    TwitchApi::RestUtil::PopulateEntitlementGrantsUploadURLResponse(jsonVal.View(), response);

    ASSERT_EQ(response.urls.size(), 2);
    ASSERT_EQ(response.urls.front(), "https://my.twitch.test/123");
}

TEST_F(TwitchApiTest, TestCodeStatusResponse_FT)
{
    Aws::String rawJson = R"(
        {
            "data":[
                {
                    "code":"KUHXV-4GXYP-AKAKK",
                    "status":"UNUSED"
                },
                {
                    "code":"XZDDZ-5SIQR-RT5M3",
                    "status":"ALREADY_CLAIMED"
                }
            ]
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::CodeStatusResponse response;
    TwitchApi::RestUtil::PopulateCodeStatusResponse(jsonVal.View(), response);

    ASSERT_EQ(response.statuses.size(), 2);
    ASSERT_EQ(response.statuses.find("KUHXV-4GXYP-AKAKK")->second, "UNUSED");
    ASSERT_EQ(response.statuses.find("XZDDZ-5SIQR-RT5M3")->second, "ALREADY_CLAIMED");
}


TEST_F(TwitchApiTest, TestDropsEntitlementsResponse_FT)
{
    Aws::String rawJson = R"(
        {
            "pagination": {
                "cursor": "abcdef"
            },
            "data": [
                {
                    "id": "fb78259e-fb81-4d1b-8333-34a06ffc24c0",
                    "benefit_id": "74c52265-e214-48a6-91b9-23b6014e8041",
                    "timestamp": "2019-01-28T04:17:53.325Z",
                    "user_id": "25009227",
                    "game_id": "33214"
                },
                {
                    "id": "862750a5-265e-4ab6-9f0a-c64df3d54dd0",
                    "benefit_id": "74c52265-e214-48a6-91b9-23b6014e8041",
                    "timestamp": "2019-01-28T04:16:53.325Z",
                    "user_id": "25009227",
                    "game_id": "33214"
                },
                {
                    "id": "d8879baa-3966-4d10-8856-15fdd62cce02",
                    "benefit_id": "cdfdc5c3-65a2-43bc-8767-fde06eb4ab2c",
                    "timestamp": "2019-01-28T04:15:53.325Z",
                    "user_id": "25009227",
                    "game_id": "33214"
                }
            ]
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::DropsEntitlementsResponse response;
    TwitchApi::RestUtil::PopulateDropsEntitlementsResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 3);
    ASSERT_EQ(response.m_data.at(0).id, "fb78259e-fb81-4d1b-8333-34a06ffc24c0");
    ASSERT_EQ(response.m_data.at(1).timestamp, "2019-01-28T04:16:53.325Z");
    ASSERT_EQ(response.m_data.at(2).game_id, "33214");
    ASSERT_EQ(response.m_pagination, "abcdef");
}

TEST_F(TwitchApiTest, TestGetTopGamesResponse_FT)
{
    Aws::String rawJson = R"(
        {
          "data": [
            {
              "id": "493057",
              "name": "TEST GAME",
              "box_art_url": "https://my.twitch.test/123"
            }
          ],
          "pagination":{"cursor":"eyJiIjpudWxsLCJhIjp7Ik9mZnNldCI6MjB9fQ=="}
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::GetTopGamesResponse response;
    TwitchApi::RestUtil::PopulateGetTopGamesResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 1);
    ASSERT_EQ(response.m_data.front().id, "493057");
    ASSERT_EQ(response.m_pagination, "eyJiIjpudWxsLCJhIjp7Ik9mZnNldCI6MjB9fQ==");
}

TEST_F(TwitchApiTest, TestGetGamesResponse_FT)
{
    Aws::String rawJson = R"(
        {
          "data": [
            {
              "id": "493057",
              "name": "TEST GAME",
              "box_art_url": "https://my.twitch.test/456"
            }
          ]
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::GetGamesResponse response;
    TwitchApi::RestUtil::PopulateGetGamesResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 1);
    ASSERT_EQ(response.m_data.front().id, "493057");
}

TEST_F(TwitchApiTest, TestHypeTrainEventsResponse_FT)
{
    Aws::String rawJson = R"(
        {
            "data": [
            {
                "id": "1b0AsbInCHZW2SQFQkCzqN07Ib2",
                "event_type": "hypetrain.progression",
                "event_timestamp": "2020-04-24T20:07:24Z",
                "version": "1.0",
                "event_data": {
                    "broadcaster_id": "270954519",
                    "cooldown_end_time": "2020-04-24T20:13:21.003802269Z",
                    "expires_at": "2020-04-24T20:12:21.003802269Z",
                    "goal": 1800,
                    "id": "70f0c7d8-ff60-4c50-b138-f3a352833b50",
                    "last_contribution": {
                        "total": 200,
                        "type": "BITS",
                        "user": "134247454"
                    },
                    "level": 2,
                    "started_at": "2020-04-24T20:05:47.30473127Z",
                    "top_contributions": [
                        {
                            "total": 600,
                            "type": "BITS",
                            "user": "134247450"
                        }
                    ],
                    "total": 600
                }
            }
            ],
            "pagination": {
                "cursor": "eyJiIjpudWxsLCJh"
            }
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::HypeTrainEventsResponse response;
    TwitchApi::RestUtil::PopulateHypeTrainEventsResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 1);
    ASSERT_EQ(response.m_data.front().id, "1b0AsbInCHZW2SQFQkCzqN07Ib2");
    ASSERT_EQ(response.m_data.front().version, "1.0");
    ASSERT_EQ(response.m_data.front().event_data.goal, 1800);
    ASSERT_EQ(response.m_data.front().event_data.last_contribution.user, "134247454");
    ASSERT_EQ(response.m_data.front().event_data.top_contributions.front().total, 600);
    ASSERT_EQ(response.m_pagination, "eyJiIjpudWxsLCJh");
}

TEST_F(TwitchApiTest, TestCheckAutoModResponse_FT)
{
    Aws::String rawJson = R"(
        {
          "data": [
            {
              "msg_id": "123",
              "is_permitted": true
            },
            {
              "msg_id": "393",
              "is_permitted": false
            }
          ]
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::CheckAutoModResponse response;
    TwitchApi::RestUtil::PopulateCheckAutoModResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 2);
    ASSERT_EQ(response.m_data.front().msg_id, "123");
    ASSERT_NE(response.m_data.front().is_permitted, response.m_data.back().is_permitted);
}

TEST_F(TwitchApiTest, TestGetBannedEventsResponse_FT)
{
    Aws::String rawJson = R"(
        {
          "data": [
            {
              "id": "1IPFqAb0p0JncbPSTEPhx8JF1Sa",
              "event_type": "moderation.user.ban",
              "event_timestamp": "2019-03-13T15:55:14Z",
              "version": "1.0",
              "event_data": {
                "broadcaster_id": "198704263",
                "broadcaster_name": "aan22209",
                "user_id": "424596340",
                "user_name": "quotrok",
                "expires_at": ""
              }
            },
            {
              "id": "1IPFsDv5cs4mxfJ1s2O9Q5flf4Y",
              "event_type": "moderation.user.unban",
              "event_timestamp": "2019-03-13T15:55:30Z",
              "version": "1.0",
              "event_data": {
                "broadcaster_id": "198704263",
                "broadcaster_name": "aan22209",
                "user_id": "424596340",
                "user_name": "quotrok",
                "expires_at": ""
              }
            },
            {
              "id": "1IPFqmlu9W2q4mXXjULyM8zX0rb",
              "event_type": "moderation.user.ban",
              "event_timestamp": "2019-03-13T15:55:19Z",
              "version": "1.0",
              "event_data": {
                "broadcaster_id": "198704263",
                "broadcaster_name": "aan22209",
                "user_id": "424596340",
                "user_name": "quotrok",
                "expires_at": ""
              }
            }
          ],
          "pagination": {
            "cursor": "eyJiIjpudWxsLCJhIjp7IkN1cnNvciI6IjE5OTYwNDI2MzoyMDIxMjA1MzE6MUlQRnFtbHU5VzJxNG1YWGpVTHlNOHpYMHJiIn"
          }
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::GetBannedEventsResponse response;
    TwitchApi::RestUtil::PopulateGetBannedEventsResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 3);
    ASSERT_EQ(response.m_data.front().id, "1IPFqAb0p0JncbPSTEPhx8JF1Sa");
    ASSERT_EQ(response.m_data.back().broadcaster_name, "aan22209");
    ASSERT_EQ(response.m_pagination, "eyJiIjpudWxsLCJhIjp7IkN1cnNvciI6IjE5OTYwNDI2MzoyMDIxMjA1MzE6MUlQRnFtbHU5VzJxNG1YWGpVTHlNOHpYMHJiIn");
}

TEST_F(TwitchApiTest, TestGetBannedUsersResponse_FT)
{
    Aws::String rawJson = R"(
        {
          "data": [
            {
              "user_id": "423374343",
              "user_name": "glowillig",
              "expires_at": "2019-03-15T02:00:28Z"
            },
            {
              "user_id": "424596340",
              "user_name": "quotrok",
              "expires_at": "2018-08-07T02:07:55Z"
            }
          ],
          "pagination": {
            "cursor": "eyJiIjpudWxsLCJhIjp7IkN1cnNvciI6IjEwMDQ3MzA2NDo4NjQwNjU3MToxSVZCVDFKMnY5M1BTOXh3d1E0dUdXMkJOMFcifX"
          }
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::GetBannedUsersResponse response;
    TwitchApi::RestUtil::PopulateGetBannedUsersResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 2);
    ASSERT_EQ(response.m_data.front().user_name, "glowillig");
    ASSERT_EQ(response.m_pagination, "eyJiIjpudWxsLCJhIjp7IkN1cnNvciI6IjEwMDQ3MzA2NDo4NjQwNjU3MToxSVZCVDFKMnY5M1BTOXh3d1E0dUdXMkJOMFcifX");
}

TEST_F(TwitchApiTest, TestGetModeratorsResponse_FT)
{
    Aws::String rawJson = R"(
        {
          "data": [
            {
              "user_id": "424596340",
              "user_name": "quotrok"
            },
            {
              "user_id": "424596341",
              "user_name": "quotrak"
            },
            {
              "user_id": "424596342",
              "user_name": "quotrik"
            },
            {
              "user_id": "424596343",
              "user_name": "quotruk"
            }
          ],
          "pagination": {
            "cursor": "eyJiIjpudWxsLCJhIjp7IkN1cnNvciI6IjEwMDQ3MzA2NDo4NjQwNjU3MToxSVZCVDFKMnY5M1BTOXh3d1E0dUdXMkJOMFcif"
          }
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::GetModeratorsResponse response;
    TwitchApi::RestUtil::PopulateGetModeratorsResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 4);
    ASSERT_EQ(response.m_data.front().user_name, "quotrok");
    ASSERT_EQ(response.m_pagination, "eyJiIjpudWxsLCJhIjp7IkN1cnNvciI6IjEwMDQ3MzA2NDo4NjQwNjU3MToxSVZCVDFKMnY5M1BTOXh3d1E0dUdXMkJOMFcif");
}

TEST_F(TwitchApiTest, TestGetModeratorEventsResponse_FT)
{
    Aws::String rawJson = R"(
        {
          "data": [
            {
              "id": "1IVBTnDSUDApiBQW4UBcVTK4hPr",
              "event_type": "moderation.moderator.remove",
              "event_timestamp": "2019-03-15T18:18:14Z",
              "version": "1.0",
              "event_data": {
                "broadcaster_id": "198704263",
                "broadcaster_name": "aan22209",
                "user_id": "423374343",
                "user_name": "glowillig"
              }
            },
            {
              "id": "1IVIPQdYIEnD8nJ376qkASDzsj7",
              "event_type": "moderation.moderator.add",
              "event_timestamp": "2019-03-15T19:15:13Z",
              "version": "1.0",
              "event_data": {
                "broadcaster_id": "198704263",
                "broadcaster_name": "aan22209",
                "user_id": "423374343",
                "user_name": "glowillig"
              }
            },
            {
              "id": "1IVBTP7gG61oXLMu7fvnRhrpsro",
              "event_type": "moderation.moderator.remove",
              "event_timestamp": "2019-03-15T18:18:11Z",
              "version": "1.0",
              "event_data": {
                "broadcaster_id": "198704263",
                "broadcaster_name": "aan22209",
                "user_id": "424596340",
                "user_name": "quotrok"
              }
            }
          ],
          "pagination": {
            "cursor": "eyJiIjpudWxsLCJhIjp7IkN1"
          }
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::GetModeratorEventsResponse response;
    TwitchApi::RestUtil::PopulateGetModeratorEventsResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 3);
    ASSERT_EQ(response.m_data.front().event_type, "moderation.moderator.remove");
    ASSERT_EQ(response.m_data.back().broadcaster_id, "198704263");
    ASSERT_EQ(response.m_pagination, "eyJiIjpudWxsLCJhIjp7IkN1");
}

TEST_F(TwitchApiTest, TestSearchCategoriesResponse_FT)
{
    Aws::String rawJson = R"(
        {
            "data": [
                {
                    "id": "33214",
                    "name": "Fortnite",
                    "box_art_url": "https://my.twitch.test/123"
                }
            ],
            "pagination": {
                "cursor": "eyJiIjpudWxsLCJhIjp7IkN"
            }
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::SearchCategoriesResponse response;
    TwitchApi::RestUtil::PopulateSearchCategoriesResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 1);
    ASSERT_EQ(response.m_data.front().id, "33214");
    ASSERT_EQ(response.m_data.front().box_art_url, "https://my.twitch.test/123");
    ASSERT_EQ(response.m_pagination, "eyJiIjpudWxsLCJhIjp7IkN");
}

TEST_F(TwitchApiTest, TestSearchChannelsResponse_FT)
{
    Aws::String rawJson = R"(
        {
            "data": [
                {
                    "broadcaster_language": "en",
                    "display_name": "loserfruit",
                    "game_id": "498000",
                    "id": "41245072",
                    "is_live": false,
                    "tag_ids": [
                        "6ea6bca4-4712-4ab9-a906-e3336a9d8039"
                    ],
                    "thumbnail_url": "https://my.twitch.test/123",
                    "title": "loserfruit",
                    "started_at": ""
                }
            ],
            "pagination": {
                "cursor": "Mg=="
            }
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::SearchChannelsResponse response;
    TwitchApi::RestUtil::PopulateSearchChannelsResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 1);
    ASSERT_EQ(response.m_data.front().broadcaster_language, "en");
    ASSERT_EQ(response.m_data.front().is_live, false);
    ASSERT_EQ(response.m_data.front().tag_ids.front(), "6ea6bca4-4712-4ab9-a906-e3336a9d8039");
    ASSERT_EQ(response.m_data.front().started_at, "");
    ASSERT_EQ(response.m_pagination, "Mg==");
}

TEST_F(TwitchApiTest, TestStreamKeyResponse_FT)
{
    Aws::String rawJson = R"(
        {
            "data": [
                {
                    "stream_key": "live_44322889_a34ub37c8ajv98a0"
                }
            ]
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::StreamKeyResponse response;
    TwitchApi::RestUtil::PopulateStreamKeyResponse(jsonVal.View(), response);

    ASSERT_EQ(response.stream_key, "live_44322889_a34ub37c8ajv98a0");
}

TEST_F(TwitchApiTest, TestGetStreamsResponse_FT)
{
    Aws::String rawJson = R"(
        {
          "data": [
            {
              "id": "26007494656",
              "user_id": "23161357",
              "user_name": "LIRIK",
              "game_id": "417752",
              "type": "live",
              "title": "Hey Guys, It's Monday - Twitter: @Lirik",
              "viewer_count": 32575,
              "started_at": "2017-08-14T16:08:32Z",
              "language": "en",
              "thumbnail_url": "https://my.twitch.test/123",
              "tag_ids":  [
                  "6ea6bca4-4712-4ab9-a906-e3336a9d8039"
              ]
            }
          ],
          "pagination": {
            "cursor": "eyJiIjpudWxsLCJhIjp7Ik9mZnNldCI6MjB9fQ=="
          }
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::GetStreamsResponse response;
    TwitchApi::RestUtil::PopulateGetStreamsResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 1);
    ASSERT_EQ(response.m_data.front().tag_ids.size(), 1);
    ASSERT_EQ(response.m_data.front().tag_ids.front(), "6ea6bca4-4712-4ab9-a906-e3336a9d8039");
    ASSERT_EQ(response.m_data.front().viewer_count, 32575);
    ASSERT_EQ(response.m_pagination, "eyJiIjpudWxsLCJhIjp7Ik9mZnNldCI6MjB9fQ==");
}

TEST_F(TwitchApiTest, TestCreateStreamMarkerResponse_FT)
{
    Aws::String rawJson = R"(
        {
          "data": [
             {
                "id": "123",
                "created_at": "2018-08-20T20:10:03Z",
                "description": "hello, this is a marker!",
                "position_seconds": 244
             }
          ]
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::CreateStreamMarkerResponse response;
    TwitchApi::RestUtil::PopulateCreateStreamMarkerResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 1);
    ASSERT_EQ(response.m_data.front().id, "123");
    ASSERT_EQ(response.m_data.front().description, "hello, this is a marker!");
}

TEST_F(TwitchApiTest, TestGetStreamMarkersResponse_FT)
{
    Aws::String rawJson = R"(
        {
          "data": [
            {
              "user_id": "123",
              "user_name": "Display Name",
              "videos": [
                {
                  "video_id": "456",
                  "markers": [
                    {
                      "id": "106b8d6243a4f883d25ad75e6cdffdc4",
                      "created_at": "2018-08-20T20:10:03Z",
                      "description": "hello, this is a marker!",
                      "position_seconds": 244,
                      "URL": "https://my.twitch.test/123"
                    }
                  ]
                }
              ]
            }
          ],
          "pagination": {
            "cursor": "eyJiIjpudWxsLCJhIjoiMjk1MjA0Mzk3OjI1Mzpib29rbWFyazoxMDZiOGQ1Y"
          }
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::GetStreamMarkersResponse response;
    TwitchApi::RestUtil::PopulateGetStreamMarkersResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 1);
    ASSERT_EQ(response.m_data.front().user_id, "123");
    ASSERT_EQ(response.m_data.front().videos.front().video_id, "456");
    ASSERT_EQ(response.m_data.front().videos.front().markers.front().position_seconds, 244);
    ASSERT_EQ(response.m_pagination, "eyJiIjpudWxsLCJhIjoiMjk1MjA0Mzk3OjI1Mzpib29rbWFyazoxMDZiOGQ1Y");
}

TEST_F(TwitchApiTest, TestGetBroadcasterSubscriptionsResponse_FT)
{
    Aws::String rawJson = R"(
        {
          "data": [
            {
              "broadcaster_id": "123",
              "broadcaster_name": "test_user",
              "is_gift": true,
              "tier": "1000",
              "plan_name": "The Ninjas",
              "user_id": "123",
              "user_name": "snoirf"
            }
          ],
          "pagination": {
            "cursor": "xxxx"
          }
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::GetBroadcasterSubscriptionsResponse response;
    TwitchApi::RestUtil::PopulateGetBroadcasterSubscriptionsResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 1);
    ASSERT_EQ(response.m_data.front().broadcaster_id, "123");
    ASSERT_EQ(response.m_data.front().plan_name, "The Ninjas");
    ASSERT_EQ(response.m_pagination, "xxxx");
}

TEST_F(TwitchApiTest, TestGetAllStreamTagsResponse_FT)
{
    Aws::String rawJson = R"(
        {
          "data": [
            {
              "tag_id": "621fb5bf-5498-4d8f-b4ac-db4d40d401bf",
              "is_auto": false,
              "localization_names": {
                "bg-bg": "Завършване без продължаване",
                "cs-cz": "Na jeden z&aacute;tah", "da-dk": "1 Continue klaret",
                "de-de": "Mit nur 1 Leben",
                "el-gr": "1 χωρίς συνέχεια",
                "en-us": "1 Credit Clear"
              },
              "localization_descriptions": {
                "bg-bg": "За потоци с акцент върху завършване на аркадна игра с монети, в която не се използва продължаване",
                "cs-cz": "Pro vys&iacute;l&aacute;n&iacute; s důrazem na plněn&iacute; her bez použit&iacute; pokračov&aacute;n&iacute;.",
                "da-dk": "Til streams med v&aelig;gt p&aring; at gennemf&oslash;re et arkadespil uden at bruge continues",
                "de-de": "F&uuml;r Streams mit dem Ziel, ein Coin-op-Arcade-Game mit nur einem Leben abzuschlie&szlig;en.",
                "el-gr": "Για μεταδόσεις με έμφαση στην ολοκλήρωση παιχνιδιών που λειτουργούν με κέρμα, χωρίς να χρησιμοποιούν συνέχειες",
                "en-us": "For streams with an emphasis on completing a coin-op arcade game without using any continues"
              }
            },
            {
              "tag_id": "7b49f69a-5d95-4c94-b7e3-66e2c0c6f6c6",
              "is_auto": false,
              "localization_names": {
                "bg-bg": "Дизайн",
                "cs-cz": "Design",
                "da-dk": "Design",
                "de-de": "Design",
                "el-gr": "Σχέδιο",
                "en-us": "Design"
              },
              "localization_descriptions": {
                "en-us": "For streams with an emphasis on the creative process of designing an object or system"
              }
            },
            {
              "tag_id": "1c628b75-b1c3-4a2f-9d1d-056c1f555f0e",
              "is_auto": true,
              "localization_names": {
                "bg-bg": "Шампион: Lux",
                "cs-cz": "Šampion: Lux",
                "da-dk": "Champion: Lux"
              },
              "localization_descriptions": {
              "en-us": "For streams featuring the champion Lux in League of Legends"
            }
           }
          ],
          "pagination": {
            "cursor": "U1HVWlMQ0pUVXlJNmJuVnNiSDE5In19"
          }
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::GetAllStreamTagsResponse response;
    TwitchApi::RestUtil::PopulateGetAllStreamTagsResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 3);
    ASSERT_FALSE(response.m_data.front().is_auto);
    ASSERT_EQ(response.m_data.front().localization_names["en-us"], "1 Credit Clear");
    ASSERT_EQ(response.m_pagination, "U1HVWlMQ0pUVXlJNmJuVnNiSDE5In19");
}

TEST_F(TwitchApiTest, TestGetStreamTagsResponse_FT)
{
    Aws::String rawJson = R"(
        {
            "data":
            [
                {
                   "tag_id": "621fb5bf-5498-4d8f-b4ac-db4d40d401bf",
                   "is_auto": false,
                   "localization_names": {
                        "bg-bg": "Завършване без продължаване",
                        "cs-cz": "Na jeden z&aacute;tah",
                        "da-dk": "1 Continue klaret"
                    },
                    "localization_descriptions": {
                        "bg-bg": "За потоци с акцент върху завършване на аркадна игра с монети, в която не се използва продължаване",
                        "cs-cz": "Pro mincov&yacute;ch ark&aacute;dov&yacute;ch her bez použit&iacute; pokračov&aacute;n&iacute;.",
                        "da-dk": "Til streams med et arkadespil uden at bruge continues"
                    }
                },
                {
                    "tag_id": "79977fb9-f106-4a87-a386-f1b0f99783dd",
                    "is_auto": false,
                    "localization_names": {
                        "bg-bg": "PvE",
                        "cs-cz": "PvE"
                    },
                    "localization_descriptions":
                    {
                         "bg-bg": "За потоци с акцент върху PVE геймплей",
                         "cs-cz": "Pro vys&iacute;l&aacute;n&iacute; s důrazem na hratelnost \"hr&aacute;č vs. prostřed&iacute;\".",
                         "da-dk": "Til streams med v&aelig;gt p&aring; spil, hvor det er spilleren mod omgivelserne."
                    }
                }
            ]
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::GetStreamTagsResponse response;
    TwitchApi::RestUtil::PopulateGetStreamTagsResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 2);
    ASSERT_EQ(response.m_data.front().tag_id, "621fb5bf-5498-4d8f-b4ac-db4d40d401bf");
    ASSERT_EQ(response.m_data.front().localization_descriptions["da-dk"], "Til streams med et arkadespil uden at bruge continues");
}

TEST_F(TwitchApiTest, TestGetUsersResponse_FT)
{
    Aws::String rawJson = R"(
        {
            "data":
            [
                {
                    "id": "44322889",
                    "login": "dallas",
                    "display_name": "dallas",
                    "type": "staff",
                    "broadcaster_type": "",
                    "description": "Just a gamer playing games and chatting",
                    "profile_image_url": "https://my.twitch.test/123",
                    "offline_image_url" : "https://my.twitch.test/456",
                    "view_count" : 191836881,
                    "email" : "login@provider.com"
                }
            ]
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::GetUsersResponse response;
    TwitchApi::RestUtil::PopulateGetUsersResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 1);
    ASSERT_EQ(response.m_data.front().login, "dallas");
    ASSERT_EQ(response.m_data.front().type, "staff");
    ASSERT_EQ(response.m_data.front().view_count, 191836881);
}

TEST_F(TwitchApiTest, TestGetUsersFollowsResponse_FT)
{
    Aws::String rawJson = R"(
        {
           "total": 12345,
           "data":
           [
              {
                 "from_id": "171003792",
                 "from_name": "IIIsutha067III",
                 "to_id": "23161357",
                 "to_name": "LIRIK",
                 "followed_at": "2017-08-22T22:55:24Z"
              },
              {
                 "from_id": "113627897",
                 "from_name": "Birdman616",
                 "to_id": "23161357",
                 "to_name": "LIRIK",
                 "followed_at": "2017-08-22T22:55:04Z"
              }
           ],
           "pagination":{
             "cursor": "eyJiIjpudWxsLCJhIjoiMTUwMzQ0MTc3NjQyNDQyMjAwMCJ9"
           }
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::GetUsersFollowsResponse response;
    TwitchApi::RestUtil::PopulateGetUsersFollowsResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 2);
    ASSERT_EQ(response.m_data.front().from_id, "171003792");
    ASSERT_EQ(response.m_data.front().to_name, "LIRIK");
    ASSERT_EQ(response.m_pagination, "eyJiIjpudWxsLCJhIjoiMTUwMzQ0MTc3NjQyNDQyMjAwMCJ9");
}

TEST_F(TwitchApiTest, TestUserBlockListResponse_FT)
{
    Aws::String rawJson = R"(
        {
            "data": [
                {
                    "id": "135093069",
                    "user_login": "bluelava",
                    "display_name": "BlueLava"
                },
                {
                    "id": "27419011",
                    "user_login": "travistyoj",
                    "display_name": "TravistyOJ"
                }
            ]
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::UserBlockListResponse response;
    TwitchApi::RestUtil::PopulateUserBlockListResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 2);
    ASSERT_EQ(response.m_data.front().id, "135093069");
    ASSERT_EQ(response.m_data.at(1).display_name, "TravistyOJ");
}

TEST_F(TwitchApiTest, TestGetUserExtensionsResponse_FT)
{
    Aws::String rawJson = R"(
        {
          "data": [
            {
              "id": "wi08ebtatdc7oj83wtl9uxwz807l8b",
              "version": "1.1.8",
              "name": "Streamlabs Leaderboard",
              "can_activate": true,
              "type": [
                "panel"
              ]
            },
            {
              "id": "d4uvtfdr04uq6raoenvj7m86gdk16v",
              "version": "2.0.2",
              "name": "Prime Subscription and Loot Reminder",
              "can_activate": true,
              "type": [
                "overlay"
              ]
            },
            {
              "id": "rh6jq1q334hqc2rr1qlzqbvwlfl3x0",
               "version": "1.1.0",
              "name": "TopClip",
              "can_activate": true,
              "type": [
                "mobile",
                "panel"
              ]
            },
            {
              "id": "zfh2irvx2jb4s60f02jq0ajm8vwgka",
              "version": "1.0.19",
              "name": "Streamlabs",
              "can_activate": true,
              "type": [
                "mobile",
                "overlay"
              ]
            },
            {
              "id": "lqnf3zxk0rv0g7gq92mtmnirjz2cjj",
              "version": "0.0.1",
              "name": "Dev Experience Test",
              "can_activate": true,
              "type": [
                "component",
                "mobile",
                "panel",
                "overlay"
              ]
            }
          ]
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::GetUserExtensionsResponse response;
    TwitchApi::RestUtil::PopulateGetUserExtensionsResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 5);
    ASSERT_EQ(response.m_data.front().type.size(), 1);
    ASSERT_EQ(response.m_data.back().type.size(), 4);
    ASSERT_EQ(response.m_data.back().type.front(), "component");
    ASSERT_EQ(response.m_data.back().id, "lqnf3zxk0rv0g7gq92mtmnirjz2cjj");
}

TEST_F(TwitchApiTest, TestGetUserActiveExtensionsResponse_FT)
{
    Aws::String rawJson = R"(
        {
          "data": {
            "panel": {
              "1": {
                "active": true,
                "id": "rh6jq1q334hqc2rr1qlzqbvwlfl3x0",
                "version": "1.1.0",
                "name": "TopClip"
              },
              "2": {
                "active": true,
                "id": "wi08ebtatdc7oj83wtl9uxwz807l8b",
                "version": "1.1.8",
                "name": "Streamlabs Leaderboard"
              },
              "3": {
                "active": true,
                "id": "naty2zwfp7vecaivuve8ef1hohh6bo",
                "version": "1.0.9",
                "name": "Streamlabs Stream Schedule & Countdown"
              }
            },
            "overlay": {
              "1": {
                "active": true,
                "id": "zfh2irvx2jb4s60f02jq0ajm8vwgka",
                "version": "1.0.19",
                "name": "Streamlabs"
              }
            },
            "component": {
              "1": {
                "active": true,
                "id": "lqnf3zxk0rv0g7gq92mtmnirjz2cjj",
                "version": "0.0.1",
                "name": "Dev Experience Test",
                "x": 0,
                "y": 0
              },
              "2": {
                "active": false
              }
            }
          }
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::GetUserActiveExtensionsResponse response;
    TwitchApi::RestUtil::PopulateGetUserActiveExtensionsResponse(jsonVal.View(), response);

    ASSERT_EQ(response.panel["1"].id, "rh6jq1q334hqc2rr1qlzqbvwlfl3x0");
    ASSERT_FALSE(response.component["2"].active);
}

TEST_F(TwitchApiTest, TestGetVideosResponse_FT)
{
    Aws::String rawJson = R"(
        {
            "data":
            [
                {
                    "id": "234482848",
                    "user_id": "67955580",
                    "user_name": "ChewieMelodies",
                    "title": "-",
                    "description": "",
                    "created_at": "2018-03-02T20:53:41Z",
                    "published_at": "2018-03-02T20:53:41Z",
                    "url": "https://my.twitch.test/123",
                    "thumbnail_url": "https://my.twitch.test/456",
                    "viewable": "public",
                    "view_count": 142,
                    "language": "en",
                    "type": "archive",
                    "duration": "3h8m33s"
                }
            ],
            "pagination":{"cursor":"eyJiIjpudWxsLCJhIjoiMTUwMzQ0MTc3NjQyNDQyMjAwMCJ9"}
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::GetVideosResponse response;
    TwitchApi::RestUtil::PopulateGetVideosResponse(jsonVal.View(), response);

    ASSERT_EQ(response.m_data.size(), 1);
    ASSERT_EQ(response.m_data.front().id, "234482848");
    ASSERT_EQ(response.m_data.front().view_count, 142);
    ASSERT_EQ(response.m_pagination, "eyJiIjpudWxsLCJhIjoiMTUwMzQ0MTc3NjQyNDQyMjAwMCJ9");
}

TEST_F(TwitchApiTest, TestDeleteVideosResponse_FT)
{
    Aws::String rawJson = R"(
        {
            "data": [
                "1234",
                "9876"
            ]
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::DeleteVideosResponse response;
    TwitchApi::RestUtil::PopulateDeleteVideosResponse(jsonVal.View(), response);

    ASSERT_EQ(response.video_keys.size(), 2);
    ASSERT_EQ(response.video_keys.front(), "1234");
}

TEST_F(TwitchApiTest, TestGetWebhookSubscriptionsResponse_FT)
{
    Aws::String rawJson = R"(
        {
           "total": 12,
           "data": [
               {
                   "topic": "https://my.twitch.test/123",
                   "callback": "https://my.twitch.test/123/callback",
                   "expires_at": "2018-07-30T20:00:00Z"
               },
               {
                   "topic": "https://my.twitch.test/456",
                   "callback": "https://my.twitch.test/456/callback",
                   "expires_at": "2018-07-30T20:03:00Z"
               }
           ],
           "pagination": {
               "cursor": "eyJiIjpudWxsLCJhIjp7IkN1cnNvciI6IkFYc2laU0k2TVN3aWFTSTZNWDAifX0"
           }
        }
    )";
    Aws::Utils::Json::JsonValue jsonVal = Aws::Utils::Json::JsonValue(rawJson);
    TwitchApi::GetWebhookSubscriptionsResponse response;
    TwitchApi::RestUtil::PopulateGetWebhookSubscriptionsResponse(jsonVal.View(), response);

    ASSERT_EQ(response.total, 12);
    ASSERT_EQ(response.data.size(), 2);
    ASSERT_EQ(response.data.front().topic, "https://my.twitch.test/123");
    ASSERT_EQ(response.m_pagination, "eyJiIjpudWxsLCJhIjp7IkN1cnNvciI6IkFYc2laU0k2TVN3aWFTSTZNWDAifX0");
}

AZ_UNIT_TEST_HOOK();
