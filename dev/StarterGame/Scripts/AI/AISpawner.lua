local utilities = require "scripts/common/utilities"

local aispawner =
{
	Properties =
	{
		Enabled = { default = true },
		OverrideDebugManager = { default = false },
		GroupId = { default = "", description = "Spawner's spawn group." },
		AlertId = { default = "", description = "Used by A.I. to call for help." },
		SpawnInCombat = { default = false, description = "When true, the A.I. will spawn actively attacking the player." },
		
		Teleport =
		{
			IsTeleportedIn = { default = false },
			SpawnEffect = { default = "SpawnTeleportIn", description = "The particle effect to play when spawning in." },
			SpawnDelay = { default = 0.5, description = "Time before this A.I. begins to teleport in.", suffix = " s" },
			SpawnDelayVariance = { default = 1.0, description = "Additional time to the SpawnDelay for randomness.", suffix = " s" },
			DelayBeforeActualSpawn = { default = 0.5, description = "Time between the particle starting and the A.I. appearing.", suffix = " s" },
		},
		
		DeathMessageTarget = {default = EntityId(), description = "When my AI dies send a message at this entity."},
		DeathMessage = {default = "", description = "Actual message to send when my entity dies."},
	},
	
	Data_GroupCountForAlive = "_alive";
	Data_GroupCountForActive = "_active";
	Data_GroupCountForDead = "_dead";
}

function aispawner:OnActivate()

	self.spawnerHandler = SpawnerComponentNotificationBus.Connect(self, self.entityId);
	self.spawnTicket = false;
	
	self.tickHandler = TickBus.Connect(self);
	self.performedFirstUpdate = false;
	
	self.enteredAITriggerEventId = GameplayNotificationId(EntityId(), "EnteredAITrigger", "float");
	self.enteredAITriggerHandler = GameplayNotificationBus.Connect(self, self.enteredAITriggerEventId);	
	self.exitedAITriggerEventId = GameplayNotificationId(EntityId(), "ExitedAITrigger", "float");
	self.exitedAITriggerHandler = GameplayNotificationBus.Connect(self, self.exitedAITriggerEventId);	
	self.spawnedDiedEventId = nil;
	self.spawnedDiedHandler = nil;
	self.spawnFightingEventId = nil;
	self.isDead = false;
	
	self.alreadyTeleportedIn = false;
end

function aispawner:OnDeactivate()

	if (self.spawnedDiedHandler ~= nil) then
		self.spawnedDiedHandler:Disconnect();
		self.spawnedDiedHandler = nil;
	end
	
	if (self.exitedAITriggerHandler ~= nil) then
		self.exitedAITriggerHandler:Disconnect();
		self.exitedAITriggerHandler = nil;
	end
	
	if (self.enteredAITriggerHandler ~= nil) then
		self.enteredAITriggerHandler:Disconnect();
		self.enteredAITriggerHandler = nil;
	end
	
	if (self.tickHandler ~= nil) then
		self.tickHandler:Disconnect();
		self.tickHandler = nil;
	end
	
	if (self.spawnerHandler ~= nil) then
		self.spawnerHandler:Disconnect();
		self.spawnerHandler = nil;
	end
end

function aispawner:OnTick(deltaTime, timePoint)

	if (self.performedFirstUpdate == false) then
		-- Increment the 'alive' counter when the spawner is initialised.
		if (self.Properties.Enabled) then
			self:AdjustPersistentDataCount(self.Data_GroupCountForAlive, 1);
		end
		
		if (not self.Properties.Teleport.IsTeleportedIn) then
			if (utilities.GetDebugManagerBool("PreventAIDisabling", false)) then
				if (self:ShouldSpawn()) then
					self:SpawnAI();
				end
			end
		else
			local particleMan = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("ParticleManager"));
			self.particleSpawnEventId = GameplayNotificationId(particleMan, "SpawnParticleEvent", "float");
		end
		
		self.performedFirstUpdate = true;
	end
	
	-- If this A.I. doesn't teleport in then unregister from the tick bus as we don't need it.
	if (not self.Properties.Teleport.IsTeleportedIn) then
		self.tickHandler:Disconnect();
		self.tickHandler = nil;
		return;
	end
	
	-- The rest of this function should only ever be hit by spawners that DO teleport in.
	if (self.teleportingIn) then
		if (self.spawnDelay > 0.0) then
			self.spawnDelay = self.spawnDelay - deltaTime;
			-- Trigger the particle.
			if (self.spawnDelay <= 0.0) then
				self.spawnDelay = 0.0;
				
				-- Get the point to spawn (plus half the height of the A.I.).
				local tm = self:GetLocationToSpawn();
				tm:SetTranslation(tm:GetTranslation() + Vector3(0.0, 0.0, 0.8));
				
				local params = ParticleSpawnerParams();
				params.transform = tm;
				params.event = self.Properties.Teleport.SpawnEffect;
				
				GameplayNotificationBus.Event.OnEventBegin(self.particleSpawnEventId, params);
			end
		elseif (self.spawnDelayPostParticle > 0.0) then
			self.spawnDelayPostParticle = self.spawnDelayPostParticle - deltaTime;
			-- Trigger the spawn.
			if (self.spawnDelayPostParticle <= 0.0) then
				self.spawnDelayPostParticle = 0.0;
				self.teleportingIn = false;
				--self.alreadyTeleportedIn = true;
				
				self:SpawnAI();
			end
		end
	end

