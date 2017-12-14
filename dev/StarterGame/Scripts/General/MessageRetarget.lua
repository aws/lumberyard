local messageretarget =
{
	Properties =
	{
		MessageIn = { default = "", description = "Message aimed at this that i am going to trigger on." },
		MessageOut = { default = "", description = "Message to send when i am triggered." },
		MessageTarget = { default = EntityId(), description = "The Object to fire the message at." },
		
		ListenToGlobal = { default = false, description = "If true, the script will listen for the incoming event on the invalid EntityId bus." },
		
		ChangePayload = { default = false, description = "Should i change the data of the message to something else."},
		PayloadDetails = {
			-- because i cannot use a drop down, i will have to make a choice here based upon something like a set of bools
			UseAsBool = { default = false, description = "Use as a bool value. \"true\" or \"yes\" or a number greater than zero, is true, otherwise false "},
			UseAsString = { default = false, description = "Use as a String value."},
			UseAsNumber = { default = false, description = "Use as a number. Converts it to a number as per Lua \"tonumber\""},
			Value = { default = "", description = "Value to be interpreted."},
			UseEntity = { default = false, description = "Use the entity specified in the entity parameter"},
			Entity = { default = EntityId(), description = "The entity to use as payload"},			
		},
		
		Delay = { default = -1, description = "Delay the outgoing message this amount.  -1 is no delay, 0 will result in a 1 frame delay, other values are in seconds" },
	},
}

function messageretarget:OnActivate()
	local listenTo = self.entityId;
	if (self.Properties.ListenToGlobal) then
		listenTo = EntityId();
	end
	self.triggerEventId = GameplayNotificationId(listenTo, self.Properties.MessageIn, "float");
	self.triggerHandler = GameplayNotificationBus.Connect(self, self.triggerEventId);

	self.fireMessageID = GameplayNotificationId(self.Properties.MessageTarget, self.Properties.MessageOut, "float");
	
	if (self.Properties.Delay ~= -1) then
		self.tickBusHandler = TickBus.Connect(self);
		self.WaitTime = -1;
		self.WaitParam = nil;
	end
end

function messageretarget:OnDeactivate()
	self.triggerHandler:Disconnect();
	self.triggerHandler = nil;
	
	if (self.Properties.Delay ~= -1) then
		self.tickBusHandler:Disconnect();
	end
end

function messageretarget:OnTick(deltaTime, timePoint)
	if (self.WaitTime >= 0) then
		self.WaitTime = self.WaitTime - deltaTime;
		
		if (self.WaitTime <= 0) then
			-- fire the message
			--Debug.Log("messageretarget:OnTick sending the message with a delay :" .. tostring(self.entityId) .. "at: "  .. tostring(self.Properties.MessageTarget) .. " message:" .. tostring(self.Properties.MessageOut) .. " param:" .. tostring(self.WaitParam));
			self:SendMessage(self.WaitParam);
			self.WaitTime = -1;
			self.WaitParam = nil;
		end
	end
end

function messageretarget:SendMessage(value)
	if(not self.Properties.ChangePayload)then
		--Debug.Log("messageretarget:SendMessage sending the message with a delay :" .. tostring(self.entityId) .. "at: "  .. tostring(self.Properties.MessageTarget) .. " message:" .. tostring(self.Properties.MessageOut) .. " param:" .. tostring(value));
		GameplayNotificationBus.Event.OnEventBegin(self.fireMessageID, value);
	else
		if(self.Properties.PayloadDetails.UseAsString)then
			--Debug.Log("messageretarget:SendMessage sending the message with a delay :" .. tostring(self.entityId) .. "at: "  .. tostring(self.Properties.MessageTarget) .. " message:" .. tostring(self.Properties.MessageOut) .. " param:" .. tostring(self.Properties.PayloadDetails.Value));
			GameplayNotificationBus.Event.OnEventBegin(self.fireMessageID, self.Properties.PayloadDetails.Value);
		elseif(self.Properties.PayloadDetails.UseAsNumber) then
			--Debug.Log("messageretarget:SendMessage sending the message with a delay :" .. tostring(self.entityId) .. "at: "  .. tostring(self.Properties.MessageTarget) .. " message:" .. tostring(self.Properties.MessageOut) .. " param:" .. tostring(tonumber(self.Properties.PayloadDetails.Value)));
			GameplayNotificationBus.Event.OnEventBegin(self.fireMessageID, tonumber(self.Properties.PayloadDetails.Value));
		elseif(self.Properties.PayloadDetails.UseAsBool) then
			local fromString = (self.Properties.PayloadDetails.Value:lower() == "true") or (self.Properties.PayloadDetails.Value:lower() == "yes");
			local fromNumber = false;
			if(tonumber(self.Properties.PayloadDetails.Value) ~= nil) then
				fromNumber = tonumber(self.Properties.PayloadDetails.Value)	> 0;
			end
			--Debug.Log("messageretarget:SendMessage sending the message with a delay :" .. tostring(self.entityId) .. "at: "  .. tostring(self.Properties.MessageTarget) .. " message:" .. tostring(self.Properties.MessageOut) .. " param:" .. tostring((fromString or fromNumber)));
			GameplayNotificationBus.Event.OnEventBegin(self.fireMessageID, (fromString or fromNumber));
		elseif(self.Properties.PayloadDetails.UseEntity) then
			--Debug.Log("messageretarget:SendMessage sending the message with a delay :" .. tostring(self.entityId) .. "at: "  .. tostring(self.Properties.MessageTarget) .. " message:" .. tostring(self.Properties.MessageOut) .. " param:" .. tostring(self.Properties.PayloadDetails.Entity));
			GameplayNotificationBus.Event.OnEventBegin(self.fireMessageID, self.Properties.PayloadDetails.Entity);
		else
			--Debug.Log("messageretarget:SendMessage sending the message with a delay :" .. tostring(self.entityId) .. "at: "  .. tostring(self.Properties.MessageTarget) .. " message:" .. tostring(self.Properties.MessageOut) .. " param:" .. tostring(nil));
			GameplayNotificationBus.Event.OnEventBegin(self.fireMessageID, nil);
		end
	end
end

function messageretarget:OnEventBegin(value)
	if (GameplayNotificationBus.GetCurrentBusId() == self.triggerEventId) then
		if (self.Properties.Delay >= 0) then
			--Debug.Log("messageretarget:OnEventBegin triggered wait:" .. tostring(self.entityId) .. "at: "  .. tostring(self.Properties.MessageTarget) .. " message:" .. tostring(self.Properties.MessageOut) .. " param:" .. tostring(value));
			self.WaitTime = self.Properties.Delay;
			self.WaitParam = value;
		else
			self:SendMessage(value);
		end
	end
end

return messageretarget;