local debugmanager =
{
	Properties =
	{
		DebugTheDebugManager = { default = true },
		AI =
		{
			EnableAISpawning = { default = true, description = "Enable the spawning of A.I. mobs." },
			EnableAICombat = { default = true, description = "Allow the A.I. to actively combat the player." },
			PreventAIDisabling = { default = false, description = "Ignores trigger volumes that enable/disable A.I." },
		},
		
		Player =
		{
			InfiniteEnergy = { default = false, description = "If true, player does not lose energy when firing." },
			GodMode = { default = false, description = "If true, the player can't lose health." },
			StartWithLauncher = { default = false, description = "If true, the player will start with the launcher enabled." },
		},
		
		Rendering =
		{
			EnableDynamicDecals = { default = true, description = "Enable dynamic decals such as the plasma rifle's scorch mark." },
		},
	},
}

function debugmanager:OnActivate()
	-- We can't gaurantee that everything will already be initialised at this point and thus
	-- some entities may not receive our messages.
	-- Additionally, as we can't control the update order we can't set the values on the first tick
	-- because it's likely that something else will want to do initialisation on the first tick and
	-- may update before us.
	-- As a result, we have to wait for a callback from the DebugManagerComponent saying that it's
	-- been activated (which SHOULD be before anything is ticked).
	self.debugHandler = DebugManagerComponentNotificationBus.Connect(self, self.entityId);
	
	self.tickBusHandler = TickBus.Connect(self);
end

function debugmanager:OnDeactivate()
	
	if (self.tickBusHandler ~= nil) then
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end
	
	if (self.debugHandler ~= nil) then
		self.debugHandler:Disconnect();
		self.debugHandler = nil;
	end
	
end

function debugmanager:OnTick(deltaTime, timePoint)
	
	if (self.Properties.Player.StartWithLauncher) then
		local playerId = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"));
		if (playerId:IsValid()) then
			GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(playerId, "EventEnableWeapon", "float"), "Grenade");
		end
	end
	
	self.tickBusHandler:Disconnect();
	self.tickBusHandler = nil;
	
end

function debugmanager:OnDebugManagerComponentActivated()

	local mt = getmetatable(self.Properties);
	self:StoreDebugVarBool("EnableAISpawning", self.Properties.AI.EnableAISpawning, mt.AI.EnableAISpawning.default);
	self:StoreDebugVarBool("EnableAICombat", self.Properties.AI.EnableAICombat, mt.AI.EnableAICombat.default);
	self:StoreDebugVarBool("PreventAIDisabling", self.Properties.AI.PreventAIDisabling, mt.AI.PreventAIDisabling.default);
	
	self:StoreDebugVarBool("InfiniteEnergy", self.Properties.Player.InfiniteEnergy, mt.Player.InfiniteEnergy.default);
	self:StoreDebugVarBool("GodMode", self.Properties.Player.GodMode, mt.Player.GodMode.default);
	self:StoreDebugVarBool("StartWithLauncher", self.Properties.Player.StartWithLauncher, mt.Player.StartWithLauncher.default);
	
	self:StoreDebugVarBool("EnableDynamicDecals", self.Properties.Rendering.EnableDynamicDecals, mt.Rendering.EnableDynamicDecals.default);
	
end

function debugmanager:ChooseDebugOrReleaseValue(debugVal, releaseVal)
	-- Use the default value if we're in a release build; otherwise use the debug value.
	local res = debugVal;
	if (Config.Build > Config.Profile) then
		res = releaseVal;
	end
	return res;
end

function debugmanager:StoreDebugVarBool(event, value, default)
	if (self.Properties.DebugTheDebugManager == true) then
		Debug.Log("Adding variable " .. tostring(event) .. " (default: " .. tostring(default) .. "): " .. tostring(value));
	end
	DebugManagerComponentRequestsBus.Event.SetDebugBool(self.entityId, event, self:ChooseDebugOrReleaseValue(value, default));
end

function debugmanager:StoreDebugVarFloat(event, value, default)
	if (self.Properties.DebugTheDebugManager == true) then
		Debug.Log("Adding variable " .. tostring(event) .. " (default: " .. tostring(default) .. "): " .. tostring(value));
	end
	DebugManagerComponentRequestsBus.Event.SetDebugFloat(self.entityId, event, self:ChooseDebugOrReleaseValue(value, default));
end

return debugmanager;