leaderboardmainmenu =
{
    Properties =
    {
        stat = {default = "score", description = "The stat to retrieve from the leaderboard" },
        localUser = {default = "LocalPlayer", description = "The name to submit scores under"}
    },
}

function leaderboardmainmenu:OnActivate()

    self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("Levels/LeaderboardSample/UI/LBMenu.uicanvas")

    -- Listen for action strings broadcast by the canvas
    self.buttonHandler = UiCanvasNotificationBus.Connect(self, self.canvasEntityId)

    local util = require("scripts.util")
    util.SetMouseCursorVisible(true)

    self.notificationHandler = CloudGemLeaderboardNotificationBus.Connect(self, self.entityId);
    self.submitCount = 0;
    math.randomseed(os.time());
end


function leaderboardmainmenu:OnDeactivate()
    self.notificationHandler:Disconnect();
	self.buttonHandler:Disconnect();
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

function leaderboardmainmenu:ShowScoresAroundPlayer(scores)
    local userIndex = 1;
    while scores[userIndex].user ~= self.Properties.localUser do
        Debug.Log("checked at index"..tostring(userIndex));
        userIndex = userIndex + 1;
        if userIndex >= #scores then
            Debug.Log("Could not find local user in scoreboard");
            self:ShowTopFiveSample(scores);
            return;
        end
    end
    Debug.Log("Found local player at ".. tostring(userIndex));
    local scoreList = {}
    local startIndex = userIndex - 2;
    if startIndex < 1 then
        startIndex = 1
    end
    for index = startIndex, startIndex + 5 do
        if #scores > index then
            table.insert(scoreList, scores[index]);
        end
    end
    self:UpdateScores(scoreList, self.Properties.stat);
end

function leaderboardmainmenu:OnPostLeaderboardRequestSuccess(response)
    if self.Properties.localUser == "" then
        self:ShowTopFiveSample(response.scores);
    else
        self:ShowTopFiveSample(response.scores);
        -- uncomment once problem is fixed with setting vector properties on objects
        -- self:ShowScoresAroundPlayer(response.scores);
    end

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
        scoreObj.value = math.random(1,100000000) * (1 + (math.random(0,99999999)/100000000));
        CloudGemLeaderboardRequestBus.Event.PostScore(self.entityId, scoreObj, nil);
    end
    self.submitCount = self.submitCount + 11;
end

function leaderboardmainmenu:SubmitScoreForLocalUser(score)
    if self.Properties.localUser == "" then
        Debug.Log("There is no localUser defined");
        return;
    end
    local scoreObj = CloudGemLeaderboard_SingleScore();
    scoreObj.stat = self.Properties.stat;
    scoreObj.user = self.Properties.localUser;
    scoreObj.value = score;
    CloudGemLeaderboardRequestBus.Event.PostScore(self.entityId, scoreObj, nil);
end

function leaderboardmainmenu:OnAction(entityId, actionName)
    if actionName == "GetScores" then
        local additionalData = CloudGemLeaderboard_AdditionalLeaderboardRequestData();
        if self.Properties.localUser ~= "" then
            Debug.Log("Adding "..self.Properties.localUser.. " to leaderboard request");
            additionalData.users:push_back(self.Properties.localUser);
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
