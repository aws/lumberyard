local messageretarget =
{
	Properties =
	{
		MessageIn = { default = "", description = "Message aimed at this that i am going to trigger on." },
		MessageOut = { default = "DronePointOfInterest", description = "Message to send when i am triggered." },
		--MessageTarget = { default = EntityId(), description = "The Object to fire the message at." },
		
		ChangePayload = { default = true, description = "Should i change the data of the message to something else."},
		PayloadDetails =
		{
			Reset = { default = false, description = "Returns the companion to the player's side." },
			Position = { default = EntityId(), description = "Entity to move to." },
			LookAt = { default = EntityId(), description = "Entity to look at." },
		},
	},
}

function messageretarget:OnActivate()
	self.triggerEventId = GameplayNotificationId(self.entityId, self.Properties.MessageIn, "float");
	self.triggerHandler = GameplayNotificationBus.Connect(self, self.triggerEventId);
	
	self.fireMessageID = GameplayNotificationId(EntityId(), self.Properties.MessageOut, "float");
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
			local payload = CompanionPOIParams();
			payload.reset = self.Properties.PayloadDetails.Reset;
			if (not payload.reset) then
				if (not self.Properties.PayloadDetails.Position:IsValid() or not self.Properties.PayloadDetails.LookAt:IsValid()) then
					Debug.Warning("Drone's point of interest values are incorrect (either position or look at is not set).");
					return;
				end
				
				payload.position = self.Properties.PayloadDetails.Position;
				payload.lookAt = self.Properties.PayloadDetails.LookAt;
			end
			
			GameplayNotificationBus.Event.OnEventBegin(self.fireMessageID, payload);
		end
	end
end

return messageretarget;