--[[
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--]]

local onentityspawner =
{
	Properties =
	{
		SpawnMessage = {default = "SpawnItem", description = "Gameplay message (with the entity as the param) to trigger the spawn."},
	},
}

function onentityspawner:OnActivate()	
	
	self.spawnTriggerEventId = GameplayNotificationId(self.entityId, self.Properties.SpawnMessage, "float");
	self.spawnTriggerHandler = GameplayNotificationBus.Connect(self, self.spawnTriggerEventId);
	Debug.Log("OnEntitySpawner::OnActivate : ID: " .. tostring(self.entityId) .. " message: " .. self.Properties.SpawnMessage .. " messageID: ".. tostring(self.spawnTriggerEventId));

	self.originalTransform = TransformBus.Event.GetWorldTM(self.entityId);	
end

function onentityspawner:OnDeactivate()
	if (self.spawnTriggerHandler ~= nil) then
		self.spawnTriggerHandler:Disconnect();
		self.spawnTriggerHandler = nil;
	end
end

function onentityspawner:OnEventBegin(value)
	Debug.Log("OnEntitySpawner::OnEventBegin : messageID: " .. tostring(GameplayNotificationBus.GetCurrentBusId()) .. "waiting for: " .. tostring(self.spawnTriggerEventId) .. " value: ".. tostring(value));
	if (GameplayNotificationBus.GetCurrentBusId() == self.spawnTriggerEventId) then
		Debug.Log("OnEntitySpawner::OnEventBegin : spawnTriggerEventId" .. tostring(value));
		local rayCastConfig = RayCastConfiguration();
		rayCastConfig.origin = TransformBus.Event.GetWorldTM(value):GetTranslation() + Vector3(0.0, 0.0, 1.0);
		rayCastConfig.direction =  Vector3(0.0, 0.0, -1.0);
		rayCastConfig.maxDistance = 3.0;
		rayCastConfig.maxHits = 1;
		rayCastConfig.physicalEntityTypes = PhysicalEntityTypes.Static + PhysicalEntityTypes.Terrain;		
		local hits = PhysicsSystemRequestBus.Broadcast.RayCast(rayCastConfig);
		if (#hits > 0) then
			local tm = Transform.CreateIdentity();
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
			Debug.Log("OnEntitySpawner: '" .. tostring(StarterGameEntityUtility.GetEntityName(value)) .. "' couldn't find a point to spawn the Item, spawning at my initial position.");
			self.spawnTicket = SpawnerComponentRequestBus.Event.SpawnAbsolute(self.entityId, self.originalTransform);
			if (self.spawnTicket == nil) then
				Debug.Log("Spawn failed");
			end
		end
	end
end

return onentityspawner;