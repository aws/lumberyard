--[[
 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--]]

--Script to test portions of the Twitch Api Gem

TwitchApi = {
    Properties = {
        enableLog = {default=true, description="Enables logging from the script"},
        userId = {default="", description="The Twitch User Id for account related requests"},
        userAccessToken = {default="", description="The User Access OAuth token for client requests"},
        appAccessToken = {default="", description="The App Access OAuth token for server requests"},
        applicationId = {default="", description="Twitch Application Id or OAuth"}
    },

    Internal = {
        broadcasterId = "",
        gameId = "",
        extensionId = "",
        extensionVersion = ""
    }
}

function TwitchApi:TestAds()
    local getAdsReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.StartCommercial(getAdsReceipt, self.Internal.broadcasterId, 60)
end

function TwitchApi:TestAnalytics()
    local getExtensionReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetExtensionAnalytics(getExtensionReceipt, "", "", "", "", "", 5)

    local getGameReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetGameAnalytics(getGameReceipt, "", "", "", "", "", 5)
end

function TwitchApi:TestBits()
    local bitsLBReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetBitsLeaderboard(bitsLBReceipt, "", "year", "", 5)
    
    local getCheermotesReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetCheermotes(getCheermotesReceipt, "")

    local getExtensionReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetExtensionTransactions(getExtensionReceipt, self.Internal.extensionId, "", "", 5)
end

function TwitchApi:TestChannels()
    local getReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetChannelInformation(getReceipt, self.Internal.broadcasterId)
    
    local modifyReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.ModifyChannelInformation(modifyReceipt, self.Properties.userId, "", "", "New Stream Title")
    
    local getEditorsReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetChannelEditors(getEditorsReceipt, self.Properties.userId)
end

function TwitchApi:TestChannelPoints()
    local createRewardReceipt = TwitchApiReceiptId()
    local createRewardSettings = CustomRewardSettings("Test Reward", "", 100, true, "", false, false, 0, false, 0, false, 0, false)
    TwitchApiRequestBus.Broadcast.CreateCustomRewards(createRewardReceipt, self.Properties.userId, createRewardSettings)

    local deleteRewardReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.DeleteCustomReward(deleteRewardReceipt, self.Properties.userId, "123")
    
    local getRewardReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetCustomReward(getRewardReceipt, self.Properties.userId, "", false)
    
    local getRewardRedemptionReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetCustomRewardRedemption(getRewardRedemptionReceipt, self.Properties.userId, "", "", "UNFULFILLED", "", "", 5)

    local updateRewardReceipt = TwitchApiReceiptId()
    local updateRewardSettings = CustomRewardSettings("Updated Test Reward", "", 10, true, "", false, false, 0, false, 0, false, 0, false)
    TwitchApiRequestBus.Broadcast.UpdateCustomReward(updateRewardReceipt, self.Properties.userId, "123", updateRewardSettings)

    local updateRewardRedemptionReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.UpdateRedemptionStatus(updateRewardRedemptionReceipt, "123", self.Properties.userId, "", "FULFILLED")
end

function TwitchApi:TestClips()
    local createReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.CreateClip(createReceipt, self.Internal.broadcasterId, false)

    local getReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetClips(getReceipt, "", self.Internal.gameId, "", "", "", "", "", 5)
end

function TwitchApi:TestEntitlements()
    local emptyStringVec = vector_basic_string_char_char_traits_char()

    local createGrantsReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.CreateEntitlementGrantsUploadURL(createGrantsReceipt, "123456789", "bulk_drops_grant")
    
    local getCodeReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetCodeStatus(getCodeReceipt, emptyStringVec, 0)
    
    local getDropReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetDropsEntitlements(getDropReceipt, "", self.Properties.userId, "", "", 5)
    
    local redeemCodeReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.RedeemCode(redeemCodeReceipt, emptyStringVec, 0)
end

function TwitchApi:TestGames()
    local topGamesReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetTopGames(topGamesReceipt, "", "", 5)

    local gamesReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetGames(gamesReceipt, self.Internal.gameId, "")
end

