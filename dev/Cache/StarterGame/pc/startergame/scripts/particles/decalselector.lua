local utilities = require "scripts/common/utilities"

local decalselector =
{
	Properties =
	{
		DecalSpawnEvent = { default = "SpawnDecalName", description = "The name of the event that will trigger the spawner." },
	},
}

function decalselector:OnActivate()

	self.eventId = GameplayNotificationId(self.entityId, self.Properties.DecalSpawnEvent, "float");
	self.handler = GameplayNotificationBus.Connect(self, self.eventId);

end

function decalselector:OnDeactivate()

	self.handler:Disconnect();
	self.handler = nil;

end

function decalselector:OnEventBegin(value)
	
	if (not utilities.GetDebugManagerBool("EnableDynamicDecals", true)) then
		return;
	end
	
	if (GameplayNotificationBus.GetCurrentBusId() == self.eventId) then
		--Debug.Log("Passing on to children!");
		
		local particleManager = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("ParticleManager"));
		if (particleManager == nil or particleManager:IsValid() == false) then
			Debug.Assert("Invalid particle manager.");
		end
		
		-- TODO: This is a temporary workaround for not rendering decals on movable objects.
		-- The engine doesn't render decals on objects that are moving and then they pop
		-- back in when the object stops moving. As a result, we just won't place any decals
		-- onto component entities that can move (crates, A.I., player, etc.).
		if (ParticleManagerComponentRequestsBus.Event.AcceptsDecals(particleManager, value)) then
			DecalSelectorComponentRequestsBus.Event.SpawnDecal(self.entityId, value);
		end
	end
	
end

return decalselector;