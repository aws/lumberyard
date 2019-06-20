
local particlespawnrequester =
{
	Properties =
	{
		ActivationEvent = { default = "ActivateEvent", description = "The name of the event that kicks this entity into action." },
		
		Particle =
		{
			ParticleSpawnEvent = { default = "SpawnParticleName", description = "The name of the particle to spawn." },
			Delay = { default = 0.0, description = "Time before the particle is actually spawned after receiving the event." },
		},
		
		Target =
		{
			FollowAnEntity = { default = false, description = "If true, the particle will follow the 'EntityToFollow'." },
			EntityToFollow = { default = EntityId(), description = "The entity the particle will move with." },
		},
	},
}

function particlespawnrequester:OnActivate()

	self.activationEventId = GameplayNotificationId(self.entityId, self.Properties.ActivationEvent, "float");
	self.activationHandler = GameplayNotificationBus.Connect(self, self.activationEventId);
	
	if (self.Properties.Target.EntityToFollow == nil or not self.Properties.Target.EntityToFollow:IsValid()) then
		self.Properties.Target.EntityToFollow = self.entityId;
	end

	self.tickHandler = TickBus.Connect(self);
	self.performedFirstUpdate = false;

end

function particlespawnrequester:OnDeactivate()

	self.activationHandler:Disconnect();
	self.activationHandler = nil;
	
	self.tickHandler:Disconnect();
	self.tickHandler = nil;

end

function particlespawnrequester:OnTick(deltaTime, timePoint)

	if (self.performedFirstUpdate == false) then
		local particleManager = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("ParticleManager"));
		if (particleManager == nil or particleManager:IsValid() == false) then
			Debug.Assert("Invalid particle manager.");
		end
		self.eventId = GameplayNotificationId(particleManager, "SpawnParticleEvent", "float");
		self:ResetSpawner();
		
		-- Debugging
		--self.needsToSpawn = true;
		
		self.performedFirstUpdate = true;
	end
	
	if (self.needsToSpawn == true) then
		self.timeToSpawn = self.timeToSpawn - deltaTime;
		if (self.timeToSpawn <= 0.0) then
			self:Spawn();
		end
	end

end

function particlespawnrequester:Spawn()

	local params = ParticleSpawnerParams();
	params.transform = TransformBus.Event.GetWorldTM(self.entityId);
	params.event = self.Properties.Particle.ParticleSpawnEvent;
	params.targetId = self.Properties.Target.EntityToFollow;
	params.attachToEntity = self.Properties.Target.FollowAnEntity;
	
	-- Override the attachment if the entity doesn't exist.
	if (self.Properties.Target.FollowAnEntity == true) then
		if (not self.Properties.Target.EntityToFollow:IsValid()) then
			params.attachToEntity = false;
			Debug.Log("The particle " .. tostring(self.Properties.Particle.ParticleSpawnEvent) .. " was meant to follow an entity, but it doesn't have one assigned.");
		end
	end
	
	GameplayNotificationBus.Event.OnEventBegin(self.eventId, params);
	--Debug.Log("Spawned " .. tostring(self.Properties.Particle.ParticleSpawnEvent));
	
	self:ResetSpawner();
	
end

function particlespawnrequester:ResetSpawner()
	self.needsToSpawn = false;
	self.timeToSpawn = self.Properties.Particle.Delay;
end

function particlespawnrequester:OnEventBegin(value)
	if (GameplayNotificationBus.GetCurrentBusId() == self.activationEventId) then
		self.needsToSpawn = true;
	end
end

return particlespawnrequester;