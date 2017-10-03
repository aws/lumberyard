local audiotriggersound =
{
	Properties =
	{
		TriggerOnEnter = { default = true },
		TriggerOnExit = { default = false },
		
		Sound = { default = "", description = "Name of the Wwise Sound Event" },
	},
	
	
}

function audiotriggersound:OnActivate()

	-- Listen for anything entering our area.
	self.triggerAreaHandler = TriggerAreaNotificationBus.Connect(self, self.entityId);
end

function audiotriggersound:OnDeactivate()

	-- Release the handler.
	self.triggerAreaHandler:Disconnect();
	self.triggerAreaHandler = nil;

	-- Reset the array.
	self.triggeredByEntities = nil;
	self.triggeredByEntitiesCount = 1;

end

function audiotriggersound:OnTriggerAreaEntered(enteringEntityId)
	
	local varSound = self.Properties.Sound;
	
	if (self.Properties.TriggerOnEnter == true) then
	Debug.Log("SND Sound Trigger Entered");
	AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, varSound);
	Debug.Log("SND Sound Event Triggered : " .. tostring(varSound));
	end

end

function audiotriggersound:OnTriggerAreaExited(enteringEntityId)
	
	if (self.Properties.TriggerOnExit == true) then
	--Debug.Log("SND Sound Trigger Exited");
	end


end

return audiotriggersound;