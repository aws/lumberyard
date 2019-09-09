
local musicmanager =
{
	Properties =
	{
		Combat =
		{
			DataKey = { default = "AI_InCombatCount", description = "The name of the persistent data to check." },
			Entered = { default = "", description = "The audio event to send when the player enters combat." },
			Exited = { default = "", description = "The audio event to send when the player exits combat." },
		},
	},
}

function musicmanager:OnActivate()
	self.aiInCombatChangeEventId = GameplayNotificationId(self.entityId, "AiInCombatChange", "float");
	self.aiInCombatChangeHandler = GameplayNotificationBus.Connect(self, self.aiInCombatChangeEventId);
	PersistentDataSystemRequestBus.Broadcast.RegisterDataChangeCallback(self.Properties.Combat.DataKey, self.aiInCombatChangeEventId.channel, self.aiInCombatChangeEventId.actionNameCrc);
	self.aiInCombat = 0;
end

function musicmanager:OnDeactivate()
	PersistentDataSystemRequestBus.Broadcast.UnRegisterDataChangeCallback(self.Properties.Combat.DataKey, self.aiInCombatChangeEventId.channel, self.aiInCombatChangeEventId.actionNameCrc);
	if (self.aiInCombatChangeHandler ~= nil) then
		self.aiInCombatChangeHandler:Disconnect();
		self.aiInCombatChangeHandler = nil;
	end
end

function musicmanager:OnEventBegin(value)
	if (GameplayNotificationBus.GetCurrentBusId() == self.aiInCombatChangeEventId) then
		--Debug.Log("AI Combat count: " .. tostring(value));
		if (value > 0 and self.aiInCombat <= 0) then
			-- Entered combat.
			AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.Properties.Combat.Entered);
			--Debug.Log("Combat Entered");
		elseif (value <= 0 and self.aiInCombat > 0) then
			-- Exited combat.
			AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.Properties.Combat.Exited);
			--Debug.Log("Combat Exited");
		end
		self.aiInCombat = value;
	end
end

return musicmanager;