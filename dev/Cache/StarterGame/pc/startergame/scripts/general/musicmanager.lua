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


local musicmanager =
{
	Properties =
	{
		Combat =
		{
			DataKey = { default = "AI_InCombatCount", description = "The name of the persistent data to check." },
			Entered = { default = "", description = "The audio event to send when the player enters combat." },
			Exited = { default = "", description = "The audio event to send when the player exits combat." },
		},
	},
}

function musicmanager:OnActivate()
	self.aiInCombatChangeEventId = GameplayNotificationId(self.entityId, "AiInCombatChange", "float");
	self.aiInCombatChangeHandler = GameplayNotificationBus.Connect(self, self.aiInCombatChangeEventId);
	PersistentDataSystemRequestBus.Broadcast.RegisterDataChangeCallback(self.Properties.Combat.DataKey, self.aiInCombatChangeEventId.channel, self.aiInCombatChangeEventId.actionNameCrc);
	self.aiInCombat = 0;
end

function musicmanager:OnDeactivate()
	PersistentDataSystemRequestBus.Broadcast.UnRegisterDataChangeCallback(self.Properties.Combat.DataKey, self.aiInCombatChangeEventId.channel, self.aiInCombatChangeEventId.actionNameCrc);
	if (self.aiInCombatChangeHandler ~= nil) then
		self.aiInCombatChangeHandler:Disconnect();
		self.aiInCombatChangeHandler = nil;
	end
end

function musicmanager:OnEventBegin(value)
	if (GameplayNotificationBus.GetCurrentBusId() == self.aiInCombatChangeEventId) then
		--Debug.Log("AI Combat count: " .. tostring(value));
		if (value > 0 and self.aiInCombat <= 0) then
			-- Entered combat.
			AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.Properties.Combat.Entered);
			--Debug.Log("Combat Entered");
		elseif (value <= 0 and self.aiInCombat > 0) then
			-- Exited combat.
			AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.Properties.Combat.Exited);
			--Debug.Log("Combat Exited");
		end
		self.aiInCombat = value;
	end
end

return musicmanager;