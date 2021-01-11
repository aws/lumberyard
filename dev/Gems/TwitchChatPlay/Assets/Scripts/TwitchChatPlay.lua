--[[
 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--]]

--Script to test portions of the Twitch ChatPlay Gem. This expects an irc chatbot to respond.

local test_TwitchChatPlay = {
    Properties = {
        channelId = {default = "", description = "The Twitch channel typically the same as userName"},
        userAccessToken = {default = "", description = "OAuth token with scope include chat:edit, chat:read, and whispers:edit"},
        userName = {default = "", description = "The Twitch user name associated with the OAuth token. eg. 'justin'"},
        chatBotNick = {default = "", description = "The Twitch user name associated with the irc chatbot. eg. 'justin'"},
        delay = {default = 4.0, suffix = "sec", description = "Seconds between test states."}
    }
}

function test_TwitchChatPlay:OnActivate()
    TwitchChatPlayRequestBus.Broadcast.ConnectToChatPlay(self.Properties.userAccessToken, self.Properties.userName)
    TwitchChatPlayRequestBus.Broadcast.JoinChannel(self.Properties.channelId)
    self.ActivateKeywordMatching = false
    self.UseTwitchPattern = false
    self.timer = 0.0
    self.states = { "KeywordFalseTwitchFalse",
                    "KeywordTrueTwitchFalse",
                    "KeywordTrueTwitchTrue",
                    "KeywordFalseTwitchTrue"}
    self.MessagesStateHandler = { "Setbasic",
                                  "addkanji",
                                  "addeuropean",
                                  "part",
                                  "end"}
    self.stateId = 1
    self.messageStateId = 1
    self.messageState = self.MessagesStateHandler[self.messageStateId]
    self.state = self.states[self.stateId]
    self.tickHandler = TickBus.Connect(self)
    self.chatHandler = TwitchChatPlayNotificationBus.Connect(self, self.entityId)
end

function test_TwitchChatPlay:IncrementState()
    if self.stateId == #self.states then
        self.stateId = 1
        if #self.states == 4 then
            table.remove(self.states, 4) --contracting the cycle
        end
    elseif self.messageState == "end" then
        self.stateId = 1
    else
        self.stateId = self.stateId + 1
    end
    self.state = self.states[self.stateId]
end

function test_TwitchChatPlay:IncrementMessageState()
    if self.messageStateId ~= #self.MessagesStateHandler then
        self.messageStateId = self.messageStateId + 1
    end
    self.messageState = self.MessagesStateHandler[self.messageStateId]
end

function test_TwitchChatPlay:KeyWordStates()
    Debug.Log("KeywordStates: " .. self.state)
    if self.state == "KeywordFalseTwitchFalse" then
        self:MessageStates()
        self.ActivateKeywordMatching = false
        self.UseTwitchPattern = false
    elseif self.state == "KeywordTrueTwitchFalse" then
        self.ActivateKeywordMatching = true
        self.UseTwitchPattern = false
    elseif self.state == "KeywordTrueTwitchTrue" then
        self.ActivateKeywordMatching = true
        self.UseTwitchPattern = true
    elseif self.state == "KeywordFalseTwitchTrue" then
        self.ActivateKeywordMatching = false
        self.UseTwitchPattern = true
    else
        Debug.Log("Undefined state reached.")
    end
    self:IncrementState()
    TwitchChatPlayRequestBus.Broadcast.ActivateKeywordMatching(self.ActivateKeywordMatching)
    TwitchChatPlayRequestBus.Broadcast.UseTwitchPattern(self.UseTwitchPattern)
    Debug.Log("==| ActivateKeywordMatching: " .. tostring(self.ActivateKeywordMatching) .. " | UseTwitchPattern: " .. tostring(self.UseTwitchPattern))
end

