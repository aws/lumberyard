surveymenu =
{
	Properties =
	{
		
	},
}

function surveymenu:OnActivate()
	-- For handling tick events.
	self.tickBusHandler = TickBus.Connect(self,self.entityId);
	
	self.activeSurveysList = {}
	self.currentSurvey = {}
	self.answerList = CloudGemInGameSurvey_AnswerList()
	self.currentQuestionIndex = 1
	self.currentQuestionEntityID = EntityId()
	self.maxAnswerChars = 1
	self.checkboxConnections = {}
	
	-- Let's load the canvas
	self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("Levels/InGameSurveySample/UI/InGameSurveyMenu.uicanvas")
	--UiCanvasBus.Event.SetEnabled(self.canvasEntityId, false)
	
	-- SHYNE --
	self.CheckboxQuestionBody = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "CheckBoxQuestion")
	UiElementBus.Event.SetIsEnabled(self.CheckboxQuestionBody, false)
	
	self.RadioButtonQuestionBody = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "RadioButtonQuestion")
	UiElementBus.Event.SetIsEnabled(self.RadioButtonQuestionBody, false)
	
	self.ScaleQuestionBody = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "SliderQuestion")
	UiElementBus.Event.SetIsEnabled(self.ScaleQuestionBody, false)
	self.SliderHandleText = EntityId()
	
	self.TextInputQuestionBody = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "TextInputQuestion")
	UiElementBus.Event.SetIsEnabled(self.TextInputQuestionBody, false)
	
	self.NextQuestionButton = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "NextQuestionButton")
	UiElementBus.Event.SetIsEnabled(self.NextQuestionButton, true);
	
	self.SubmitResultsButton = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "SubmitResultsButton")
	UiElementBus.Event.SetIsEnabled(self.SubmitResultsButton, false)

	self.BackButton = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "BackButton")
	UiElementBus.Event.SetIsEnabled(self.BackButton, false)
	
	-- Listen for action strings broadcast by the canvas
	self.buttonHandler = UiCanvasNotificationBus.Connect(self, self.canvasEntityId)

	local util = require("scripts.util")
	util.SetMouseCursorVisible(true)
	
	self.notificationHandler = CloudGemInGameSurveyNotificationBus.Connect(self, self.entityId)
	local emptyString = ""
	CloudGemInGameSurveyRequestBus.Event.GetActiveSurvey_metadata(self.entityId, 10, emptyString, emptyString, nil)
end

function surveymenu:OnTick(deltaTime,timePoint)
	

end

function surveymenu:OnDeactivate()
	self.notificationHandler:Disconnect();
	self.tickBusHandler:Disconnect();
	self.buttonHandler:Disconnect()
	self:ResetCheckboxConnections(nil, 0)
end


function surveymenu:OnCheckboxStateChange(entityId, checked)
	self:UpdateButtons()
end

function surveymenu:ResetCheckboxConnections(layout, checkboxCount)
	for idx = 1, #self.checkboxConnections do
		self.checkboxConnections[idx]:Disconnect()
	end
	
	self.checkboxConnections = {}
	
	for idx = 1, checkboxCount do
		local checkboxElement = UiElementBus.Event.GetChild(layout, idx - 1)
		local handler = UiCheckboxNotificationBus.Connect(self, checkboxElement)
		table.insert(self.checkboxConnections, handler)
	end
end

-- STEP ONE SURVEY METADATA
function surveymenu:OnGetActiveSurvey_metadataRequestSuccess(response)

	Debug.Log("OnGetActiveSurvey_metadataRequestSuccess succeeded")
	
	for surveyCount = 1, #response.metadata_list do
		table.insert(self.activeSurveysList, response.metadata_list[surveyCount])	
	end
	
	if #self.activeSurveysList >= 1 then
		CloudGemInGameSurveyRequestBus.Event.GetActiveSurveys(self.entityId, self.activeSurveysList[1].survey_id, 0, 10000, nil)
	end
end

function surveymenu:OnGetActiveSurvey_metadataRequestError(errorMsg)
	Debug.Log("GetActiveSurveys Error " .. errorMsg)
end

--STEP 2 GET THE CURRENT SURVEY
function surveymenu:OnGetActiveSurveysRequestSuccess(response)
	Debug.Log("GetActiveSurveys succeeded")
	self.currentSurvey = response
	self.currentQuestionIndex = 1
	self.answerList = CloudGemInGameSurvey_AnswerList()
	
	for questionCount = 1, #self.currentSurvey.questions do
		-- Prebuild our answer list
		local answer = CloudGemInGameSurvey_Answer()
		answer.question_id = self.currentSurvey.questions[questionCount].question_id
		self.answerList.answers:push_back(answer)
	end
	self:UpdateQuestion()
	self:UpdateButtons()
