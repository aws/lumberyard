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