function TwitchApi:TestHypeTrain()
    local hypeTrainReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetHypeTrainEvents(hypeTrainReceipt, self.Internal.broadcasterId, "", "", 5)
end

function TwitchApi:TestModeration()
    local emptyStringVec = vector_basic_string_char_char_traits_char()

    local checkAutoModReceipt = TwitchApiReceiptId()
    local autoModVec = vector_CheckAutoModInputDatum()
    local autoModElem = CheckAutoModInputDatum()
    autoModElem.MsgId = "123"
    autoModElem.MsgText = "Hello World!"
    autoModElem.UserId = self.Properties.userId
    autoModVec:push_back(autoModElem)
    TwitchApiRequestBus.Broadcast.CheckAutoModStatus(checkAutoModReceipt, self.Properties.userId, autoModVec)

    local bannedEventsReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetBannedEvents(bannedEventsReceipt, self.Properties.userId, emptyStringVec, "", 5)

    local bannedUsersReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetBannedUsers(bannedUsersReceipt, self.Properties.userId, emptyStringVec, "", "")

    local modsReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetModerators(modsReceipt, self.Properties.userId, emptyStringVec, "")

    local modEventsReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetModeratorEvents(modEventsReceipt, self.Properties.userId, emptyStringVec)
end

function TwitchApi:TestSearch()
    local searchCategoriesReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.SearchCategories(searchCategoriesReceipt, "fort", "", 5)

    local searchChannelsReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.SearchChannels(searchChannelsReceipt, "loserfruit", "", false, 5)
end

function TwitchApi:TestStreams()
    local emptyStringVec = vector_basic_string_char_char_traits_char()
    
    local streamKeyReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetStreamKey(streamKeyReceipt, self.Properties.userId)
    
    local streamsReceipt = TwitchApiReceiptId()
    local gameIdVector = vector_basic_string_char_char_traits_char()
    gameIdVector:push_back(self.Internal.gameId)
    TwitchApiRequestBus.Broadcast.GetStreams(streamsReceipt, gameIdVector, emptyStringVec, emptyStringVec, emptyStringVec, "", "", 5)

    --This will 404 if the target user is not streaming at the time of the call
    local markersReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.CreateStreamMarker(markersReceipt, self.Properties.userId, "")

    --This will 404 if the target user does not have any VODs
    local getMarkersReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetStreamMarkers(getMarkersReceipt, self.Properties.userId, "", "", "", 5)
end

function TwitchApi:TestSubscriptions()
    local emptyStringVec = vector_basic_string_char_char_traits_char()

    --Broadcaster Id must be equal to User Id assosciated with the OAuth Token
    local subsReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetBroadcasterSubscriptions(subsReceipt, self.Properties.userId, emptyStringVec)
end

function TwitchApi:TestTags()
    local emptyStringVec = vector_basic_string_char_char_traits_char()

    local getAllTagsReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetAllStreamTags(getAllTagsReceipt, emptyStringVec, "", 5)

    --Broadcaster Id must be equal to User Id assosciated with the OAuth Token in order to get a non-empty response
    local getTagsReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetStreamTags(getTagsReceipt, self.Properties.userId)    
end

function TwitchApi:TestUsers()
    local emptyStringVec = vector_basic_string_char_char_traits_char()

    local getUsersReceipt = TwitchApiReceiptId()
    local userIdVec = vector_basic_string_char_char_traits_char()
    userIdVec:push_back(self.Properties.userId)
    TwitchApiRequestBus.Broadcast.GetUsers(getUsersReceipt, userIdVec, emptyStringVec)

    local updateReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.UpdateUser(updateReceipt, "Fun Stream!")

    local getFollowsReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetUserFollows(getFollowsReceipt, "", self.Internal.broadcasterId, "", 15)
    
    local createFollowsReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.CreateUserFollows(createFollowsReceipt, self.Properties.userId, self.Internal.broadcasterId, false)
    
    local deleteFollowsReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.DeleteUserFollows(deleteFollowsReceipt, self.Properties.userId, self.Internal.broadcasterId)
    
    local blockReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.BlockUser(blockReceipt, self.Internal.broadcasterId, "", "")
    
    local getBlockListReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetUserBlockList(getBlockListReceipt, self.Properties.userId, "", 5)
    
    local unblockReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.UnblockUser(unblockReceipt, self.Internal.broadcasterId)

    local getExtensionsReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetUserExtensions(getExtensionsReceipt)

    --Setup AND activate extensions on your Twitch channel to receive data here
    local getActiveExtensionsReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetUserActiveExtensions(getActiveExtensionsReceipt, "")

    local updateUserExtensionsReceipt = TwitchApiReceiptId()
    local panelVector = vector_GetUserActiveExtensionsComponent()
    local emptyVector = vector_GetUserActiveExtensionsComponent()
    local panelElem = GetUserActiveExtensionsComponent()
    panelElem.Active = true
    panelElem.Id = self.Internal.extensionId
    panelElem.Version = self.Internal.extensionVersion
    panelVector:push_back(panelElem)
    TwitchApiRequestBus.Broadcast.UpdateUserExtensions(updateUserExtensionsReceipt, panelVector, emptyVector, emptyVector)