function test_TwitchChatPlay:MessageStates()
    if self.messageState == "Setbasic" then
        TwitchChatPlayRequestBus.Broadcast.SetHandlerForKeyword("Testing")
        TwitchChatPlayRequestBus.Broadcast.SetHandlerForKeywordToReturnFormattedResponse("Second")
        TwitchChatPlayRequestBus.Broadcast.SetHandlerForKeywordToReturnFormattedResponse("Single")
        self:IncrementMessageState()
    elseif self.messageState == "addkanji" then
        TwitchChatPlayRequestBus.Broadcast.RemoveKeyword("Testing")
        TwitchChatPlayRequestBus.Broadcast.RemoveKeyword("Second")
        TwitchChatPlayRequestBus.Broadcast.RemoveKeyword("Single")
        TwitchChatPlayRequestBus.Broadcast.SetHandlerForKeyword("試")
        TwitchChatPlayRequestBus.Broadcast.SetHandlerForKeyword("㔈") -- 㔈 test1 e3 94 88 testing for byte match against e3 94 89
        TwitchChatPlayRequestBus.Broadcast.SetHandlerForKeywordToReturnFormattedResponse("ツ")
        self:IncrementMessageState()
    elseif self.messageState == "addeuropean" then
        TwitchChatPlayRequestBus.Broadcast.RemoveKeyword("試")
        TwitchChatPlayRequestBus.Broadcast.RemoveKeyword("㔈")
        TwitchChatPlayRequestBus.Broadcast.RemoveKeyword("ツ")
        TwitchChatPlayRequestBus.Broadcast.SetHandlerForKeywordToReturnFormattedResponse("größeren")
        TwitchChatPlayRequestBus.Broadcast.SetHandlerForKeyword("mañana")
        TwitchChatPlayRequestBus.Broadcast.SetHandlerForKeyword("тестування")
        self:IncrementMessageState()
    elseif self.messageState == "part" then
        --TwitchChatPlayRequestBus.Broadcast.SendChannelMessage("!part")
        TwitchChatPlayRequestBus.Broadcast.RemoveKeyword("größeren")
        TwitchChatPlayRequestBus.Broadcast.RemoveKeyword("mañana")
        TwitchChatPlayRequestBus.Broadcast.RemoveKeyword("тестування")
        Debug.Log("TwitchChatPlayRequestBus.Broadcast.LeaveChannel")
        TwitchChatPlayRequestBus.Broadcast.LeaveChannel()
        MeshComponentRequestBus.Event.SetVisibility(self.entityId, false)
        self:IncrementMessageState()
    elseif self.messageState == "end" then
        self:IncrementMessageState()
    else
        Debug.Log("Undefined state reached.")
        self:IncrementMessageState()
    end
    Debug.Log("MessageStates: " .. self.messageState)
end

-- Event Handlers

function test_TwitchChatPlay:OnTwitchChatPlayMessages(message)
    if self.messageStateId == 1 then
        currentKeywordState = self.MessagesStateHandler[self.messageStateId]
    else
        currentKeywordState = self.MessagesStateHandler[self.messageStateId -1]
    end
    TwitchChatPlayRequestBus.Broadcast.SendChannelMessage(message)
    Debug.Log("OnTwitchChatPlayMessages | ActivateKeywordMatching: " .. tostring(self.ActivateKeywordMatching) .. " | UseTwitchPattern: " .. tostring(self.UseTwitchPattern) .. " | MessageStates: " .. currentKeywordState .. " |==| " .. string.gsub(string.gsub(message, self.Properties.chatBotNick, "chatBotNick"), self.Properties.channelId, "channelId"))
end

function test_TwitchChatPlay:OnTwitchChatPlayFormattedMessages(userName, message)
    if self.messageStateId == 1 then
        currentKeywordState = self.MessagesStateHandler[self.messageStateId]
    else
        currentKeywordState = self.MessagesStateHandler[self.messageStateId -1]
    end
    if string.find(userName, self.Properties.channelId) and not string.find(userName, self.Properties.chatBotNick) then
        --this would be unexpected if two seperate accounts are used, but if only one account is used this is expected
        Debug.Log("OnTwitchChatPlayFormattedMessages channelId == userName (" .. self.Properties.channelId .. " == " .. userName .. ")")
    end
    Debug.Log(userName .. " | OnTwitchChatPlayFormattedMessages | ActivateKeywordMatching: " .. tostring(self.ActivateKeywordMatching) .. " | UseTwitchPattern: " .. tostring(self.UseTwitchPattern) .. " | MessageStates: " .. currentKeywordState .. " |==| " .. message)
end

function test_TwitchChatPlay:OnTick(deltaTime, timePoint)
    self.timer = self.timer + deltaTime
    if (self.timer > self.Properties.delay) then
        TwitchChatPlayRequestBus.Broadcast.SendChannelMessage("!trigger")
        TwitchChatPlayRequestBus.Broadcast.SendChannelMessage("!special")
        self:KeyWordStates()
        self.timer = 0.0
    end
end

function test_TwitchChatPlay:OnDeactivate()
    self.tickHandler:Disconnect()
    self.chatHandler:Disconnect()
end

return test_TwitchChatPlay