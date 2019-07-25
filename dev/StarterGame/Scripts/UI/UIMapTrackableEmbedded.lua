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
local utilities = require "scripts/common/utilities"
local UIMapTrackableEmbedded =
{
	Properties =
	{		
		Messages = {
			ShowMarkerEvent = { default = "ShowMarkerEvent", description = "The event used to enable/disable the marker (1 to enable; anything else disables)" },
			SetValueEvent = { default = "SetValueEvent", description = "set the Value of the tracker (its position on the slider)." },
			SetImageEvent = { default = "SetImageEvent", description = "set the Image of the tracker." },
			SetHighlightedEvent = { default = "SetHighlightedEvent", description = "set the tracker to be Highlighted." },
			DestroyEvent = { default = "DestroyEvent", description = "set the tracker to be destroyed." },
		},
		MessagesOut = {
			Destroyed = { default = "MarkerDestroyedEvent", description = "The event sent when i have been asked to be destroyed and i am ready to be deleted" },
		},
		
		ScreenHookup = {
			Image = { default = EntityId(), description = "The image that i am dislaying" },
			FadeTime = { default = 1.0, description = "Time taken for fades"}
		},
		
		DebugStateMachine = { default = false, description = "Switch on the debugging information for the state machine." },
	},
	
	States = 
    {
		Inactive = 
        {      
            OnEnter = function(self, sm)
				-- set all the things to be hidden
				UiFaderBus.Event.Fade(sm.UserData.entityId, 0, 0.0);
            end,
            OnExit = function(self, sm)				
			end,            
            OnUpdate = function(self, sm, deltaTime)
            end,
            Transitions =
            {		
				Destroying = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.destroyMe;
                    end	
				},
            },
			
        },
        Hidden = 
        {      
            OnEnter = function(self, sm)
				-- set all the things to be hidden
				sm.UserData:Fade(0);
            end,
            OnExit = function(self, sm)				
			end,            
            OnUpdate = function(self, sm, deltaTime)
            end,
            Transitions =
            {
				Shown = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.showMe and (not sm.UserData.inactive);
                    end	
				},
 				Inactive = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.inactive;
                    end	
				}
           },
        },
		Shown = 
        {      
            OnEnter = function(self, sm)
				-- set all the things to be hidden
				sm.UserData:Fade(1);
            end,
            OnExit = function(self, sm)	
			end,            
            OnUpdate = function(self, sm, deltaTime)
            end,
            Transitions =
            {
				Flashing = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.flashMe and sm.UserData.showMe and (not sm.UserData.inactive);
                    end	
				},
				Destroying = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.destroyMe and (not sm.UserData.inactive);
                    end	
				},
 				Inactive = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.inactive;
                    end	
				}
			},
        },
		Flashing = 
        {      
            OnEnter = function(self, sm)
				sm.UserData:Fade(0.5);
				self.timer = 0;
				self.fadingDown = true;
            end,
            OnExit = function(self, sm)				
			end,            
            OnUpdate = function(self, sm, deltaTime)
				self.timer = self.timer + deltaTime;
				
				if (self.fadingDown and (self.timer >= self.Properties.ScreenHookup.FadeTime)) then
					self.fadingDown = false;
					sm.UserData:Fade(1);
				elseif ((not self.fadingDown) and (self.timer >= (2 * self.Properties.ScreenHookup.FadeTime))) then
					self.timer = self.timer - self.Properties.ScreenHookup.FadeTime;
					sm.UserData:Fade(0.5);
					self.fadingDown = true;
				end
            end,
            Transitions =
            {
				Destroying = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.destroyMe and (not sm.UserData.inactive);
                    end
				},
				Shown = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.showMe and (not sm.UserData.flashMe) and (not sm.UserData.inactive);
                    end	
				},
 				Inactive = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.inactive;
                    end	
				}
            },
        },
		Destroying = 
        {      
            OnEnter = function(self, sm)
				-- set all the things to be hidden
				sm.UserData:Fade(0);
				self.timer = 0;
            end,
            OnExit = function(self, sm)				

			end,            
            OnUpdate = function(self, sm, deltaTime)
				self.timer = self.timer + deltaTime;
				
				if (self.timer >= sm.UserData.Properties.ScreenHookup.FadeTime) then
					-- send message saying that i have been destroyed
					GameplayNotificationBus.Event.OnEventBegin(sm.UserData.SetDestroyedEventId, sm.UserData.entityId);
					
					sm.UserData.inactive = true;
					sm.UserData.destroyMe = false;
				end
            end,
            Transitions =
            {
 				Inactive = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.inactive;
                    end	
				}
            },
        },
	}
}

