local utilities = require "scripts/common/utilities"

local aispawner =
{
	Properties =
	{
		Enabled = { default = true },
		OverrideDebugManager = { default = false },
		GroupId = { default = "", description = "Spawner's spawn group." },
		AlertId = { default = "", description = "Used by A.I. to call for help." },
		
		DeathMessageTarget = {default = EntityId(), description = "When my AI dies send a message at this entity."},
		DeathMessage = {default = "", description = "Actual message to send when my entity dies."},
	},
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
	self.isDead = false;
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
		if (utilities.GetDebugManagerBool("PreventAIDisabling", false)) then
			self:SpawnAI();
		end
		
		if (self:ShouldSpawn()) then
			self:AdjustLivingCount(1);
		end
		
		self.performedFirstUpdate = true;
	end
	
	-- Unregister from the tick bus.
	self.tickHandler:Disconnect();
	self.tickHandler = nil;

end

function aispawner:ShouldSpawn()
	-- Check with the debug manager whether or not we should spawn A.I.
	local shouldSpawn = true;
	if (self.Properties.OverrideDebugManager == false) then
		shouldSpawn = utilities.GetDebugManagerBool("EnableAISpawning", shouldSpawn);
	end
	
	return shouldSpawn and self.Properties.Enabled and self.isDead == false;
end

function aispawner:SpawnAI()
	if (self.spawnedEntityId == nil) then -- Will be nil if the spawner is not enabled or spawning failed
		-- Spawn the A.I.
		if (self:ShouldSpawn()) then
			-- Look below the spawner for a piece of terrain to spawn on.
			local spawnerPos = TransformBus.Event.GetWorldTM(self.entityId);
			local rayCastConfig = RayCastConfiguration();
			rayCastConfig.origin = spawnerPos:GetTranslation();
			rayCastConfig.direction =  Vector3(0.0, 0.0, -1.0);
			rayCastConfig.maxDistance = 2.0;
			rayCastConfig.maxHits = 1;
			rayCastConfig.physicalEntityTypes = PhysicalEntityTypes.Static + PhysicalEntityTypes.Terrain;
			local hits = PhysicsSystemRequestBus.Broadcast.RayCast(rayCastConfig);
			if (#hits > 0) then
				local tm = spawnerPos;
				tm:SetTranslation(hits[1].position);
				self.spawnTicket = SpawnerComponentRequestBus.Event.SpawnAbsolute(self.entityId, tm);
				if (self.spawnTicket == nil) then
					Debug.Log("Spawn failed");
				end
			else
				-- This asserts if we couldn't find terrain below the spawner to place the
				-- slice.
				-- This would need to be changed if we had flying enemies (obviously).
				-- I want this to be an assert, but apparently asserts and warnings don't
				-- do anything so I'll have to keep it as a log.
				Debug.Log("AISpawner: '" .. tostring(StarterGameUtility.GetEntityName(self.entityId)) .. "' couldn't find a point to spawn the A.I.");
			end
		end
	else
		--Debug.Log("AISpawner " .. tostring(StarterGameUtility.GetEntityName(self.entityId)) .. " tried to spawn an A.I. with one already active.");
	end
end

function aispawner:DespawnAI()
	if (self.spawnedEntityId ~= nil) then -- Will be nil if the spawner is not enabled or spawning failed
		self:DisableAI();
		GameEntityContextRequestBus.Broadcast.DestroyDynamicSliceByEntity(self.spawnedEntityId);
		--local parentId = StarterGameUtility.GetParentEntity(self.spawnedEntityId);
		--GameEntityContextRequestBus.Broadcast.DestroyGameEntityAndDescendants(parentId);
		--StarterGameUtility.DeactivateGameEntityAndDescendants(parentId);
		self.spawnedEntityId = nil;
	else
		--Debug.Log("AISpawner " .. tostring(StarterGameUtility.GetEntityName(self.entityId)) .. " tried to despawn an A.I. without having one active.");
	end
end

function aispawner:EnableAI()
	if (self.spawnedEntityId ~= nil) then -- Will be nil if the spawner is not enabled or spawning failed
		local eventId = GameplayNotificationId(self.spawnedEntityId, "Enable", "float");
		GameplayNotificationBus.Event.OnEventBegin(eventId, self.entityId);
	end
end

function aispawner:DisableAI()
	if (self.spawnedEntityId ~= nil) then -- Will be nil if the spawner is not enabled or spawning failed
		local eventId = GameplayNotificationId(self.spawnedEntityId, "Disable", "float");
		GameplayNotificationBus.Event.OnEventBegin(eventId, self.entityId);
	end
end

function aispawner:EnteredAITrigger(groupId)
	if (groupId == self.Properties.GroupId) then
		self:SpawnAI();
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
		self:AdjustLivingCount(-1);
	end
end

function aispawner:AdjustLivingCount(adjustment)
	if (self.Properties.GroupId ~= "" and self.Properties.GroupId ~= nil) then
		local counterName = tostring(self.Properties.GroupId) .. "_alive";
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

	local isMainEntity = StarterGameUtility.EntityHasTag(spawnedEntityId, "AICharacter");
	if (self.spawnTicket == ticket and isMainEntity) then
		--Debug.Log("Spawned A.I. : " .. tostring(spawnedEntityId));
		self.spawnedEntityId = spawnedEntityId;
		local eventId = GameplayNotificationId(spawnedEntityId, "AISpawned", "float");
		GameplayNotificationBus.Event.OnEventBegin(eventId, self.entityId);
		
		local alertIdEventId = GameplayNotificationId(spawnedEntityId, "SetAlertID", "float");
		GameplayNotificationBus.Event.OnEventBegin(alertIdEventId, self.Properties.AlertId);
				
		self.spawnedDiedEventId = GameplayNotificationId(spawnedEntityId, "HealthEmpty", "float");
		self.spawnedDiedHandler = GameplayNotificationBus.Connect(self, self.spawnedDiedEventId);
	end

end

return aispawner;