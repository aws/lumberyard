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

local doublesidedtriggerreceiver =
{
	Properties =
	{
		EventsToReceive =
		{
			OnEnterLeft = { default = "DoubleSidedTriggerEnterLeftEvent" },
			OnEnterRight = { default = "DoubleSidedTriggerEnterRightEvent" },

			OnExitLeftToLeft = { default = "DoubleSidedTriggerExitLeftToLeftEvent" },
			OnExitLeftToRight = { default = "DoubleSidedTriggerExitLeftToRightEvent" },
			OnExitRightToLeft = { default = "DoubleSidedTriggerExitRightToLeftEvent" },
			OnExitRightToRight = { default = "DoubleSidedTriggerExitRightToRightEvent" },
		},
	},
}

function doublesidedtriggerreceiver:OnActivate()

	-- Listen for the event.
	self.onEnterLeftEventId = GameplayNotificationId(self.entityId, self.Properties.EventsToReceive.OnEnterLeft, "float");
	self.onEnterLeftEventHandler = GameplayNotificationBus.Connect(self, self.onEnterLeftEventId);
	self.onEnterRightEventId = GameplayNotificationId(self.entityId, self.Properties.EventsToReceive.OnEnterRight, "float");
	self.onEnterRightEventHandler = GameplayNotificationBus.Connect(self, self.onEnterRightEventId);
	-- LeftToX
	self.onExitLeftToLeftEventId = GameplayNotificationId(self.entityId, self.Properties.EventsToReceive.OnExitLeftToLeft, "float");
	self.onExitLeftToLeftEventHandler = GameplayNotificationBus.Connect(self, self.onExitLeftToLeftEventId);
	self.onExitLeftToRightEventId = GameplayNotificationId(self.entityId, self.Properties.EventsToReceive.OnExitLeftToRight, "float");
	self.onExitLeftToRightEventHandler = GameplayNotificationBus.Connect(self, self.onExitLeftToRightEventId);
	-- RightToX
	self.onExitRightToLeftEventId = GameplayNotificationId(self.entityId, self.Properties.EventsToReceive.OnExitRightToLeft, "float");
	self.onExitRightToLeftEventHandler = GameplayNotificationBus.Connect(self, self.onExitRightToLeftEventId);
	self.onExitRightToRightEventId = GameplayNotificationId(self.entityId, self.Properties.EventsToReceive.OnExitRightToRight, "float");
	self.onExitRightToRightEventHandler = GameplayNotificationBus.Connect(self, self.onExitRightToRightEventId);

end

function doublesidedtriggerreceiver:OnDeactivate()

	-- Release the handlers.
	self.onEnterLeftEventHandler:Disconnect();
	self.onEnterLeftEventHandler = nil;
	self.onEnterRightEventHandler:Disconnect();
	self.onEnterRightEventHandler = nil;
	self.onExitLeftToLeftEventHandler:Disconnect();
	self.onExitLeftToLeftEventHandler = nil;
	self.onExitLeftToRightEventHandler:Disconnect();
	self.onExitLeftToRightEventHandler = nil;
	self.onExitRightToLeftEventHandler:Disconnect();
	self.onExitRightToLeftEventHandler = nil;
	self.onExitRightToRightEventHandler:Disconnect();
	self.onExitRightToRightEventHandler = nil;

end

function doublesidedtriggerreceiver:ReactToEnterLeft(enteringEntityId)
	Debug.Log("ReactToEnterLeft because of entity " .. tostring(enteringEntityId));
end

function doublesidedtriggerreceiver:ReactToEnterRight(enteringEntityId)
	Debug.Log("ReactToEnterRight because of entity " .. tostring(enteringEntityId));
end

function doublesidedtriggerreceiver:ReactToExitLeftToLeft(enteringEntityId)
	Debug.Log("ReactToExitLeftToLeft because of entity " .. tostring(enteringEntityId));
end

function doublesidedtriggerreceiver:ReactToExitLeftToRight(enteringEntityId)
	Debug.Log("ReactToExitLeftToRight because of entity " .. tostring(enteringEntityId));
end

function doublesidedtriggerreceiver:ReactToExitRightToLeft(enteringEntityId)
	Debug.Log("ReactToExitRightToLeft because of entity " .. tostring(enteringEntityId));
end

function doublesidedtriggerreceiver:ReactToExitRightToRight(enteringEntityId)
	Debug.Log("ReactToExitRightToRight because of entity " .. tostring(enteringEntityId));
end

function doublesidedtriggerreceiver:OnEventBegin(value)

	-- onEnter
	if (GameplayNotificationBus.GetCurrentBusId() == self.onEnterLeftEventId) then
		self:ReactToEnterLeft(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.onEnterRightEventId) then
		self:ReactToEnterRight(value);
	-- onExitLeft
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.onExitLeftToLeftEventId) then
		self:ReactToExitLeftToLeft(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.onExitLeftToRightEventId) then
		self:ReactToExitLeftToRight(value);
	-- onExitRight
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.onExitRightToLeftEventId) then
		self:ReactToExitRightToLeft(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.onExitRightToRightEventId) then
		self:ReactToExitRightToRight(value);
	end

end

return doublesidedtriggerreceiver;