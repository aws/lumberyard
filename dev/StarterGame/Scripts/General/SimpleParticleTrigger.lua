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

local simpleparticletrigger =
{
	Properties =
	{
		EventPlay = { default = "PlayEffect", description = "The name of the event that will enable the particle effect.", },
	},
}

function simpleparticletrigger:OnActivate()

	self.playEventId = GameplayNotificationId(self.entityId, self.Properties.EventPlay, "float");
	self.playHandler = GameplayNotificationBus.Connect(self, self.playEventId);

end

function simpleparticletrigger:OnDeactivate()

	self.playHandler:Disconnect();
	self.playHandler = nil;
	
end

function simpleparticletrigger:OnEventBegin(value)

	if (GameplayNotificationBus.GetCurrentBusId() == self.playEventId) then
		ParticleComponentRequestBus.Event.SetVisibility(self.entityId, true);
		ParticleComponentRequestBus.Event.Enable(self.entityId, true);
	end

end

return simpleparticletrigger;