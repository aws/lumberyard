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

local triggereventaudioreceiver =
{
	Properties =
	{
		EventName = { default = "", description = "The name of the event to send." },
		
		Delay = { default = 0.0, description = "Delay before the sound is played." },
		
		Sound = { default = "", description = "The sound event to trigger." },

		TriggerOnce = { default = true },
	},
}

function triggereventaudioreceiver:OnActivate()
	
	-- Listen for the event.
	self.eventId = GameplayNotificationId(self.entityId, self.Properties.EventName, "float");
	self.eventHandler = GameplayNotificationBus.Connect(self, self.eventId);
	
	self.tickBusHandler = nil;
	
end

function triggereventaudioreceiver:OnDeactivate()
	
	-- Release the handlers.
	if (self.tickBusHandler ~= nil) then
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end
	
	if (self.eventHandler ~= nil) then
		self.eventHandler:Disconnect();
		self.eventHandler = nil;
	end
	
end

function triggereventaudioreceiver:OnTick(deltaTime, timePoint)
	
	self.delayRemaining = self.delayRemaining - deltaTime;
	--Debug.Log("TriggerEventAudioReceiver: DelayRemaining: " .. tostring(self.delayRemaining));
	if (self.delayRemaining <= 0.0) then
		self:PlaySound();
		
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end
	
end

function triggereventaudioreceiver:OnEventBegin(value)
	
	if (GameplayNotificationBus.GetCurrentBusId() == self.eventId) then
		--Debug.Log("TriggerEventAudioReceiver: SND Sound Event Received");
		if (self.Properties.Delay == 0.0) then
			self:PlaySound();
		else
			if (self.tickBusHandler == nil) then
				self.tickBusHandler = TickBus.Connect(self);
			end
			self.delayRemaining = self.Properties.Delay;
		end
		
		-- Only allow us to be triggered once by the event.
		if (self.Properties.TriggerOnce) then
			self.eventHandler:Disconnect();
		end
	end
	
end

function triggereventaudioreceiver:PlaySound()
	
	AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.Properties.Sound);
	--Debug.Log("TriggerEventAudioReceiver: SND Sound Event Triggered : " .. tostring(self.Properties.Sound));
	
end

return triggereventaudioreceiver;