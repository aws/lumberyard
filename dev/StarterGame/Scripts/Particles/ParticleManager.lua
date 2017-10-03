
local particlemanager =
{
	Properties =
	{
		
	},
}

function particlemanager:OnActivate()

	self.spawnParticleEventId = GameplayNotificationId(self.entityId, "SpawnParticleEvent", "float");
	self.spawnParticleHandler = GameplayNotificationBus.Connect(self, self.spawnParticleEventId);
	self.spawnDecalEventId = GameplayNotificationId(self.entityId, "SpawnDecalEvent", "float");
	self.spawnDecalHandler = GameplayNotificationBus.Connect(self, self.spawnDecalEventId);

end

function particlemanager:OnDeactivate()

	self.spawnParticleHandler:Disconnect();
	self.spawnParticleHandler = nil;
	self.spawnDecalHandler:Disconnect();
	self.spawnDecalHandler = nil;

end

function particlemanager:OnEventBegin(value)

	--Debug.Log("Something");
	if (GameplayNotificationBus.GetCurrentBusId() == self.spawnParticleEventId) then
		--Debug.Log("Passing on to children!");
		ParticleManagerComponentRequestsBus.Event.SpawnParticle(self.entityId, value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.spawnDecalEventId) then
		--Debug.Log("Passing on to children!");
		ParticleManagerComponentRequestsBus.Event.SpawnDecal(self.entityId, value);
	end

end

return particlemanager;