end

function surveymenu:OnGetActiveSurveysRequestError(errorMsg)
	Debug.Log("GetActiveSurveys Error")
end



function surveymenu:OnAction(entityId, actionName)
	if actionName == "NextQuestion" then
		self:SaveResults()
		self.currentQuestionIndex = self.currentQuestionIndex + 1
		self:UpdateQuestion()
		self:UpdateButtons()
	elseif actionName == "PreviousQuestion" then
		self:SaveResults()
		self.currentQuestionIndex = self.currentQuestionIndex - 1
		self:UpdateQuestion()
		self:UpdateButtons()		
	elseif actionName == "SubmitResults" then
		-- Still gotta save, I'm on the last question
		self:SaveResults()
		self:SubmitResults()
	elseif actionName == "TextInputChange" then
		self:UpdateCharCounter()
	elseif actionName == "SliderValueChange" then
		self:UpdateSliderValueDisplay()
	end
end

function surveymenu:UpdateQuestion()
	
	-- More questions left?
	if self.currentQuestionIndex <= #self.currentSurvey.questions then
		local question = self.currentSurvey.questions[self.currentQuestionIndex]
		local questionType = question.type
		local multiple_select = question.multiple_select
		-- Should we check if this is the same question type to avoid hiding and showing
		if self.currentQuestionEntityID:IsValid() then
			UiElementBus.Event.SetIsEnabled(self.currentQuestionEntityID, false);
		end
		
		-- Predefine types will use radio buttons or checkboxes
		if questionType == "predefined" then
			-- Select the proper type of question's body
			-- Checkboxes
			if multiple_select == true then
				self.currentQuestionEntityID = self.CheckboxQuestionBody
			-- Radio buttons
			else
				-- Show this new type of question's body
				self.currentQuestionEntityID = self.RadioButtonQuestionBody
			end
			
			UiElementBus.Event.SetIsEnabled(self.currentQuestionEntityID, true)
				
			local numPredefines = #question.predefines
			
			-- Here we use a dynamic layout so retrieve it
			local layoutColumnEntityId = UiElementBus.Event.GetChild(self.currentQuestionEntityID, 0)
			-- Set the number of checkboxes that we want
			UiDynamicLayoutBus.Event.SetNumChildElements(layoutColumnEntityId, 0)
			UiDynamicLayoutBus.Event.SetNumChildElements(layoutColumnEntityId, numPredefines)
			
			for predefineCount = 1, numPredefines do
				local checkboxElement = UiElementBus.Event.GetChild(layoutColumnEntityId, predefineCount-1)
				local checkboxText = UiElementBus.Event.FindChildByName(checkboxElement, "Text")
				UiTextBus.Event.SetText(checkboxText, tostring(question.predefines[predefineCount]))
			end
			
			self:ResetCheckboxConnections(layoutColumnEntityId, numPredefines)
			
			-- since we support that back option, we need to restore the saved values
			-- Checkboxes
			if multiple_select == true then
				for selectedIndex = 1, #self.answerList.answers[self.currentQuestionIndex].answer do
					local selectedCheckbox = tonumber(self.answerList.answers[self.currentQuestionIndex].answer[selectedIndex])
					local checkBox = UiElementBus.Event.GetChild(layoutColumnEntityId, selectedCheckbox)
					UiCheckboxBus.Event.SetState(checkBox, true)
				end
				
			-- Radio buttons
			else	
				if #self.answerList.answers[self.currentQuestionIndex].answer >= 1 then
					local selectedRadio = tonumber(self.answerList.answers[self.currentQuestionIndex].answer[1])
					local radioButton = UiElementBus.Event.GetChild(layoutColumnEntityId, selectedRadio)
					local radioButtonGroup = UiRadioButtonBus.Event.GetGroup(radioButton)
					UiRadioButtonGroupBus.Event.SetState(radioButtonGroup, radioButton, true)
				else
				    local radioButton = UiElementBus.Event.GetChild(layoutColumnEntityId, 0)
					local radioButtonGroup = UiRadioButtonBus.Event.GetGroup(radioButton)
					UiRadioButtonGroupBus.Event.SetState(radioButtonGroup, radioButton, true)
				end
			end
			
		-- Scale type should use a slider with a label on the left and a label on the right
		elseif questionType == "scale" then
			-- Show this new type of question's body
			self.currentQuestionEntityID = self.ScaleQuestionBody
			UiElementBus.Event.SetIsEnabled(self.currentQuestionEntityID, true)
			local scaleMin = question.min
			local scaleMax = question.max
			local scaleMinLabel = question.min_label
			local scaleMaxLabel = question.max_label
			
			-- Get the slider and set it's scale
			local sliderElement = UiElementBus.Event.FindChildByName(self.currentQuestionEntityID, "Slider")
			UiSliderBus.Event.SetMinValue(sliderElement, scaleMin)
			UiSliderBus.Event.SetMaxValue(sliderElement, scaleMax)
			UiSliderBus.Event.SetStepValue(sliderElement, 1)
			local initValue = math.floor((scaleMax-scaleMin)/2) + 1
			-- since we support that back option, we need to restore the saved values
			if #self.answerList.answers[self.currentQuestionIndex].answer >= 1 then
				initValue = tonumber(self.answerList.answers[self.currentQuestionIndex].answer[1])
			end
			UiSliderBus.Event.SetValue(sliderElement, initValue)
			--Set the min/max text labels
			local minLabelText = UiElementBus.Event.FindChildByName(sliderElement, "MinLabel")
			UiTextBus.Event.SetText(minLabelText, tostring(scaleMin).." "..tostring(scaleMinLabel))
			local maxLabelText = UiElementBus.Event.FindChildByName(sliderElement, "MaxLabel")
			UiTextBus.Event.SetText(maxLabelText, tostring(scaleMax).." "..tostring(scaleMaxLabel))
			self:UpdateSliderValueDisplay()
			
			
			
		-- text will use a text box limited to a certain number of characters
		elseif questionType == "text" then
			self.currentQuestionEntityID = self.TextInputQuestionBody
			UiElementBus.Event.SetIsEnabled(self.currentQuestionEntityID, true)
			local textInputElement = UiElementBus.Event.FindChildByName(self.TextInputQuestionBody, "Text Input")
			self.maxAnswerChars = question.max_chars
			UiTextInputBus.Event.SetMaxStringLength(textInputElement, self.maxAnswerChars)
			-- since we support that back option, we need to restore the saved values
			if #self.answerList.answers[self.currentQuestionIndex].answer >= 1 then
				local textValue = self.answerList.answers[self.currentQuestionIndex].answer[1]
				UiTextInputBus.Event.SetText(textInputElement, textValue)
			else
				UiTextInputBus.Event.SetText(textInputElement, "")
			end
			self:UpdateCharCounter()
			
		end
		
		local displayEntity = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "SurveyQuestion")
		UiTextBus.Event.SetText(displayEntity, self.currentSurvey.questions[self.currentQuestionIndex].title)
	end
