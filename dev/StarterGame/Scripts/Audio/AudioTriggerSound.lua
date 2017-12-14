local audiotriggersound =
{
	Properties =
	{
		TriggerOnEnter = { default = true },
		TriggerOnExit = { default = false },
		
		Delay = { default = 0.0, description = "Delay before the sound is played" },
		
		Sound = { default = "", description = "Name of the Wwise Sound Event" },
	},
}

function audiotriggersound:OnActivate()
	
	-- Listen for anything entering our area.
	self.triggerAreaHandler = TriggerAreaNotificationBus.Connect(self, self.entityId);
	
	self.tickBusHandler = nil;
	
end

function audiotriggersound:OnDeactivate()
	
	-- Release the handlers.
	if (self.tickBusHandler ~= nil) then
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end
	
	if (self.triggerAreaHandler ~= nil) then
		self.triggerAreaHandler:Disconnect();
		self.triggerAreaHandler = nil;
	end
	
end

function audiotriggersound:OnTick(deltaTime, timePoint)
	
	self.delayRemaining = self.delayRemaining - deltaTime;
	--Debug.Log("AudioTriggerSound: DelayRemaining: " .. tostring(self.delayRemaining));
	if (self.delayRemaining <= 0.0) then
		self:PlaySound();
		
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end
	
end

function audiotriggersound:OnTriggerAreaEntered(enteringEntityId)
	
	if (self.Properties.TriggerOnEnter == true) then
		--Debug.Log("AudioTriggerSound: SND Sound Trigger Entered");
		if (self.Properties.Delay == 0.0) then
			self:PlaySound();
		else
			if (self.tickBusHandler == nil) then
				self.tickBusHandler = TickBus.Connect(self);
			end
			self.delayRemaining = self.Properties.Delay;
		end
	end

end

function audiotriggersound:OnTriggerAreaExited(enteringEntityId)
	
	if (self.Properties.TriggerOnExit == true) then
		--Debug.Log("SND Sound Trigger Exited");
	end
	
end

function audiotriggersound:PlaySound()
	
	AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.Properties.Sound);
	--Debug.Log("AudioTriggerSound: SND Sound Event Triggered : " .. tostring(self.Properties.Sound));
	
end

return audiotriggersound;