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

#include <TwitchApi/BaseTypes.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>

namespace TwitchApi
{
    /*
    * Compresses many parameters for creating and editing custom rewards into a single struct to pass to the bus
    */
    struct CustomRewardSettings
    {
        AZ_TYPE_INFO(CustomRewardSettings, "{C46B795B-B448-4EC3-BC56-7F78737BBB49}");

        CustomRewardSettings()
            : m_title("")
            , m_prompt("")
            , m_cost(0)
            , m_is_enabled(true)
            , m_background_color("#FFFFFF")
            , m_is_user_input_required(false)
            , m_is_max_per_stream_enabled(false)
            , m_max_per_stream(0)
            , m_is_max_per_user_per_stream_enabled(false)
            , m_max_per_user_per_stream(0)
            , m_is_global_cooldown_enabled(false)
            , m_global_cooldown_seconds(0)
            , m_should_redemptions_skip_request_queue(false)
        {}

        CustomRewardSettings(const AZStd::string& title, const AZStd::string& prompt, const AZ::u64 cost, const bool is_enabled,
            const AZStd::string& background_color, const bool is_user_input_required,const bool is_max_per_stream_enabled,
            const AZ::u64 max_per_stream, const bool is_max_per_user_per_stream_enabled, const AZ::u64 max_per_user_per_stream,
            const bool is_global_cooldown_enabled, const AZ::u64 global_cooldown_seconds, const bool should_redemptions_skip_request_queue)
            : m_title(title)
            , m_prompt(prompt)
            , m_cost(cost)
            , m_is_enabled(is_enabled)
            , m_background_color(background_color)
            , m_is_user_input_required(is_user_input_required)
            , m_is_max_per_stream_enabled(is_max_per_stream_enabled)
            , m_max_per_stream(max_per_stream)
            , m_is_max_per_user_per_stream_enabled(is_max_per_stream_enabled)
            , m_max_per_user_per_stream(max_per_user_per_stream)
            , m_is_global_cooldown_enabled(is_global_cooldown_enabled)
            , m_global_cooldown_seconds(global_cooldown_seconds)
            , m_should_redemptions_skip_request_queue(should_redemptions_skip_request_queue)
        {}

        bool m_is_enabled;
        bool m_is_user_input_required;
        bool m_is_max_per_stream_enabled;
        bool m_is_max_per_user_per_stream_enabled;
        bool m_is_global_cooldown_enabled;
        bool m_should_redemptions_skip_request_queue;
        AZ::u64 m_cost;
        AZ::u64 m_max_per_stream;
        AZ::u64 m_max_per_user_per_stream;
        AZ::u64 m_global_cooldown_seconds;
        AZStd::string m_title;
        AZStd::string m_prompt;
        AZStd::string m_background_color;
    };

    /*
    * Twitch Response API Objects, current documentation available at https://dev.twitch.tv/docs/api/reference
    * The following structs capture responses to Twitch API Requests as listed in the API reference documentation
    */

    struct CommercialDatum
    {
        AZ_TYPE_INFO(CommercialDatum, "{842B7F5F-4E49-4C93-9760-24A60749037E}");

        AZ::u64 length;
        AZ::u64 retry_after;
        AZStd::string message;
    };
    TwitchApi_CreateResponseTypeClass(CommercialDatum, CommercialResponse, "{B41CE070-EFE4-4EA1-818F-C8DC215A4662}");
    TwitchApi_CreateReturnTypeClass(CommercialResult, CommercialResponse, "{B6603759-EE0C-42E5-90D9-A82906F8206A}");

    struct ExtensionAnalyticsDatum
    {
        AZ_TYPE_INFO(ExtensionAnalyticsDatum, "{B523FBC4-717C-4FEF-A114-2A237A037832}");

        AZStd::string extension_id;
        AZStd::string type;
        AZStd::string started_at;
        AZStd::string ended_at;
        AZStd::string url;
    };
    TwitchApi_CreatePaginationResponseTypeClass(ExtensionAnalyticsDatum, ExtensionAnalyticsResponse,
        "{B27DA325-1EAF-4AB5-A75A-B46192F7BB73}");
    TwitchApi_CreateReturnTypeClass(ExtensionAnalyticsResult, ExtensionAnalyticsResponse, "{4F8B1F29-FCB8-45ED-9C2E-0541DF98AC31}");

    struct GameAnalyticsDatum
    {
        AZ_TYPE_INFO(GameAnalyticsDatum, "{D98BAF87-BB65-483A-B2DC-964795F5AEE4}");

