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

local jackstrafefacingprovider = 
{
    Properties = 
    {
		Camera = {default = EntityId()},
    },
}

--------------------------------------------------
-- Component behavior
--------------------------------------------------

function jackstrafefacingprovider:OnActivate()    

	-- Tick needed to detect aim timeout
	self.tickBusHandler = TickBus.Connect(self);

end

function jackstrafefacingprovider:OnDeactivate()
	self.tickBusHandler:Disconnect();
	self.tickBusHandler = nil;
end
function jackstrafefacingprovider:OnTick(deltaTime, timePoint)
	local cameraTm = TransformBus.Event.GetWorldTM(self.Properties.Camera);
	local flatForward = cameraTm:GetColumn(0):Cross(Vector3(0,0,-1));
	self.setStrafeFacingId = GameplayNotificationId(self.entityId, "SetStrafeFacing", "float");
	GameplayNotificationBus.Event.OnEventBegin(self.setStrafeFacingId, flatForward);
end
	

return jackstrafefacingprovider;