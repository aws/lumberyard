local triggereventaudioreceiver =
{
	Properties =
	{
		EventName = { default = "", description = "The name of the event to send." },
		
		Sound = { default = "", description = "The sound event to trigger." },

		TriggerOnce = { default = true },
	},
}

function triggereventaudioreceiver:OnActivate()

	-- Listen for the event.
	self.eventId = GameplayNotificationId(self.entityId, self.Properties.EventName, "float");
	self.eventHandler = GameplayNotificationBus.Connect(self, self.eventId);

	self.alreadyTriggered = false;
end

function triggereventaudioreceiver:OnDeactivate()

	-- Release the handler.
	self.eventHandler:Disconnect();
	self.eventHandler = nil;

end

function triggereventaudioreceiver:OnEventBegin(value)

	local varSound = self.Properties.Sound;

	if (GameplayNotificationBus.GetCurrentBusId() == self.eventId) then
		AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, varSound);
		Debug.Log("SND Sound Event Received & Triggered : " .. tostring(varSound));
	end

	-- Only allow us to be triggered once by the event.
	if (self.Properties.TriggerOnce) then
		if (self.alreadyTriggered) then
			self.eventHandler:Disconnect();
		else
			self.alreadyTriggered = true;
		end
	end

end

return triggereventaudioreceiver;