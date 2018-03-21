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
        if #scores > scoreCount then
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


function leaderboardmainmenu:SubmitRandomScores()
    for count = self.submitCount, self.submitCount+10 do
        local user = "GeneratedUser_"..tostring(count);
        Debug.Log("submitting score for "..user);
        local scoreObj = CloudGemLeaderboard_SingleScore();
        scoreObj.user = user;
        scoreObj.stat = self.Properties.stat;
        scoreObj.value = math.random(1,40) * (1 + (math.random(0,99)/100));
        CloudGemLeaderboardRequestBus.Event.PostScore(self.entityId, scoreObj, nil);
    end
    self.submitCount = self.submitCount + 11;
end

function leaderboardmainmenu:SubmitScoreForLocalUser(score)
    if self.username == "" then
        Debug.Log("Log in using CloudGemPlayerAccount sample level");
        return;
    end
    local scoreObj = CloudGemLeaderboard_SingleScore();
    scoreObj.stat = self.Properties.stat;
    scoreObj.user = self.username;
    scoreObj.value = score;
    CloudGemLeaderboardRequestBus.Event.PostScore(self.entityId, scoreObj, nil);
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
    elseif actionName == "RandomBatch" then
        self:SubmitRandomScores();
        Debug.Log("Submiting random score batch");
    elseif actionName == "SubmitUser10" then
        self:SubmitScoreForLocalUser(10.0);
        Debug.Log("Submiting score 10");
    elseif actionName == "SubmitUser25" then
        self:SubmitScoreForLocalUser(25.0);
        Debug.Log("Submiting score 25");
    elseif actionName == "SubmitUser50" then
        self:SubmitScoreForLocalUser(50.0);
        Debug.Log("Submiting score 50");
    end
end

function leaderboardmainmenu:OnPostScoreRequestSuccess(response)
    Debug.Log("Successfully submitted score for "..response.user ..":"..response.value);
end

function leaderboardmainmenu:OnPostScoreRequestError(error)
    Debug.Log("Error submitting score: ".. error.message);
end

function leaderboardmainmenu:GenerateSampleScore()
    local scoreList = {}
    local scoreObject = CloudGemLeaderboard_SingleScore();
    scoreObject.stat = "score";
    scoreObject.user = "Player 1";
    scoreObject.value = 14.0;
    scoreObject.estimated_rank = 2;

    local scoreObject2 = CloudGemLeaderboard_SingleScore();
    scoreObject2.stat = "score";
    scoreObject2.user = "Player 2";
    scoreObject2.value = 15.0;
    scoreObject2.estimated_rank = 1;

    table.insert(scoreList, scoreObject);
    table.insert(scoreList, scoreObject2);
    self:UpdateScores(scoreList, self.Properties.stat);
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
