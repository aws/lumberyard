local accountutils = require "Scripts/LeaderboardAccountUtils"

leaderboardmainmenu =
{
    Properties =
    {
        stat = {default = "score", description = "The stat to retrieve from the leaderboard" },
        localUser = {default = "LocalPlayer", description = "The name to submit scores under"}
    },
    tasks = {}
}

function leaderboardmainmenu:OnActivate()

    self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("Levels/LeaderboardSample/UI/LBMenu.uicanvas")
    self.PlayerAccountWarningText = "Using CloudGemPlayerAccount, look at console logs for instructions"
    -- Listen for action strings broadcast by the canvas
    self.buttonHandler = UiCanvasNotificationBus.Connect(self, self.canvasEntityId)
    self.accountutils = nil
    local util = require("scripts.util")
    util.SetMouseCursorVisible(true)
    self.tickBusHandler = TickBus.Connect(self, self.entityId);
    self.notificationHandler = CloudGemLeaderboardNotificationBus.Connect(self, self.entityId);
    self.submitCount = 0;
    math.randomseed(os.time());
    self.username = ''
    self:SetPlayerAccountWarningActive(false);

    self:CheckForPlayerAccountGem();
end

function leaderboardmainmenu:GetAccountUtils()
    if self.accountutils == nil then
        self.accountutils = accountutils:new(self)
    end
    return self.accountutils
end

function leaderboardmainmenu:CheckForPlayerAccountGem()
    self:GetAccountUtils():PlayerAccountsEnabled(
        function(serviceStatusResult)
            if serviceStatusResult.wasSuccessful then
                self:SetPlayerAccountWarningActive(true);
                Debug.Log("Player account gem is active, will use logged in user from PlayerAccountSample level")
                self:GetPlayerAccountInfo()
            else
                Debug.Log("Player account gem is NOT active")
                self.username = self.Properties.localUser
            end
        end
    )
end

function leaderboardmainmenu:SetPlayerAccountWarningActive(active)
    local label = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "PlayerAccountWarning")
    if active then
        UiTextBus.Event.SetText(label, self.PlayerAccountWarningText)
    else
        UiTextBus.Event.SetText(label, "")
    end
end

function leaderboardmainmenu:GetPlayerAccountInfo()
    self:GetAccountUtils():GetUsername(
        function(currentUserParameters, playerAccountParameters)
            local currentUserResult = unpack(currentUserParameters)
            if (currentUserResult.wasSuccessful) then
                self.username = currentUserResult.username;
            else
                Debug.Log("Did not find username from CloudGemPlayerAccount, if that gem is enabled sign in through the sample level");
                self.username = self.Properties.localUser
            end
            Debug.Log("using username "..self.username)
        end
    )
end

function leaderboardmainmenu:OnDeactivate()
    self.notificationHandler:Disconnect();
    self.buttonHandler:Disconnect();
    self.tickBusHandler:Disconnect();
    self.accountutils = nil;
end

function leaderboardmainmenu:OnTick(deltaTime, timePoint)
    local task = nil
    repeat
        task = table.remove(self.tasks)
        if task then
            task()
        end
    until not task
end

function leaderboardmainmenu:RunOnMainThread(task)
    table.insert(self.tasks, task)
end


function leaderboardmainmenu:ShowTopFiveSample(scores)
    local scoreList = {}
    for scoreCount = 1, 5 do -- just show first 5
        if #scores >= scoreCount then
            table.insert(scoreList, scores[scoreCount]);
        end
    end
    self:UpdateScores(scoreList, self.Properties.stat);
end

function leaderboardmainmenu:OnPostLeaderboardRequestSuccess(response)
    self:ShowTopFiveSample(response.scores);
end

function leaderboardmainmenu:OnPostLeaderboardRequestError(error)
   Debug.Log("Error getting leaderboard");
end


function leaderboardmainmenu:OnAction(entityId, actionName)
    if actionName == "GetScores" then
        local additionalData = CloudGemLeaderboard_AdditionalLeaderboardRequestData();
        if self.username ~= "" then
            Debug.Log("Adding "..self.username.. " to leaderboard request");
            additionalData.users:push_back(self.username);
        end
        Debug.Log("getting leaderboard for " .. self.Properties.stat);
        CloudGemLeaderboardRequestBus.Event.PostLeaderboard(self.entityId, self.Properties.stat, additionalData, nil);
        -- self:GenerateSampleScore();
    end
end

function leaderboardmainmenu:UpdateScores(scoreList, metricName)
    local uiText = "Scores : ".. tostring(metricName)
    local displayEntity = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "Title")
    UiTextBus.Event.SetText(displayEntity, uiText)

    for scoreCount = 0, table.getn(scoreList) - 1 do
        --tables are 1 based
        local tableIndex = scoreCount + 1
        local textFieldName = "Score".. tostring(tableIndex)
        Debug.Log("Find Name for : " .. textFieldName);
        local displayEntity = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, textFieldName)
        local uiText = tostring(scoreList[tableIndex].estimated_rank)..". "..scoreList[tableIndex].user .. " : " ..scoreList[tableIndex].value
        UiTextBus.Event.SetText(displayEntity, uiText)
    end
end

return leaderboardmainmenu
