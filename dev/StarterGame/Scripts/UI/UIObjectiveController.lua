--[[
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--]]

local StateMachine = require "scripts/common/StateMachine"
-- this state machine is for the text on the UI for objective info display
-- there are two instances of this state machine running, one for primary and one for secondary
local uiobjectivecontroller =
{
	Properties =
	{
		--CodeEntity = { default = EntityId() }, -- to be removed
		
		Messages = {
			SetObjectiveText = { default = "SetObjectiveTextEvent", description = "Set the Objective Text" },
			SetObjectiveScreenID = { default = "SetObjectiveScreenIDEvent", description = "Set the Objective Screen Element" },
		},
		
		ScreenHookup = {
			TextID = { default = -1, description = "The ID text to manipulate." },
		},
		
		ObjectiveWaitCycleTime = { default = 0.5, description = "Cycle time on eht ewaiting fade" },
		ObjectiveWaitText = { default = "...", description = "The text to display when waiting for an objective" },
		ObjectiveCharScrambleTime = { default = 0.5, description = "The time to spend on each char scrambling it when writing or clearing it" },
		ObjectiveShownCycleTime = { default = 1.0, description = "The cycle time on the transparancy of a shown objective" },
		
		DebugStateMachine = { default = false, description = "Should this state machine show debug messages" },
	},
	ObjectiveStates = {
		-- No Current Objective
        Waiting = 
        {      
            OnEnter = function(self, sm)
				self.timer = 0.0;
				sm.currentText = sm.UserData.Properties.ObjectiveWaitText;
				StarterGameUIUtility.UITextSetter(sm.UserData.canvasEntityId, sm.TextID, sm.currentText);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.TextID, 0.0, 0.0);
            end,
            OnExit = function(self, sm)
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.TextID, 1, sm.UserData.Properties.ObjectiveWaitCycleTime);
            end,            
            OnUpdate = function(self, sm, deltaTime)
				self.timer = self.timer + deltaTime;
				local fadeValue = 1 - Math.Cos((self.timer / sm.UserData.Properties.ObjectiveWaitCycleTime) * math.pi * 2);
				StarterGameUIUtility.UITextSetter(sm.UserData.canvasEntityId, sm.TextID, sm.currentText);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.TextID, fadeValue, 0.0);
            end,
            Transitions =
            {
				Writing = 
				{
                    Evaluate = function(state, sm)
						return (sm.desiredText ~= "") and (sm.desiredText ~= sm.currentText);
                    end	
				}
            },
        },
		Writing = 
        {      
            OnEnter = function(self, sm)
				self.timer = 0.0;
				self.complete = false;
				sm.currentText = "";
				self.numCharsToWrite = sm.desiredText:len();
				StarterGameUIUtility.UITextSetter(sm.UserData.canvasEntityId, sm.TextID, sm.currentText);
            end,
            OnExit = function(self, sm)
				sm.currentText = sm.desiredText;
				StarterGameUIUtility.UITextSetter(sm.UserData.canvasEntityId, sm.TextID, sm.currentText);
            end,            
            OnUpdate = function(self, sm, deltaTime)
				self.timer = self.timer + deltaTime;
				local chars = math.floor(self.timer / sm.UserData.Properties.ObjectiveCharScrambleTime);
				if(chars >= self.numCharsToWrite) then
					self.complete = true;
				else
					sm.currentText = (sm.desiredText:sub(0, chars) .. string.char(math.random(35,126))); -- these ascii codes are actual visible non formatting characters (skipping ! and ", because " causes problems when text is parsed)
					StarterGameUIUtility.UITextSetter(sm.UserData.canvasEntityId, sm.TextID, sm.currentText);
				end
            end,
            Transitions =
            {
				Showing = 
				{
                    Evaluate = function(state, sm)
						return (state.complete == true);
                    end	
				}
            },
        },
		Showing = 
        {      
            OnEnter = function(self, sm)
				self.timer = 0.0;
				StarterGameUIUtility.UITextSetter(sm.UserData.canvasEntityId, sm.TextID, sm.desiredText);
            end,
            OnExit = function(self, sm)
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.TextID, 1.0, sm.UserData.Properties.ObjectiveShownCycleTime);
            end,            
            OnUpdate = function(self, sm, deltaTime)
				self.timer = self.timer + deltaTime;
				local fadeValue = Math.Cos((self.timer / sm.UserData.Properties.ObjectiveShownCycleTime) * math.pi * 2);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.TextID, fadeValue, 0.0);
            end,
            Transitions =
            {
				Removing = 
				{
                    Evaluate = function(state, sm)
						return (sm.currentText ~= sm.desiredText);
                    end	
				}
            },
        },
		Removing = 
        {      
            OnEnter = function(self, sm)
				self.timer = sm.currentText:len() * sm.UserData.Properties.ObjectiveCharScrambleTime;
				self.complete = false;
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.TextID, 1.0, sm.UserData.Properties.ObjectiveShownCycleTime)
            end,
            OnExit = function(self, sm)
				sm.currentText = "";
				StarterGameUIUtility.UITextSetter(sm.UserData.canvasEntityId, sm.TextID, sm.currentText);
            end,            
            OnUpdate = function(self, sm, deltaTime)
				self.timer = self.timer - deltaTime;
				local chars = math.floor(self.timer / sm.UserData.Properties.ObjectiveCharScrambleTime);
				if(self.timer <= 0) then
					self.complete = true;
				else
					sm.currentText = (sm.currentText:sub(0, chars) .. string.char(math.random(35,126))); -- these ascii codes are actual visible non formatting characters
					StarterGameUIUtility.UITextSetter(sm.UserData.canvasEntityId, sm.TextID, sm.currentText);
				end
            end,
            Transitions =
            {
				Waiting = 
				{
                    Evaluate = function(state, sm)
						return (state.complete == true);
                    end	
				}
            },
        },
	},
}

