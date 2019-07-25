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

local rotateobject =
{
	Properties =
	{		
		GraphicTurnTime = { default = 3, description = "Time to rotate a full rotation" },
	},
}

function rotateobject:OnActivate()
	self.tickBusHandler = TickBus.Connect(self);
		
	self.rotateTimer = 0;	
end

function rotateobject:OnDeactivate()		
	if(not self.tickBusHandler == nil) then
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end
end

function rotateobject:OnTick(deltaTime, timePoint)

	local twoPi = 6.283185307179586476925286766559;

	if(not (self.Properties.GraphicTurnTime == 0)) then
		self.rotateTimer = self.rotateTimer + (deltaTime / self.Properties.GraphicTurnTime);
		if(self.rotateTimer >= twoPi) then
			self.rotateTimer = self.rotateTimer - twoPi;
		end
	end
	
	
	--Debug.Log("Pickup graphic values:  Rotation: " .. self.rotateTimer .. " Height: " .. self.bobTimer);
	local tm = Transform.CreateRotationZ(self.rotateTimer);
	TransformBus.Event.SetLocalTM(self.entityId, tm);

end

return rotateobject;