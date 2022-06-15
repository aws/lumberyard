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

#include <TwitchApiSystemComponent.h>
#include <TwitchApiReflection.h>
#include <TwitchApiRestUtil.h>

#include <aws/core/utils/Array.h>
#include <aws/core/utils/memory/stl/AWSString.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/std/string/conversions.h>

namespace TwitchApi
{
    void TwitchApiSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TwitchApiSystemComponent, AZ::Component>()
                ->Version(0);

            Internal::ReflectSerialization(*serialize);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<TwitchApiSystemComponent>("Twitch Api", "Provides access to the Twitch Api API")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            Internal::ReflectBehaviors(*behaviorContext);
        }
    }

    void TwitchApiSystemComponent::SetApplicationId(const AZStd::string& twitchApplicationId)
    {
        bool success = false;

        if (m_applicationId.empty())
        {
            if (IsValidTwitchAppId(twitchApplicationId))
            {
                m_applicationId = twitchApplicationId;
                success = true;
            }
            else
            {
                AZ_Warning("TwitchApiSystemComponent::SetApplicationID", false, "Invalid Twitch Application ID! Request ignored!");
            }
        }
        else
        {
            AZ_Warning("TwitchApiSystemComponent::SetApplicationID", false, "Twitch Application ID is already set! Request ignored!");
        }
    }

    void TwitchApiSystemComponent::SetUserId(const AZStd::string& userId)
    {
        AZStd::string rUserId = "";
        if (IsValidFriendId(userId))
        {
            m_cachedClientId = rUserId = userId;
        }

        TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::UserIdNotify, rUserId);
    }

    void TwitchApiSystemComponent::SetUserAccessToken(const AZStd::string& token)
    {
        AZStd::string rToken = "";
        if (IsValidOAuthToken(token))
        {
            m_cachedUserAccessToken = rToken = token;
        }

        TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::UserAccessTokenNotify, rToken);
    }

    void TwitchApiSystemComponent::SetAppAccessToken(const AZStd::string& token)
    {
        AZStd::string rToken = "";
        if (IsValidOAuthToken(token))
        {
            m_cachedAppAccessToken = rToken = token;
        }

        TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::AppAccessTokenNotify, rToken);
    }

    AZStd::string_view TwitchApiSystemComponent::GetApplicationId() const
    {
        return m_applicationId;
    }

    AZStd::string_view TwitchApiSystemComponent::GetUserId() const
    {
        return m_cachedClientId;
    }

    AZStd::string_view TwitchApiSystemComponent::GetUserAccessToken() const
    {
        return m_cachedUserAccessToken;
    }

    AZStd::string_view TwitchApiSystemComponent::GetAppAccessToken() const
    {
        return m_cachedAppAccessToken;
    }

    //--- Ads ---//
    void TwitchApiSystemComponent::StartCommercial(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZ::u64 length)
    {
        AZStd::string url = "channels/commercial";

        Aws::Utils::Json::JsonValue jsonBody;
        jsonBody.WithString("broadcaster_id", broadcaster_id.c_str());
        jsonBody.WithInt64("length", length);
        Aws::String body(jsonBody.View().WriteCompact());

        m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_POST, m_restUtil->GetUserBearerContentHeaders(), body.c_str(),
            [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRestError);
            CommercialResponse commercialResponse;

            if (httpCode == Aws::Http::HttpResponseCode::OK)
            {
                rc = ResultCode::Success;
                TwitchApi::RestUtil::PopulateCommercialResponse(jsonDoc, commercialResponse);
            }
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnStartCommercial,
                CommercialResult(commercialResponse, receipt, rc));
        });
    }

    //--- Analytics ---//
    void TwitchApiSystemComponent::GetExtensionAnalytics(ReceiptId& receipt, const AZStd::string& started_at, const AZStd::string& ended_at,
        const AZStd::string& extension_id, const AZStd::string& after, const AZStd::string& type, AZ::u8 max_results)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetAppAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetExtensionAnalytics,
                ExtensionAnalyticsResult(ExtensionAnalyticsResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "analytics/extensions";
            AddStringParam(url, "started_at", started_at);
            AddStringParam(url, "ended_at", ended_at);
            AddStringParam(url, "extension_id", extension_id);
            AddStringParam(url, "after", after);
            AddStringParam(url, "type", type);
            AddIntParam(url, "first", max_results);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                ExtensionAnalyticsResponse analyticsResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateExtensionAnalyticsResponse(jsonDoc, analyticsResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetExtensionAnalytics,
                    ExtensionAnalyticsResult(analyticsResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::GetGameAnalytics(ReceiptId& receipt, const AZStd::string& started_at, const AZStd::string& ended_at,
        const AZStd::string& game_id, const AZStd::string& after, const AZStd::string& type, AZ::u8 max_results)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetGameAnalytics,
                GameAnalyticsResult(GameAnalyticsResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "analytics/games";
            AddStringParam(url, "started_at", started_at);
            AddStringParam(url, "ended_at", ended_at);
            AddStringParam(url, "game_id", game_id);
            AddStringParam(url, "after", after);
            AddStringParam(url, "type", type);
            AddIntParam(url, "first", max_results);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                GameAnalyticsResponse analyticsResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateGameAnalyticsResponse(jsonDoc, analyticsResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetGameAnalytics,
                    GameAnalyticsResult(analyticsResponse, receipt, rc));
            });
        }
    }

    //--- Bits ---//
    void TwitchApiSystemComponent::GetBitsLeaderboard(ReceiptId& receipt, const AZStd::string& started_at,
        const AZStd::string& period, const AZStd::string& user_id, AZ::u8 max_results)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetBitsLeaderboard,
                BitsLeaderboardResult(BitsLeaderboardResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "bits/leaderboard";
            AddStringParam(url, "started_at", started_at);
            AddStringParam(url, "period", period);
            AddStringParam(url, "user_id", user_id);
            AddStringParam(url, "count", AZStd::to_string(max_results));

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                BitsLeaderboardResponse bitsResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateBitsLeaderboardResponse(jsonDoc, bitsResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetBitsLeaderboard,
                    BitsLeaderboardResult(bitsResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::GetCheermotes(ReceiptId& receipt, const AZStd::string& broadcaster_id)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetAppAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetCheermotes,
                CheermotesResult(CheermotesResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "bits/cheermotes";
            AddStringParam(url, "broadcaster_id", broadcaster_id);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                CheermotesResponse bitsResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateCheermotesResponse(jsonDoc, bitsResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetCheermotes,
                    CheermotesResult(bitsResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::GetExtensionTransactions(ReceiptId& receipt, const AZStd::string& extension_id,
        const AZStd::string& transaction_id, const AZStd::string& after, AZ::u8 max_results)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetExtensionTransactions,
                ExtensionTransactionsResult(ExtensionTransactionsResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "extensions/transactions";
            AddStringParam(url, "extension_id", extension_id);
            AddStringParam(url, "transaction_id", transaction_id);
            AddStringParam(url, "after", after);
            AddIntParam(url, "first", max_results);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetAppBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                ExtensionTransactionsResponse xactResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateExtensionTransactionsResponse(jsonDoc, xactResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetExtensionTransactions,
                    ExtensionTransactionsResult(xactResponse, receipt, rc));
            });
        }
    }

    //--- Channels ---//
    void TwitchApiSystemComponent::GetChannelInformation(ReceiptId& receipt, const AZStd::string& broadcaster_id)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetAppAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetChannelInformation,
                ChannelInformationResult(ChannelInformationResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "channels";
            AddStringParam(url, "broadcaster_id", broadcaster_id);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                ChannelInformationResponse channelResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateChannelInformationResponse(jsonDoc, channelResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetChannelInformation,
                    ChannelInformationResult(channelResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::ModifyChannelInformation(ReceiptId& receipt, const AZStd::string& broadcaster_id,
        const AZStd::string& game_id, const AZStd::string& broadcaster_language, const AZStd::string& title)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetAppAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnModifyChannelInformation, receipt, rc);
        }
        else
        {
            AZStd::string url = "channels";
            AddStringParam(url, "broadcaster_id", broadcaster_id);

            Aws::Utils::Json::JsonValue jsonBody;
            if (!game_id.empty())
            {
                jsonBody.WithString("game_id", game_id.c_str());
            }
            if (!broadcaster_language.empty())
            {
                jsonBody.WithString("broadcaster_language", broadcaster_language.c_str());
            }
            if (!title.empty())
            {
                jsonBody.WithString("title", title.c_str());
            }
            Aws::String body(jsonBody.View().WriteCompact());
            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_PATCH, m_restUtil->GetUserBearerContentHeaders(), body.c_str(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);

                if (httpCode == Aws::Http::HttpResponseCode::NO_CONTENT)
                {
                    rc = ResultCode::Success;
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnModifyChannelInformation, receipt, rc);
            });
        }
    }

    void TwitchApiSystemComponent::GetChannelEditors(ReceiptId& receipt, const AZStd::string& broadcaster_id)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetAppAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetChannelEditors,
                ChannelEditorsResult(ChannelEditorsResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "channels/editors";
            AddStringParam(url, "broadcaster_id", broadcaster_id);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                ChannelEditorsResponse channelResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateChannelEditorsResponse(jsonDoc, channelResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetChannelEditors,
                    ChannelEditorsResult(channelResponse, receipt, rc));
            });
        }
    }

    Aws::String PopulateCustomRewardsRequestBody(const CustomRewardSettings& rewardSettings)
    {
        Aws::Utils::Json::JsonValue jsonBody;

        jsonBody.WithString("title", rewardSettings.m_title.c_str());
        jsonBody.WithString("prompt", rewardSettings.m_prompt.c_str());
        jsonBody.WithInt64("cost", rewardSettings.m_cost);
        jsonBody.WithBool("is_enabled", rewardSettings.m_is_enabled);
        if (!rewardSettings.m_background_color.empty())
        {
            jsonBody.WithString("background_color", rewardSettings.m_background_color.c_str());
        }
        jsonBody.WithBool("is_user_input_required", rewardSettings.m_is_user_input_required);
        jsonBody.WithBool("is_max_per_stream_enabled", rewardSettings.m_is_max_per_stream_enabled);
        if (rewardSettings.m_is_max_per_stream_enabled)
        {
            jsonBody.WithInt64("max_per_stream", rewardSettings.m_max_per_stream);
        }
        jsonBody.WithBool("is_max_per_user_per_stream_enabled", rewardSettings.m_is_max_per_user_per_stream_enabled);
        if (rewardSettings.m_is_max_per_user_per_stream_enabled)
        {
            jsonBody.WithInt64("max_per_user_per_stream", rewardSettings.m_max_per_user_per_stream);
        }
        jsonBody.WithBool("is_global_cooldown_enabled", rewardSettings.m_is_global_cooldown_enabled);
        if (rewardSettings.m_is_global_cooldown_enabled)
        {
            jsonBody.WithInt64("global_cooldown_seconds", rewardSettings.m_global_cooldown_seconds);
        }
        jsonBody.WithBool("should_redemptions_skip_request_queue", rewardSettings.m_should_redemptions_skip_request_queue);

        return Aws::String(jsonBody.View().WriteCompact());
    }

    //--- Channel Points ---//
    void TwitchApiSystemComponent::CreateCustomRewards(ReceiptId& receipt, const AZStd::string& broadcaster_id,
        const CustomRewardSettings& rewardSettings)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnCreateCustomRewards,
                CustomRewardsResult(CustomRewardsResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "channel_points/custom_rewards";
            AddStringParam(url, "broadcaster_id", broadcaster_id);

            Aws::String body = PopulateCustomRewardsRequestBody(rewardSettings);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_POST, m_restUtil->GetUserBearerHeader(), body.c_str(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                CustomRewardsResponse rewardsResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateCustomRewardsResponse(jsonDoc, rewardsResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnCreateCustomRewards,
                    CustomRewardsResult(rewardsResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::DeleteCustomReward(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::string& id)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnDeleteCustomReward, receipt, rc);
        }
        else
        {
            AZStd::string url = "channel_points/custom_rewards";
            AddStringParam(url, "broadcaster_id", broadcaster_id);
            AddStringParam(url, "id", id);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_DELETE, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                CustomRewardsResponse rewardsResponse;

                if (httpCode == Aws::Http::HttpResponseCode::NO_CONTENT)
                {
                    rc = ResultCode::Success;
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnDeleteCustomReward, receipt, rc);
            });
        }
    }

    void TwitchApiSystemComponent::GetCustomReward(ReceiptId& receipt, const AZStd::string& broadcaster_id,
        const AZStd::string& id, const bool only_manageable_rewards)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetCustomReward,
                CustomRewardsResult(CustomRewardsResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "channel_points/custom_rewards";
            AddStringParam(url, "broadcaster_id", broadcaster_id);
            AddStringParam(url, "id", id);
            AddBoolParam(url, "only_manageable_rewards", only_manageable_rewards);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                CustomRewardsResponse rewardsResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateCustomRewardsResponse(jsonDoc, rewardsResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetCustomReward,
                    CustomRewardsResult(rewardsResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::GetCustomRewardRedemption(ReceiptId& receipt, const AZStd::string& broadcaster_id,
        const AZStd::string& reward_id, const AZStd::string& id, const AZStd::string& status, const AZStd::string& sort,
        const AZStd::string& after, AZ::u8 max_results)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetCustomRewardRedemption,
                CustomRewardsRedemptionResult(CustomRewardsRedemptionResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "channel_points/custom_rewards/redemptions";
            AddStringParam(url, "broadcaster_id", broadcaster_id);
            AddStringParam(url, "reward_id", reward_id);
            AddStringParam(url, "id", id);
            AddStringParam(url, "status", status);
            AddStringParam(url, "sort", sort);
            AddStringParam(url, "after", after);
            AddIntParam(url, "first", max_results);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                CustomRewardsRedemptionResponse rewardsResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateCustomRewardsRedemptionResponse(jsonDoc, rewardsResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetCustomRewardRedemption,
                    CustomRewardsRedemptionResult(rewardsResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::UpdateCustomReward(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::string& id,
        const CustomRewardSettings& rewardSettings)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnUpdateCustomReward,
                CustomRewardsResult(CustomRewardsResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "channel_points/custom_rewards";
            AddStringParam(url, "broadcaster_id", broadcaster_id);
            AddStringParam(url, "id", id);

            Aws::String body = PopulateCustomRewardsRequestBody(rewardSettings);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_PATCH, m_restUtil->GetUserBearerContentHeaders(), body.c_str(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                CustomRewardsResponse rewardsResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateCustomRewardsResponse(jsonDoc, rewardsResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnUpdateCustomReward,
                    CustomRewardsResult(rewardsResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::UpdateRedemptionStatus(ReceiptId& receipt, const AZStd::string& id, const AZStd::string& broadcaster_id,
        const AZStd::string& reward_id, const AZStd::string& status)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnUpdateRedemptionStatus,
                CustomRewardsRedemptionResult(CustomRewardsRedemptionResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "channel_points/custom_rewards/redemptions";
            AddStringParam(url, "id", id);
            AddStringParam(url, "broadcaster_id", broadcaster_id);
            AddStringParam(url, "reward_id", reward_id);

            Aws::Utils::Json::JsonValue jsonBody;
            jsonBody.WithString("status", status.c_str());
            Aws::String body(jsonBody.View().WriteCompact());

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_PATCH, m_restUtil->GetUserBearerContentHeaders(), body.c_str(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                CustomRewardsRedemptionResponse rewardsResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateCustomRewardsRedemptionResponse(jsonDoc, rewardsResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnUpdateRedemptionStatus,
                    CustomRewardsRedemptionResult(rewardsResponse, receipt, rc));
            });
        }
    }

    //--- Clips ---//
    void TwitchApiSystemComponent::CreateClip(ReceiptId& receipt, const AZStd::string& broadcaster_id, const bool has_delay)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnCreateClip,
                CreateClipResult(CreateClipResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "clips";
            AddStringParam(url, "broadcaster_id", broadcaster_id);
            AddStringParam(url, "has_delay", has_delay ? "true": "false");

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_POST, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                CreateClipResponse clipResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateCreateClipResponse(jsonDoc, clipResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnCreateClip, CreateClipResult(clipResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::GetClips(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::string& game_id,
        const AZStd::string& clip_id, const AZStd::string& started_at, const AZStd::string& ended_at, const AZStd::string& before,
        const AZStd::string& after, AZ::u8 max_results)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetClips, GetClipsResult(GetClipsResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "clips";
            AddStringParam(url, "broadcaster_id", broadcaster_id);
            AddStringParam(url, "game_id", game_id);
            AddStringParam(url, "id", clip_id);
            AddStringParam(url, "started_at", started_at);
            AddStringParam(url, "ended_at", ended_at);
            AddStringParam(url, "before", before);
            AddStringParam(url, "after", after);
            AddIntParam(url, "first", max_results);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                GetClipsResponse clipResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateGetClipsResponse(jsonDoc, clipResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetClips, GetClipsResult(clipResponse, receipt, rc));
            });
        }
    }

    //--- Entitlements ---//
    void TwitchApiSystemComponent::CreateEntitlementGrantsUploadURL(ReceiptId& receipt, const AZStd::string& manifest_id,
        const AZStd::string& entitlement_type)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetAppAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnCreateEntitlementGrantsUploadURL,
                EntitlementsGrantUploadResult(EntitlementsGrantUploadResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "entitlements/upload";
            AddStringParam(url, "manifest_id", manifest_id);
            AddStringParam(url, "entitlement_type", entitlement_type);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_POST, m_restUtil->GetAppBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                EntitlementsGrantUploadResponse entitlementResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateEntitlementGrantsUploadURLResponse(jsonDoc, entitlementResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnCreateEntitlementGrantsUploadURL,
                    EntitlementsGrantUploadResult(entitlementResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::GetCodeStatus(ReceiptId& receipt, const AZStd::vector<AZStd::string>& codes, const AZ::u64 user_id)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetAppAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetCodeStatus,
                CodeStatusResult(CodeStatusResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "entitlements/codes";
            for (const AZStd::string& code : codes)
            {
                AddStringParam(url, "code", code);
            }
            AddIntParam(url, "user_id", user_id);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetAppBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                CodeStatusResponse codeResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateCodeStatusResponse(jsonDoc, codeResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetCodeStatus,
                    CodeStatusResult(codeResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::GetDropsEntitlements(ReceiptId& receipt, const AZStd::string& id, const AZStd::string& user_id,
        const AZStd::string& game_id, const AZStd::string& after, AZ::u8 max_results)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetAppAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetDropsEntitlements,
                DropsEntitlementsResult(DropsEntitlementsResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "entitlements/drops";
            AddStringParam(url, "id", id);
            AddStringParam(url, "user_id", user_id);
            AddStringParam(url, "game_id", game_id);
            AddStringParam(url, "after", after);
            AddIntParam(url, "first", max_results);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetAppBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                DropsEntitlementsResponse dropResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateDropsEntitlementsResponse(jsonDoc, dropResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetDropsEntitlements,
                    DropsEntitlementsResult(dropResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::RedeemCode(ReceiptId& receipt, const AZStd::vector<AZStd::string>& codes, const AZ::u64 user_id)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetAppAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnRedeemCode,
                CodeStatusResult(CodeStatusResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "entitlements/code";
            for (const AZStd::string& code : codes)
            {
                AddStringParam(url, "code", code);
            }
            AddIntParam(url, "user_id", user_id);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_POST, m_restUtil->GetAppBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                CodeStatusResponse codeResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateCodeStatusResponse(jsonDoc, codeResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnRedeemCode, CodeStatusResult(codeResponse, receipt, rc));
            });
        }
    }

    //--- Games ---//
    void TwitchApiSystemComponent::GetTopGames(ReceiptId& receipt, const AZStd::string& before, const AZStd::string& after, AZ::u8 max_results)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetTopGames,
                GetTopGamesResult(GetTopGamesResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "games/top";
            AddStringParam(url, "before", before);
            AddStringParam(url, "after", after);
            AddIntParam(url, "first", max_results);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                GetTopGamesResponse gamesResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateGetTopGamesResponse(jsonDoc, gamesResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetTopGames,
                    GetTopGamesResult(gamesResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::GetGames(ReceiptId& receipt, const AZStd::string& game_id, const AZStd::string& game_name)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetGames, GetGamesResult(GetGamesResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "games";
            AddStringParam(url, "id", game_id);
            AddStringParam(url, "name", game_name);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                GetGamesResponse gamesResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateGetGamesResponse(jsonDoc, gamesResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetGames, GetGamesResult(gamesResponse, receipt, rc));
            });
        }
    }

    //--- Hype Train ---//
    void TwitchApiSystemComponent::GetHypeTrainEvents(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::string& id,
        const AZStd::string& cursor, AZ::u8 max_results)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetAppAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetHypeTrainEvents,
                HypeTrainEventsResult(HypeTrainEventsResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "hypetrain/events";
            AddStringParam(url, "broadcaster_id", broadcaster_id);
            AddStringParam(url, "id", id);
            AddStringParam(url, "cursor", cursor);
            AddIntParam(url, "first", max_results);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                HypeTrainEventsResponse hypeResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateHypeTrainEventsResponse(jsonDoc, hypeResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetHypeTrainEvents,
                    HypeTrainEventsResult(hypeResponse, receipt, rc));
            });
        }
    }

    //--- Moderation ---//
    void TwitchApiSystemComponent::CheckAutoModStatus(ReceiptId& receipt, const AZStd::string& broadcaster_id,
        const AZStd::vector<CheckAutoModInputDatum>& filterTargets)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnCheckAutoModStatus,
                CheckAutoModResult(CheckAutoModResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "moderation/enforcements/status";
            AddStringParam(url, "broadcaster_id", broadcaster_id);

            // Construct JSON body from component data
            Aws::Utils::Json::JsonValue jsonBody;

            Aws::Utils::Array<Aws::Utils::Json::JsonValue> dataArray(filterTargets.size());
            AZ::u64 datumIdx = 0;
            for (const CheckAutoModInputDatum& datum : filterTargets)
            {
                Aws::Utils::Json::JsonValue datumBody;
                datumBody.WithString("msg_id", datum.msg_id.c_str());
                datumBody.WithString("msg_text", datum.msg_text.c_str());
                datumBody.WithString("user_id", datum.user_id.c_str());
                dataArray[datumIdx++] = datumBody;
            }
            jsonBody.WithArray("data", dataArray);

            Aws::String body(jsonBody.View().WriteCompact());

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_POST, m_restUtil->GetUserBearerContentHeaders(), body.c_str(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                CheckAutoModResponse modResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;

                    // Populate response object from JSON
                    TwitchApi::RestUtil::PopulateCheckAutoModResponse(jsonDoc, modResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnCheckAutoModStatus,
                    CheckAutoModResult(modResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::GetBannedEvents(ReceiptId& receipt, const AZStd::string& broadcaster_id,
        const AZStd::vector<AZStd::string>& user_ids, const AZStd::string& after, AZ::u8 max_results)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetBannedEvents,
                GetBannedEventsResult(GetBannedEventsResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "moderation/banned/events";
            AddStringParam(url, "broadcaster_id", broadcaster_id);
            for (const AZStd::string& user_id : user_ids)
            {
                AddStringParam(url, "user_id", user_id);
            }
            AddStringParam(url, "after", after);
            AddIntParam(url, "first", max_results);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                GetBannedEventsResponse bannedResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;

                    // Populate response object from JSON
                    TwitchApi::RestUtil::PopulateGetBannedEventsResponse(jsonDoc, bannedResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetBannedEvents,
                    GetBannedEventsResult(bannedResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::GetBannedUsers(ReceiptId& receipt, const AZStd::string& broadcaster_id,
        const AZStd::vector<AZStd::string>& user_ids, const AZStd::string& before, const AZStd::string& after)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetBannedUsers,
                GetBannedUsersResult(GetBannedUsersResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "moderation/banned";
            AddStringParam(url, "broadcaster_id", broadcaster_id);
            for (const AZStd::string& user_id : user_ids)
            {
                AddStringParam(url, "user_id", user_id);
            }
            AddStringParam(url, "before", before);
            AddStringParam(url, "after", after);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                GetBannedUsersResponse bannedResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateGetBannedUsersResponse(jsonDoc, bannedResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetBannedUsers,
                    GetBannedUsersResult(bannedResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::GetModerators(ReceiptId& receipt, const AZStd::string& broadcaster_id,
        const AZStd::vector<AZStd::string>& user_ids, const AZStd::string& after)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetModerators,
                GetModeratorsResult(GetModeratorsResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "moderation/moderators";
            AddStringParam(url, "broadcaster_id", broadcaster_id);
            for (const AZStd::string& user_id : user_ids)
            {
                AddStringParam(url, "user_id", user_id);
            }
            AddStringParam(url, "after", after);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                GetModeratorsResponse modResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateGetModeratorsResponse(jsonDoc, modResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetModerators,
                    GetModeratorsResult(modResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::GetModeratorEvents(ReceiptId& receipt, const AZStd::string& broadcaster_id,
        const AZStd::vector<AZStd::string>& user_ids)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetModeratorEvents,
                GetModeratorEventsResult(GetModeratorEventsResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "moderation/moderators/events";
            AddStringParam(url, "broadcaster_id", broadcaster_id);
            for (const AZStd::string& user_id : user_ids)
            {
                AddStringParam(url, "user_id", user_id);
            }

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                GetModeratorEventsResponse modResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateGetModeratorEventsResponse(jsonDoc, modResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetModeratorEvents,
                    GetModeratorEventsResult(modResponse, receipt, rc));
            });
        }
    }

    //--- Search ---//
    void TwitchApiSystemComponent::SearchCategories(ReceiptId& receipt, const AZStd::string& query, const AZStd::string& after,
        AZ::u8 max_results)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnSearchCategories, SearchCategoriesResult(SearchCategoriesResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "search/categories";
            AddStringParam(url, "query", query);
            AddStringParam(url, "after", after);
            AddIntParam(url, "first", max_results);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                SearchCategoriesResponse searchResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateSearchCategoriesResponse(jsonDoc, searchResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnSearchCategories,
                    SearchCategoriesResult(searchResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::SearchChannels(ReceiptId& receipt, const AZStd::string& query, const AZStd::string& after,
        const bool live_only, AZ::u8 max_results)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnSearchChannels,
                SearchChannelsResult(SearchChannelsResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "search/channels";
            AddStringParam(url, "query", query);
            AddBoolParam(url, "live_only", live_only);
            AddStringParam(url, "after", after);
            AddIntParam(url, "first", max_results);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                SearchChannelsResponse searchResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateSearchChannelsResponse(jsonDoc, searchResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnSearchChannels,
                    SearchChannelsResult(searchResponse, receipt, rc));
            });
        }
    }

    //--- Streams ---//
    void TwitchApiSystemComponent::GetStreamKey(ReceiptId& receipt, const AZStd::string& broadcaster_id)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetAppAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetStreamKey,
                StreamKeyResult(StreamKeyResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "streams/key";
            AddStringParam(url, "broadcaster_id", broadcaster_id);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                StreamKeyResponse keyResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateStreamKeyResponse(jsonDoc, keyResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetStreamKey, StreamKeyResult(keyResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::GetStreams(ReceiptId& receipt, const AZStd::vector<AZStd::string>& game_ids,
        const AZStd::vector<AZStd::string>& user_ids, const AZStd::vector<AZStd::string>& user_logins,
        const AZStd::vector<AZStd::string>& languages, const AZStd::string& before, const AZStd::string& after, AZ::u8 max_results)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetStreams,
                GetStreamsResult(GetStreamsResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "streams";
            for (const AZStd::string& game_id : game_ids)
            {
                AddStringParam(url, "game_id", game_id);
            }
            for (const AZStd::string& user_id : user_ids)
            {
                AddStringParam(url, "user_id", user_id);
            }
            for (const AZStd::string& user_login : user_logins)
            {
                AddStringParam(url, "user_login", user_login);
            }
            for (const AZStd::string& language : languages)
            {
                AddStringParam(url, "language", language);
            }
            AddStringParam(url, "before", before);
            AddStringParam(url, "after", after);
            AddIntParam(url, "first", max_results);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                GetStreamsResponse streamsResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateGetStreamsResponse(jsonDoc, streamsResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetStreams,
                    GetStreamsResult(streamsResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::CreateStreamMarker(ReceiptId& receipt, const AZStd::string& user_id, const AZStd::string& description)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnCreateStreamMarker,
                CreateStreamMarkerResult(CreateStreamMarkerResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "streams/markers";

            Aws::Utils::Json::JsonValue jsonBody;
            jsonBody.WithString("user_id", user_id.c_str());
            if (!description.empty())
            {
                jsonBody.WithString("description", description.c_str());
            }
            Aws::String body(jsonBody.View().WriteCompact());


            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_POST, m_restUtil->GetUserBearerContentHeaders(), body.c_str(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                CreateStreamMarkerResponse streamsResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateCreateStreamMarkerResponse(jsonDoc, streamsResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnCreateStreamMarker,
                    CreateStreamMarkerResult(streamsResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::GetStreamMarkers(ReceiptId& receipt, const AZStd::string& user_id, const AZStd::string& video_id,
        const AZStd::string& before, const AZStd::string& after, AZ::u8 max_results)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetStreamMarkers,
                GetStreamMarkersResult(GetStreamMarkersResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "streams/markers";
            AddStringParam(url, "user_id", user_id);
            AddStringParam(url, "video_id", video_id);
            AddStringParam(url, "before", before);
            AddStringParam(url, "after", after);
            AddIntParam(url, "first", max_results);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                GetStreamMarkersResponse streamsResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateGetStreamMarkersResponse(jsonDoc, streamsResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetStreamMarkers,
                    GetStreamMarkersResult(streamsResponse, receipt, rc));
            });
        }
    }

    //--- Subscriptions ---//
    void TwitchApiSystemComponent::GetBroadcasterSubscriptions(ReceiptId& receipt, const AZStd::string& broadcaster_id,
        const AZStd::vector<AZStd::string>& user_ids)
    {
          receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetBroadcasterSubscriptions,
                GetBroadcasterSubscriptionsResult(GetBroadcasterSubscriptionsResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "subscriptions";
            AddStringParam(url, "broadcaster_id", broadcaster_id);
            for (const AZStd::string& user_id : user_ids)
            {
                AddStringParam(url, "user_id", user_id);
            }

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                GetBroadcasterSubscriptionsResponse subResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateGetBroadcasterSubscriptionsResponse(jsonDoc, subResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetBroadcasterSubscriptions,
                    GetBroadcasterSubscriptionsResult(subResponse, receipt, rc));
            });
        }
    }

    //--- Tags ---//
    void TwitchApiSystemComponent::GetAllStreamTags(ReceiptId& receipt, const AZStd::vector<AZStd::string>& tag_ids,
        const AZStd::string& after, AZ::u8 max_results)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetAppAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetAllStreamTags,
                GetAllStreamTagsResult(GetAllStreamTagsResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "tags/streams";
            AddStringParam(url, "after", after);
            for (const AZStd::string& tag_id : tag_ids)
            {
                AddStringParam(url, "tag_id", tag_id);
            }
            AddIntParam(url, "first", max_results);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                GetAllStreamTagsResponse tagsResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateGetAllStreamTagsResponse(jsonDoc, tagsResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetAllStreamTags,
                    GetAllStreamTagsResult(tagsResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::GetStreamTags(ReceiptId& receipt, const AZStd::string& broadcaster_id)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetAppAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetStreamTags,
                GetStreamTagsResult(GetStreamTagsResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "streams/tags";
            AddStringParam(url, "broadcaster_id", broadcaster_id);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                GetStreamTagsResponse tagsResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateGetStreamTagsResponse(jsonDoc, tagsResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetStreamTags,
                    GetStreamTagsResult(tagsResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::ReplaceStreamTags(ReceiptId& receipt, const AZStd::string& broadcaster_id,
        const AZStd::vector<AZStd::string>& tag_ids)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetAppAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnReplaceStreamTags, receipt, rc);
        }
        else
        {
            AZStd::string url = "streams/tags";
            AddStringParam(url, "broadcaster_id", broadcaster_id);

            Aws::Utils::Json::JsonValue jsonBody;
            Aws::Utils::Array<Aws::String> jsonTagIds(tag_ids.size());
            {
                int idx = 0;
                for (const AZStd::string& tag_id : tag_ids)
                {
                    jsonTagIds[idx] = Aws::String(tag_id.c_str());
                }
                ++idx;
            }
            jsonBody.WithArray("tag_ids", jsonTagIds);
            Aws::String body(jsonBody.View().WriteCompact());

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_PUT, m_restUtil->GetUserBearerContentHeaders(), body.c_str(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);

                if (httpCode == Aws::Http::HttpResponseCode::NO_CONTENT)
                {
                    rc = ResultCode::Success;
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnReplaceStreamTags, receipt, rc);
            });
        }
    }

    //--- Users ---//
    void TwitchApiSystemComponent::GetUsers(ReceiptId& receipt, const AZStd::vector<AZStd::string>& user_ids,
        const AZStd::vector<AZStd::string>& user_logins)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetUsers, GetUsersResult(GetUsersResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "users";
            for (const AZStd::string& user_id : user_ids)
            {
                AddStringParam(url, "id", user_id);
            }
            for (const AZStd::string& user_login : user_logins)
            {
                AddStringParam(url, "login", user_login);
            }

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                GetUsersResponse usersResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateGetUsersResponse(jsonDoc, usersResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetUsers, GetUsersResult(usersResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::UpdateUser(ReceiptId& receipt, const AZStd::string& description)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnUpdateUser, GetUsersResult(GetUsersResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "users";
            AddStringParam(url, "description", description);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_PUT, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                GetUsersResponse usersResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateGetUsersResponse(jsonDoc, usersResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnUpdateUser, GetUsersResult(usersResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::GetUserFollows(ReceiptId& receipt, AZStd::string from_id, const AZStd::string& to_id,
        const AZStd::string& after, AZ::u8 max_results)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetUsersFollows,
                GetUsersFollowsResult(GetUsersFollowsResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "users/follows";
            AddStringParam(url, "from_id", from_id);
            AddStringParam(url, "to_id", to_id);
            AddIntParam(url, "first", max_results);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                GetUsersFollowsResponse usersResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateGetUsersFollowsResponse(jsonDoc, usersResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetUsersFollows,
                    GetUsersFollowsResult(usersResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::CreateUserFollows(ReceiptId& receipt, const AZStd::string& from_id, const AZStd::string& to_id,
        const bool allow_notifications)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnCreateUserFollows, receipt, rc);
        }
        else
        {
            AZStd::string url = "users/follows";
            AddStringParam(url, "from_id", from_id);
            AddStringParam(url, "to_id", to_id);
            AddBoolParam(url, "allow_notifications", allow_notifications);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_POST, m_restUtil->GetUserBearerContentHeaders(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);

                if (httpCode == Aws::Http::HttpResponseCode::NO_CONTENT)
                {
                    rc = ResultCode::Success;
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnCreateUserFollows, receipt, rc);
            });
        }
    }

    void TwitchApiSystemComponent::DeleteUserFollows(ReceiptId& receipt, const AZStd::string& from_id, const AZStd::string& to_id)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnDeleteUserFollows, receipt, rc);
        }
        else
        {
            AZStd::string url = "users/follows";
            AddStringParam(url, "from_id", from_id);
            AddStringParam(url, "to_id", to_id);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_DELETE, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);

                if (httpCode == Aws::Http::HttpResponseCode::NO_CONTENT)
                {
                    rc = ResultCode::Success;
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnDeleteUserFollows, receipt, rc);
            });
        }
    }

    void TwitchApiSystemComponent::GetUserBlockList(ReceiptId& receipt, const AZStd::string& broadcaster_id, const AZStd::string& after,
        AZ::u8 max_results)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetUserBlockList,
                UserBlockListResult(UserBlockListResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "users/blocks";
            AddStringParam(url, "broadcaster_id", broadcaster_id);
            AddStringParam(url, "after", after);
            AddIntParam(url, "first", max_results);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                UserBlockListResponse blockResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateUserBlockListResponse(jsonDoc, blockResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetUserBlockList,
                    UserBlockListResult(blockResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::BlockUser(ReceiptId& receipt, const AZStd::string& target_user_id, const AZStd::string& source_context,
        const AZStd::string& reason)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnBlockUser, receipt, rc);
        }
        else
        {
            AZStd::string url = "users/blocks";
            AddStringParam(url, "target_user_id", target_user_id);
            AddStringParam(url, "source_context", source_context);
            AddStringParam(url, "reason", reason);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_PUT, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);

                if (httpCode == Aws::Http::HttpResponseCode::NO_CONTENT)
                {
                    rc = ResultCode::Success;
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnBlockUser, receipt, rc);
            });
        }
    }

    void TwitchApiSystemComponent::UnblockUser(ReceiptId& receipt, const AZStd::string& target_user_id)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnUnblockUser, receipt, rc);
        }
        else
        {
            AZStd::string url = "users/blocks";
            AddStringParam(url, "target_user_id", target_user_id);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_DELETE, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);

                if (httpCode == Aws::Http::HttpResponseCode::NO_CONTENT)
                {
                    rc = ResultCode::Success;
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnUnblockUser, receipt, rc);
            });
        }
    }

    void TwitchApiSystemComponent::GetUserExtensions(ReceiptId& receipt)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetUserExtensions,
                GetUserExtensionsResult(GetUserExtensionsResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "users/extensions/list";

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                GetUserExtensionsResponse usersResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateGetUserExtensionsResponse(jsonDoc, usersResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetUserExtensions,
                    GetUserExtensionsResult(usersResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::GetUserActiveExtensions(ReceiptId& receipt, const AZStd::string& user_id)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetUserActiveExtensions,
                GetUserActiveExtensionsResult(GetUserActiveExtensionsResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "users/extensions";
            AddStringParam(url, "user_id", user_id);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                GetUserActiveExtensionsResponse usersResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateGetUserActiveExtensionsResponse(jsonDoc, usersResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetUserActiveExtensions,
                    GetUserActiveExtensionsResult(usersResponse, receipt, rc));
            });
        }
    }

    //! Return a JsonValue representation of the input data list
    Aws::Utils::Json::JsonValue GetUserExtensionJsonObject(const AZStd::vector<GetUserActiveExtensionsComponent>& components)
    {
        Aws::Utils::Json::JsonValue data;
        AZ::u64 idx = 1;
        for (const GetUserActiveExtensionsComponent& component : components)
        {
            Aws::Utils::Json::JsonValue datum;
            datum.WithBool("active", component.active);
            if (!component.id.empty())
            {
                datum.WithString("id", component.id.c_str());
            }
            if (!component.version.empty())
            {
                datum.WithString("version", component.version.c_str());
            }
            datum.WithInt64("x", component.x);
            datum.WithInt64("y", component.y);

            data.WithObject(AZStd::to_string(idx).c_str(), datum);
        }
        return data;
    }

    void TwitchApiSystemComponent::UpdateUserExtensions(ReceiptId& receipt, const AZStd::vector<GetUserActiveExtensionsComponent>& panel,
        const AZStd::vector<GetUserActiveExtensionsComponent>& overlay, const AZStd::vector<GetUserActiveExtensionsComponent>& component)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnUpdateUserExtensions,
                GetUserActiveExtensionsResult(GetUserActiveExtensionsResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "users/extensions";

            // Construct JSON body from component data
            Aws::Utils::Json::JsonValue jsonBody;
            Aws::Utils::Json::JsonValue dataBody;

            dataBody.WithObject("panel", GetUserExtensionJsonObject(panel));
            dataBody.WithObject("overlay", GetUserExtensionJsonObject(overlay));
            dataBody.WithObject("component", GetUserExtensionJsonObject(component));
            jsonBody.WithObject("data", dataBody);

            Aws::String body(jsonBody.View().WriteCompact());

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_PUT, m_restUtil->GetUserBearerContentHeaders(), body.c_str(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                GetUserActiveExtensionsResponse usersResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateGetUserActiveExtensionsResponse(jsonDoc, usersResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnUpdateUserExtensions,
                    GetUserActiveExtensionsResult(usersResponse, receipt, rc));
            });
        }
    }

    //--- Videos ---//
    void TwitchApiSystemComponent::GetVideos(ReceiptId& receipt, const AZStd::vector<AZStd::string>& video_ids,
        const AZStd::string& user_id, const AZStd::string& game_id, const AZStd::string& video_type, const AZStd::string& sort_order,
        const AZStd::string& period, const AZStd::string& language, const AZStd::string& before, const AZStd::string& after,
        AZ::u8 max_results)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetVideos, GetVideosResult(GetVideosResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "videos";
            for (const AZStd::string& video_id : video_ids)
            {
                AddStringParam(url, "id", video_id);
            }
            AddStringParam(url, "user_id", user_id);
            AddStringParam(url, "game_id", game_id);
            AddStringParam(url, "type", video_type);
            AddStringParam(url, "sort", sort_order);
            AddStringParam(url, "period", period);
            AddStringParam(url, "before", before);
            AddStringParam(url, "after", after);
            AddStringParam(url, "language", language);
            AddIntParam(url, "first", max_results);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                GetVideosResponse videosResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateGetVideosResponse(jsonDoc, videosResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetVideos, GetVideosResult(videosResponse, receipt, rc));
            });
        }
    }

    void TwitchApiSystemComponent::DeleteVideos(ReceiptId& receipt, const AZStd::vector<AZStd::string>& video_ids)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetUserAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnDeleteVideos,
                DeleteVideosResult(DeleteVideosResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "videos";
            for (const AZStd::string& video_id : video_ids)
            {
                AddStringParam(url, "id", video_id);
            }

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_DELETE, m_restUtil->GetUserBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                DeleteVideosResponse videosResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateDeleteVideosResponse(jsonDoc, videosResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnDeleteVideos,
                    DeleteVideosResult(videosResponse, receipt, rc));
            });
        }
    }

    //--- Webhooks ---//
    void TwitchApiSystemComponent::GetWebhookSubscriptions(ReceiptId& receipt, const AZStd::string& after, AZ::u8 max_results)
    {
        receipt.SetId(GetReceipt());
        ResultCode rc(GetAppAuthResultStatus());

        if (rc != ResultCode::Success)
        {
            TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetWebhookSubscriptions,
                GetWebhookSubscriptionsResult(GetWebhookSubscriptionsResponse(), receipt, rc));
        }
        else
        {
            AZStd::string url = "webhooks/subscriptions";
            AddStringParam(url, "after", after);
            AddIntParam(url, "first", max_results);

            m_restUtil->AddHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, m_restUtil->GetAppBearerHeader(),
                [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRestError);
                GetWebhookSubscriptionsResponse webhooksResponse;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;
                    TwitchApi::RestUtil::PopulateGetWebhookSubscriptionsResponse(jsonDoc, webhooksResponse);
                }
                TwitchApiNotifyBus::QueueBroadcast(&TwitchApiNotifyBus::Events::OnGetWebhookSubscriptions,
                    GetWebhookSubscriptionsResult(webhooksResponse, receipt, rc));
            });
        }
    }

    ResultCode TwitchApiSystemComponent::GetUserAuthResultStatus() const
    {
        if (m_cachedUserAccessToken.empty() || m_restUtil == nullptr)
        {
            return ResultCode::TwitchRestError;
        }

        return ResultCode::Success;
    }

    ResultCode TwitchApiSystemComponent::GetAppAuthResultStatus() const
    {
        if (m_cachedAppAccessToken.empty() || m_restUtil == nullptr)
        {
            return ResultCode::TwitchRestError;
        }

        return ResultCode::Success;
    }

    bool TwitchApiSystemComponent::IsValidString(AZStd::string_view str, AZStd::size_t minLength, AZStd::size_t maxLength) const
    {
        bool isValid = false;

        /*
        **   Per Twitch:
        **   The string is alpha-numeric + dashes (0-9, a-f, A-F, -).
        **   The minimum length is 1 and there is no maximum length currently.
        */

        if ((str.size() >= minLength) && (str.size() < maxLength))
        {
            AZStd::size_t found = str.find_first_not_of("0123456789abcdefABCDEF-");

            if (found == AZStd::string::npos)
            {
                isValid = true;
            }
        }

        return isValid;
    }

    bool TwitchApiSystemComponent::IsValidTwitchAppId(AZStd::string_view twitchAppId) const
    {
        static const AZStd::size_t kMinIdLength = 24;   // min id length
        static const AZStd::size_t kMaxIdLength = 64;   // max id length
        bool isValid = false;

        if ((twitchAppId.size() >= kMinIdLength) && (twitchAppId.size() < kMaxIdLength))
        {
            AZStd::size_t found = twitchAppId.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyz");

            if (found == AZStd::string::npos)
            {
                isValid = true;
            }
        }

        return isValid;
    }

    bool TwitchApiSystemComponent::IsValidFriendId(AZStd::string_view friendId) const
    {
        // The min id length should be 1
        // The max id length will be huge, since there is no official max length, this will allow for a large id.

        return IsValidString(friendId, 1, 256);
    }

    bool TwitchApiSystemComponent::IsValidOAuthToken(AZStd::string_view oAuthToken) const
    {
        // Twitch OAuth tokens are exactly length 30

        if (oAuthToken.size() == 30)
        {
            AZStd::size_t found = oAuthToken.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyz");

            if (found == AZStd::string::npos)
            {
                return true;
            }
        }

        return false;
    }

    bool TwitchApiSystemComponent::IsValidSyncToken(AZStd::string_view syncToken) const
    {
        // sync tokens are either empty of a opaque token of a min length of 8, but no longer than 64

        return syncToken.empty() ? true : IsValidString(syncToken, 8, 64);
    }

    void TwitchApiSystemComponent::AddStringParam(AZStd::string& URI, AZStd::string_view key, AZStd::string_view value) const
    {
        if (!value.empty() && !key.empty())
        {
            AZStd::string delim = (URI.find('?') != AZStd::string::npos) ? "&" : "?";
            URI = AZStd::string::format("%s%s%.*s=%.*s", URI.c_str(), delim.c_str(),
                aznumeric_cast<int>(key.size()), key.data(), aznumeric_cast<int>(value.size()), value.data());
        }
    }

    void TwitchApiSystemComponent::AddIntParam(AZStd::string& URI, AZStd::string_view key, const AZ::u64 value) const
    {
        AddStringParam(URI, key, AZStd::to_string(value));
    }

    void TwitchApiSystemComponent::AddBoolParam(AZStd::string& URI, AZStd::string_view key, const bool value) const
    {
        AddStringParam(URI, key, value ? "true" : "false");
    }

    void TwitchApiSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("TwitchApiService"));
    }

    void TwitchApiSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("TwitchApiService"));
    }

    void TwitchApiSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void TwitchApiSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void TwitchApiSystemComponent::Init()
    {
        m_restUtil = ITwitchApiRest::Alloc();
    }

    void TwitchApiSystemComponent::OnSystemTick()
    {
        if (m_restUtil != nullptr)
        {
            m_restUtil->FlushEvents();
        }
    }

    void TwitchApiSystemComponent::Activate()
    {
        TwitchApiRequestBus::Handler::BusConnect();
        AZ::SystemTickBus::Handler::BusConnect();
    }

    void TwitchApiSystemComponent::Deactivate()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();
        TwitchApiRequestBus::Handler::BusDisconnect();
    }

    AZ::u64 TwitchApiSystemComponent::GetReceipt()
    {
        return ++m_receiptCounter;
    }
}
