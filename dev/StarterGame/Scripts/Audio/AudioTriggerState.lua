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

local audiotriggerstate =
{
	Properties =
	{
		TriggerOnEnter = { default = true },
		TriggerOnExit = { default = false },
		
		StateGroup = { default = "", description = "Name of the Wwise State Group" },
		State = { default = "", description = "Name of the Wwise State within the Group" },
	},
	
	
}

function audiotriggerstate:OnActivate()
	-- Listen for anything entering our area.
	self.triggerAreaHandler = TriggerAreaNotificationBus.Connect(self, self.entityId);
end

function audiotriggerstate:OnDeactivate()

	-- Release the handler.
	self.triggerAreaHandler:Disconnect();
	self.triggerAreaHandler = nil;

	-- Reset the array.
	self.triggeredByEntities = nil;
	self.triggeredByEntitiesCount = 1;

end

function audiotriggerstate:OnTriggerAreaEntered(enteringEntityId)
	
	local varStateGroup = self.Properties.StateGroup;
	local varState = self.Properties.State;
	
	if (self.Properties.TriggerOnEnter == true) then
	Debug.Log("SND State Trigger Entered");
	AudioSwitchComponentRequestBus.Event.SetSwitchState(self.entityId, varStateGroup,varState);
	Debug.Log("State Group Set to = : " .. tostring(varStateGroup));
	Debug.Log("State Set to = : " .. tostring(varState));
	end

end

function audiotriggerstate:OnTriggerAreaExited(enteringEntityId)
	
	if (self.Properties.TriggerOnExit == true) then
	--Debug.Log("SND State Trigger Exited");
	end


end

return audiotriggerstate;
