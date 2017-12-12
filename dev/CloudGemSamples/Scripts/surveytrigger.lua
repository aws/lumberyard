surveytrigger =
{
    Properties =
    {
    },
}

function surveytrigger:OnActivate()
	-- For handling tick events.
    self.tickBusHandler = TickBus.Connect(self,self.entityId);
	
	self.activeSurveysList = {}
	self.currentSurvey = {}
	self.currentQuestionIndex = 0
	self.displayTimer = 0.0
	self.currentQuestionEntityID = EntityId()
	

    self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("Levels/InGameSurveySample/UI/InGameSurveyMenu.uicanvas")
	--UiCanvasBus.Event.SetEnabled(self.canvasEntityId, false)

    self.notificationHandler = CloudGemInGameSurveyNotificationBus.Connect(self, self.entityId)

	local emptyString = ""
	--CloudGemInGameSurveyRequestBus.Event.GetActiveSurvey_metadata(self.entityId, 10, emptyString, emptyString, nil)
	
end

function surveytrigger:OnTick(deltaTime,timePoint)
	

end

function surveytrigger:OnDeactivate()
    self.notificationHandler:Disconnect();
    self.tickBusHandler:Disconnect();
	self.buttonHandler:Disconnect()
end

-- STEP ONE SURVEY METADATA
function surveytrigger:OnGetActiveSurvey_metadataRequestSuccess(response)

	Debug.Log("OnGetActiveSurvey_metadataRequestSuccess succeeded")
    --self.activeSurveysList = {}
    Debug.Log("Response survey metadatalist: "..tostring(#response.metadata_list));
   
	if #response.metadata_list >= 1 then
		UiCanvasBus.Event.SetEnabled(self.canvasEntityId, true)
		-- Listen for action strings broadcast by the canvas
		self.buttonHandler = UiCanvasNotificationBus.Connect(self, self.canvasEntityId)
	end
end

function surveytrigger:OnGetActiveSurvey_metadataRequestError(errorMsg)
	Debug.Log("GetActiveSurveys Error")
end

function surveytrigger:OnAction(entityId, actionName)
    Debug.Log(tostring(entityId) .. ": " .. actionName)
		
	if actionName == "SubmitResults" then
		UiCanvasBus.Event.SetEnabled(self.canvasEntityId, false)
    end
end

return surveytrigger