function UIMapTrackableEmbedded:OnActivate()
	--Debug.Log("UIMapTrackableEmbedded:OnActivate()");
	self.enabled = false;
	
	-- Listen for enable/disable events.
	self.ShowMarkerEventId = GameplayNotificationId(self.entityId, self.Properties.Messages.ShowMarkerEvent, "float");
	self.ShowMarkerHandler = GameplayNotificationBus.Connect(self, self.ShowMarkerEventId);
	self.SetValueEventId = GameplayNotificationId(self.entityId, self.Properties.Messages.SetValueEvent, "float");
	self.SetValueHandler = GameplayNotificationBus.Connect(self, self.SetValueEventId);
	self.SetImageEventId = GameplayNotificationId(self.entityId, self.Properties.Messages.SetImageEvent, "float");
	self.SetImageHandler = GameplayNotificationBus.Connect(self, self.SetImageEventId);
	self.SetHighlightedEventId = GameplayNotificationId(self.entityId, self.Properties.Messages.SetHighlightedEvent, "float");
	self.SetHighlightedHandler = GameplayNotificationBus.Connect(self, self.SetHighlightedEventId);
	self.DestroyEventId = GameplayNotificationId(self.entityId, self.Properties.Messages.DestroyEvent, "float");
	self.DestroyHandler = GameplayNotificationBus.Connect(self, self.DestroyEventId);
	
	self.SetDestroyedEventId = GameplayNotificationId(EntityId(), self.Properties.MessagesOut.Destroyed, "float");
	
	--Debug.Log("UIPlayerHUD ID: " .. tostring(self.entityId));
	UiFaderBus.Event.Fade(self.entityId, 1, 0.0);
	
	self.inactive = false;
	self.showMe = false;
	self.flashMe = false;
	self.destroyMe = false;
	
	-- state machine hookup
    self.StateMachine = {}
    setmetatable(self.StateMachine, StateMachine);
    self.StateMachine:Start("UIMapTracker", self.entityId, self, self.States, false, "Hidden", self.Properties.DebugStateMachine)
end

function UIMapTrackableEmbedded:OnDeactivate()
	--Debug.Log("UIMapTrackableEmbedded:OnDeactivate()");
	
	self.StateMachine:Stop();
	
	self.ShowMarkerHandler:Disconnect();
	self.ShowMarkerHandler = nil;
	self.SetValueHandler:Disconnect();
	self.SetValueHandler = nil;
	self.SetImageHandler:Disconnect();
	self.SetImageHandler = nil;
	self.SetHighlightedHandler:Disconnect();
	self.SetHighlightedHandler = nil;
	self.DestroyHandler:Disconnect();
	self.DestroyHandler = nil;
end

function UIMapTrackableEmbedded:SetValue(value)
	--Debug.Log("UIMapTrackableEmbedded:SetValue: " .. value);
	UiScrollerBus.Event.SetValue(self.entityId, value);
end

function UIMapTrackableEmbedded:SetImage(value)
	--Debug.Log("SetImage: " .. value);
	UiImageBus.Event.SetSpritePathname(self.Properties.ScreenHookup.Image, value);
end

function UIMapTrackableEmbedded:ShowMarker(value)
	--Debug.Log("UIMapTrackableEmbedded:ShowMarker " .. value);
	UiFaderBus.Event.Fade(self.entityId, value, 1.0);
end

function UIMapTrackableEmbedded:Fade(value)
	local currentFade = UiFaderBus.Event.GetFadeValue(self.entityId);
	if(currentFade ~= nil) then
		local fadeSpeed = 0;
		if (self.Properties.ScreenHookup.FadeTime ~= 0) then
			fadeSpeed = (math.abs(currentFade - value) / self.Properties.ScreenHookup.FadeTime);
		end
		UiFaderBus.Event.Fade(self.entityId, value, fadeSpeed);
	end
end

function UIMapTrackableEmbedded:OnEventBegin(value)
	--Debug.Log("Recieved message");
	if (GameplayNotificationBus.GetCurrentBusId() == self.SetValueEventId) then
		self:SetValue(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.ShowMarkerEventId) then
		self.showMe = value; 
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetImageEventId) then
		self:SetImage(value); 
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetHighlightedEventId) then
		self.flashMe = value; 
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.DestroyEventId) then
		self.destroyMe = true;
	end
end

return UIMapTrackableEmbedded;
