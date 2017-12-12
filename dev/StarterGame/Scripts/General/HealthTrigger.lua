local healthtrigger =
{
	Properties =
	{
		EventToSend = { default = "BroadcastAlert", description = "The event to send." },
		Health = { default = 50, description = "'EventToSend' will trigger when below this threshold." },
		TriggerOnce = { default = true, description = "If set to false then it will fire continously (so don't do it)." },
	},
}

function healthtrigger:OnActivate()
	self.sendingEventId = GameplayNotificationId(self.entityId, self.Properties.EventToSend, "float");
	self.tickBusHandler = TickBus.Connect(self);
	
	self.triggered = false;
end

function healthtrigger:OnDeactivate()
	if (self.tickBusHandler ~= nil) then
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end
end

function healthtrigger:OnTick(deltaTime, timepoint)
	local health = 0.0;
	health = StatRequestBus.Event.GetValue(self.entityId, health);	-- the 'health' parameter isn't used
	if (health <= self.Properties.Health) then
		GameplayNotificationBus.Event.OnEventBegin(self.sendingEventId, health);
		
		if (self.Properties.TriggerOnce) then
			self.tickBusHandler:Disconnect();
			self.tickBusHandler = nil;
		end
	end
end

return healthtrigger;