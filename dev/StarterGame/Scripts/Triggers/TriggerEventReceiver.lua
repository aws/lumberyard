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

local triggereventreceiver =
{
	Properties =
	{
		EventName = { default = "", description = "The name of the event to send." },

		TriggerOnce = { default = true },

		OnReceive =
		{
			Suicide = { default = true },
		},
	},
}

function triggereventreceiver:OnActivate()

	-- Listen for the event.
	self.eventId = GameplayNotificationId(self.entityId, self.Properties.EventName, "float");
	self.eventHandler = GameplayNotificationBus.Connect(self, self.eventId);

	self.alreadyTriggered = false;

end

function triggereventreceiver:OnDeactivate()

	-- Release the handler.
	self.eventHandler:Disconnect();
	self.eventHandler = nil;

end

function triggereventreceiver:ReactToEvent(value)

	-- Only allow us to be triggered once by the event.
	if (self.Properties.TriggerOnce) then
		if (self.alreadyTriggered) then
			self.eventHandler:Disconnect();
		else
			self.alreadyTriggered = true;
		end
	end

	-- Delete self.
	if (self.Properties.OnReceive.Suicide) then
		GameEntityContextRequestBus.Broadcast.DestroyGameEntity(self.entityId);
	end

end

function triggereventreceiver:OnEventBegin(value)

	if (GameplayNotificationBus.GetCurrentBusId() == self.eventId) then
		self:ReactToEvent(value);
	end

end

return triggereventreceiver;