        AZStd::string game_id;
        AZStd::string type;
        AZStd::string started_at;
        AZStd::string ended_at;
        AZStd::string url;
    };
    TwitchApi_CreatePaginationResponseTypeClass(GameAnalyticsDatum, GameAnalyticsResponse, "{2F0F9CE4-D8BA-420E-BCE8-5643C3735877}");
    TwitchApi_CreateReturnTypeClass(GameAnalyticsResult, GameAnalyticsResponse, "{80B70AED-6EC3-4DCB-A2FC-F1BA4D73AFFC}");

    struct BitsLeaderboardDatum
    {
        AZ_TYPE_INFO(BitsLeaderboardDatum, "{996849AD-E55A-4FC9-8694-28884376137B}");

        AZ::u64 rank;
        AZ::u64 score;
        AZStd::string user_id;
        AZStd::string user_name;
    };

    struct BitsLeaderboardResponse
    {
        AZ_TYPE_INFO(BitsLeaderboardResponse, "{4A47E5CB-23B8-4AC2-A830-03D1B34654D1}");

        AZ::u64 total;
        AZStd::string started_at;
        AZStd::string ended_at;
        AZStd::vector<BitsLeaderboardDatum> data;
    };
    TwitchApi_CreateReturnTypeClass(BitsLeaderboardResult, BitsLeaderboardResponse, "{CBDABA7D-9799-47CF-B357-C4EB9EE485D3}");

    struct CheermotesImageSetDatum
    {
        AZ_TYPE_INFO(CheermotesImageSetDatum, "{5795213B-1FA0-48AC-B011-8633F3E10898}");

        AZStd::string url_1x;
        AZStd::string url_15x;
        AZStd::string url_2x;
        AZStd::string url_3x;
        AZStd::string url_4x;
    };

    struct CheermotesImagesDatum
    {
        AZ_TYPE_INFO(CheermotesImagesDatum, "{CE66F782-B706-4C3C-95E7-453EE2A84401}");

        CheermotesImageSetDatum dark_animated;
        CheermotesImageSetDatum dark_static;
        CheermotesImageSetDatum light_animated;
        CheermotesImageSetDatum light_static;
    };

    struct CheermotesTiersDatum
    {
        AZ_TYPE_INFO(CheermotesTiersDatum, "{6FC9A694-3761-49D8-B34A-92A5C7A2A72B}");

        bool can_cheer;
        bool show_in_bits_card;
        AZ::u64 min_bits;
        AZStd::string id;
        AZStd::string color;
        CheermotesImagesDatum images;
    };

    struct CheermotesDatum
    {
        AZ_TYPE_INFO(CheermotesDatum, "{1EA58FE6-54B6-4148-A9A9-E9FA1FCBFB4B}");

        bool is_charitable;
        AZ::u64 order;
        AZStd::string prefix;
        AZStd::string type;
        AZStd::string last_updated;
        AZStd::vector<CheermotesTiersDatum> tiers;
    };
    TwitchApi_CreatePaginationResponseTypeClass(CheermotesDatum, CheermotesResponse, "{56534FCE-EBA9-44D0-8CB7-3EE806735E8D}");
    TwitchApi_CreateReturnTypeClass(CheermotesResult, CheermotesResponse, "{18CF62A5-D29A-4E49-AD36-505BC405BE60}");

    struct ExtensionTransactionProductDatum
    {
        AZ_TYPE_INFO(ExtensionTransactionProductDatum, "{F0C94CE1-0109-48CB-86E7-0D5CE91D82A7}");

        bool broadcast;
        bool inDevelopment;
        AZ::u64 cost_amount;
        AZStd::string cost_type;
        AZStd::string domain;
        AZStd::string sku;
        AZStd::string displayName;
    };

    struct ExtensionTransactionDatum
    {
        AZ_TYPE_INFO(ExtensionTransactionDatum, "{FEEA1C35-EB33-47B2-8487-3BE7FED10371}");

        AZStd::string id;
        AZStd::string timestamp;
        AZStd::string broadcaster_id;
        AZStd::string broadcaster_name;
        AZStd::string user_id;
        AZStd::string user_name;
        AZStd::string product_type;
        ExtensionTransactionProductDatum product_data;
    };
    TwitchApi_CreatePaginationResponseTypeClass(ExtensionTransactionDatum, ExtensionTransactionsResponse,
        "{6B97F30E-62BF-4B8C-9D4B-999C8083AE7F}");
    TwitchApi_CreateReturnTypeClass(ExtensionTransactionsResult, ExtensionTransactionsResponse, "{E3E034C0-0604-4B36-8432-13CAB70C198E}");