end

function aispawner:ShouldSpawn()
	-- Check with the debug manager whether or not we should spawn A.I.
	local shouldSpawn = true;
	if (self.Properties.OverrideDebugManager == false) then
		shouldSpawn = utilities.GetDebugManagerBool("EnableAISpawning", shouldSpawn);
	end
	
	return shouldSpawn and self.Properties.Enabled and self.isDead == false;
end

function aispawner:BeginTeleportingIn()
	self.teleportingIn = true;
	self.spawnDelay = self.Properties.Teleport.SpawnDelay + utilities.RandomPlusMinus(self.Properties.Teleport.SpawnDelayVariance, 0.5);
	self.spawnDelayPostParticle = self.Properties.Teleport.DelayBeforeActualSpawn;
	
	if (self.spawnDelay <= 0.0) then
		self.spawnDelay = 0.0001;
	end
	if (self.spawnDelayPostParticle <= 0.0) then
		self.spawnDelayPostParticle = 0.0001;
	end
end

function aispawner:SpawnAI()
	if (self.spawnedEntityId == nil) then -- Will be nil if the spawner is not enabled or spawning failed
		-- Spawn the A.I.
		self.spawnTicket = SpawnerComponentRequestBus.Event.SpawnAbsolute(self.entityId, self:GetLocationToSpawn());
		if (self.spawnTicket == nil) then
			Debug.Log("AISpawner: '" .. tostring(StarterGameEntityUtility.GetEntityName(self.entityId)) .. "' spawn failed");
		end
	else
		--Debug.Log("AISpawner " .. tostring(StarterGameEntityUtility.GetEntityName(self.entityId)) .. " tried to spawn an A.I. with one already active.");
	end
end

function aispawner:DespawnAI()
	if (self.spawnedEntityId ~= nil) then -- Will be nil if the spawner is not enabled or spawning failed
		-- Only decrement the alive counter if the A.I. is not dead at the time of despawning.
		if (self.isDead == false) then
			self:AdjustPersistentDataCount(self.Data_GroupCountForActive, -1);
		end
	
		self:DisableAI();
		self.spawnedDiedEventId = nil;
		self.spawnedDiedHandler:Disconnect();
		self.spawnedDiedHandler = nil;
		self.spawnFightingEventId = nil;
		GameEntityContextRequestBus.Broadcast.DestroyDynamicSliceByEntity(self.spawnedEntityId);
		self.spawnedEntityId = nil;
	else
		--Debug.Log("AISpawner " .. tostring(StarterGameEntityUtility.GetEntityName(self.entityId)) .. " tried to despawn an A.I. without having one active.");
	end
end

function aispawner:EnableAI()
	if (self.spawnedEntityId ~= nil) then -- Will be nil if the spawner is not enabled or spawning failed
		local eventId = GameplayNotificationId(self.spawnedEntityId, "Enable", "float");
		GameplayNotificationBus.Event.OnEventBegin(eventId, self.entityId);
		
		-- Make sure they continue fighting the player.
		if (self.Properties.SpawnInCombat) then
			GameplayNotificationBus.Event.OnEventBegin(self.spawnFightingEventId, true);
		end
	end
end

function aispawner:DisableAI()
	if (self.spawnedEntityId ~= nil) then -- Will be nil if the spawner is not enabled or spawning failed
		local eventId = GameplayNotificationId(self.spawnedEntityId, "Disable", "float");
		GameplayNotificationBus.Event.OnEventBegin(eventId, self.entityId);
	end
end

