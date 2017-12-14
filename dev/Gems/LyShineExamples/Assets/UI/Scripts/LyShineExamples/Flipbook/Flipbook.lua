----------------------------------------------------------------------------------------------------
--
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--
--
----------------------------------------------------------------------------------------------------

local SequenceStates = 
{
	Properties = 
	{
		FlipbookContent = {default = EntityId()},
		StateText = {default = EntityId()},
		PlayStateText = {default = EntityId()},
		PlayStateImage = {default = EntityId()},
		StartButton = {default = EntityId()},
		StopButton = {default = EntityId()},
		LoopTypeDropdown = {default = EntityId()},
	},
}

function SequenceStates:OnActivate()
	self.requiresInit = true
    self.numLoops = 0
    self.sequenceStartedString = "Sequence Started"
	self.tickBusHandler = TickBus.Connect(self);
end

function SequenceStates:OnTick(deltaTime, timePoint)
	if self.requiresInit == true then
		self.requiresInit = false
	
		self.canvas = UiElementBus.Event.GetCanvas(self.entityId)
		
		-- Handler for OnAction callbacks
		self.canvasNotificationBusHandler = UiCanvasNotificationBus.Connect(self, self.canvas)
		
		-- Handler for flipbook animation start/stop callbacks
		self.flipbookNotificationBusHandler = UiFlipbookAnimationNotificationsBus.Connect(self, self.Properties.FlipbookContent)
		
		-- Handler for LoopType dropd-down callback
		self.dropdownNotificationBusHandler = UiDropdownNotificationBus.Connect(self, self.Properties.LoopTypeDropdown)
		
		-- Initialize button states
		UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StartButton, true)
		UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StopButton, false)
	else
		local isPlaying = UiFlipbookAnimationBus.Event.IsPlaying(self.Properties.FlipbookContent)
		if isPlaying == true then
			UiImageBus.Event.SetColor(self.Properties.PlayStateImage, Color(0, 255, 0))
			UiTextBus.Event.SetColor(self.Properties.PlayStateText, Color(0, 255, 0))
		else
			UiImageBus.Event.SetColor(self.Properties.PlayStateImage, Color(255, 0, 0))
			UiTextBus.Event.SetColor(self.Properties.PlayStateText, Color(255, 0, 0))
		end
	end										
end


function SequenceStates:OnDeactivate()
	self.tickBusHandler:Disconnect()
	self.canvasNotificationBusHandler:Disconnect()
	self.flipbookNotificationBusHandler:Disconnect()
	self.dropdownNotificationBusHandler:Disconnect()
end

function SequenceStates:OnAction(entityId, actionName)
	if actionName == "StartPressed" then
		UiFlipbookAnimationBus.Event.Start(self.Properties.FlipbookContent)
	elseif actionName == "StopPressed" then	
		UiFlipbookAnimationBus.Event.Stop(self.Properties.FlipbookContent)
	end
end

function SequenceStates:OnAnimationStarted()
	-- Setup button states
	UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StartButton, false)
	UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StopButton, true)
	
	-- Set sequence state text
	UiTextBus.Event.SetText(self.Properties.StateText, self.sequenceStartedString)
    
    self.numLoops = 0
end

function SequenceStates:OnAnimationStopped()
	-- Update button states	
	UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StartButton, true)
	UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StopButton, false)

	-- Set sequence state text
	UiTextBus.Event.SetText(self.Properties.StateText, "Sequence Stopped")
end

function SequenceStates:OnLoopSequenceCompleted()
    self.numLoops = self.numLoops + 1

    local pluralString = ""
    if self.numLoops > 1 then
        pluralString = "s"
    end

    UiTextBus.Event.SetText(self.Properties.StateText, self.sequenceStartedString .. " (" .. tostring(self.numLoops) .. " loop" .. pluralString .. ")")
end

function SequenceStates:OnDropdownValueChanged(optionEntityId)
	local dropdownText = UiDropdownOptionBus.Event.GetTextElement(optionEntityId)
	local textValue = UiTextBus.Event.GetText(dropdownText)
	
	if textValue == "None" then
		UiFlipbookAnimationBus.Event.SetLoopType(self.Properties.FlipbookContent, eUiFlipbookAnimationLoopType_None)
	elseif textValue == "Linear" then
		UiFlipbookAnimationBus.Event.SetLoopType(self.Properties.FlipbookContent, eUiFlipbookAnimationLoopType_Linear)
	elseif textValue =="PingPong" then
		UiFlipbookAnimationBus.Event.SetLoopType(self.Properties.FlipbookContent, eUiFlipbookAnimationLoopType_PingPong)
	end
end

return SequenceStates
