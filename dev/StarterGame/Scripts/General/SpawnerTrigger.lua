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

local spawnertrigger =
{
	Properties =
	{
		EventSetPos = { default = "SpawnerSetPos", description = "The name of the event that will set the spawner's position.", },
		EventSetForward = { default = "SpawnerSetForward", description = "The name of the event that will set the spawner's forward vector.", },
		EventSpawn = { default = "SpawnEffect", description = "The name of the event that will trigger the spawner." },
	},
}

function spawnertrigger:OnActivate()

	self.setPosEventId = GameplayNotificationId(self.entityId, self.Properties.EventSetPos, "float");
	self.setPosHandler = GameplayNotificationBus.Connect(self, self.setPosEventId);
	self.setForwardEventId = GameplayNotificationId(self.entityId, self.Properties.EventSetForward, "float");
	self.setForwardHandler = GameplayNotificationBus.Connect(self, self.setForwardEventId);
	self.spawnEventId = GameplayNotificationId(self.entityId, self.Properties.EventSpawn, "float");
	self.spawnHandler = GameplayNotificationBus.Connect(self, self.spawnEventId);
	
	self.spawnTicket = false;
	
	self:ResetPosAndForward();
	
end

function spawnertrigger:OnDeactivate()

	self.setPosHandler:Disconnect();
	self.setPosHandler = nil;
	self.setForwardHandler:Disconnect();
	self.setForwardHandler = nil;
	self.spawnHandler:Disconnect();
	self.spawnHandler = nil;

end

function spawnertrigger:ResetPosAndForward()

	self.objPos = Vector3(0.0, 0.0, 0.0);
	self.objForward = Vector3(0.0, 1.0, 0.0);	-- default forward vector

end

function spawnertrigger:SpawnSomething()

	-- Spawn something.
	local tm = Transform.CreateIdentity();
	
	-- If the forward vector is the default then we can just use the identity, otherwise
	-- we'll have to construct the transform using the provided forward vector.
	if (self.objForward ~= Vector3(0.0, 1.0, 0.0)) then
		--Debug.Log("Forward: " .. tostring(self.objForward));
		local forward = self.objForward:GetNormalized();
		local up = Vector3(0.0, 0.0, 1.0);
		local right = up:Cross(forward):GetNormalized();
		up = forward:Cross(right):GetNormalized();
		tm:SetColumn(0, right);
		tm:SetColumn(1, forward);
		tm:SetColumn(2, up);
	end
	
	tm:SetTranslation(self.objPos);
	self.spawnTicket = SpawnerComponentRequestBus.Event.SpawnAbsolute(self.entityId, tm);
	if (self.spawnTicket == nil) then
		Debug.Log("Spawn failed");
	end
	
	-- Prepare for the next spawn event.
	self:ResetPosAndForward();
	
end

function spawnertrigger:OnEventBegin(value)

	if (GameplayNotificationBus.GetCurrentBusId() == self.spawnEventId) then
		self:SpawnSomething();
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.setPosEventId) then
		self.objPos = value;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.setForwardEventId) then
		self.objForward = value;
	end

end

return spawnertrigger;