function aispawner:GetLocationToSpawn()
	-- Look below the spawner for a piece of terrain to spawn on.
	local tm = Transform.CreateIdentity();
	local spawnerPos = TransformBus.Event.GetWorldTM(self.entityId);
	local rayCastConfig = RayCastConfiguration();
	rayCastConfig.origin = spawnerPos:GetTranslation();
	rayCastConfig.direction =  Vector3(0.0, 0.0, -1.0);
	rayCastConfig.maxDistance = 2.0;
	rayCastConfig.maxHits = 1;
	rayCastConfig.physicalEntityTypes = PhysicalEntityTypes.Static + PhysicalEntityTypes.Terrain;
	local hits = PhysicsSystemRequestBus.Broadcast.RayCast(rayCastConfig);
	if (#hits > 0) then
		tm = spawnerPos;
		tm:SetTranslation(hits[1].position);
	else
		-- This asserts if we couldn't find terrain below the spawner to place the
		-- slice.
		-- This would need to be changed if we had flying enemies (obviously).
		-- I want this to be an assert, but apparently asserts and warnings don't
		-- do anything so I'll have to keep it as a log.
		Debug.Log("AISpawner: '" .. tostring(StarterGameEntityUtility.GetEntityName(self.entityId)) .. "' couldn't find a point to spawn the A.I.");
	end
	
	return tm;
end

function aispawner:EnteredAITrigger(groupId)
	if (groupId == self.Properties.GroupId) then
		if (self:ShouldSpawn()) then
			if (self.Properties.Teleport.IsTeleportedIn and not self.alreadyTeleportedIn) then
				self:BeginTeleportingIn();
			else
				self:SpawnAI();
			end
		end
	end
end

function aispawner:ExitedAITrigger(groupId)
	if (groupId == self.Properties.GroupId) then
		self:DespawnAI();
	end
end

function aispawner:SpawnedDied()
	--Debug.Log("A.I. died : " .. tostring(self.spawnedEntityId));
	if ((self.Properties.DeathMessageTarget ~= EntityId()) and (self.Properties.DeathMessage ~= "")) then
		--Debug.Log("A.I. deadMessage to : " .. tostring(self.Properties.DeathMessageTarget) .. ", with message : \"" .. self.Properties.DeathMessage .. "\" param : " .. tostring(self.spawnedEntityId));
		local eventId = GameplayNotificationId(self.Properties.DeathMessageTarget, self.Properties.DeathMessage, "float");
		--Debug.Log("A.I. deadMessage messageID : " .. tostring(eventId));
		GameplayNotificationBus.Event.OnEventBegin(eventId, self.spawnedEntityId);
	end
	
	if (self.isDead == false) then
		self.isDead = true;
		self:AdjustPersistentDataCount(self.Data_GroupCountForAlive, -1);
		self:AdjustPersistentDataCount(self.Data_GroupCountForActive, -1);
		self:AdjustPersistentDataCount(self.Data_GroupCountForDead, 1);
	end
end

function aispawner:AdjustPersistentDataCount(suffix, adjustment)
	if (self.Properties.GroupId ~= "" and self.Properties.GroupId ~= nil) then
		local counterName = tostring(self.Properties.GroupId) .. tostring(suffix);
		local value = 0;
		if (PersistentDataSystemRequestBus.Broadcast.HasData(counterName)) then
			value = PersistentDataSystemRequestBus.Broadcast.GetData(counterName);
			-- Currently the manipulator will create a 'nil' data element when it's activated
			-- so we have to check for that.
			if (value == nil) then
				value = 0;
			end
		end
		value = value + adjustment;
		local res = PersistentDataSystemRequestBus.Broadcast.SetData(counterName, value, true);
		--Debug.Log("Set " .. tostring(counterName) .. " to " .. tostring(value) .. "? " .. tostring(res));
	end
end

function aispawner:OnEventBegin(value)
	if (GameplayNotificationBus.GetCurrentBusId() == self.enteredAITriggerEventId) then
		self:EnteredAITrigger(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.exitedAITriggerEventId) then
		self:ExitedAITrigger(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.spawnedDiedEventId) then
		self:SpawnedDied();
	end
end

function aispawner:OnEntitySpawned(ticket, spawnedEntityId)

	local isMainEntity = StarterGameEntityUtility.EntityHasTag(spawnedEntityId, "AICharacter");
	if (self.spawnTicket == ticket and isMainEntity) then
		--Debug.Log("Spawned A.I. : " .. tostring(spawnedEntityId));
		self.spawnedEntityId = spawnedEntityId;
		local eventId = GameplayNotificationId(spawnedEntityId, "AISpawned", "float");
		GameplayNotificationBus.Event.OnEventBegin(eventId, self.entityId);
		
		local alertIdEventId = GameplayNotificationId(spawnedEntityId, "SetAlertID", "float");
		GameplayNotificationBus.Event.OnEventBegin(alertIdEventId, self.Properties.AlertId);
		
		if (self.Properties.SpawnInCombat) then
			self.spawnFightingEventId = GameplayNotificationId(spawnedEntityId, "FightPlayer", "float");
			GameplayNotificationBus.Event.OnEventBegin(self.spawnFightingEventId, true);
		end
				
		self.spawnedDiedEventId = GameplayNotificationId(spawnedEntityId, "HealthEmpty", "float");
		self.spawnedDiedHandler = GameplayNotificationBus.Connect(self, self.spawnedDiedEventId);
		
		self:AdjustPersistentDataCount(self.Data_GroupCountForActive, 1);
	end

end

return aispawner;