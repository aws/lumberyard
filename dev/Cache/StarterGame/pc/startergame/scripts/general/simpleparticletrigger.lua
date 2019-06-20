local simpleparticletrigger =
{
	Properties =
	{
		EventPlay = { default = "PlayEffect", description = "The name of the event that will enable the particle effect.", },
	},
}

function simpleparticletrigger:OnActivate()

	self.playEventId = GameplayNotificationId(self.entityId, self.Properties.EventPlay, "float");
	self.playHandler = GameplayNotificationBus.Connect(self, self.playEventId);

end

function simpleparticletrigger:OnDeactivate()

	self.playHandler:Disconnect();
	self.playHandler = nil;
	
end

function simpleparticletrigger:OnEventBegin(value)

	if (GameplayNotificationBus.GetCurrentBusId() == self.playEventId) then
		ParticleComponentRequestBus.Event.SetVisibility(self.entityId, true);
		ParticleComponentRequestBus.Event.Enable(self.entityId, true);
	end

end

return simpleparticletrigger;