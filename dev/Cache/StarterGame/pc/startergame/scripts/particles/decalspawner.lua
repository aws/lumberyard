
local decalspawner =
{
	Properties =
	{
		SurfaceName = { default = "mat_default", description = "The name of the surface that this spawner relates to." },
		PoolSize = { default = 15, description = "How many decals to have in this pool." },
		DebugSpawner = { default = false, description = "If true, additional debug information will be displayed in the console." },
	},
}

function decalspawner:OnActivate()

	self.tickBusHandler = TickBus.Connect(self);
	self.performedFirstUpdate = false;
	
	self.spawnerHandler = SpawnerComponentNotificationBus.Connect(self, self.entityId);
	self.spawnTickets = {};
	self.spawnParams = {};

end

function decalspawner:OnDeactivate()
	
	if (self.spawnerHandler ~= nil) then
		self.spawnerHandler:Disconnect();
		self.spawnerHandler = nil;
	end
	
	if (self.tickBusHandler ~= nil) then
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end

end

function decalspawner:OnTick(deltaTime, timePoint)

	if (self.performedFirstUpdate == false) then
		-- Find our parent that will hold the pool of decals.
		self.parent = StarterGameEntityUtility.GetParentEntity(self.entityId);
		
		-- Set the surface index.
		self.surfaceIndex = StarterGameMaterialUtility.GetSurfaceIndexFromName(self.Properties.SurfaceName);
		
		-- Only try to create the pool if we found an entity that will hold them.
		for i = 1, self.Properties.PoolSize do
			self:SpawnSomething(nil);
		end
	end
	
	-- We shouldn't need to do subsequent updates. We're only here to populate
	-- the pools with decal entities.
	if (self.tickBusHandler ~= nil) then
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end

end

function decalspawner:SpawnSomething(params)

	-- Spawn something.
	local ticket = SpawnerComponentRequestBus.Event.SpawnAbsolute(self.entityId, Transform.CreateIdentity());
	if (ticket == nil) then
		Debug.Log("Spawn failed");
	else
		if (self.Properties.DebugSpawner == true) then
			Debug.Log("Spawn success");
		end
		table.insert(self.spawnTickets, ticket);
	end
	
end

function decalspawner:OnEntitySpawned(ticket, spawnedEntityId)
	--Debug.Log("Spawned: " .. tostring(ticket) .. " - " .. tostring(spawnedEntityId));
	
	local found = false;
	for i,t in ipairs(self.spawnTickets) do
		if (ticket == t) then
			DecalSelectorComponentRequestsBus.Event.AddDecalToPool(self.parent, spawnedEntityId, self.surfaceIndex);
			table.remove(self.spawnTickets, i);
			
			found = true;
			break;
		end
	end
	
	if (not found) then
		Debug.Log("Couldn't find the ticket for the spawned entity: " .. tostring(spawnedEntityId));
	else
		if (self.Properties.DebugSpawner == true) then
			Debug.Log("Ticket was found: " .. tostring(spawnedEntityId));
		end
	end
end

return decalspawner;
