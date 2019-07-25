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

local simpleanimationtrigger =
{
	Properties =
	{
		EventPlay = { default = "PlayAnimation", description = "The name of the event that will start playing the animation.", },
		AnimationName = { default = "", description = "Name of animation to play.  Leave blank to play default animation.", },
		EndLoopAnimationName = { default = "", description = "Name of looping animation to play when main anim finishes.", },
	},
}

function simpleanimationtrigger:OnActivate()

	self.playEventId = GameplayNotificationId(self.entityId, self.Properties.EventPlay, "float");
	self.playHandler = GameplayNotificationBus.Connect(self, self.playEventId);

	if (self.Properties.EndLoopAnimationName ~= "") then
		self.animNotifyHandler = SimpleAnimationComponentNotificationBus.Connect(self, self.entityId);
	end
end

function simpleanimationtrigger:OnDeactivate()

	self.playHandler:Disconnect();
	self.playHandler = nil;

	if (self.animNotifyHandler ~= nil) then
		self.animNotifyHandler:Disconnect();
		self.animNotifyHandler = nil;
	end

end

function simpleanimationtrigger:OnAnimationStopped(layer)

	local animInfo = AnimatedLayer(self.Properties.EndLoopAnimationName, 0, true, 1.0, 0.0);
	SimpleAnimationComponentRequestBus.Event.StartAnimation(self.entityId, animInfo);
	
end

function simpleanimationtrigger:OnEventBegin(value)

	if (GameplayNotificationBus.GetCurrentBusId() == self.playEventId) then
		if (self.Properties.AnimationName ~= nil) then
		    local animInfo = AnimatedLayer(self.Properties.AnimationName, 0, false, 1.0, 0.0);
			SimpleAnimationComponentRequestBus.Event.StartAnimation(self.entityId, animInfo);
		else
			SimpleAnimationComponentRequestBus.Event.StartDefaultAnimations(self.entityId);
		end
	end

end

return simpleanimationtrigger;