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

local stattrigger =
{
	Properties =
	{
		EventToSend = { default = "BroadcastAlert", description = "The event to send." },
		TriggerValue = { default = 50, description = "'EventToSend' will trigger when below this threshold." },
		TriggerOnce = { default = true, description = "If set to false then it will fire continously (so don't do it)." },
		Stat = { default = "Health", description = "The name of the stat that you want to test." },
	},
}

function stattrigger:OnActivate()
	self.sendingEventId = GameplayNotificationId(self.entityId, self.Properties.EventToSend, "float");
	self.valueChangedEventId = GameplayNotificationId(self.entityId, (self.Properties.Stat .. "Changed"), "float");
	self.valueChangedHandler = GameplayNotificationBus.Connect(self, self.valueChangedEventId);

	self.triggered = false;
end

function stattrigger:OnDeactivate()
	if (self.valueChangedHandler ~= nil) then
		self.valueChangedHandler:Disconnect();
		self.valueChangedHandler = nil;
	end
end

function stattrigger:OnEventBegin(value)
	--Debug.Log("stattrigger:OnEventBegin Recieved message");
	if (GameplayNotificationBus.GetCurrentBusId() == self.valueChangedEventId) then
		if (value <= self.Properties.TriggerValue) then
			GameplayNotificationBus.Event.OnEventBegin(self.sendingEventId, value);
			--Debug.Log("stattrigger:OnEventBegin triggered!");
			if (self.Properties.TriggerOnce) then
				if (self.valueChangedHandler ~= nil) then
					self.valueChangedHandler:Disconnect();
					self.valueChangedHandler = nil;
				end
			end
		end
	end
end

return stattrigger;