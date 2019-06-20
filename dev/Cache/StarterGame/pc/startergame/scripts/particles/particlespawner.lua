
local particlespawner =
{
	Properties =
	{
		ParticleSpawnEvent = { default = "SpawnParticleName", description = "The name of the event that will trigger the spawner." },
		PoolSize = { default = 20, description = "The maximum number of particles of this type." },
		DebugSpawner = { default = false, description = "If true, additional debug information will be displayed in the console." },
	},
}

function particlespawner:OnActivate()

	-- Handler for when the manager tells us to spawn a particle somewhere.
	self.spawnEventId = GameplayNotificationId(self.entityId, self.Properties.ParticleSpawnEvent, "float");
	self.spawnHandler = GameplayNotificationBus.Connect(self, self.spawnEventId);
	
	-- Handler for when we spawn particles at the start of the game so we
	-- can record the entity IDs of the particle entities.
	self.poolHandler = SpawnerComponentNotificationBus.Connect(self, self.entityId);
	self.spawnTickets = {};
	
	-- Tick handler.
    self.tickBusHandler = TickBus.Connect(self);
	self.performedFirstUpdate = false;
	
	-- Tables to store the particles and their params.
	self.particles = {};
	self.particlesParams = {};
	self.particleIndex = 1;		-- this is the last particle that was "spawned"
	
	self.originalTM = Transform.CreateIdentity();

end

function particlespawner:OnDeactivate()
	
	if (self.tickBusHandler ~= nil) then
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end
	
	if (self.poolHandler ~= nil) then
		self.poolHandler:Disconnect();
		self.poolHandler = nil;
	end

	if (self.spawnHandler ~= nil) then
		self.spawnHandler:Disconnect();
		self.spawnHandler = nil;
	end

end

function particlespawner:OnTick(deltaTime, timePoint)

	if (self.performedFirstUpdate == false) then
		-- Create the pool of particles.
		for i = 1, self.Properties.PoolSize do
			self:SpawnParticleForPool();
		end
		
		self.performedFirstUpdate = true;
	end
	
	for i = 1, self.Properties.PoolSize do
		local particle = self.particles[i];
		local params = self.particlesParams[i];
		if (particle ~= nil and particle:IsValid() and params ~= nil) then
			-- If any particles need updating during their lifetime (such as being
			-- moved away after a set duration or disabled) then this is where those
			-- operations should be done.
		end
	end
end

function particlespawner:SpawnParticleForPool()
	local spawnTicket = SpawnerComponentRequestBus.Event.SpawnAbsolute(self.entityId, Transform:CreateIdentity());
	if (spawnTicket == nil) then
		Debug.Log("ParticleSpawner [" .. tostring(self.Properties.ParticleSpawnEvent) .. "]: Spawn failed");
	else
		if (self.Properties.DebugSpawner == true) then
			Debug.Log("ParticleSpawner [" .. tostring(self.Properties.ParticleSpawnEvent) .. "]: Spawn success");
		end
		table.insert(self.spawnTickets, spawnTicket);
	end
end

function particlespawner:SpawnParticle(params)

	local particle = self.particles[self.particleIndex];
	self.particlesParams[self.particleIndex] = params;
	
	-- Position the particle.
	TransformBus.Event.SetWorldTM(particle, params.transform * self.originalTM);
	if (self.Properties.DebugSpawner == true) then
		Debug.Log("ParticleSpawner [" .. tostring(self.Properties.ParticleSpawnEvent) .. "]: Positioning particle at " .. tostring(params.transform:GetTranslation()) .. " with impulse " .. tostring(params.impulse));
	end
	
	-- Parent the particle.
	if (params.attachToEntity == true and params.targetId:IsValid()) then
		TransformBus.Event.SetParent(particle, params.targetId);
	end
	
	-- Send the parameters to the particle (even though most won't care).
	local eventId = GameplayNotificationId(particle, "ParticlePostSpawnParams", "float");
	GameplayNotificationBus.Event.OnEventBegin(eventId, params);
	
	-- Disable it (just to make sure) and then enable it.
	ParticleComponentRequestBus.Event.Enable(particle, false);
	ParticleComponentRequestBus.Event.Enable(particle, true);
	
	-- Prepare for the next particle.
	self.particleIndex = self.particleIndex + 1;
	if (self.particles[self.particleIndex] == nil) then
		if (self.Properties.DebugSpawner == true) then
			if (self.particleIndex ~= self.Properties.PoolSize) then
				Debug.Log("ParticleSpawner [" .. tostring(self.Properties.ParticleSpawnEvent) .. "]: Spawner only has " .. tostring(self.particleIndex - 1) .. " particles.");
			end
		end
		self.particleIndex = 1;
	end
end

function particlespawner:OnEventBegin(value)

	--Debug.Log("Spawner received");
	if (GameplayNotificationBus.GetCurrentBusId() == self.spawnEventId) then
		if (self.Properties.DebugSpawner == true) then
			Debug.Log("ParticleSpawner [" .. tostring(self.Properties.ParticleSpawnEvent) .. "]: Spawner received " .. tostring(value.event));
		end
		self:SpawnParticle(value);
	end

end

-- TODO: Convert this to use 'OnEntitiesSpawned' so we only get one callback per slice
-- rather than a callback for each entity in each slice.
function particlespawner:OnEntitySpawned(ticket, spawnedEntityId)
	local found = false;
	for i,t in ipairs(self.spawnTickets) do
		if (ticket == t) then
			-- If this slot is already filled then it means the parent node of the slice has
			-- already been registered, so ignore this one.
			if (self.particles[i] ~= nil) then
				return;
			end
			
			self.particles[i] = spawnedEntityId;
			ParticleComponentRequestBus.Event.Enable(self.particles[i], false);
			
			if (self.Properties.DebugSpawner == true) then
				Debug.Log("ParticleSpawner [" .. tostring(self.Properties.ParticleSpawnEvent) .. "]: Spawned " .. tostring(StarterGameEntityUtility.GetEntityName(spawnedEntityId)) .. " into slot " .. tostring(i));
			end
			
			found = true;
			break;
		end
	end
	
	if (not found) then
		if (self.Properties.DebugSpawner == true) then
			Debug.Log("ParticleSpawner [" .. tostring(self.Properties.ParticleSpawnEvent) .. "]: Couldn't find the ticket for the spawned entity: " .. tostring(spawnedEntityId));
		end
	else
		-- Save the scale and rotation of the particle before it's trashed
		-- from multiple uses.
		self.originalTM = TransformBus.Event.GetWorldTM(spawnedEntityId);
		self.originalTM:SetTranslation(Vector3(0.0, 0.0, 0.0));
		if (self.Properties.DebugSpawner == true) then
			Debug.Log("ParticleSpawner [" .. tostring(self.Properties.ParticleSpawnEvent) .. "]: Ticket was found: " .. tostring(spawnedEntityId));
		end
	end
end

return particlespawner;