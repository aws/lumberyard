local uicrosshairembedded =
{
	Properties =
	{
		Messages = {
			SetStatusEvent = { default = "SetCrosshairStatusEvent", description = "sets the status effect to on / off" },
			SetHiddenEvent = { default = "SetCrosshairHiddenEvent", description = "set this as hidden" },
		},

		StartingState = { default = 0, description = "The starting state of the item (0 == hidden)" },
		
		ScreenHookup = {
			StateChangeFaderAID = { default = EntityId(), description = "Fader for the normal state" },
			StateChangeFaderBID = { default = EntityId(), description = "Fader for the alternate state" },
			HiddenFaderID = { default = EntityId(), description = "Fader to hide this crosshair" },
		},
	},
}

function uicrosshairembedded:OnActivate()

	-- needed for a time when everything is created in order to setup the handlers, because the screen that i reside on does not exist when i get activated.
	self.tickBusHandler = TickBus.Connect(self);
end

function uicrosshairembedded:SetupHandlers()
	self.canvas = UiElementBus.Event.GetCanvas(self.entityId);
	if (self.canvas == nil) then
		Debug.Log("uicrosshairembedded:SetupHandlers() : i dont have a canvas ?!");
	end

	-- Listen for enable/disable events.	
	self.SetStatusEventId = GameplayNotificationId(self.canvas, self.Properties.Messages.SetStatusEvent, "float");
	self.SetStatusHandler = GameplayNotificationBus.Connect(self, self.SetStatusEventId);
	self.SetHiddenEventId = GameplayNotificationId(self.canvas, self.Properties.Messages.SetHiddenEvent, "float");
	self.SetHiddenHandler = GameplayNotificationBus.Connect(self, self.SetHiddenEventId);
end


function uicrosshairembedded:OnTick(deltaTime, timePoint)
	self.tickBusHandler:Disconnect();
	self.tickBusHandler = nil;
	
	self:SetupHandlers();
	
	self:SetHidden(self.Properties.StartingState)
end

function uicrosshairembedded:OnDeactivate()
	--Debug.Log("uicrosshairembedded:OnDeactivate()");
	self.SetStatusHandler:Disconnect();
	self.SetStatusHandler = nil;
	self.SetHiddenHandler:Disconnect();
	self.SetHiddenHandler = nil;
end

function uicrosshairembedded:SetHidden(value)		
	--Debug.Log("uicrosshairembedded:SetHidden: [" .. tostring(self.entityId) .. "] " .. value);

	local fadeValue = 1;
	if(value == 0) then
		fadeValue = 0;
	end
	
	UiFaderBus.Event.SetFadeValue(self.Properties.ScreenHookup.HiddenFaderID, fadeValue);	
end

function uicrosshairembedded:SetStatus(value)
	if (self.state == value) then
		return;
	end;
	
	--Debug.Log("uicrosshairembedded:SetStatus: [" .. tostring(self.entityId) .. "] " .. tostring(value));
	self.state = value;
	
	local fadeValue = 1;
	if(value == 0) then
		fadeValue = 0;
	end
	
	UiFaderBus.Event.SetFadeValue(self.Properties.ScreenHookup.StateChangeFaderAID, 1.0 - fadeValue);
	UiFaderBus.Event.SetFadeValue(self.Properties.ScreenHookup.StateChangeFaderBID, fadeValue);
end

function uicrosshairembedded:OnEventBegin(value)
	--Debug.Log("uicrosshairembedded:OnEventBegin");
	if (GameplayNotificationBus.GetCurrentBusId() == self.SetHiddenEventId) then
		self:SetHidden(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetStatusEventId) then
		self:SetStatus(value); 
	end
end

return uicrosshairembedded;