end

function surveymenu:UpdateCharCounter()
	local textInputElement = UiElementBus.Event.FindChildByName(self.TextInputQuestionBody, "Text Input")
	local inputText = UiTextInputBus.Event.GetText(textInputElement)
	local counterString = tostring(#inputText).."/"..tostring(self.maxAnswerChars)
			
	local charCounterText = UiElementBus.Event.FindChildByName(textInputElement, "CharCounterText")
	UiTextBus.Event.SetText(charCounterText, counterString)
end

function surveymenu:UpdateSliderValueDisplay()
	local sliderElement = UiElementBus.Event.FindChildByName(self.currentQuestionEntityID, "Slider")
	local sliderValue = UiSliderBus.Event.GetValue(sliderElement)
	if self.SliderHandleText:IsValid() == false then
		local trackElement = UiElementBus.Event.FindChildByName(sliderElement, "Track")
		local handleElement = UiElementBus.Event.FindChildByName(trackElement, "Handle")
		self.SliderHandleText = UiElementBus.Event.FindChildByName(handleElement, "Text")
	end
	UiTextBus.Event.SetText(self.SliderHandleText, tostring(sliderValue))
end

function surveymenu:UpdateButtons()
	if self.currentSurvey ~= nil then
		local canAdvance = true
		
		if self.currentQuestionIndex <= #self.currentSurvey.questions then
			local question = self.currentSurvey.questions[self.currentQuestionIndex]
			local questionType = question.type
			local multiple_select = question.multiple_select
			
			if questionType == "predefined" and multiple_select then
				-- Only allow advancement if at least one of the checkboxes is selected
				local numPredefines = #question.predefines
				local layoutColumnEntityId = UiElementBus.Event.GetChild(self.CheckboxQuestionBody, 0)
				canAdvance = false

				for predefineCount = 1, numPredefines do
					local checkboxElement = UiElementBus.Event.GetChild(layoutColumnEntityId, predefineCount-1)
					local isChecked = UiCheckboxBus.Event.GetState(checkboxElement)
					canAdvance = canAdvance or isChecked
					Debug.Log("Checked: " .. tostring(canAdvance))
				end
			end
		end
	
		local numQuestions = #self.currentSurvey.questions
		if self.currentQuestionIndex >= numQuestions then
			-- Disable next question button and enable Submit button
			UiElementBus.Event.SetIsEnabled(self.NextQuestionButton, false)
			UiElementBus.Event.SetIsEnabled(self.SubmitResultsButton, canAdvance)
		else
			UiElementBus.Event.SetIsEnabled(self.NextQuestionButton, canAdvance)
			UiElementBus.Event.SetIsEnabled(self.SubmitResultsButton, false)
		end
		
		if self.currentQuestionIndex <= 1 then
			UiElementBus.Event.SetIsEnabled(self.BackButton, false)
		elseif numQuestions > 1 then
			UiElementBus.Event.SetIsEnabled(self.BackButton, true)
		end
	end
end

function surveymenu:SaveResults()
    if self.currentQuestionIndex > #self.answerList.answers then
	    Debug.Log("Skipping SaveResults Current index: "..tonumber(self.currentQuestionIndex).." Answer List Size: "..tonumber(#self.answerList.answers))
	    return
    end

    Debug.Log("Saving results")
    -- Flush whatever is already in there to support the back option
    self.answerList.answers[self.currentQuestionIndex].answer:clear()

	local newAnswer = CloudGemInGameSurvey_Answer()
	
	-- Checkboxes
	if self.currentQuestionEntityID == self.CheckboxQuestionBody then
		-- Here we use a dynamic layout so retrieve it
		local layoutColumnEntityId = UiElementBus.Event.GetChild(self.currentQuestionEntityID, 0)
		local numChilds = UiElementBus.Event.GetNumChildElements(layoutColumnEntityId)
		
		for childIndex = 1, numChilds do
			local checkBox = UiElementBus.Event.GetChild(layoutColumnEntityId, childIndex-1)
			-- If that checkbox is checked
			if UiCheckboxBus.Event.GetState(checkBox) == true then
				-- Pushing the answer as 0 based because the CGP and the rest of the planet is 0 based
				newAnswer.answer:push_back(tostring(childIndex-1))
			end
		end
		
	-- Radio buttons
	elseif self.currentQuestionEntityID == self.RadioButtonQuestionBody then
		-- Here we use a dynamic layout so retrieve it
		local layoutColumnEntityId = UiElementBus.Event.GetChild(self.currentQuestionEntityID, 0)
		local numChilds = UiElementBus.Event.GetNumChildElements(layoutColumnEntityId)
		
		for childIndex = 1, numChilds do
			local radioButton = UiElementBus.Event.GetChild(layoutColumnEntityId, childIndex-1)
			-- If that checkbox is checked
			if UiRadioButtonBus.Event.GetState(radioButton) == true then
				-- Pushing the answer as 0 based because the CGP and the rest of the planet is 0 based
				newAnswer.answer:push_back(tostring(childIndex-1))
			end
		end
	-- Scale
	elseif self.currentQuestionEntityID == self.ScaleQuestionBody then
		local sliderElement = UiElementBus.Event.FindChildByName(self.currentQuestionEntityID, "Slider")
		local sliderValue = UiSliderBus.Event.GetValue(sliderElement)
		newAnswer.answer:push_back(tostring(sliderValue))
	-- Text input
	elseif self.currentQuestionEntityID == self.TextInputQuestionBody then
		local textInputElement = UiElementBus.Event.FindChildByName(self.TextInputQuestionBody, "Text Input")
		local inputText = UiTextInputBus.Event.GetText(textInputElement)
		if #inputText == 0 then
			inputText = ""
		end 
		newAnswer.answer:push_back(inputText)
	end
	
	newAnswer.question_id = self.answerList.answers[self.currentQuestionIndex].question_id
	self.answerList.answers[self.currentQuestionIndex] = newAnswer
end
		
function surveymenu:SubmitResults()
	CloudGemInGameSurveyRequestBus.Event.PostActiveSurveysAnswers(self.entityId, self.currentSurvey.survey_id, self.answerList, nil)
end


function surveymenu:OnPostActiveSurveysAnswersRequestSuccess(response)
	Debug.Log("PostActiveSurveysAnswers Success")
	UiCanvasBus.Event.SetEnabled(self.canvasEntityId, false)
end

function surveymenu:OnPostActiveSurveysAnswersRequestError(errorMsg)
	Debug.Log("PostActiveSurveysAnswers Error: "..tostring(errorMsg))
end

return surveymenu