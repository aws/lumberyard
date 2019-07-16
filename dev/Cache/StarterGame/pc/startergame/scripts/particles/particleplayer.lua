
local particleplayer =
{
	Properties =
	{
		TriggerEvent = { default = "", description = "The name of the event that will trigger the particle." },
		Entity = { default = EntityId(), description = "The entity that holds the particle component that should be reset." },
	},
}

function particleplayer:OnActivate()
	
	-- Listen for the event that will trigger the particle.
	self.triggerEventId = GameplayNotificationId(self.entityId, self.Properties.TriggerEvent, "float");
	self.triggerHandler = GameplayNotificationBus.Connect(self, self.triggerEventId);
	
	if (self.Properties.Entity == nil or not self.Properties.Entity:IsValid()) then
		self.Properties.Entity = self.entityId;
	end
	
end

function particleplayer:OnDeactivate()
	
	if (self.triggerHandler ~= nil) then
		self.triggerHandler:Disconnect();
		self.triggerHandler = nil;
	end
	
end

function particleplayer:OnEventBegin(value)

	if (GameplayNotificationBus.GetCurrentBusId() == self.triggerEventId) then
		ParticleComponentRequestBus.Event.Enable(self.Properties.Entity, false);
		
		if (value) then
			ParticleComponentRequestBus.Event.Enable(self.Properties.Entity, true);
		end
	end

end


return particleplayer;