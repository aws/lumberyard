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

local simplemotiontrigger =
{
	Properties =
	{
		EventPlay = { default = "StartRockslide", description = "The name of the event that will start playing the animation.", },
	},
}

function simplemotiontrigger:OnActivate()
	self.playEventId = GameplayNotificationId(self.entityId, self.Properties.EventPlay, "float");
	self.playHandler = GameplayNotificationBus.Connect(self, self.playEventId);
end

function simplemotiontrigger:OnDeactivate()
	self.playHandler:Disconnect();
	self.playHandler = nil;
end

function simplemotiontrigger:OnEventBegin(value)
	if (GameplayNotificationBus.GetCurrentBusId() == self.playEventId) then
		SimpleMotionComponentRequestBus.Event.PlayMotion(self.entityId);
	end
end

return simplemotiontrigger;