local simplemotiontrigger =
{
	Properties =
	{
		EventPlay = { default = "StartRockslide", description = "The name of the event that will start playing the animation.", },
	},
}

function simplemotiontrigger:OnActivate()
	self.playEventId = GameplayNotificationId(self.entityId, self.Properties.EventPlay, "float");
	self.playHandler = GameplayNotificationBus.Connect(self, self.playEventId);
end

function simplemotiontrigger:OnDeactivate()
	self.playHandler:Disconnect();
	self.playHandler = nil;
end

function simplemotiontrigger:OnEventBegin(value)
	if (GameplayNotificationBus.GetCurrentBusId() == self.playEventId) then
		SimpleMotionComponentRequestBus.Event.PlayMotion(self.entityId);
	end
end

return simplemotiontrigger;