end

function TwitchApi:TestVideos()
    local emptyStringVec = vector_basic_string_char_char_traits_char()

    local videosReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetVideos(videosReceipt, emptyStringVec, self.Internal.broadcasterId, "", "", "", "", "", "", "", 10)
    
    local deleteVideosReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.DeleteVideos(deleteVideosReceipt, emptyStringVec)
end

function TwitchApi:TestWebhooks()
    --This requires a Application (Server) OAuth Token
    local webhookReceipt = TwitchApiReceiptId()
    TwitchApiRequestBus.Broadcast.GetWebhookSubscriptions(webhookReceipt, "", 10)
end

function TwitchApi:OnActivate()
    self.TwitchApiHandler = TwitchApiNotifyBus.Connect(self)

    --If the Twitch Application Id/OAuth is not set in the exe, then it must be set here!
    TwitchApiRequestBus.Broadcast.SetApplicationId(self.Properties.applicationId)

    -- User and Auth Requests --
    TwitchApiRequestBus.Broadcast.SetUserId(self.Properties.userId)

    TwitchApiRequestBus.Broadcast.SetUserAccessToken(self.Properties.userAccessToken)

    TwitchApiRequestBus.Broadcast.SetAppAccessToken(self.Properties.appAccessToken)

    --Requires Twitch Partner or Affiliate status
    --self:TestAds()
    self:TestAnalytics()
    self:TestBits()
    self:TestChannels()
    --Requires Twitch Partner or Affiliate status
    --self:TestChannelPoints()
    self:TestClips()
    self:TestEntitlements()
    self:TestGames()
    self:TestHypeTrain()
    self:TestModeration()
    self:TestSearch()
    self:TestStreams()
    self:TestSubscriptions()
    self:TestTags()
    self:TestUsers()
    --Requires an account with VODs
    self:TestVideos()
    self:TestWebhooks()
end

function TwitchApi:TryAndLog(logValue)
    if self.Properties.enableLog then  
        Debug.Log(logValue)
    end
end 

function TwitchApi:OnDeactivate()
    if self.TwitchApiHandler ~= nil then
        self.TwitchApiHandler:Disconnect()
    end
end