    struct ChannelInformationDatum
    {
        AZ_TYPE_INFO(ChannelInformationDatum, "{470F317A-BE40-47C8-879E-3CFCAB2E1F1E}");

        AZStd::string broadcaster_id;
        AZStd::string broadcaster_name;
        AZStd::string broadcaster_language;
        AZStd::string game_id;
        AZStd::string game_name;
        AZStd::string title;
    };
    TwitchApi_CreateResponseTypeClass(ChannelInformationDatum, ChannelInformationResponse, "{D9133308-B3F0-42A6-92E5-3D8B24A33D03}");
    TwitchApi_CreateReturnTypeClass(ChannelInformationResult, ChannelInformationResponse, "{53C1EE74-A96B-4FDC-A4C1-794F740E5E43}");

    struct ChannelEditorsDatum
    {
        AZ_TYPE_INFO(ChannelEditorsDatum, "{29A8D37C-3941-4954-A353-7C4A1FBA49B6}");

        AZStd::string user_id;
        AZStd::string user_name;
        AZStd::string created_at;
    };
    TwitchApi_CreateResponseTypeClass(ChannelEditorsDatum, ChannelEditorsResponse, "{D40D8152-B76E-4DEE-A5FD-15802D9DDA15}");
    TwitchApi_CreateReturnTypeClass(ChannelEditorsResult, ChannelEditorsResponse, "{D59130B0-04ED-4648-BC31-E998138BEB46}");

    struct CustomRewardsImageDatum
    {
        AZ_TYPE_INFO(CustomRewardsImageDatum, "{D50739C3-6971-4208-B59D-DDB52563E37D}");

        AZStd::string url_1x;
        AZStd::string url_2x;
        AZStd::string url_4x;
    };

    struct CustomRewardsDatum
    {
        AZ_TYPE_INFO(CustomRewardsDatum, "{184F4186-EEDC-4D8C-B438-BE70E596B5E5}");

        bool is_enabled;
        bool is_user_input_required;
        bool is_max_per_stream_enabled;
        bool is_max_per_user_per_stream_enabled;
        bool is_global_cooldown_enabled;
        bool is_paused;
        bool is_in_stock;
        bool should_redemptions_skip_request_queue;
        AZ::u64 cost;
        AZ::u64 max_per_stream;
        AZ::u64 max_per_user_per_stream;
        AZ::u64 global_cooldown_seconds;
        AZ::u64 redemptions_redeemed_current_stream;
        AZStd::string broadcaster_id;
        AZStd::string broadcaster_name;
        AZStd::string id;
        AZStd::string background_color;
        AZStd::string title;
        AZStd::string prompt;
        AZStd::string cooldown_expires_at;
        CustomRewardsImageDatum default_image;
        CustomRewardsImageDatum image;

    };
    TwitchApi_CreateResponseTypeClass(CustomRewardsDatum, CustomRewardsResponse, "{740549B1-7BD0-4910-88D0-80129A107746}");
    TwitchApi_CreateReturnTypeClass(CustomRewardsResult, CustomRewardsResponse, "{A8E29FC8-7F21-48CE-8483-8AB0B6494E5F}");

    struct CustomRewardsRedemptionRewardDatum
    {
        AZ_TYPE_INFO(CustomRewardsRedemptionRewardDatum, "{A1C0177A-0A0A-4882-AC67-E6A7A70FC6FF}");

        AZ::u64 cost;
        AZStd::string id;
        AZStd::string title;
        AZStd::string prompt;
    };

    struct CustomRewardsRedemptionDatum
    {
        AZ_TYPE_INFO(CustomRewardsRedemptionDatum, "{B70AF336-1D41-4AB7-9ECB-AF522CC90907}");

        AZStd::string broadcaster_name;
        AZStd::string broadcaster_id;
        AZStd::string id;
        AZStd::string user_id;
        AZStd::string user_name;
        AZStd::string user_input;
        AZStd::string status;
        AZStd::string redeemed_at;
        CustomRewardsRedemptionRewardDatum reward;

    };
    TwitchApi_CreatePaginationResponseTypeClass(CustomRewardsRedemptionDatum, CustomRewardsRedemptionResponse,
        "{B4A34FB4-A197-4050-A972-A9D4A9954105}");
    TwitchApi_CreateReturnTypeClass(CustomRewardsRedemptionResult, CustomRewardsRedemptionResponse,
        "{18C5CB8C-E226-4588-8276-904D7871EEA3}");

    struct CreateClipDatum
    {
        AZ_TYPE_INFO(CreateClipDatum, "{D9FFE699-CF93-44F8-B0EB-D3E252DE9B28}");

