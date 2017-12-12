local doublesidedtrigger =
{
	Properties =
	{
		Side1 =
		{
			EventName = { default = "EnterLeftEvent", description = "The name of the event to listen for." },
			EventNameExit = { default = "ExitLeftEvent", description = "The name of the event to listen for upon exit (uses 'EventName' if not set)." },
		},

		Side2 =
		{
			EventName = { default = "EnterRightEvent", description = "The name of the event to listen for." },
			EventNameExit = { default = "ExitRightEvent", description = "The name of the event to listen for upon exit (uses 'EventName' if not set)." },
		},

		EventsToSend =
		{
			OnEnterLeft = { default = "DoubleSidedTriggerEnterLeftEvent" },
			OnEnterRight = { default = "DoubleSidedTriggerEnterRightEvent" },

			OnExitLeftToLeft = { default = "DoubleSidedTriggerExitLeftToLeftEvent" },
			OnExitLeftToRight = { default = "DoubleSidedTriggerExitLeftToRightEvent" },
			OnExitRightToLeft = { default = "DoubleSidedTriggerExitRightToLeftEvent" },
			OnExitRightToRight = { default = "DoubleSidedTriggerExitRightToRightEvent" },
		},

		ReceiverEntity = { default = EntityId(), description = "The entity to receive the events that this script will send." },

		TriggerOncePerEntity = { default = true },
	},
}

function doublesidedtrigger:OnActivate()

	-- Listen to the events from our internal trigger volumes.
	self.onEnterLeftEventId = GameplayNotificationId(self.entityId, self.Properties.Side1.EventName, "float");
	self.onEnterLeftEventHandler = GameplayNotificationBus.Connect(self, self.onEnterLeftEventId);
	self.onExitLeftEventId = GameplayNotificationId(self.entityId, self.Properties.Side1.EventNameExit, "float");
	self.onExitLeftEventHandler = GameplayNotificationBus.Connect(self, self.onExitLeftEventId);
	self.onEnterRightEventId = GameplayNotificationId(self.entityId, self.Properties.Side2.EventName, "float");
	self.onEnterRightEventHandler = GameplayNotificationBus.Connect(self, self.onEnterRightEventId);
	self.onExitRightEventId = GameplayNotificationId(self.entityId, self.Properties.Side2.EventNameExit, "float");
	self.onExitRightEventHandler = GameplayNotificationBus.Connect(self, self.onExitRightEventId);

	-- Listen for anything entering our area.
	self.triggerAreaHandler = TriggerAreaNotificationBus.Connect(self, self.entityId);

	-- This is the array we'll use to record which entities have triggered this area.
	self.triggeredByEntities = {};
	self.triggeredByEntitiesCount = 1;

	self.playerId = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"));
	
	if (not self.Properties.ReceiverEntity:IsValid()) then
		self.Properties.ReceiverEntity = self.entityId;
	end

end

function doublesidedtrigger:OnDeactivate()

	-- Release the event handlers.
	self.onEnterLeftEventHandler:Disconnect();
	self.onEnterLeftEventHandler = nil;
	self.onExitLeftEventHandler:Disconnect();
	self.onExitLeftEventHandler = nil;
	self.onEnterRightEventHandler:Disconnect();
	self.onEnterRightEventHandler = nil;
	self.onExitRightEventHandler:Disconnect();
	self.onExitRightEventHandler = nil;

	-- Release the handler.
	self.triggerAreaHandler:Disconnect();
	self.triggerAreaHandler = nil;

	-- Reset the array.
	self.triggeredByEntities = nil;
	self.triggeredByEntitiesCount = 1;

end

function doublesidedtrigger:OnTriggerAreaEntered(enteringEntityId)
	--Debug.Log("OnTriggerAreaEntered (Main) " .. tostring(self.entityId) .. " - " .. tostring(enteringEntityId));

	self:RecordEntityTriggeringMain(enteringEntityId, true);

end

