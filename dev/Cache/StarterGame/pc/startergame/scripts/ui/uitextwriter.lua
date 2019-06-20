local StateMachine = require "scripts/common/StateMachine"
-- this state machine is for manipulating the text on the UI for info display
local uitextwriter =
{
	Properties =
	{
		MessageTarget = { default = EntityId(), description = "Listen to messages with the given name aimed at given entity, blank to listen to broadcasts" },
		MessageSetText = { default = "SetTextEvent", description = "Message to set the text to write" },
				
		WaitCycleTime = { default = 0.5, description = "Cycle time on eht ewaiting fade" },
		WaitText = { default = "...", description = "The text to display when waiting for text or it is empty" },
		CharScrambleTime = { default = 0.05, description = "The time to spend on each char scrambling it when writing or clearing it" },
		ShownCycleTime = { default = 1.0, description = "The cycle time on the transparancy of a shown objective" },
		TransparancyMin = { default = 0.75, description = "The cycle time on the transparancy of a shown objective" },
		
		DebugStateMachine = { default = false, description = "Should this state machine show debug messages" },
	},
	ObjectiveStates = {
		-- No Current Objective
        Waiting = 
        {      
            OnEnter = function(self, sm)
				self.timer = 0.0;
				sm.currentText = sm.UserData.Properties.WaitText;
				UiTextBus.Event.SetText(sm.UserData.entityId, sm.currentText);
				UiFaderBus.Event.Fade(sm.UserData.entityId, 0.0, 0.0);
            end,
            OnExit = function(self, sm)
				UiFaderBus.Event.Fade(sm.UserData.entityId, 1, sm.UserData.Properties.WaitCycleTime);
            end,            
            OnUpdate = function(self, sm, deltaTime)
				self.timer = self.timer + deltaTime;
				local fadeValue = (Math.Cos((self.timer / sm.UserData.Properties.WaitCycleTime) * math.pi * 2) * (1 - sm.UserData.Properties.TransparancyMin)) + sm.UserData.Properties.TransparancyMin;
				UiTextBus.Event.SetText(sm.UserData.entityId, sm.currentText);
				UiFaderBus.Event.Fade(sm.UserData.entityId, fadeValue, 0.0);
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
				UiTextBus.Event.SetText(sm.UserData.entityId, sm.currentText);
            end,
            OnExit = function(self, sm)
				sm.currentText = sm.desiredText;
				UiTextBus.Event.SetText(sm.UserData.entityId, sm.currentText);
            end,            
            OnUpdate = function(self, sm, deltaTime)
				self.timer = self.timer + deltaTime;
				local chars = math.floor(self.timer / sm.UserData.Properties.CharScrambleTime);
				if(chars >= self.numCharsToWrite) then
					self.complete = true;
				else
					sm.currentText = (sm.desiredText:sub(0, chars) .. string.char(math.random(35,126))); -- these ascii codes are actual visible non formatting characters (skipping ! and ", because " causes problems when text is parsed)
					UiTextBus.Event.SetText(sm.UserData.entityId, sm.currentText);
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
				UiTextBus.Event.SetText(sm.UserData.entityId, sm.desiredText);
            end,
            OnExit = function(self, sm)
				UiFaderBus.Event.Fade(sm.UserData.entityId, 1.0, sm.UserData.Properties.ShownCycleTime);
            end,            
            OnUpdate = function(self, sm, deltaTime)
				self.timer = self.timer + deltaTime;
				local fadeValue = (Math.Cos((self.timer / sm.UserData.Properties.ShownCycleTime) * math.pi * 2) * (1 - sm.UserData.Properties.TransparancyMin)) + sm.UserData.Properties.TransparancyMin;
				UiFaderBus.Event.Fade(sm.UserData.entityId, fadeValue, 0.0);
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
				self.timer = sm.currentText:len() * sm.UserData.Properties.CharScrambleTime;
				self.complete = false;
				UiFaderBus.Event.Fade(sm.UserData.entityId, 1.0, sm.UserData.Properties.ShownCycleTime)
            end,
            OnExit = function(self, sm)
				sm.currentText = "";
				UiTextBus.Event.SetText(sm.UserData.entityId, sm.currentText);
            end,            
            OnUpdate = function(self, sm, deltaTime)
				self.timer = self.timer - deltaTime;
				local chars = math.floor(self.timer / sm.UserData.Properties.CharScrambleTime);
				if(self.timer <= 0) then
					self.complete = true;
				else
					sm.currentText = (sm.currentText:sub(0, chars) .. string.char(math.random(35,126))); -- these ascii codes are actual visible non formatting characters
					UiTextBus.Event.SetText(sm.UserData.entityId, sm.currentText);
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

function uitextwriter:OnActivate()
	--Debug.Log("uitextwriter:OnActivate()");
			
	-- Listen for value set events.
	self.setObjectiveEventId = GameplayNotificationId(self.Properties.MessageTarget, self.Properties.MessageSetText, "float");
	self.setObjectiveHandler = GameplayNotificationBus.Connect(self, self.setObjectiveEventId);
	
	-- dont start the state machine untill i have the scrren that i am on
	self.stateMachineStarted = false;
	self.canvasEntityId = nil;
	
	self.stateMachine = {}
    setmetatable(self.stateMachine, StateMachine);
	self.stateMachine.desiredText = "";
	self.stateMachine.currentText = "";
	
	self.stateMachine:Start("UITextWriterSM", self.entityId, self, self.ObjectiveStates, false, "Waiting", self.Properties.DebugStateMachine)
end

function uitextwriter:OnDeactivate()
	self.stateMachine:Stop();
	
	self.setObjectiveHandler:Disconnect();
	self.setObjectiveHandler = nil;
end

function uitextwriter:OnEventBegin(value)
	if (GameplayNotificationBus.GetCurrentBusId() == self.setObjectiveEventId) then
		--Debug.Log("uitextwriter:OnEventBegin:setObjective: " .. tostring(value));
		self.stateMachine.desiredText = value;
	end
end

return uitextwriter;
