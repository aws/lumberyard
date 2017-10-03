local uicrosshair =
{
	Properties =
	{
		Messages = {
			EnableUIEvent = { default = "EnableUIEvent", description = "The event used to enable/disable the crosshair (1 to enable; anything else disables)" },
			HideUIEvent = { default = "HideUIEvent", description = "The event used to set the whole UI visible / invisible" },
			SetStatusEvent = { default = "SetCrosshairStatusEvent", description = "sets the status effect to on / off" },
			SetHiddenEvent = { default = "SetCrosshairHiddenEvent", description = "set this as hidden" },
		},

		Owner = { default = EntityId() };
		StartingState = { default = 0, description = "The starting state of the item (0 == hidden)" },
		
		ScreenHookup = {
			File = { default = "UI/Canvases/UICrosshairPistol.uicanvas", description = "The file path in the projects directory of the screen to use"},
			AllUIFaderID = { default = -1, description = "The ID of the All UI fader" },
			StateChangeFaderAID = { default = -1, description = "Fader for the normal state" },
			StateChangeFaderBID = { default = -1, description = "Fader for the alternate state" },
			HiddenFaderID = { default = -1, description = "Fader to hide this crosshair" },
		},
	},
}

function uicrosshair:OnActivate()

	self.enabled = false;
	
	-- Listen for enable/disable events.	
	self.EnableUIEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.EnableUIEvent, "float");
	self.EnableUIHandler = GameplayNotificationBus.Connect(self, self.EnableUIEventId);
	self.HideUIEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.HideUIEvent, "float");
	self.HideUIHandler = GameplayNotificationBus.Connect(self, self.HideUIEventId);
	self.SetStatusEventId = GameplayNotificationId(self.Properties.Owner, self.Properties.Messages.SetStatusEvent, "float");
	self.SetStatusHandler = GameplayNotificationBus.Connect(self, self.SetStatusEventId);
	self.SetHiddenEventId = GameplayNotificationId(self.entityId, self.Properties.Messages.SetHiddenEvent, "float");
	self.SetHiddenHandler = GameplayNotificationBus.Connect(self, self.SetHiddenEventId);
	
	--Debug.Log("uicrosshair ID: " .. tostring(self.entityId));
	self:EnableHud(true);

	self:SetHidden(self.Properties.StartingState)
end

function uicrosshair:OnDeactivate()
	--Debug.Log("uicrosshair:OnDeactivate()");
	self:EnableHud(false);

	self.EnableUIHandler:Disconnect();
	self.EnableUIHandler = nil;
	self.HideUIHandler:Disconnect();
	self.HideUIHandler = nil;
	self.SetStatusHandler:Disconnect();
	self.SetStatusHandler = nil;
	self.SetHiddenHandler:Disconnect();
	self.SetHiddenHandler = nil;
end

function uicrosshair:EnableHud(enabled)

	if (enabled == self.enabled) then
		return;
	end
	
	--Debug.Log("uicrosshair:EnableHud() " .. tostring(enabled));
	
	if (enabled) then
		-- IMPORTANT: The 'canvas ID' is different to 'self.entityId'.
		self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas(self.Properties.ScreenHookup.File);

		--Debug.Log("uicrosshair:CanvasLoad(): " .. tostring(self.canvasEntityId));

		-- Listen for action strings broadcast by the canvas.
		self.uiCanvasNotificationLuaBusHandler = UiCanvasNotificationLuaBus.Connect(self, self.canvasEntityId);
		
		self.visible = true;
	else
		self.visible = false;
		
		UiCanvasManagerBus.Broadcast.UnloadCanvas(self.canvasEntityId);

		self.uiCanvasNotificationLuaBusHandler:Disconnect();
		self.uiCanvasNotificationLuaBusHandler = nil;
	end
	self.enabled = enabled;
end

function uicrosshair:HideHud(enabled)
	if (self.enabled == false) then
		return;
	end
	
	--Debug.Log("uicrosshair:HideHud(): " .. tostring(enabled));
	
	local fadeValue = 1;
	if(enabled) then
		fadeValue = 0;
	end
	StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.AllUIFaderID, fadeValue, 1.0);	
end

function uicrosshair:SetHidden(value)
	if (self.enabled == false) then
		return;
	end
		
	--Debug.Log("uicrosshair:SetHidden: [" .. tostring(self.entityId) .. "] " .. value);

	local fadeValue = 1;
	if(value == 0) then
		fadeValue = 0;
	end
	
	StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.HiddenFaderID, fadeValue, 0.0);	
end

function uicrosshair:SetStatus(value)
	if (self.enabled == false) then
		return;
	end
	
	if (self.state == value) then
		return;
	end;
	
	--Debug.Log("uicrosshair:SetStatus: [" .. tostring(self.entityId) .. "] " .. value);

	self.state = value;
	
	local fadeValue = 1;
	if(value == 0) then
		fadeValue = 0;
	end
		
	StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.StateChangeFaderAID, 1.0 - fadeValue, 0);
	StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.StateChangeFaderBID, fadeValue, 0);
end

function uicrosshair:OnEventBegin(value)
	--Debug.Log("uicrosshair:OnEventBegin");
	if (GameplayNotificationBus.GetCurrentBusId() == self.EnableUIEventId) then
		self:EnableHud(value == 1);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.HideUIEventId) then
		self:HideHud(value == 1);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetHiddenEventId) then
		self:SetHidden(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetStatusEventId) then
		self:SetStatus(value); 
	end
end

return uicrosshair;