function doublesidedtrigger:OnTriggerAreaExited(enteringEntityId)
	--Debug.Log("OnTriggerAreaExited (Main) " .. tostring(self.entityId) .. " - " .. tostring(enteringEntityId));

	self:RecordEntityTriggeringMain(enteringEntityId, false);

end

function doublesidedtrigger:OnEventBegin(value)

	if (GameplayNotificationBus.GetCurrentBusId() == self.onEnterLeftEventId) then
		self:RecordEntityTriggeringLeft(value, true);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.onExitLeftEventId) then
		self:RecordEntityTriggeringLeft(value, false);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.onEnterRightEventId) then
		self:RecordEntityTriggeringRight(value, true);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.onExitRightEventId) then
		self:RecordEntityTriggeringRight(value, false);
	end

end

function doublesidedtrigger:RecordEntityTriggeringMain(entityId, entering)

	local index = self:FindEntityInList(entityId);
	if (index == 0) then
		index = self:AddEntityToList(entityId);
	end

	self.triggeredByEntities[index].enteredMain = entering;
	self.triggeredByEntities[index].exitedMain = not entering;

	self:EvaluateState(index);

end

function doublesidedtrigger:RecordEntityTriggeringLeft(entityId, entering)

	local index = self:FindEntityInList(entityId);
	if (index == 0) then
		index = self:AddEntityToList(entityId);
	end

	-- We don't care if they entered the left side when already inside the trigger box.
	if (entering == true and self.triggeredByEntities[index].entered) then
		--Debug.Log("Ignoring left - " .. tostring(entering));
		self.triggeredByEntities[index].exitedLeft = false;
		self.triggeredByEntities[index].exitedRight = false;
		return;
	end

	-- If we originally entered through the left then keep that information when we
	-- exit so that if, upon exit, both of these are true then we went out the same
	-- way we came in.
	self.triggeredByEntities[index].enteredLeft = self.triggeredByEntities[index].enteredLeft or entering;
	self.triggeredByEntities[index].exitedLeft = not entering;

	-- If we're exiting this side then we CAN'T be exiting the other side.
	if (not entering) then
		self.triggeredByEntities[index].exitedRight = false;
	end

	self:EvaluateState(index);

end

function doublesidedtrigger:RecordEntityTriggeringRight(entityId, entering)

	local index = self:FindEntityInList(entityId);
	if (index == 0) then
		index = self:AddEntityToList(entityId);
	end

	-- We don't care if they entered the right side when already inside the trigger box.
	if (entering == true and self.triggeredByEntities[index].entered) then
		--Debug.Log("Ignoring right - " .. tostring(entering));
		self.triggeredByEntities[index].exitedLeft = false;
		self.triggeredByEntities[index].exitedRight = false;
		return;
	end

	-- If we originally entered through the right then keep that information when we
	-- exit so that if, upon exit, both of these are true then we went out the same
	-- way we came in.
	self.triggeredByEntities[index].enteredRight = self.triggeredByEntities[index].enteredRight or entering;
	self.triggeredByEntities[index].exitedRight = not entering;

	-- If we're exiting this side then we CAN'T be exiting the other side.
	if (not entering) then
		self.triggeredByEntities[index].exitedLeft = false;
	end

	self:EvaluateState(index);

end

