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