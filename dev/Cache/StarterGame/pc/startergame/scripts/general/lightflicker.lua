----------------------------------------------------------------------------------------------------
--
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--
--
----------------------------------------------------------------------------------------------------
local LightFlicker =
{
    Properties =
    {
        FlickerInterval = 1.0,
        LightEntity = { default=EntityId(), description="Light entity to manipulate."},
    },
}

function LightFlicker:OnActivate()

    self.FlickerCountdown = self.Properties.FlickerInterval;
	-- If no light entity assigned, use our entity
    if (not self.Properties.LightEntity:IsValid()) then
        self.Properties.LightEntity = self.entityId;
    end

    Debug.Assert(self.Properties.LightEntity:IsValid(), "No entity is attached!");

    self.tickBusHandler = TickBus.Connect(self)
	Light.Event.Toggle(self.Properties.LightEntity)

    --Debug.Log("LightFlicker starting for entity: " .. tostring(self.Properties.LightEntity.id));
end

function LightFlicker:OnTick(deltaTime, timePoint)
    self.FlickerCountdown = self.FlickerCountdown - deltaTime;
    if (self.FlickerCountdown < 0.0) then
   	 	Light.Event.Toggle(self.Properties.LightEntity)
		self.FlickerCountdown = self.Properties.FlickerInterval;
    end
end

function LightFlicker:OnDeactivate()
	self.tickBusHandler:Disconnect()
end

return LightFlicker;