function doublesidedtrigger:EvaluateState(index)

	local entityId = self.triggeredByEntities[index].entityId;

	-- Report the status of this entity.
	--self.triggeredByEntities[index]:Report();

	-- Check if we're exiting.
	if (self.triggeredByEntities[index].entered) then
		self.sentAnEvent = false;

		if (self.triggeredByEntities[index].exitedMain) then
			if (self.triggeredByEntities[index].exitedLeft) then
				if (self.triggeredByEntities[index].enteredLeft) then
					self:SendEventToThisEntity(entityId, self.Properties.EventsToSend.OnExitLeftToLeft);
				elseif (self.triggeredByEntities[index].enteredRight) then
					self:SendEventToThisEntity(entityId, self.Properties.EventsToSend.OnExitRightToLeft);
				end
			elseif (self.triggeredByEntities[index].exitedRight) then
				if (self.triggeredByEntities[index].enteredLeft) then
					self:SendEventToThisEntity(entityId, self.Properties.EventsToSend.OnExitLeftToRight);
				elseif (self.triggeredByEntities[index].enteredRight) then
					self:SendEventToThisEntity(entityId, self.Properties.EventsToSend.OnExitRightToRight);
				end
			end
		end

		if (self.sentAnEvent) then
			self:ResetEntityInList(index);
		end
	-- Check if we're entering.
	elseif (self.triggeredByEntities[index].enteredMain) then
		self.sentAnEvent = false;

		if (self.triggeredByEntities[index].enteredLeft) then
			self:SendEventToThisEntity(entityId, self.Properties.EventsToSend.OnEnterLeft);
		elseif (self.triggeredByEntities[index].enteredRight) then
			self:SendEventToThisEntity(entityId, self.Properties.EventsToSend.OnEnterRight);
		end

		if (self.sentAnEvent) then
			self.triggeredByEntities[index].entered = true;
		end
	end

end

function doublesidedtrigger:SendEventToThisEntity(enteringEntityId, eventName)

	if (eventName == nil or eventName == "") then
		Debug.Warning("We don't have a valid event name to send regarding " .. enteringEntityId .. ".");
		return;
	end

	local receiverId = self.entityId;
	if (self.Properties.ReceiverEntity:IsValid()) then
		receiverId = self.Properties.ReceiverEntity;
	end

	Debug.Log("Sending " .. eventName .. " to " .. tostring(receiverId));
	local notificationId = GameplayNotificationId(receiverId, eventName, "float");
	GameplayNotificationBus.OnEventBegin(notificationId, enteringEntityId);

	self.sentAnEvent = true;

end

function doublesidedtrigger:AddEntityToList(entityId)

	local index = self.triggeredByEntitiesCount;
	self.triggeredByEntitiesCount = self.triggeredByEntitiesCount + 1;

	self.triggeredByEntities[index] = EntityInfo:new(nil, "info");
	self.triggeredByEntities[index].entityId = entityId;

	return index;

end

function doublesidedtrigger:ResetEntityInList(i)

	self.triggeredByEntities[i].entered = false;
	self.triggeredByEntities[i].enteredMain = false;
	self.triggeredByEntities[i].enteredLeft = false;
	self.triggeredByEntities[i].enteredRight = false;
	self.triggeredByEntities[i].exitedMain = false;
	self.triggeredByEntities[i].exitedLeft = false;
	self.triggeredByEntities[i].exitedRight = false;

end

function doublesidedtrigger:FindEntityInList(entity)

	for i=1, self.triggeredByEntitiesCount do
		if (self.triggeredByEntities[i] ~= nil) then
			if (self.triggeredByEntities[i].entityId == entity) then
				return i;
			end
		end
	end

	return 0;

end


EntityInfo =
{
	entityId = 0,
	entered = false,
	enteredMain = false,
	enteredLeft = false,
	enteredRight = false,
	exitedMain = false,
	exitedLeft = false,
	exitedRight = false,
}

function EntityInfo:new(o, name)
	o = o or {name = name};
	setmetatable(o, self);
	self.__index = self;
	return o;
end

function EntityInfo:Report()
	Debug.Log("EntityInfo: " .. tostring(self.entityId));
	Debug.Log("    EnMain: " .. tostring(self.enteredMain));
	Debug.Log("    EnLeft: " .. tostring(self.enteredLeft));
	Debug.Log("    EnRight: " .. tostring(self.enteredRight));
	Debug.Log("    ExMain: " .. tostring(self.exitedMain));
	Debug.Log("    ExLeft: " .. tostring(self.exitedLeft));
	Debug.Log("    ExRight: " .. tostring(self.exitedRight));
end

return doublesidedtrigger;