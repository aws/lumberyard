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

local jackaimdirectionprovider = 
{
    Properties = 
    {
		Camera = {default = EntityId()},
    },
}

--------------------------------------------------
-- Component behavior
--------------------------------------------------

function jackaimdirectionprovider:OnActivate()    
	self.requestAimUpdateEventId = GameplayNotificationId(self.entityId, "RequestAimUpdate", "float");
	self.requestAimUpdateHandler = GameplayNotificationBus.Connect(self, self.requestAimUpdateEventId);

	self.SetAimDirectionId = GameplayNotificationId(self.entityId, "SetAimDirection", "float");
	self.SetAimOriginId = GameplayNotificationId(self.entityId, "SetAimOrigin", "float");
end

function jackaimdirectionprovider:OnEventBegin(value)
	if (GameplayNotificationBus.GetCurrentBusId() == self.requestAimUpdateEventId) then
		local camTm = TransformBus.Event.GetWorldTM(self.Properties.Camera);
		local pos = camTm:GetTranslation();
		local dir = camTm:GetColumn(1);
		GameplayNotificationBus.Event.OnEventBegin(self.SetAimDirectionId, dir);
		GameplayNotificationBus.Event.OnEventBegin(self.SetAimOriginId, pos);	
	end
end
	

return jackaimdirectionprovider;