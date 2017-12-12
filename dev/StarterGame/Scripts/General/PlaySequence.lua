local playsequence =
{
	Properties =
	{
		TriggerMessage = { default = "Play", description = "Message aimed at this that i am going to trigger on." },
		SequenceToPlay = { default = "", description = "The name of the sequence in track view to play." },
	},
}

function playsequence:OnActivate()
	self.triggerEventId = GameplayNotificationId(self.entityId, self.Properties.TriggerMessage, "float");
	self.triggerHandler = GameplayNotificationBus.Connect(self, self.triggerEventId);
end

function playsequence:OnDeactivate()
	self.triggerHandler:Disconnect();
	self.triggerHandler = nil;
end

function playsequence:OnEventBegin(value)
	if (GameplayNotificationBus.GetCurrentBusId() == self.triggerEventId) then
		StarterGameUtility.PlaySequence(self.Properties.SequenceToPlay);
	end
end

return playsequence;