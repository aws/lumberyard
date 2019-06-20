
local triggereventsimple =
{
	Properties =
	{
		EventNameEnter = { default = "", description = "The name of the event to send on enter." },
		EventNameExit = { default = "", description = "The name of the event to send on exit." },

		Recipient = { default = EntityId(), description = "Entity to send the event to." },
	},
}

function triggereventsimple:OnActivate()
	-- Listen for anything entering our area.
	self.triggerAreaHandler = TriggerAreaNotificationBus.Connect(self, self.entityId);

	if (not self.Properties.Recipient:IsValid()) then
		self.Properties.Recipient = self.entityId;
	end

	if (self.Properties.EventNameEnter ~= "") then
		self.enterEventId = GameplayNotificationId(self.Properties.Recipient, self.Properties.EventNameEnter, "float");
	end
	if (self.Properties.EventNameExit ~= "") then
		self.exitEventId = GameplayNotificationId(self.Properties.Recipient, self.Properties.EventNameExit, "float");
	end

end

function triggereventsimple:OnDeactivate()
	-- Release the handler.
	self.triggerAreaHandler:Disconnect();
	self.triggerAreaHandler = nil;
end

function triggereventsimple:OnTriggerAreaEntered(enteringEntityId)
	--Debug.Log("OnTriggerAreaEntered " .. tostring(self.entityId));
	if (self.enterEventId ~= nil) then
		--Debug.Log("Sending event " .. self.Properties.EventNameEnter);
		GameplayNotificationBus.Event.OnEventBegin(self.enterEventId, enteringEntityId);
	end
end

function triggereventsimple:OnTriggerAreaExited(exitingEntityId)
	--Debug.Log("OnTriggerAreaExited " .. tostring(self.entityId));
	if (self.exitEventId ~= nil) then
		--Debug.Log("Sending event " .. self.Properties.EventNameExit);
		GameplayNotificationBus.Event.OnEventBegin(self.exitEventId, exitingEntityId);
	end
end

return triggereventsimple;