        AZStd::string id;
        AZStd::string edit_url;
    };
    TwitchApi_CreateResponseTypeClass(CreateClipDatum, CreateClipResponse, "{77E9EBD8-4334-4A35-B64B-A5D8DFB5D223}");
    TwitchApi_CreateReturnTypeClass(CreateClipResult, CreateClipResponse, "{1FDB2EF7-9D58-461C-BA4A-F9046AB2ED20}");

    struct GetClipsDatum
    {
        AZ_TYPE_INFO(GetClipsDatum, "{4993CFD7-3678-45FF-82EE-EE6975FB1876}");

        AZ::u64 view_count;
        AZStd::string broadcaster_id;
        AZStd::string broadcaster_name;
        AZStd::string created_at;
        AZStd::string creator_id;
        AZStd::string creator_name;
        AZStd::string embed_url;
        AZStd::string game_id;
        AZStd::string id;
        AZStd::string language;
        AZStd::string thumbnail_url;
        AZStd::string title;
        AZStd::string url;
        AZStd::string video_id;
    };
    TwitchApi_CreatePaginationResponseTypeClass(GetClipsDatum, GetClipsResponse, "{94F81FDA-9F58-4B6E-A0BC-DC7632758EE5}");
    TwitchApi_CreateReturnTypeClass(GetClipsResult, GetClipsResponse, "{125E182F-5EB5-4B84-A421-113D5A19E8B0}");

    struct EntitlementsGrantUploadResponse
    {
        AZ_TYPE_INFO(EntitlementsGrantUploadResponse, "{9FC592CF-49CE-4972-AE28-AF69D82D8A30}");

        AZStd::vector<AZStd::string> urls;
    };
    TwitchApi_CreateReturnTypeClass(EntitlementsGrantUploadResult, EntitlementsGrantUploadResponse,
        "{8F411D82-2E93-4DC2-84F5-852D65C76CE6}");

    struct CodeStatusResponse
    {
        AZ_TYPE_INFO(CodeStatusResponse, "{CEC595CD-FCB3-4125-994A-C60F2A42E824}");

        AZStd::unordered_map<AZStd::string, AZStd::string> statuses;
    };
    TwitchApi_CreateReturnTypeClass(CodeStatusResult, CodeStatusResponse, "{C481387D-6752-4402-A5DB-BBFE41EC4035}");

    struct DropsEntitlementsDatum
    {
        AZ_TYPE_INFO(DropsEntitlementsDatum, "{B6C07A51-52D8-4621-8386-33160D53376A}");

        AZStd::string id;
        AZStd::string benefit_id;
        AZStd::string timestamp;
        AZStd::string user_id;
        AZStd::string game_id;
    };
    TwitchApi_CreatePaginationResponseTypeClass(DropsEntitlementsDatum, DropsEntitlementsResponse,
        "{6726692E-C185-4609-9749-C37294F43B44}");
    TwitchApi_CreateReturnTypeClass(DropsEntitlementsResult, DropsEntitlementsResponse, "{E93F2E00-7FA8-4E8F-B330-4A75A60FB53C}");

    struct GetGamesDatum
    {
        AZ_TYPE_INFO(GetGamesDatum, "{58A30DBE-E027-48A0-ADFB-D4F4B555C756}");

        AZStd::string box_art_url;
        AZStd::string id;
        AZStd::string name;
    };
    TwitchApi_CreatePaginationResponseTypeClass(GetGamesDatum, GetTopGamesResponse, "{D28405AA-EFF6-461E-9F2D-E4C0119C7477}");
    TwitchApi_CreateReturnTypeClass(GetTopGamesResult, GetTopGamesResponse, "{F13E57FC-F068-4B16-BD80-5B1D27B398B0}");
    TwitchApi_CreateResponseTypeClass(GetGamesDatum, GetGamesResponse, "{5290464A-6F41-4BB3-8D51-30490BA0D5A6}");
    TwitchApi_CreateReturnTypeClass(GetGamesResult, GetGamesResponse, "{D8EBBCA2-D653-4B23-B1E6-D8DB9FF45373}");

    struct HypeTrainContributionDatum
    {
        AZ_TYPE_INFO(HypeTrainContributionDatum, "{1E872570-6189-4902-B115-AD623DD1E1B8}");

        AZ::u64 total;
        AZStd::string type;
        AZStd::string user;
    };

    struct HypeTrainEventDatum
    {
        AZ_TYPE_INFO(HypeTrainEventDatum, "{E29A994A-6809-4117-9861-E4D26F1E29CF}");

