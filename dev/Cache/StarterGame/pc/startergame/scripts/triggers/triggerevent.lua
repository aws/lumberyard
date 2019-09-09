
local triggerevent =
{
	Properties =
	{
		EventName = { default = "", description = "The name of the event to send." },
		EventNameExit = { default = "", description = "The name of the event to send upon exit (uses 'EventName' if not set)." },

		TriggerOnEnter = { default = true },
		TriggerOnExit = { default = false },

		Recipients =
		{
			Entity = { default = EntityId(), description = "Entities to send the event to." },
			Entities = { default = -1, description = "Index of entities from an 'Object Lister' that should receive the event." },
	
			TagFilters =
			{
				-- TODO: Should be changed to be a vector.
				SendToTags = { default = "", description = "Send the event to all entites with these tags." },
				-- TODO: Should be changed to be a vector.
				ExcludedTags = { default = "", description = "Don't send the event to any entities with these tags (superseeds 'SendToTags')." },
			},
		},
		
		Frequency =
		{
			TriggerOncePerEntity = { default = true },
		},
	},
}

function triggerevent:OnActivate()

	-- Listen for anything entering our area.
	self.triggerAreaHandler = TriggerAreaNotificationBus.Connect(self, self.entityId);

	-- This is the array we'll use to record which entities have triggered this area.
	self.triggeredByEntities = {};
	self.triggeredByEntitiesCount = 1;
	
	if (not self.Properties.Recipients.Entity:IsValid()) then
		self.Properties.Recipients.Entity = self.entityId;
	end

	-- This is the event name that'll be used (so it can be determined inside the callback
	-- functions and keep the rest of the code cleaner).
	self.EventNameToUse = "";

end

function triggerevent:OnDeactivate()

	-- Release the handler.
	self.triggerAreaHandler:Disconnect();
	self.triggerAreaHandler = nil;

	-- Reset the array.
	self.triggeredByEntities = nil;
	self.triggeredByEntitiesCount = 1;

end

function triggerevent:OnTriggerAreaEntered(enteringEntityId)
	--Debug.Log("OnTriggerAreaEntered " .. tostring(self.entityId));

	-- If we're meant to send the event on enter and this entity hasn't triggered us already, send the event.
	if (self.Properties.TriggerOnEnter and not self:EntityHasAlreadyTriggeredMe(enteringEntityId)) then
		self.EventNameToUse = self.Properties.EventName;
		self:SendTheEvent(enteringEntityId);
	end

end

function triggerevent:OnTriggerAreaExited(enteringEntityId)
	--Debug.Log("OnTriggerAreaExited " .. tostring(self.entityId));

	-- If we're meant to send the event on exit and this entity hasn't triggered us already, send the event.
	if (self.Properties.TriggerOnExit and not self:EntityHasAlreadyTriggeredMe(enteringEntityId)) then
		-- Use the normal event name if an exit name hasn't been specified.
		if (self.Properties.EventNameExit ~= "") then
			self.EventNameToUse = self.Properties.EventNameExit;
		else
			self.EventNameToUse = self.Properties.EventName;
		end

		self:SendTheEvent(enteringEntityId);
	end

end

function triggerevent:SendTheEvent(enteringEntityId)

	-- Make sure we don't broadcast because of this entity again.
	self:AddEntityToList(enteringEntityId);

	if (self.EventNameToUse == nil or self.EventNameToUse == "") then
		Debug.Warning(self.entityId .. " was triggered but doesn't have a valid event name.");
		return;
	end

	-- Send the event to the entities.
	if (self.Properties.Recipients.Entity:IsValid()) then
		local entityId = self.Properties.Recipients.Entity;

		if (self:EntityDoesNotContainAnExcludedTag(entityId)) then
			self:SendTheEventToThisEntity(enteringEntityId, entityId);
		end
	end
	
	-- TODO: This will need to be made into a loop once we make the tags Properties vectors.
	-- Send the event to the tagged entites.
	if (self.Properties.Recipients.TagFilters.SendToTags ~= "") then
		local tag = self.Properties.Recipients.TagFilters.SendToTags;
		-- TODO: 'RequestTaggedEntities()' currently only gets the last entity; rework this code
		-- once it gets all entities.
		local taggedEntities = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32(tag));

		if (self:EntityDoesNotContainAnExcludedTag(taggedEntities)) then
			self:SendTheEventToThisEntity(enteringEntityId, taggedEntities);
		end
	end
end

function triggerevent:SendTheEventToThisEntity(enteringEntityId, entityId)

	local notificationId = GameplayNotificationId(entityId, self.EventNameToUse, "float");
	GameplayNotificationBus.Event.OnEventBegin(notificationId, enteringEntityId);

end

function triggerevent:EntityDoesNotContainAnExcludedTag(entityId)

	-- I could move this check into 'OnActivate' but I'm going to do it here just in case
	-- we decide that we want the tag settings to be dynamic (i.e. add/remove them at runtime).
	if (self.Properties.Recipients.TagFilters.ExcludedTags == nil or self.Properties.Recipients.TagFilters.ExcludedTags == "") then
		return true;
	end

	-- Unfortunately I can't get a TagComponent from an entity and read off its tags so I'll have
	-- to get all the entities that contain the excluded tags and check if any of the IDs match
	-- the parameter.
	if (self.Properties.Recipients.TagFilters.ExcludedTags ~= "") then
		local tag = self.Properties.Recipients.TagFilters.ExcludedTags;
		local taggedEntities = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32(tag));
		-- TODO: 'RequestTaggedEntities()' currently only gets the last entity; rework this code
		-- once it gets all entities.

		if (tostring(entityId) == tostring(taggedEntities)) then
			return false;
		end
	end

	return true;

end

function triggerevent:EntityHasAlreadyTriggeredMe(enteringEntityId)

	if (not self.Properties.Frequency.TriggerOncePerEntity) then
		return false;
	end

	for i=1, self.triggeredByEntitiesCount do
		if (self.triggeredByEntities[i] == enteringEntityId) then
			return true;
		end
	end

	return false;

end

function triggerevent:AddEntityToList(enteringEntityId)

	-- Don't bother recording it if we don't care.
	if (not self.Properties.Frequency.TriggerOncePerEntity) then
		return;
	end

	self.triggeredByEntitiesCount = self.triggeredByEntitiesCount + 1;
	self.triggeredByEntities[self.triggeredByEntitiesCount] = enteringEntityId;

end

return triggerevent;