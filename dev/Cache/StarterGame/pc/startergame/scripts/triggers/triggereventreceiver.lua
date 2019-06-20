local triggereventreceiver =
{
	Properties =
	{
		EventName = { default = "", description = "The name of the event to send." },

		TriggerOnce = { default = true },

		OnReceive =
		{
			Suicide = { default = true },
		},
	},
}

function triggereventreceiver:OnActivate()

	-- Listen for the event.
	self.eventId = GameplayNotificationId(self.entityId, self.Properties.EventName, "float");
	self.eventHandler = GameplayNotificationBus.Connect(self, self.eventId);

	self.alreadyTriggered = false;

end

function triggereventreceiver:OnDeactivate()

	-- Release the handler.
	self.eventHandler:Disconnect();
	self.eventHandler = nil;

end

function triggereventreceiver:ReactToEvent(value)

	-- Only allow us to be triggered once by the event.
	if (self.Properties.TriggerOnce) then
		if (self.alreadyTriggered) then
			self.eventHandler:Disconnect();
		else
			self.alreadyTriggered = true;
		end
	end

	-- Delete self.
	if (self.Properties.OnReceive.Suicide) then
		GameEntityContextRequestBus.Broadcast.DestroyGameEntity(self.entityId);
	end

end

function triggereventreceiver:OnEventBegin(value)

	if (GameplayNotificationBus.GetCurrentBusId() == self.eventId) then
		self:ReactToEvent(value);
	end

end

return triggereventreceiver;