        AZ::u64 goal;
        AZ::u64 level;
        AZ::u64 total;
        AZStd::string broadcaster_id;
        AZStd::string cooldown_end_time;
        AZStd::string expires_at;
        AZStd::string id;
        AZStd::string started_at;
        HypeTrainContributionDatum last_contribution;
        AZStd::vector<HypeTrainContributionDatum> top_contributions;
    };

    struct HypeTrainEventsDatum
    {
        AZ_TYPE_INFO(HypeTrainEventsDatum, "{B7933459-FAC6-4E6F-BDE3-DF065D5ED249}");

        AZStd::string id;
        AZStd::string event_type;
        AZStd::string event_timestamp;
        AZStd::string version;
        HypeTrainEventDatum event_data;
    };
    TwitchApi_CreatePaginationResponseTypeClass(HypeTrainEventsDatum, HypeTrainEventsResponse,
        "{78B6510E-FCFC-4BF9-B5FE-CA11559FACA5}");
    TwitchApi_CreateReturnTypeClass(HypeTrainEventsResult, HypeTrainEventsResponse, "{51F7ADA6-EC9B-4C03-955B-DC1692E55DE2}");

    struct CheckAutoModInputDatum
    {
        AZ_TYPE_INFO(CheckAutoModInputDatum, "{B6258FA4-975E-4910-A5F4-192F9535C14A}");

        AZStd::string msg_id;
        AZStd::string msg_text;
        AZStd::string user_id;
    };

    struct CheckAutoModResponseDatum
    {
        AZ_TYPE_INFO(CheckAutoModResponseDatum, "{BCEDFB54-F8A2-4EAB-85E1-0E9EAECAE141}");

        bool is_permitted;
        AZStd::string msg_id;
    };
    TwitchApi_CreateResponseTypeClass(CheckAutoModResponseDatum, CheckAutoModResponse, "{D5407194-AFB5-4F46-9D91-8E780EAE7B15}");
    TwitchApi_CreateReturnTypeClass(CheckAutoModResult, CheckAutoModResponse, "{79DAAF5D-6424-4070-883D-D641A7B16FA3}");

    struct GetBannedEventsDatum
    {
        AZ_TYPE_INFO(GetBannedEventsDatum, "{E13F8875-D9F8-4850-A220-FBCA1905EC8D}");

        AZStd::string id;
        AZStd::string event_type;
        AZStd::string event_timestamp;
        AZStd::string version;
        AZStd::string broadcaster_id;
        AZStd::string broadcaster_name;
        AZStd::string expires_at;
        AZStd::string user_id;
        AZStd::string user_name;
    };
    TwitchApi_CreatePaginationResponseTypeClass(GetBannedEventsDatum, GetBannedEventsResponse, "{07CAEBE0-8268-40D6-B75A-51012E6DE01D}");
    TwitchApi_CreateReturnTypeClass(GetBannedEventsResult, GetBannedEventsResponse, "{00CFA8FE-A352-42BE-A765-02BFE29E8980}");

    struct GetBannedUsersDatum
    {
        AZ_TYPE_INFO(GetBannedUsersDatum, "{97FAC068-CC1C-48EB-8B22-E45A075329B0}");

        AZStd::string user_id;
        AZStd::string user_name;
        AZStd::string expires_at;
    };
    TwitchApi_CreatePaginationResponseTypeClass(GetBannedUsersDatum, GetBannedUsersResponse, "{4D25FCCC-A217-450F-A400-A77B23A8B9E1}");
    TwitchApi_CreateReturnTypeClass(GetBannedUsersResult, GetBannedUsersResponse, "{6E7271D9-3A5A-45AF-B421-15D661957A39}");

    struct GetModeratorsDatum
    {
        AZ_TYPE_INFO(GetModeratorsDatum, "{DC5729DA-55DE-49A9-B631-02234F9CD63A}");

        AZStd::string user_id;
        AZStd::string user_name;
    };
    TwitchApi_CreatePaginationResponseTypeClass(GetModeratorsDatum, GetModeratorsResponse, "{B1ED2169-7EAB-4325-BAFE-B10F821089D6}");
    TwitchApi_CreateReturnTypeClass(GetModeratorsResult, GetModeratorsResponse, "{8DD93755-D61B-467A-8D67-7200B1599F97}");

    struct GetModeratorEventsDatum
    {
        AZ_TYPE_INFO(GetModeratorEventsDatum, "{E811D5B5-6DC8-4806-B8A9-F31FA7245825}");

