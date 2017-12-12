surveymenu =
{
    Properties =
    {
		SubmitResultsButton = {default = EntityId()},
		NextQuestionButton = {default = EntityId()},
		CheckboxQuestionBody = {default = EntityId()},
		RadioButtonQuestionBody = {default = EntityId()},
		SliderQuestionBody = {default = EntityId()},
		TextInputQuestionBody = {default = EntityId()}
    },
}

function surveymenu:OnActivate()
	-- For handling tick events.
    self.tickBusHandler = TickBus.Connect(self,self.entityId);
	
	self.activeSurveysList = {}
	self.currentSurvey = {}
	self.currentQuestionIndex = 0
	self.currentQuestionEntityID = EntityId()
	
	-- SHYNE --
	UiElementBus.Event.SetIsEnabled(self.Properties.CheckboxQuestionBody, false)
	UiElementBus.Event.SetIsEnabled(self.Properties.RadioButtonQuestionBody, false)
	UiElementBus.Event.SetIsEnabled(self.Properties.SliderQuestionBody, false)
	UiElementBus.Event.SetIsEnabled(self.Properties.TextInputQuestionBody, false)
	
	UiElementBus.Event.SetIsEnabled(self.Properties.NextQuestionButton, true);
	UiElementBus.Event.SetIsEnabled(self.Properties.SubmitResultsButton, false)

    --self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("Levels/InGameSurveySample/UI/InGameSurveyMenu.uicanvas")
	--UiCanvasBus.Event.SetEnabled(self.canvasEntityId, false)

    -- Listen for action strings broadcast by the canvas
	self.canvasEntityId = UiElementBus.Event.GetCanvas(self.entityId);
	self.buttonHandler = UiCanvasNotificationBus.Connect(self, self.canvasEntityId)

    local util = require("scripts.util")
    util.SetMouseCursorVisible(true)
	
    self.notificationHandler = CloudGemInGameSurveyNotificationBus.Connect(self, self.entityId)
	Debug.Log("CloudGemInGameSurveyRequestBus.Event.GetActiveSurvey_metadata")
	local emptyString = ""
	CloudGemInGameSurveyRequestBus.Event.GetActiveSurvey_metadata(self.entityId, 10, emptyString, emptyString, nil)
	
end

function surveymenu:OnTick(deltaTime,timePoint)
	

end

function surveymenu:OnDeactivate()
    self.notificationHandler:Disconnect();
    self.tickBusHandler:Disconnect();
	self.buttonHandler:Disconnect()
end

-- STEP ONE SURVEY METADATA
function surveymenu:OnGetActiveSurvey_metadataRequestSuccess(response)

	Debug.Log("OnGetActiveSurvey_metadataRequestSuccess succeeded")
    --self.activeSurveysList = {}
    Debug.Log("Response survey metadatalist: "..tostring(#response.metadata_list));
    
    for surveyCount = 1, #response.metadata_list do
        Debug.Log(tostring(response.metadata_list[surveyCount].survey_id))
		table.insert(self.activeSurveysList, response.metadata_list[surveyCount])	
    end
	
	if #self.activeSurveysList >= 1 then
		CloudGemInGameSurveyRequestBus.Event.GetActiveSurveys(self.entityId, self.activeSurveysList[1].survey_id, 0, 0, nil)
	end
end

function surveymenu:OnGetActiveSurvey_metadataRequestError(errorMsg)
	Debug.Log("GetActiveSurveys Error")
end

--STEP 2 GET THE CURRENT SURVEY
function surveymenu:OnGetActiveSurveysRequestSuccess(response)
    Debug.Log("GetActiveSurveys succeeded")
	self.currentSurvey = response
	self.currentQuestionIndex = 0
	
    --self.displayTimer = 0.0
    Debug.Log("Response survey: "..tostring(self.currentSurvey.survey_id));
    
    for questionCount = 1, #self.currentSurvey.questions do
        Debug.Log(tostring(self.currentSurvey.questions[questionCount].title))
    end
	--UiCanvasBus.Event.SetEnabled(self.canvasEntityId, true)
	self:UpdateQuestion()
end

function surveymenu:OnGetActiveSurveysRequestError(errorMsg)
    Debug.Log("GetActiveSurveys Error")
end



function surveymenu:OnAction(entityId, actionName)
    Debug.Log(tostring(entityId) .. ": " .. actionName)

    if actionName == "NextQuestion" then
       	self:UpdateQuestion()
		if self.currentQuestionIndex >= #self.currentSurvey.questions then
			UiElementBus.Event.SetIsEnabled(entityId, false);
			UiElementBus.Event.SetIsEnabled(self.Properties.SubmitResultsButton, true) 
		end
		
	elseif actionName == "SubmitResults" then
		self:SubmitResults()
    end
end

function surveymenu:UpdateQuestion()
	self.currentQuestionIndex = self.currentQuestionIndex + 1
	-- More questions left?
	if self.currentQuestionIndex <= #self.currentSurvey.questions then
		local question = self.currentSurvey.questions[self.currentQuestionIndex]
		local questionType = question.type
		Debug.Log("Question Type =" .. tostring(questionType))
		-- Should we check if this is the same question type to avoid hiding and showing
		if self.currentQuestionEntityID:IsValid() then
			UiElementBus.Event.SetIsEnabled(self.currentQuestionEntityID, false);
		end
		
		-- Predefine types will use radio buttons or checkboxes
		if questionType == "predefined" then
			-- Show this new type of question's body
			self.currentQuestionEntityID = self.Properties.CheckboxQuestionBody
			UiElementBus.Event.SetIsEnabled(self.currentQuestionEntityID, true)
			
			local numPredefines = #question.predefines
			Debug.Log("Type predefined with num predefines =" .. tostring(numPredefines))
			
			-- Here we use a dynamic layout so retrieve it
			local layoutColumnEntityId = UiElementBus.Event.GetChild(self.currentQuestionEntityID, 0)
			-- Set the number of checkboxes that we want
			UiDynamicLayoutBus.Event.SetNumChildElements(layoutColumnEntityId, numPredefines)
			
			for predefineCount = 1, numPredefines do
				local checkboxElement = UiElementBus.Event.GetChild(layoutColumnEntityId, predefineCount-1)
				local checkboxText = UiElementBus.Event.FindChildByName(checkboxElement, "Text")
				UiTextBus.Event.SetText(checkboxText, tostring(question.predefines[predefineCount]))
        			Debug.Log(tostring(question.predefines[predefineCount]))
			end
		-- Scale type should use a slider with a label on the left and a label on the right
		elseif questionType == "scale" then
			-- Show this new type of question's body
			self.currentQuestionEntityID = self.Properties.ScaleQuestionBody
			UiElementBus.Event.SetIsEnabled(self.currentQuestionEntityID, true)
			local scaleMin = question.min
			local scaleMax = question.max
			local scaleMinLabel = question.min_label
			local scaleMaxLabel = question.max_label
		-- text will use a text box limited to a certain number of characters
		elseif questionType == "text" then
			self.currentQuestionEntityID = self.Properties.TextInputQuestionBody
			UiElementBus.Event.SetIsEnabled(self.currentQuestionEntityID, true)
			local maxAnswerChars = question.max_chars
			Debug.Log("Type text with max char:" .. tostring(maxAnswerChars))
		end
		
    	local displayEntity = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "SurveyQuestion")
    	UiTextBus.Event.SetText(displayEntity, self.currentSurvey.questions[self.currentQuestionIndex].title)
	end
end

function surveymenu:SubmitResults()
end

return surveymenu