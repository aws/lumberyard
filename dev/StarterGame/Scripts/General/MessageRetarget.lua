local messageretarget =
{
	Properties =
	{
		MessageIn = { default = "", description = "Message aimed at this that i am going to trigger on." },
		MessageOut = { default = "", description = "Message to send when i am triggered." },
		MessageTarget = { default = EntityId(), description = "The Object to fire the message at." },
		
		ChangePayload = { default = false, description = "Should i change the data of the message to something else."},
		PayloadDetails = {
			-- because i cannot use a drop down, i will have to make a choice here based upon someting like a set of bools
			UseAsBool = { default = false, description = "Use as a bool value. \"true\" or \"yes\" or a number greater than zero, is true, otherwise false "},
			UseAsString = { default = false, description = "Use as a String value."},
			UseAsNumber = { default = false, description = "Use as a number. Converts it to a number as per Lua \"tonumber\""},
			Value = { default = "", description = "Value to be interpreted."},
		},
	},
}

function messageretarget:OnActivate()
	self.triggerEventId = GameplayNotificationId(self.entityId, self.Properties.MessageIn, "float");
	self.triggerHandler = GameplayNotificationBus.Connect(self, self.triggerEventId);

	self.fireMessageID = GameplayNotificationId(self.Properties.MessageTarget, self.Properties.MessageOut, "float");
end

function messageretarget:OnDeactivate()
	self.triggerHandler:Disconnect();
	self.triggerHandler = nil;
end

function messageretarget:OnEventBegin(value)
	if (GameplayNotificationBus.GetCurrentBusId() == self.triggerEventId) then
		if(not self.Properties.ChangePayload)then
			GameplayNotificationBus.Event.OnEventBegin(self.fireMessageID, value);
		else
			if(self.Properties.PayloadDetails.UseAsString)then
				GameplayNotificationBus.Event.OnEventBegin(self.fireMessageID, self.Properties.PayloadDetails.Value);
			elseif(self.Properties.PayloadDetails.UseAsNumber) then
				GameplayNotificationBus.Event.OnEventBegin(self.fireMessageID, tonumber(self.Properties.PayloadDetails.Value));
			elseif(self.Properties.PayloadDetails.UseAsBool) then
				local fromString = (self.Properties.PayloadDetails.Value:lower() == "true") or (self.Properties.PayloadDetails.Value:lower() == "yes");
				local fromNumber = tonumber(self.Properties.PayloadDetails.Value) > 0;
				GameplayNotificationBus.Event.OnEventBegin(self.fireMessageID, (fromString or fromNumber));
			else
				GameplayNotificationBus.Event.OnEventBegin(self.fireMessageID, nil);
			end
		end
	end
end

return messageretarget;