        AZStd::string id;
        AZStd::string event_type;
        AZStd::string event_timestamp;
        AZStd::string version;
        AZStd::string broadcaster_id;
        AZStd::string broadcaster_name;
        AZStd::string user_id;
        AZStd::string user_name;
    };
    TwitchApi_CreatePaginationResponseTypeClass(GetModeratorEventsDatum, GetModeratorEventsResponse,
        "{8E22AA2C-7B12-4BE1-B812-48CD8DBDAE7A}");
    TwitchApi_CreateReturnTypeClass(GetModeratorEventsResult, GetModeratorEventsResponse, "{456692D1-15B8-40C6-A086-4E25B09A6671}");

    struct SearchCategoriesDatum
    {
        AZ_TYPE_INFO(SearchCategoriesDatum, "{20913E76-DB59-463D-9EF8-318E4746F9A6}");

        AZStd::string id;
        AZStd::string name;
        AZStd::string box_art_url;
    };
    TwitchApi_CreatePaginationResponseTypeClass(SearchCategoriesDatum, SearchCategoriesResponse, "{FEE3F5D9-B75F-4354-B019-3E9E7225B1B2}");
    TwitchApi_CreateReturnTypeClass(SearchCategoriesResult, SearchCategoriesResponse, "{E0F05936-4596-4503-A8C8-EF7B488489E7}");

    struct SearchChannelsDatum
    {
        AZ_TYPE_INFO(SearchChannelsDatum, "{0EC6BEB2-15E7-4EE0-9024-3BFCAD6B86AD}");

        bool is_live;
        AZStd::string broadcaster_language;
        AZStd::string display_name;
        AZStd::string game_id;
        AZStd::string id;
        AZStd::string thumbnail_url;
        AZStd::string title;
        AZStd::string started_at;
        AZStd::vector<AZStd::string> tag_ids;
    };
    TwitchApi_CreatePaginationResponseTypeClass(SearchChannelsDatum, SearchChannelsResponse, "{FDFFEC6E-A1BC-4FF5-933D-7A6FACD8734A}");
    TwitchApi_CreateReturnTypeClass(SearchChannelsResult, SearchChannelsResponse, "{08A2821B-AB15-47C4-9107-8F660F5C7224}");

    struct StreamKeyResponse
    {
        AZ_TYPE_INFO(StreamKeyResponse, "{2A744111-7AE1-43C5-8C4F-211402A8EA9C}");

        AZStd::string stream_key;
    };
    TwitchApi_CreateReturnTypeClass(StreamKeyResult, StreamKeyResponse, "{AFD5F44B-59AA-4074-9560-070C26EB455B}");

    struct GetStreamsDatum
    {
        AZ_TYPE_INFO(GetStreamsDatum, "{72B7AE43-A9BA-46C9-8586-82818B151C9E}");

        AZ::u64 viewer_count;
        AZStd::string game_id;
        AZStd::string id;
        AZStd::string language;
        AZStd::string started_at;
        AZStd::string thumbnail_url;
        AZStd::string title;
        AZStd::string type;
        AZStd::string user_id;
        AZStd::string user_name;
        AZStd::vector<AZStd::string> tag_ids;
    };
    TwitchApi_CreatePaginationResponseTypeClass(GetStreamsDatum, GetStreamsResponse, "{0563F352-CFB7-4E15-AD23-176CF287E3D6}");
    TwitchApi_CreateReturnTypeClass(GetStreamsResult, GetStreamsResponse, "{68827D5A-DD9B-4B2F-8FD3-A75A3E6C47D1}");

    struct CreateStreamMarkerDatum
    {
        AZ_TYPE_INFO(CreateStreamMarkerDatum, "{6165A201-2221-40AF-B918-B73740ED8B86}");

        AZStd::string created_at;
        AZStd::string description;
        AZStd::string id;
        AZStd::string position_seconds;
    };
    TwitchApi_CreateResponseTypeClass(CreateStreamMarkerDatum, CreateStreamMarkerResponse, "{3D40552F-BF5B-4B48-B1DD-F9A6B400104F}");
    TwitchApi_CreateReturnTypeClass(CreateStreamMarkerResult, CreateStreamMarkerResponse, "{154A7E0D-1AE7-4C25-9D25-ACD8D7B39D63}");

    struct GetStreamMarkersMarkerDatum
    {
        AZ_TYPE_INFO(GetStreamMarkersMarkerDatum, "{82ABFC9A-0E52-421B-9F90-9A1657C3E802}");

        AZ::u64 position_seconds;
        AZStd::string id;
        AZStd::string created_at;
        AZStd::string description;
        AZStd::string URL;
    };

    struct GetStreamMarkersVideoDatum
    {
        AZ_TYPE_INFO(GetStreamMarkersVideoDatum, "{8C951C62-583D-4BF1-8241-0AEF4CAAEF66}");