function uiobjectivecontroller:OnActivate()
	--Debug.Log("uiobjectivecontroller:OnActivate()");
			
	-- Listen for value set events.
	self.setObjectiveEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.SetObjectiveText, "float");
	self.setObjectiveHandler = GameplayNotificationBus.Connect(self, self.setObjectiveEventId);
	self.setObjectiveScreenIDEventId = GameplayNotificationId(self.entityId, self.Properties.Messages.SetObjectiveScreenID, "float");
	self.setObjectiveScreenIDHandler = GameplayNotificationBus.Connect(self, self.setObjectiveScreenIDEventId);
	
	-- dont start the state machine untill i have the scrren that i am on
	self.stateMachineStarted = false;
	self.canvasEntityId = nil;
	
	self.stateMachine = {}
    setmetatable(self.stateMachine, StateMachine);
	self.stateMachine.desiredText = "";
	self.stateMachine.currentText = "";
	self.stateMachine.TextID = self.Properties.ScreenHookup.TextID;
end

function uiobjectivecontroller:OnDeactivate()
	self.setObjectiveHandler:Disconnect();
	self.setObjectiveHandler = nil;
	self.setObjectiveScreenIDHandler:Disconnect();
	self.setObjectiveScreenIDHandler = nil;

	if (self.stateMachineStarted == true) then
		self.stateMachine:Stop();
		self.stateMachineStarted = false;
	end
end

function uiobjectivecontroller:OnEventBegin(value)
	if (GameplayNotificationBus.GetCurrentBusId() == self.setObjectiveScreenIDEventId) then
		--Debug.Log("uiobjectivecontroller:OnEventBegin:ScreenID: " .. tostring(value));
		if((self.stateMachineStarted == false) and (value ~= nil)) then
			-- i have been given a screen to use so start the state machine
			self.canvasEntityId = value;
			self.stateMachine:Start("ObjectiveSM", self.entityId, self, self.ObjectiveStates, false, "Waiting", self.Properties.DebugStateMachine)
			self.stateMachineStarted = true;
		elseif((self.stateMachineStarted == true) and (value == nil)) then
			-- my screen has dissappeared, kill my state machine
			self.stateMachine:Stop();
			self.canvasEntityId = value;
			self.stateMachineStarted = false;
		end
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.setObjectiveEventId) then
		--Debug.Log("uiobjectivecontroller:OnEventBegin:setObjective: " .. tostring(value));
		self.stateMachine.desiredText = value;
	end
end

return uiobjectivecontroller;
