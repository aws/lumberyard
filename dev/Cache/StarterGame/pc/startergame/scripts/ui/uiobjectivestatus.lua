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

local UIObjectiveStatus =
{
	Properties =
	{		
		Messages = {
			EnableUIEvent = { default = "EnableUIEvent", description = "The event used to enable the whole UI" }, -- maybe not neccessary?
			HideUIEvent = { default = "HideUIEvent", description = "The event used to set the whole UI visible / invisible" },
			SetStatusEvent = { default = "SetStatusEvent", description = "sets the status effect to on / off" },
			HideStatusEvent = { default = "SetTrackerEvent", description = "set this as hidden" },
		},
				
		StartingState = { default = 0, description = "The starting state of the item" },
		FadeTime = { default = 1, description = "The Time it takes for the state to fade between states" },
				
		ScreenHookup = {
			File = { default = "UI/Canvases/UIObjectiveStatus.uicanvas", description = "The file path in the projects directory of the scrren to use"},
			AllUIFaderID = { default = -1, description = "The ID of the All UI fader" },
			ActiveFaderID = { default = -1, description = "ID the fader for the objective tracker" },
			StateChangeFaderID = { default = -1, description = "Fades up and down whenever the state changes" },
			HiddenFaderID = { default = -1, description = "Fader to hide this objective" },
		},
	},
}

function UIObjectiveStatus:OnActivate()
	--Debug.Log("UIObjectiveStatus:OnActivate()");

	self.enabled = false;
	
	-- Listen for enable/disable events.
	self.EnableUIEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.EnableUIEvent, "float");
	self.EnableUIHandler = GameplayNotificationBus.Connect(self, self.EnableUIEventId);
	self.HideUIEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.HideUIEvent, "float");
	self.HideUIHandler = GameplayNotificationBus.Connect(self, self.HideUIEventId);
	self.SetStatusEventId = GameplayNotificationId(self.entityId, self.Properties.Messages.SetStatusEvent, "float");
	self.SetStatusHandler = GameplayNotificationBus.Connect(self, self.SetStatusEventId);
	self.HideStatusEventId = GameplayNotificationId(self.entityId, self.Properties.Messages.HideStatusEvent, "float");
	self.HideStatusHandler = GameplayNotificationBus.Connect(self, self.HideStatusEventId);
	
	--Debug.Log("UIObjectiveStatus ID: " .. tostring(self.entityId));
	self:EnableHud(true);

	self.state = self.Properties.StartingState;
	local fadeValue = 0
	if (self.state == 1) then
		fadeValue = 1;
	end
	StarterGameUIUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.ActiveFaderID, fadeValue, 0.0);
end

function UIObjectiveStatus:OnDeactivate()
	--Debug.Log("UIObjectiveStatus:OnDeactivate()");
	self:StartTicking(false);
	self:EnableHud(false);

	self.EnableUIHandler:Disconnect();
	self.EnableUIHandler = nil;
	self.HideUIHandler:Disconnect();
	self.HideUIHandler = nil;
	self.SetStatusHandler:Disconnect();
	self.SetStatusHandler = nil;
	self.HideStatusHandler:Disconnect();
	self.HideStatusHandler = nil;
end

function UIObjectiveStatus:StartTicking(wantToTick)
	if ((self.tickBusHandler ~= nil) and (not wantToTick)) then
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;	
	elseif ((self.tickBusHandler == nil) and (wantToTick)) then
		self.tickBusHandler = TickBus.Connect(self);
	end	
end

function UIObjectiveStatus:OnTick(deltaTime, timePoint)
	self.faderTime = self.faderTime - deltaTime;

	if (self.faderTime <= 0) then
		self.faderTime = 0;

		StarterGameUIUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.StateChangeFaderID, 0.0, 0.5);
		self:StartTicking(false);
	end
end

function UIObjectiveStatus:EnableHud(enabled)

	if (enabled == self.enabled) then
		return;
	end
	
	--Debug.Log("UIObjectiveStatus:EnableHud()" .. tostring(enabled));
	
	if (enabled) then
		-- IMPORTANT: The 'canvas ID' is different to 'self.entityId'.
		self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas(self.Properties.ScreenHookup.File);

		--Debug.Log("UIObjectiveStatus:CanvasLoad(): " .. tostring(self.canvasEntityId));
		
		-- Listen for action strings broadcast by the canvas.
		self.uiCanvasNotificationLuaBusHandler = UiCanvasNotificationLuaBus.Connect(self, self.canvasEntityId);
		
		--UIPlayerHudRequestBus.Event.SetCanvasID(self.entityId, self.canvasEntityId);
		
		self.visible = true;
	else
		self.visible = false;
		
		UiCanvasManagerBus.Broadcast.UnloadCanvas(self.canvasEntityId);

		self.uiCanvasNotificationLuaBusHandler:Disconnect();
		self.uiCanvasNotificationLuaBusHandler = nil;
	end
	self.enabled = enabled;
end

function UIObjectiveStatus:HideHud(enabled)
	if (self.enabled == false) then
		return;
	end
	
	--Debug.Log("UIObjectiveStatus:HideHud(): " .. tostring(enabled));
	
	local fadeValue = 1;
	if(enabled) then
		fadeValue = 0;
	end
	StarterGameUIUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.AllUIFaderID, fadeValue, 1.0);	
end

function UIObjectiveStatus:HideStatus(value)
	if (self.enabled == false) then
		return;
	end
		
	--Debug.Log("HideStatus: " .. value);

	local fadeValue = 1;
	if(value == 0) then
		fadeValue = 0;
	end
	
	StarterGameUIUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.HiddenFaderID, fadeValue, 1.0);	
end

function UIObjectiveStatus:SetStatus(value)
	if (self.enabled == false) then
		return;
	end
	
	if (self.state == value) then
		return;
	end;
	
	--Debug.Log("UIObjectiveStatus:SetStatus " .. value);

	self.state = value;
	
	local fadeValue = 1;
	if(value == 0) then
		fadeValue = 0;
	end
	
	self.fadeTime = self.Properties.FadeTime;
	StarterGameUIUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.ActiveFaderID, fadeValue, self.fadeTime);
	StarterGameUIUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.StateChangeFaderID, 1.0, self.fadeTime);
end

function UIObjectiveStatus:OnEventBegin(value)
	--Debug.Log("Recieved message");
	if (GameplayNotificationBus.GetCurrentBusId() == self.EnableUIEventId) then
		self:EnableHud(value == 1);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.HideUIEventId) then
		self:HideHud(value == 1);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.HideStatusEventId) then
		self:HideStatus(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetStatusEventId) then
		self:SetStatus(value); 
	end
end

return UIObjectiveStatus;
