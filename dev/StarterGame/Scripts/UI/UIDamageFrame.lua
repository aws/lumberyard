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

local UIDamageFrame =
{
	Properties =
	{
		NextFrame = { default = EntityId(), description = "The next frame to be shown." },
		Fade = { default = false, description = "If ticked, will fade over the duration." },
	},
}

function UIDamageFrame:OnActivate()
	--Debug.Log("UIDamageFrame:OnActivate()");
	
	self.tickBusHandler = nil;
	self.durationFull = 0.0;
	self.duration = 0.0;
	self.atLeastOneFrame = false;
	
	self.playFrameEventId = GameplayNotificationId(self.entityId, "PlayFrame", "float");
	self.playFrameHandler = GameplayNotificationBus.Connect(self, self.playFrameEventId);
	
	-- Start invisible.
	UiFaderBus.Event.Fade(self.entityId, 0, 0);
end

function UIDamageFrame:OnDeactivate()
	--Debug.Log("UIDamageFrame:OnDeactivate()");
	
	if (self.tickBusHandler ~= nil) then
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end
	if (self.playFrameHandler ~= nil) then
		self.playFrameHandler:Disconnect();
		self.playFrameHandler = nil;
	end
end

function UIDamageFrame:OnTick(deltaTime, timePoint)
	self.duration = self.duration - deltaTime;
	if (self.atLeastOneFrame) then
		-- If fading, set the fade speed (because the fader uses instead of time).
		if (self.Properties.Fade) then
			local opacity = self.duration / self.durationFull;
			--Debug.Log("Opacity: " .. tostring(opacity));
			UiFaderBus.Event.Fade(self.entityId, opacity, 0);
		end
		
		if (self.duration <= 0.0) then
			UiFaderBus.Event.Fade(self.entityId, 0, 0);
			
			if (self.Properties.NextFrame ~= nil and self.Properties.NextFrame:IsValid()) then
				local eventId = GameplayNotificationId(self.Properties.NextFrame, "PlayFrame", "float");
				GameplayNotificationBus.Event.OnEventBegin(eventId, self.durationFull);
			end
			
			self.tickBusHandler:Disconnect();
			self.tickBusHandler = nil;
		end
	else
		self.atLeastOneFrame = true;
	end
end

function UIDamageFrame:OnEventBegin(value)
	--Debug.Log("UIDamageFrame: Recieved message " .. tostring(StarterGameEntityUtility.GetEntityName(self.entityId)));
	local busId = GameplayNotificationBus.GetCurrentBusId();
	if (busId == self.playFrameEventId) then
		UiFaderBus.Event.Fade(self.entityId, 1, 0);
		
		if (self.tickBusHandler == nil) then
			self.tickBusHandler = TickBus.Connect(self);
		end
		self.durationFull = value;
		self.duration = value;
		self.atLeastOneFrame = false;
	end
end

return UIDamageFrame;
