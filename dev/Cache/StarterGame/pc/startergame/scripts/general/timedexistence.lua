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

local timedexistence =
{
	Properties =
	{
		Lifespan = { default = 2, description = "Time that the entity will exist for.", suffix = " seconds" }
	},
}

function timedexistence:OnActivate()

	--Debug.Log("Activated");
	self.lifeRemaining = self.Properties.Lifespan;
	self.tickBusHandler = TickBus.Connect(self);

end

function timedexistence:OnDeactivate()

	if (self.tickBusHandler ~= nil) then
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end

end

function timedexistence:OnTick(deltaTime, timepoint)

	self.lifeRemaining = self.lifeRemaining - deltaTime;
	if (self.lifeRemaining <= 0.0) then
		--Debug.Log("Deleting " .. tostring(self.entityId));
		self:OnDeactivate();

		GameEntityContextRequestBus.Broadcast.DestroyGameEntity(self.entityId);
	end

end

return timedexistence;