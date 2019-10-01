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

local characterstatedeathhandler = 
{
    Properties = 
    {
		LensFlare = {default = EntityId()},
    }
}

function characterstatedeathhandler:OnActivate() 
    self.onDeathEventId = GameplayNotificationId(self.entityId, "OnDeath", "float");
    self.onDeathHandler = GameplayNotificationBus.Connect(self, self.onDeathEventId);
end   

function characterstatedeathhandler:OnDeactivate()
    self.onDeathHandler:Disconnect();
    self.onDeathHandler = nil;
end

function characterstatedeathhandler:OnEventBegin(value)
    if (GameplayNotificationBus.GetCurrentBusId() == self.onDeathEventId) then
		LensFlareComponentRequestBus.Event.TurnOffLensFlare(self.Properties.LensFlare);
    end
end

return characterstatedeathhandler;