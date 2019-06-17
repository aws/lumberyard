
local uihider =
{
	Properties =
	{	-- these differ only in the expected param being the other way round, but having both is desirable for ease of use
		HideUIEvent = { default = "HideUIEvent", description = "The event used to set the whole UI visible / invisible" },
		ShowUIEvent = { default = "ShowUIEvent", description = "The event used to set the whole UI visible / invisible" },
		FadeDurationUIEvent = { default = "FadeDurationUIEvent", description = "The event used to set the FadeTime value." },
		MessageTarget = {default = EntityId(), description = "The event fired at this element is what i should listen to, leave balnk to listen to broadcasts."},
		FadeTime = { default = 1, description = "The time in seconds to fade the UI element" },
		InitallyVisible = { default = true, description = "should i be visible at the start" },
	},
}

function uihider:OnActivate()	
	self.HideUIEventId = GameplayNotificationId(self.Properties.MessageTarget, self.Properties.HideUIEvent, "float");
	self.HideUIHandler = GameplayNotificationBus.Connect(self, self.HideUIEventId);
	self.ShowUIEventId = GameplayNotificationId(self.Properties.MessageTarget, self.Properties.ShowUIEvent, "float");
	self.ShowUIHandler = GameplayNotificationBus.Connect(self, self.ShowUIEventId);
	self.FadeDurationUIEventId = GameplayNotificationId(self.Properties.MessageTarget, self.Properties.FadeDurationUIEvent, "float");
	self.FadeDurationUIHandler = GameplayNotificationBus.Connect(self, self.FadeDurationUIEventId);
	
	local fadeValue = 0;
	if (self.Properties.InitallyVisible) then
		fadeValue = 1;
	end
	UiFaderBus.Event.Fade(self.entityId, fadeValue, 0);
end

function uihider:OnDeactivate()
	self.HideUIHandler:Disconnect();
	self.HideUIHandler = nil;
	self.ShowUIHandler:Disconnect();
	self.ShowUIHandler = nil;
	self.FadeDurationUIHandler:Disconnect();
	self.FadeDurationUIHandler = nil;
end

function uihider:OnEventBegin(value)
	local busId = GameplayNotificationBus.GetCurrentBusId();
	--Debug.Log("Recieved message");
	local hide = 0;
	local triggered = false;
	if (busId == self.HideUIEventId) then
		hide = (value == 1);
		triggered = true;
	elseif(busId == self.ShowUIEventId) then
		hide = (value ~= 1);
		triggered = true;
	elseif(busId == self.FadeDurationUIEventId) then
		self.Properties.FadeTime = value;
	end
		
	if (triggered) then
		local currentFade = UiFaderBus.Event.GetFadeValue(self.entityId);
		local newFadeValue = 1;
		if(hide) then
			newFadeValue = 0;
		end
		
		local fadeSpeed = 0 
		if (self.Properties.FadeTime ~= 0) then
			fadeSpeed = (math.abs(currentFade - newFadeValue) / self.Properties.FadeTime);
		end
    	
		UiFaderBus.Event.Fade(self.entityId, newFadeValue, fadeSpeed);
	end
end

return uihider;