        AZStd::string video_id;
        AZStd::vector<GetStreamMarkersMarkerDatum> markers;
    };

    struct GetStreamMarkersDatum
    {
        AZ_TYPE_INFO(GetStreamMarkersDatum, "{F02ED5D7-21E3-46C0-B040-3B60D4AC2AC9}");

        AZStd::string user_id;
        AZStd::string user_name;
        AZStd::vector<GetStreamMarkersVideoDatum> videos;
    };
    TwitchApi_CreatePaginationResponseTypeClass(GetStreamMarkersDatum, GetStreamMarkersResponse, "{01789ED0-0803-4D32-8C81-E7AACFFF278F}");
    TwitchApi_CreateReturnTypeClass(GetStreamMarkersResult, GetStreamMarkersResponse, "{DD89620A-68AC-4C09-B923-9E559D62A4B4}");

    struct GetBroadcasterSubscriptionsDatum
    {
        AZ_TYPE_INFO(GetBroadcasterSubscriptionsDatum, "{8568D33D-4372-41CC-829A-C283FDA723D1}");

        bool is_gift;
        AZStd::string broadcaster_id;
        AZStd::string broadcaster_name;
        AZStd::string tier;
        AZStd::string plan_name;
        AZStd::string user_id;
        AZStd::string user_name;
    };
    TwitchApi_CreatePaginationResponseTypeClass(GetBroadcasterSubscriptionsDatum, GetBroadcasterSubscriptionsResponse,
        "{C8C4DE73-291B-4571-A22B-70D44C461D65}");
    TwitchApi_CreateReturnTypeClass(GetBroadcasterSubscriptionsResult, GetBroadcasterSubscriptionsResponse,
        "{79A7A326-4ADC-4AFD-B343-B7DFCD105250}");

    struct GetStreamTagsDatum
    {
        AZ_TYPE_INFO(GetStreamTagsDatum, "{054648B2-7103-44E6-B091-2F711F710A9A}");

        bool is_auto;
        AZStd::string tag_id;
        AZStd::unordered_map<AZStd::string, AZStd::string> localization_names;
        AZStd::unordered_map<AZStd::string, AZStd::string> localization_descriptions;
    };
    TwitchApi_CreatePaginationResponseTypeClass(GetStreamTagsDatum, GetAllStreamTagsResponse, "{048AA2C7-1922-42A6-8E63-1B31BBC0C703}");
    TwitchApi_CreateReturnTypeClass(GetAllStreamTagsResult, GetAllStreamTagsResponse, "{52A262B7-490D-4925-919D-309776EE6356}");
    TwitchApi_CreateResponseTypeClass(GetStreamTagsDatum, GetStreamTagsResponse, "{9FFDB3D4-F14F-4D86-B699-95D674DBBF84}");
    TwitchApi_CreateReturnTypeClass(GetStreamTagsResult, GetStreamTagsResponse, "{E614D128-0826-4686-8E35-369956AC4593}");

    struct GetUsersDatum
    {
        AZ_TYPE_INFO(GetUsersDatum, "{C71D7FD3-3D00-4663-92F4-81DA505179C2}");

        AZ::u64 view_count;
        AZStd::string broadcaster_type;
        AZStd::string description;
        AZStd::string display_name;
        AZStd::string email;
        AZStd::string id;
        AZStd::string login;
        AZStd::string offline_image_url;
        AZStd::string profile_image_url;
        AZStd::string type;
    };
    TwitchApi_CreateResponseTypeClass(GetUsersDatum, GetUsersResponse, "{F30CC0C6-6DAF-4234-8057-357B486CC440}");
    TwitchApi_CreateReturnTypeClass(GetUsersResult, GetUsersResponse, "{DA2C0941-D708-4CA6-9DAB-471DD8FDB1CC}");

    struct GetUsersFollowsDatum
    {
        AZ_TYPE_INFO(GetUsersFollowsDatum, "{577C8FB9-6AF7-4A9F-A4D4-91A144751BA1}");

        AZStd::string followed_at;
        AZStd::string from_id;
        AZStd::string from_name;
        AZStd::string to_id;
        AZStd::string to_name;
        AZStd::string total;
    };
    TwitchApi_CreatePaginationResponseTypeClass(GetUsersFollowsDatum, GetUsersFollowsResponse, "{8EEF8422-EFC5-4CD1-8D2D-7D24E6EE97A8}");
    TwitchApi_CreateReturnTypeClass(GetUsersFollowsResult, GetUsersFollowsResponse, "{4A8EFD8A-BA31-4631-91C3-4603BB0F6A35}");