-- Ads notifications ---
function TwitchApi:OnStartCommercial(result)
    if (#result.Value.Data == 0) then
        TwitchApi:TryAndLog("OnStartCommercial: No result returned.")
        assert(false)
    else
        TwitchApi:TryAndLog("OnStartCommercial: Length: " ..tostring(result.Value.Data[1].Length))
        assert(result.Result == TwitchApiResultCode.TwitchRestError)
    end
end

-- Analytics notifications ---
function TwitchApi:OnGetExtensionAnalytics(result)
    --Requires user have ownership over an extension
    TwitchApi:TryAndLog("OnGetExtensionAnalytics: Extensions Found: " ..tostring(#result.Value.Data))
    assert(result.Result == TwitchApiResultCode.TwitchRestError)
end

function TwitchApi:OnGetGameAnalytics(result)
    --Requires user have ownership over a game
    TwitchApi:TryAndLog("OnGetGameAnalytics: Games Found:" ..tostring(#result.Value.Data))
    assert(result.Result == TwitchApiResultCode.Success)
end

-- Bits notifications ---
function TwitchApi:OnGetBitsLeaderboard(result)
    TwitchApi:TryAndLog("OnGetBitsLeaderboard: Total: " ..tostring(result.Value.Total))
    assert(result.Result == TwitchApiResultCode.Success)
end

function TwitchApi:OnGetCheermotes(result)
    if (#result.Value.Data == 0) then
        TwitchApi:TryAndLog("OnGetCheermotes: No cheers found. Check your inputs.")
        assert(false)
    else
        TwitchApi:TryAndLog("OnGetCheermotes: Cheersmotes Found: " ..tostring(#result.Value.Data).." First result tier color: " ..tostring(result.Value.Data[1].Tiers[1].Color))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

function TwitchApi:OnGetExtensionTransactions(result)
    --Requires owned extensions with bits transactions
    TwitchApi:TryAndLog("OnGetExtensionTransactions: Extensions Found: " ..tostring(#result.Value.Data))
    assert(result.Result == TwitchApiResultCode.TwitchRestError)
end

-- Channel notifications ---
function TwitchApi:OnGetChannelInformation(result)
    --Requires stream to be online
    if (#result.Value.Data == 0) then
        TwitchApi:TryAndLog("OnGetChannelInformation: No Channel Info found.")
        assert(false)
    else
        TwitchApi:TryAndLog("OnGetChannelInformation: Broadcaster Name: " ..tostring(result.Value.Data[1].BroadcasterName).. " Stream Title: " ..tostring(result.Value.Data[1].Title))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

function TwitchApi:OnModifyChannelInformation(receipt, rc)
    TwitchApi:TryAndLog("OnModifyChannelInformation: Success: " ..tostring(rc == TwitchApiResultCode.Success))
    assert(rc == TwitchApiResultCode.Success)
end

function TwitchApi:OnGetChannelEditors(result)
    TwitchApi:TryAndLog("OnGetChannelEditors: Editors Found: " ..tostring(#result.Value.Data))
    assert(result.Result == TwitchApiResultCode.Success)
end

-- Channel Points notifications ---
function TwitchApi:OnCreateCustomRewards(result)
    if (#result.Value.Data == 0) then
        TwitchApi:TryAndLog("OnCreateCustomRewards: No Reward created.")
        assert(false)
    else
        TwitchApi:TryAndLog("OnCreateCustomRewards: Reward Id: " ..tostring(result.Value.Data[1].Id))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

function TwitchApi:OnDeleteCustomReward(receipt, rc)
    TwitchApi:TryAndLog("OnDeleteCustomReward: Success: " ..tostring(rc == TwitchApiResultCode.Success))
    assert(rc == TwitchApiResultCode.Success)
end

function TwitchApi:OnGetCustomReward(result)
    if (#result.Value.Data == 0) then
        TwitchApi:TryAndLog("OnGetCustomReward: No Reward found.")
        assert(false)
    else
        TwitchApi:TryAndLog("OnGetCustomReward: Reward Title: " ..tostring(result.Value.Data[1].Title))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

function TwitchApi:OnGetCustomRewardRedemption(result)
    if (#result.Value.Data == 0) then
        TwitchApi:TryAndLog("OnGetCustomRewardRedemption: No Reward Redemptions found.")
        assert(false)
    else
        TwitchApi:TryAndLog("OnGetCustomRewardRedemption: Reward Id: " ..tostring(result.Value.Data[1].Id))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

function TwitchApi:OnUpdateCustomReward(result)
    if (#result.Value.Data == 0) then
        TwitchApi:TryAndLog("OnUpdateCustomReward: No Reward found.")
        assert(false)
    else
        TwitchApi:TryAndLog("OnUpdateCustomReward: Reward Title: " ..tostring(result.Value.Data[1].Title))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

function TwitchApi:OnUpdateRedemptionStatus(result)
    if (#result.Value.Data == 0) then
        TwitchApi:TryAndLog("OnUpdateRedemptionStatus: No Reward Redemptions found.")
        assert(false)
    else
        TwitchApi:TryAndLog("OnUpdateRedemptionStatus: Reward Id: " ..tostring(result.Value.Data[1].Id))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

-- Clips notifications ---
function TwitchApi:OnCreateClip(result)
    --Requires stream to be online
    TwitchApi:TryAndLog("OnCreateClip: Clips Found : " ..tostring(#result.Value.Data))
    assert(result.Result == TwitchApiResultCode.TwitchRestError)
end

function TwitchApi:OnGetClips(result)
    if (#result.Value.Data == 0) then
        TwitchApi:TryAndLog("OnGetClips: No clips found. Check your inputs.")
        assert(false)
    else
        TwitchApi:TryAndLog("OnGetClips: Title1: " ..tostring(result.Value.Data[1].Title))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

-- Entitlements notifications ---
function TwitchApi:OnCreateEntitlementGrantsUploadURL(result)
    --Requires a company with a registered game
    TwitchApi:TryAndLog("OnCreateEntitlementGrantsUploadURL: " ..tostring(result))
    assert(result.Result == TwitchApiResultCode.TwitchRestError)
end

function TwitchApi:OnGetCodeStatus(result)
    --Requires a company with a registered game
    TwitchApi:TryAndLog("OnGetCodeStatus: " ..tostring(result))
    assert(result.Result == TwitchApiResultCode.TwitchRestError)
end

function TwitchApi:OnGetDropsEntitlements(result)
    --Requires a company with a registered game
    TwitchApi:TryAndLog("OnGetDropsEntitlements: " ..tostring(result))
    assert(result.Result == TwitchApiResultCode.TwitchRestError)
end

function TwitchApi:OnRedeemCode(result)
    --Requires a company with a registered game
    TwitchApi:TryAndLog("OnRedeemCode: " ..tostring(result))
    assert(result.Result == TwitchApiResultCode.TwitchRestError)
end

-- Games notifications ---
function TwitchApi:OnGetTopGames(result)
    if (#result.Value.Data == 0) then
        TwitchApi:TryAndLog("OnGetTopGames: No games found on Twitch. Check your Bearer tokens.")
        assert(false)
    else
        TwitchApi:TryAndLog("OnGetTopGames: Game1: " ..tostring(result.Value.Data[1].Name))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

function TwitchApi:OnGetGames(result)
    if (#result.Value.Data == 0) then
        TwitchApi:TryAndLog("OnGetGames: No results returned. Check your inputs.")
    else 
        TwitchApi:TryAndLog("OnGetGames: Game1: " ..tostring(result.Value.Data[1].Name))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

-- Hype Train notifications ---
function TwitchApi:OnGetHypeTrainEvents(result)
    TwitchApi:TryAndLog("OnGetHypeTrainEvents: Events Found: " ..tostring(#result.Value.Data))
    assert(result.Result == TwitchApiResultCode.Success)
end

-- Moderation notifications ---
function TwitchApi:OnCheckAutoModStatus(result)
    if (#result.Value.Data == 0) then
        TwitchApi:TryAndLog("OnCheckAutoModStatus: No results returned. Check if AutoMod is enabled at https://dashboard.twitch.tv/")
    else 
        TwitchApi:TryAndLog("OnCheckAutoModStatus: Message Permitted: " ..tostring(result.Value.Data[1].IsPermitted))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

function TwitchApi:OnGetBannedEvents(result)
    TwitchApi:TryAndLog("OnGetBannedEvents: Events Found: " ..tostring(#result.Value.Data))
    assert(result.Result == TwitchApiResultCode.Success)
end

function TwitchApi:OnGetBannedUsers(result)
    TwitchApi:TryAndLog("OnGetBannedUsers: Users Found: " ..tostring(#result.Value.Data))
    assert(result.Result == TwitchApiResultCode.Success)
end

function TwitchApi:OnGetModerators(result)
    TwitchApi:TryAndLog("OnGetModerators: Moderators Found: " ..tostring(#result.Value.Data))
    assert(result.Result == TwitchApiResultCode.Success)
end

function TwitchApi:OnGetModeratorEvents(result)
    TwitchApi:TryAndLog("OnGetModeratorEvents: ModEvents Found: " ..tostring(#result.Value.Data))
    assert(result.Result == TwitchApiResultCode.Success)
end

-- Search notifications ---
function TwitchApi:OnSearchCategories(result)
    if (#result.Value.Data == 0) then
        TwitchApi:TryAndLog("OnSearchCategories: No Categories, check your inputs.")
    else
        TwitchApi:TryAndLog("OnSearchCategories: Categories Found: " ..tostring(#result.Value.Data))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

function TwitchApi:OnSearchChannels(result)
    if (#result.Value.Data == 0) then
        TwitchApi:TryAndLog("OnSearchChannels: No Channels, check your inputs.")
    else
        TwitchApi:TryAndLog("OnSearchChannels: Channels Found: " ..tostring(#result.Value.Data))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

-- Streams notifications ---
function TwitchApi:OnGetStreamKey(result)
    TwitchApi:TryAndLog("OnGetStreamKey: Stream Key: " ..tostring(result.Value.StreamKey))
    assert(result.Result == TwitchApiResultCode.Success)
end

function TwitchApi:OnGetStreams(result)
    if (#result.Value.Data == 0) then
        TwitchApi:TryAndLog("OnGetStreams: No active streams found, check your inputs.")
    else
        TwitchApi:TryAndLog("OnGetStreams: Title1: " ..tostring(result.Value.Data[1].Title))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

function TwitchApi:OnCreateStreamMarker(result)
    --Requires target stream be active
    --TwitchApi:TryAndLog("OnCreateStreamMarker: Desc1: " ..tostring(result.Value.Data[1].Description))
    --assert(result.Result == TwitchApiResultCode.Success)
end

function TwitchApi:OnGetStreamMarkers(result)
    --Requires target streams have VODs
    --TwitchApi:TryAndLog("OnGetStreamMarkers: UserId: " ..tostring(result.Value.Data[1].UserId))
    --assert(result.Result == TwitchApiResultCode.Success)
end

-- Subscriptions notifications ---
function TwitchApi:OnGetBroadcasterSubscriptions(result)
    TwitchApi:TryAndLog("OnGetBroadcasterSubscriptions: Subs Found: " ..tostring(#result.Value.Data))
    assert(result.Result == TwitchApiResultCode.Success)
end

-- Tags notifications ---
function TwitchApi:OnGetAllStreamTags(result)
    if (#result.Value.Data == 0) then
        TwitchApi:TryAndLog("OnGetAllStreamTags: No stream tags found on Twitch, check your Bearer tokens.")
        assert(false)
    else
        TwitchApi:TryAndLog("OnGetAllStreamTags: TagId1: " ..tostring(result.Value.Data[1].TagId))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

function TwitchApi:OnGetStreamTags(result)
    if (#result.Value.Data == 0) then
        TwitchApi:TryAndLog("OnGetStreamTags: No tags found, only tags set in the last 72h will be returned.")
    else
        TwitchApi:TryAndLog("OnGetStreamTags: TagId1: " ..tostring(result.Value.Data[1].TagId))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

function TwitchApi:OnReplaceStreamTags(receipt, rc)
    TwitchApi:TryAndLog("OnReplaceStreamTags: Success: " ..tostring(rc == TwitchApiResultCode.Success))
    assert(rc == TwitchApiResultCode.Success)
end

-- Users notifications ---
function TwitchApi:OnGetUsers(result)
    if (#result.Value.Data == 0) then
        TwitchApi:TryAndLog("OnGetUsers: No users found.")
        assert(false)
    else
        TwitchApi:TryAndLog("OnGetUsers: DisplayName1: " ..tostring(result.Value.Data[1].DisplayName))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

function TwitchApi:OnUpdateUser(result)
    if (#result.Value.Data == 0) then
        TwitchApi:TryAndLog("OnUpdateUser: Unable to update user. The target user must match the user for the Bearer token.")
    else
        TwitchApi:TryAndLog("OnUpdateUser: DisplayName1: " ..tostring(result.Value.Data[1].DisplayName))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

function TwitchApi:OnGetUsersFollows(result)
    if (#result.Value.Data == 0) then
        TwitchApi:TryAndLog("OnGetUsersFollows: No followers found, check that the from or to user has follows.")
        assert(false)
    else
        TwitchApi:TryAndLog("OnGetUsersFollows: FromName1: " ..tostring(result.Value.Data[1].FromName))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

function TwitchApi:OnCreateUserFollows(receipt, rc)
    TwitchApi:TryAndLog("OnCreateUserFollows: Success: " ..tostring(rc == TwitchApiResultCode.Success))
    assert(rc == TwitchApiResultCode.Success)
end

function TwitchApi:OnDeleteUserFollows(receipt, rc)
    TwitchApi:TryAndLog("OnDeleteUserFollows: Success: " ..tostring(rc == TwitchApiResultCode.Success))
    assert(rc == TwitchApiResultCode.Success)
end

function TwitchApi:OnGetUserBlockList(result)
    if (#result.Value.Data == 0) then
        TwitchApi:TryAndLog("OnGetUserBlockList: No blocks found, check that the user has blocks.")
        assert(false)
    else
        TwitchApi:TryAndLog("OnGetUserBlockList: DisplayName1: " ..tostring(result.Value.Data[1].DisplayName))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

function TwitchApi:OnBlockUser(receipt, rc)
    TwitchApi:TryAndLog("OnBlockUser: Success: " ..tostring(rc == TwitchApiResultCode.Success))
    assert(rc == TwitchApiResultCode.Success)
end

function TwitchApi:OnUnblockUser(receipt, rc)
    TwitchApi:TryAndLog("OnUnblockUser: Success: " ..tostring(rc == TwitchApiResultCode.Success))
    assert(rc == TwitchApiResultCode.Success)
end

function TwitchApi:OnGetUserExtensions(result)
    if (#result.Value.Data == 0) then
        TwitchApi:TryAndLog("OnGetUserExtensions: No extensions found, add extensions at https://dashboard.twitch.tv/")
    else
        TwitchApi:TryAndLog("OnGetUserExtensions: Name1: " ..tostring(result.Value.Data[1].Name))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

function TwitchApi:OnGetUserActiveExtensions(result)
    if not result.Value.Panel.Has(result.Value.Panel, "1") then
        TwitchApi:TryAndLog("OnGetUserActiveExtensions: There is no extension active as a panel. Try adding one on your Twitch page.")
    else
        local outcome = result.Value.Panel.At(result.Value.Panel, "1")
        local extensionName = tostring(outcome.GetValue(outcome).Name)
        local extensionId = tostring(outcome.GetValue(outcome).Id)
        local extensionVersion = tostring(outcome.GetValue(outcome).Version)
        TwitchApi:TryAndLog("OnGetUserActiveExtensions: Panel1 Name (Id) version: " .. extensionName .. " (" .. extensionId .. ") " .. extensionVersion)
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

function TwitchApi:OnUpdateUserExtensions(result)
    if not result.Value.Panel.Has(result.Value.Panel, "1") then
        TwitchApi:TryAndLog("OnUpdateUserExtensions: There is no extension active as a panel. Try adding one on your Twitch page.")
    else
        local outcome = result.Value.Panel.At(result.Value.Panel, "1")
        TwitchApi:TryAndLog("OnUpdateUserExtensions: Panel1 Name: " ..tostring(outcome.GetValue(outcome).Name))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

-- Videos notifications ---
function TwitchApi:OnGetVideos(result)
    if not (#result.Value.Data > 0) then
        TwitchApi:TryAndLog("OnGetVideos: Account has no videos, try a broadcaster with videos.")
    else
        TwitchApi:TryAndLog("OnGetVideos: Title1: " ..tostring(result.Value.Data[1].Title))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

function TwitchApi:OnDeleteVideos(result)
    if not (#result.Value.VideoKeys > 0) then
        TwitchApi:TryAndLog("OnDeleteVideos: No Videos deleted.")
    else
        TwitchApi:TryAndLog("OnDeleteVideos: Deleted Video Key: " ..tostring(result.Value.VideoKeys[1]))
        assert(result.Result == TwitchApiResultCode.Success)
    end
end

-- Webhooks notifications ---
function TwitchApi:OnGetWebhookSubscriptions(result)
    TwitchApi:TryAndLog("OnGetWebhookSubscriptions: Total: " ..tostring(result.Value.Total))
    assert(result.Result == TwitchApiResultCode.Success)
end

return TwitchApi