leaderboarddedicated =
{
    Properties =
    {
        stat = {default = "score", description = "The stat to retrieve from the leaderboard" },
        localUser = {default = "LocalPlayer", description = "The name to submit scores under"}
    }
}

function leaderboarddedicated:OnActivate()
    self.notificationHandler = CloudGemLeaderboardNotificationBus.Connect(self, self.entityId);
    self.tickBusHandler = TickBus.Connect(self, self.entityId);
    self.submitCount = 0;
end

function leaderboarddedicated:OnTick(deltaTime, timePoint)
    local scoreObj = CloudGemLeaderboard_SingleScore();
    self:SubmitRandomScores();
    Debug.Log("Submitting score for leaderboard: " .. scoreObj.stat .. ", username: " .. scoreObj.user .. ", value: " .. scoreObj.value);
    self.tickBusHandler:Disconnect();
end

function leaderboarddedicated:OnPostScoreRequestSuccess(response)
    Debug.Log("Successfully submitted score for "..response.user ..":"..response.value);
end

function leaderboarddedicated:OnPostScoreRequestError(error)
    Debug.Log("Error submitting score: ".. error.message);
end

function leaderboarddedicated:SubmitRandomScores()
    for count = self.submitCount, self.submitCount+10 do
        local user = "GeneratedUser_"..tostring(count);
        Debug.Log("submitting score for "..user);
        local scoreObj = CloudGemLeaderboard_SingleScore();
        scoreObj.user = user;
        scoreObj.stat = self.Properties.stat;
        scoreObj.value = math.random(1,40) * (1 + (math.random(0,99)/100));
        CloudGemLeaderboardRequestBus.Event.PostScoreDedicated(self.entityId, scoreObj, nil);
    end
    self.submitCount = self.submitCount + 11;
end


function leaderboarddedicated:OnDeactivate()
    self.notificationHandler:Disconnect();
end

return leaderboarddedicated