    struct UserBlockListDatum
    {
        AZ_TYPE_INFO(UserBlockListDatum, "{2205FBCC-0942-4D16-B651-DC11D35AC4F1}");

        AZStd::string id;
        AZStd::string user_login;
        AZStd::string display_name;
    };
    TwitchApi_CreatePaginationResponseTypeClass(UserBlockListDatum, UserBlockListResponse, "{69AB1804-08EA-4F9C-B970-0FC2913E5D38}");
    TwitchApi_CreateReturnTypeClass(UserBlockListResult, UserBlockListResponse, "{0A012873-12D8-4886-B8F7-88641345E89E}");

    struct GetUserExtensionsDatum
    {
        AZ_TYPE_INFO(GetUserExtensionsDatum, "{905B8C17-5CA7-4FEC-9C4C-EC16184BE769}");

        bool can_activate;
        AZStd::string id;
        AZStd::string name;
        AZStd::string version;
        AZStd::vector<AZStd::string> type;
    };
    TwitchApi_CreateResponseTypeClass(GetUserExtensionsDatum, GetUserExtensionsResponse, "{BDA2B692-FA55-4B30-B7AE-B01E80FF5739}");
    TwitchApi_CreateReturnTypeClass(GetUserExtensionsResult, GetUserExtensionsResponse, "{D303DA23-7214-46D8-8951-E8BCA1A67EF2}");

    struct GetUserActiveExtensionsComponent
    {
        AZ_TYPE_INFO(GetUserActiveExtensionsComponent, "{DC4B67B1-23D0-48FD-A2A7-F4A10CD41317}");

        bool active;
        AZ::s64 x;
        AZ::s64 y;
        AZStd::string id;
        AZStd::string name;
        AZStd::string version;
    };

    struct GetUserActiveExtensionsResponse
    {
        AZ_TYPE_INFO(GetUserActiveExtensionsResponse, "{F0CDF095-8B9D-4674-ACA1-5472A62782D7}");

        AZStd::unordered_map<AZStd::string, GetUserActiveExtensionsComponent> panel;
        AZStd::unordered_map<AZStd::string, GetUserActiveExtensionsComponent> overlay;
        AZStd::unordered_map<AZStd::string, GetUserActiveExtensionsComponent> component;

    };
    TwitchApi_CreateReturnTypeClass(GetUserActiveExtensionsResult, GetUserActiveExtensionsResponse,
        "{D14E8158-AAB7-4E30-9BB1-8377A0B8C217}");

    struct GetVideosDatum
    {
        AZ_TYPE_INFO(GetVideosDatum, "{7F238D6A-E444-4414-B95A-5E3F02A20569}");

        AZ::u64 view_count;
        AZStd::string created_at;
        AZStd::string description;
        AZStd::string duration;
        AZStd::string id;
        AZStd::string language;
        AZStd::string published_at;
        AZStd::string thumbnail_url;
        AZStd::string title;
        AZStd::string type;
        AZStd::string url;
        AZStd::string user_id;
        AZStd::string user_name;
        AZStd::string viewable;
    };
    TwitchApi_CreatePaginationResponseTypeClass(GetVideosDatum, GetVideosResponse, "{EB07FFD8-019A-4451-9E39-89F980837FF7}");
    TwitchApi_CreateReturnTypeClass(GetVideosResult, GetVideosResponse, "{184F1C42-2902-45A6-8814-CCAA91376745}");

    struct DeleteVideosResponse
    {
        AZ_TYPE_INFO(DeleteVideosResponse, "{907F9321-1AB8-418F-B552-3476E48F626B}");

        AZStd::vector<AZStd::string> video_keys;
    };
    TwitchApi_CreateReturnTypeClass(DeleteVideosResult, DeleteVideosResponse, "{E460A2CB-55D1-477B-B7D9-803F8385C66B}");

    struct GetWebhookSubscriptionsDatum
    {
        AZ_TYPE_INFO(GetWebhookSubscriptionsDatum, "{69685AF5-D677-46CE-957E-14ED2A2061DF}");

        AZStd::string callback;
        AZStd::string expires_at;
        AZStd::string topic;
    };

    struct GetWebhookSubscriptionsResponse : public PaginationResponse
    {
        AZ_TYPE_INFO(GetWebhookSubscriptionsResponse, "{5AB20671-FF02-43EC-8CE3-C0FBF59C036B}");

        AZ::u64 total;
        AZStd::vector<GetWebhookSubscriptionsDatum> data;
    };
    TwitchApi_CreateReturnTypeClass(GetWebhookSubscriptionsResult, GetWebhookSubscriptionsResponse,
        "{81609BA7-3BAF-42DA-A64D-C944210D1CCF}");
}
