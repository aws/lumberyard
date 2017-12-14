local StateMachine = require "scripts/common/StateMachine"

local uiplayerhud =
{
	Properties =
	{
		--CodeEntity = { default = EntityId() }, -- to be removed
		
		Messages = { 
			HealthChangedEventIn = { default = "HealthChanged", description = "The event used to set the health" },
			EnergyChangedEventIn = { default = "EnergyChanged", description = "The event used to set the Energy" },
			HealthMaxChangedEventIn = { default = "HealthSetMaxEvent", description = "The event used to set the health max" },
			EnergyMaxChangedEventIn = { default = "EnergySetMaxEvent", description = "The event used to set the Energy max" },
			SetBlueKeyVisible   = { default = "SetBlueKeyVisibleEvent", 	description = "Set the Blue Key icon is Visible" },
			SetBlueKeyCollected = { default = "SetBlueKeyCollectedEvent", 	description = "Set the Blue Key icon is Collected" },
			SetRedKeyVisible   = { default = "SetRedKeyVisibleEvent", 	description = "Set the Red Key icon is Visible" },
			SetRedKeyCollected = { default = "SetRedKeyCollectedEvent", description = "Set the Red Key icon is Collected" },

			AddTracker = { default = "AddTrackerEvent", description = "Adds a tracker tracking the chosen element" },
			RemoveTracker = { default = "RemoveTrackerEvent", description = "Removes a tracker tracking the chosen element" },
			ShowTracker = { default = "ShowTrackerEvent", description = "Makes the tracker for the chosen element visible." },
			HideTracker = { default = "HideTrackerEvent", description = "Makes the tracker for the chosen element hidden." },
			HighlightTracker = { default = "HighlightTrackerEvent", description = "The event used to make the marker flash" },
			UnHighlightTracker = { default = "UnHighlightTrackerEvent", description = "The event used to make the marker flash" },
			
			DestroyedTracker = { default = "MarkerDestroyedEvent", description = "The event sent when i have been asked to be destroyed and i am ready to be deleted" },
		},
		
		MapEntityTag = { default = "PlayerCamera", description = "The Tag of the entity for the map to be hooked off" },
		
		ScreenHookup = {
			StatusBars = {
				HealthBarID = { default = EntityId(), description = "The health Bar slider" },
				EnergyBarID = { default = EntityId(), description = "The energy Bar slider" },
				
				HealthMaxBarID = { default = EntityId(), description = "The health max Bar slider" },
				EnergyMaxBarID = { default = EntityId(), description = "The energy max Bar slider" },
				
				HealthFlasherID = { default = EntityId(), description = "The Health Bar full fader" },
				HealthLowFlasherID = { default = EntityId(), description = "The Health Bar low fader" },
				EnergyFlasherID = { default = EntityId(), description = "The Energy Bar full fader" },
				EnergyLowFlasherID = { default = EntityId(), description = "The Energy Bar low fader" },
			},
			MapBar = {
				SliderBackgroundAID = { default = EntityId(), description = "ID the fader for the map Background A" },
				SliderBackgroundBID = { default = EntityId(), description = "ID the fader for the map Background B" },
				TrackerBaseID = { default = EntityId(), description = "ID the a tracker setup to clone for items being tracked on the map." },
				TrackerAngle = { default = 157.5, description = "The Visible angle on the tracker, degrees" },
			},
			Objectives = {
				BlueKeyVisibleID = { default = EntityId(), description = "The Blue key active" },
				BlueKeyCollectedID = { default = EntityId(), description = "The Blue key highlight" },
				RedKeyVisibleID = { default = EntityId(), description = "The Red key active" },
				RedKeyCollectedID = { default = EntityId(), description = "The Red key highlight" },
			},
		},
	},
}

function uiplayerhud:OnActivate()

	self.firstUpdate = true;
	
	--Debug.Log("uiplayerhud:OnActivate()");
		
	self.setBlueKeyVisibleEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.SetBlueKeyVisible, "float");
	self.setBlueKeyVisibleHandler = GameplayNotificationBus.Connect(self, self.setBlueKeyVisibleEventId);
	self.setBlueKeyCollectedEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.SetBlueKeyCollected, "float");
	self.setBlueKeyCollectedHandler = GameplayNotificationBus.Connect(self, self.setBlueKeyCollectedEventId);
	self.setRedKeyVisibleEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.SetRedKeyVisible, "float");
	self.setRedKeyVisibleHandler = GameplayNotificationBus.Connect(self, self.setRedKeyVisibleEventId);
	self.setRedKeyCollectedEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.SetRedKeyCollected, "float");
	self.setRedKeyCollectedHandler = GameplayNotificationBus.Connect(self, self.setRedKeyCollectedEventId);
	
	self.AddTrackerEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.AddTracker, "float");
	self.AddTrackerHandler = GameplayNotificationBus.Connect(self, self.AddTrackerEventId);
	self.RemoveTrackerEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.RemoveTracker, "float");
	self.RemoveTrackerHandler = GameplayNotificationBus.Connect(self, self.RemoveTrackerEventId);
	self.ShowTrackerEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.ShowTracker, "float");
	self.ShowTrackerHandler = GameplayNotificationBus.Connect(self, self.ShowTrackerEventId);
	self.HideTrackerEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.HideTracker, "float");
	self.HideTrackerHandler = GameplayNotificationBus.Connect(self, self.HideTrackerEventId);
	self.HighlightTrackerEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.HighlightTracker, "float");
	self.HighlightTrackerHandler = GameplayNotificationBus.Connect(self, self.HighlightTrackerEventId);
	self.UnHighlightTrackerEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.UnHighlightTracker, "float");
	self.UnHighlightTrackerHandler = GameplayNotificationBus.Connect(self, self.UnHighlightTrackerEventId);
	self.DestroyedTrackerEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.DestroyedTracker, "float");
	self.DestroyedTrackerHandler = GameplayNotificationBus.Connect(self, self.DestroyedTrackerEventId);
	
	self.EnergyLastValue = 100;
	self.EnergyFadeTime = 0;
	self.EnergyLowFadeTime = 0;
	self.HealthLastValue = 100;
	self.HealthFadeTime = 0;
	self.HealthLowFadeTime = 0;
	self.ObjectiveFaderTime1 = 0;
	self.ObjectiveFaderTime2 = 0;
	self.ObjectiveFaderTime3 = 0;
	
	self.TrackedItems = {};
	
	--Debug.Log("UIPlayerHUD ID: " .. tostring(self.entityId));
	
	self.tickHandler = TickBus.Connect(self);
end

function uiplayerhud:OnDeactivate()
	
	self.healthChangedHandler:Disconnect();
	self.healthChangedHandler = nil;
	self.energyChangedHandler:Disconnect();
	self.energyChangedHandler = nil;
	
	self.healthMaxChangedHandler:Disconnect();
	self.healthMaxChangedHandler = nil;
	self.energyMaxChangedHandler:Disconnect();
	self.energyMaxChangedHandler = nil;
	
	self.setBlueKeyVisibleHandler:Disconnect();
	self.setBlueKeyVisibleHandler = nil;
	self.setBlueKeyCollectedHandler:Disconnect();
	self.setBlueKeyCollectedHandler = nil;
	self.setRedKeyVisibleHandler:Disconnect();
	self.setRedKeyVisibleHandler = nil;
	self.setRedKeyCollectedHandler:Disconnect();
	self.setRedKeyCollectedHandler = nil;
	
	self.AddTrackerHandler:Disconnect();
	self.AddTrackerHandler = nil;
	self.RemoveTrackerHandler:Disconnect();
	self.RemoveTrackerHandler = nil;
	self.ShowTrackerHandler:Disconnect();
	self.ShowTrackerHandler = nil;
	self.HideTrackerHandler:Disconnect();
	self.HideTrackerHandler = nil;
	self.HighlightTrackerHandler:Disconnect();
	self.HighlightTrackerHandler = nil;
	self.UnHighlightTrackerHandler:Disconnect();
	self.UnHighlightTrackerHandler = nil;
	self.DestroyedTrackerHandler:Disconnect();
	self.DestroyedTrackerHandler = nil;
	
	self.tickHandler:Disconnect();
	self.tickHandler = nil;
end

function uiplayerhud:OnTick(deltaTime, timePoint)
		
	if(self.firstUpdate == true)then
		self.firstUpdate = false;
		
		-- this is necessary because they possibly dont exist at creation time yet
		self.playerID = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"));
		self.mapEntityID = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32(self.Properties.MapEntityTag));
		
		-- Listen for value set events.
		self.healthChangedEventId = GameplayNotificationId(self.playerID, self.Properties.Messages.HealthChangedEventIn, "float");
		self.healthChangedHandler = GameplayNotificationBus.Connect(self, self.healthChangedEventId);
		self.energyChangedEventId = GameplayNotificationId(self.playerID, self.Properties.Messages.EnergyChangedEventIn, "float");
		self.energyChangedHandler = GameplayNotificationBus.Connect(self, self.energyChangedEventId);
		
		self.healthMaxChangedEventId = GameplayNotificationId(self.playerID, self.Properties.Messages.HealthMaxChangedEventIn, "float");
		self.healthMaxChangedHandler = GameplayNotificationBus.Connect(self, self.healthMaxChangedEventId);
		self.energyMaxChangedEventId = GameplayNotificationId(self.playerID, self.Properties.Messages.EnergyMaxChangedEventIn, "float");
		self.energyMaxChangedHandler = GameplayNotificationBus.Connect(self, self.energyMaxChangedEventId);

		self.canvas = UiElementBus.Event.GetCanvas(self.entityId);
		
		self:SetEnergy(50);
		self:SetHealth(50);
		self:SetEnergyMax(50);
		self:SetHealthMax(50);
	end
	
	local fadeValue = 0;
	
	-- flash health Full
	if (self.HealthFadeTime > 0.0) then
		self.HealthFadeTime = self.HealthFadeTime - deltaTime;
		if (self.HealthFadeTime <= 0.0) then
			fadeValue = 0;
		elseif (self.HealthFadeTime <= 0.5) then
			fadeValue = self.HealthFadeTime * 2.0;
		else
			fadeValue = (1.0 - self.HealthFadeTime) * 2.0;
		end
		UiFaderBus.Event.Fade(self.Properties.ScreenHookup.StatusBars.HealthFlasherID, fadeValue, 0);
	end

	-- flash health low
	self.HealthLowFadeTime = self.HealthLowFadeTime - deltaTime;
	if (self.HealthLowFadeTime < 0) then
		if (self.HealthLastValue < 25.0) then
			self.HealthLowFadeTime = self.HealthLowFadeTime + 1.0;
		else
			self.HealthLowFadeTime = 0;
		end
	else
		self.HealthLowFadeTime = 0;
	end
	if (self.HealthLowFadeTime <= 0.0) then
		fadeValue = 0;
	elseif (self.HealthLowFadeTime <= 0.5) then
		fadeValue = self.HealthLowFadeTime * 2.0;
	else
		fadeValue = (1.0 - self.HealthLowFadeTime) * 2.0;
	end
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.StatusBars.HealthLowFlasherID, fadeValue, 0);

	-- flash energy Full
	if (self.EnergyFadeTime > 0.0) then
		self.EnergyFadeTime = self.EnergyFadeTime - deltaTime;
		if (self.EnergyFadeTime <= 0.0) then
			fadeValue = 0;
		elseif (self.EnergyFadeTime <= 0.5) then
			fadeValue = self.EnergyFadeTime * 2.0;
		else
			fadeValue = (1.0 - self.EnergyFadeTime) * 2.0;
		end
		UiFaderBus.Event.Fade(self.Properties.ScreenHookup.StatusBars.EnergyFlasherID, fadeValue, 0);
	end

	-- flash energy low
	self.EnergyLowFadeTime = self.EnergyLowFadeTime - deltaTime;
	if (self.EnergyLowFadeTime <= 0) then
		if (self.EnergyLastValue < 25.0) then
			self.EnergyLowFadeTime = self.EnergyLowFadeTime + 1.0;
		else
			self.EnergyLowFadeTime = 0;
		end
	else
		self.EnergyLowFadeTime = 0;
	end
	if (self.EnergyLowFadeTime <= 0.0) then
		fadeValue = 0;
	elseif (self.EnergyLowFadeTime <= 0.5) then
		fadeValue = self.EnergyLowFadeTime * 2.0;
	else
		fadeValue = (1.0 - self.EnergyLowFadeTime) * 2.0;
	end
	UiFaderBus.Event.Fade(self.Properties.ScreenHookup.StatusBars.EnergyLowFlasherID, fadeValue, 0);
		
	-- update map
	--self.mapEntityID = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32(self.Properties.MapEntityTag));
	if ((self.mapEntityID ~= nil) and (self.mapEntityID ~= EntityId())) then
		local camTm = TransformBus.Event.GetWorldTM(self.mapEntityID);
		local trackerPos = camTm:GetTranslation();
		local trackerDir = camTm:GetColumn(1):GetNormalized();
		trackerDir.z = 0;
		trackerDir = trackerDir:GetNormalized();
		local trackerRight = camTm:GetColumn(0);
		trackerRight.z = 0;
		trackerRight = trackerRight:GetNormalized();
		
		-- map start
		local mapValue = 0.5;
		
		local dirNorth = Vector3(0,1,0);
		
		local dirDot = trackerDir:Dot(dirNorth);
		local rightDot = trackerRight:Dot(dirNorth);
		local linearAngle = Math.ArcCos(dirDot) / math.pi;
		
		mapValue = (linearAngle + 1.0) * 0.5
		if(rightDot >= 0) then
		else
			mapValue = (mapValue * -1.0) + 1.0;
		end
		
		-- value is currenlty "0->1" for the full 360
		-- making it only show the foward 90
		--mapValue = self:Clamp(((mapValue - 0.5) * 4.0) + 0.5, 0.0, 1.0); 
		self:SetMapValue(mapValue);
		
		-- update all of the tracked objects
		--Debug.Log("Updatetrackers " .. #self.TrackedItems);
		--for i=#self.TrackedItems,1,-1 do
		--	self.TrackedItems[i]:Update(trackerPos, trackerDir, trackerRight, self.Properties.ScreenHookup.MapBar.TrackerAngle);
		--end
		for i, item in ipairs(self.TrackedItems) do
			--Debug.Log("     Updatetracked: " .. i);
			if (item ~= nil) then
				item:Update(trackerPos, trackerDir, trackerRight, self.Properties.ScreenHookup.MapBar.TrackerAngle);
			end
		end
	end
end

function uiplayerhud:SetHealth(value)	
	if (self.HealthLastValue ~= value) then
		--Debug.Log("SetHealth " .. value);
		if ((value == 100.0) and (self.HealthLastValue ~= 100.0)) then
			-- hit max health, do that flash
			self.HealthFadeTime = 1.0;
		end
		self.HealthLastValue = value;
		
		UiSliderBus.Event.SetValue(self.Properties.ScreenHookup.StatusBars.HealthBarID, value);	
	end
end

function uiplayerhud:SetEnergy(value)

	if(value ~= self.EnergyLastValue) then
		--Debug.Log("SetEnergy " .. value);
		if ((value == 100.0) and (self.EnergyLastValue ~= 100.0)) then
			-- hit max health, do that flash
			self.EnergyFadeTime = 1.0;
		end
		self.EnergyLastValue = value;
		UiSliderBus.Event.SetValue(self.Properties.ScreenHookup.StatusBars.EnergyBarID, value);	
	end
end

function uiplayerhud:SetHealthMax(value)
	UiSliderBus.Event.SetValue(self.Properties.ScreenHookup.StatusBars.HealthMaxBarID, value);	
end

function uiplayerhud:SetEnergyMax(value)
	UiSliderBus.Event.SetValue(self.Properties.ScreenHookup.StatusBars.EnergyMaxBarID, value);	
end

function uiplayerhud:SetMapValue(value)
	--Debug.Log("SetMapValue " .. value);
		
	UiSliderBus.Event.SetValue(self.Properties.ScreenHookup.MapBar.SliderBackgroundAID, value);
	UiSliderBus.Event.SetValue(self.Properties.ScreenHookup.MapBar.SliderBackgroundBID, 1.0 - value);
end

function uiplayerhud:CreateTracker(params)
	local parentEntityId = UiElementBus.Event.GetParent(self.Properties.ScreenHookup.MapBar.TrackerBaseID);

	table.insert(self.TrackedItems, MapTrackedInfo:new(params, StarterGameUIUtility.CloneElement(self.Properties.ScreenHookup.MapBar.TrackerBaseID, parentEntityId)) );
end

function uiplayerhud:RemoveTracker(entity)
	--Debug.Log("RemoveTracker " .. tostring(entity));
	for i=#self.TrackedItems,1,-1 do
		if (self.TrackedItems[i].entityId == entity) then
			self.TrackedItems[i]:removing();
		end
	end
end

function uiplayerhud:DestroyTracker(entity)
	--Debug.Log("DestroyTracker " .. tostring(entity));
	for i=#self.TrackedItems,1,-1 do
		if (self.TrackedItems[i].trackerId == entity) then
			self.TrackedItems[i]:cleanUp();
			table.remove(self.TrackedItems, i);
		end
	end
end

function uiplayerhud:ShowTracker(entity, show)
	--Debug.Log("ShowTracker " .. tostring(entity));
	for i,v in pairs(self.TrackedItems) do
		if (v.entityId == entity) then
			v:show(show);
		end
	end
end

function uiplayerhud:HighlightTracker(entity, highlight)
	--Debug.Log("HighlightTracker " .. tostring(entity));
	for i,v in pairs(self.TrackedItems) do
		if (v.entityId == entity) then
			v:highlight(highlight);
		end
	end
end

MapTrackedInfo =
{
	entityId = nil,
	trackerId = nil,
	trackerShowId = nil,
	trackerUpdateId = nil,
	trackerImageId = nil,
	trackerHighlightId = nil,
	trackerDestroyId = nil,
	beingDeleted = false,
}
MapTrackedInfo.__index = MapTrackedInfo

setmetatable(MapTrackedInfo, {
  __call = function (cls, ...)
    return cls.new(...)
  end,
})

function MapTrackedInfo:new(params, tracker)
	--Debug.Log("MapTrackedInfo:new : " .. tostring(params.trackedItem) .. " : " .. tostring(tracker));	
	local self = setmetatable({}, MapTrackedInfo);
	self.entityId = params.trackedItem;
	self.trackerId = tracker;
	self.trackerShowId = GameplayNotificationId(self.trackerId, "ShowMarkerEvent", "float");
	self.trackerUpdateId = GameplayNotificationId(self.trackerId, "SetValueEvent", "float");
	self.trackerImageId = GameplayNotificationId(self.trackerId, "SetImageEvent", "float");
	self.trackerHighlightId = GameplayNotificationId(self.trackerId, "SetHighlightedEvent", "float");
	self.trackerDestroyId = GameplayNotificationId(self.trackerId, "DestroyEvent", "float");
	
	if (params.graphic ~= "") then
		GameplayNotificationBus.Event.OnEventBegin(self.trackerImageId, params.graphic);
	end
	if (params.visible) then
		GameplayNotificationBus.Event.OnEventBegin(self.trackerShowId, params.visible);
	end
	
	return self;
end

function MapTrackedInfo:cleanUp()
	--Debug.Log("MapTrackedInfo:cleanUp");		
	StarterGameUIUtility.DeleteElement(self.trackerId);
	self.entityId = nil;
	self.trackerId = nil;
	self.trackerShowId = nil;
	self.trackerUpdateId = nil;
	self.trackerImageId = nil;
	self.trackerHighlightId = nil;
	self.beingDeleted = false;
end

function MapTrackedInfo:show(value)
	--Debug.Log("MapTrackedInfo:show");		
	GameplayNotificationBus.Event.OnEventBegin(self.trackerShowId, value);
end

function MapTrackedInfo:removing()
	--Debug.Log("MapTrackedInfo:removing");	
	self.beingDeleted = true;	
	GameplayNotificationBus.Event.OnEventBegin(self.trackerDestroyId, 1);
end

function MapTrackedInfo:highlight(value)
	--Debug.Log("MapTrackedInfo:highlight");		
	GameplayNotificationBus.Event.OnEventBegin(self.trackerShowId, value);
end

function MapTrackedInfo:Clamp(_n, _min, _max)
	if (_n > _max) then _n = _max; end
	if (_n < _min) then _n = _min; end
	return _n;
end

function MapTrackedInfo:Update(trackerPos, trackerDir, trackerRight, angle)
	local trackerValue = 0.5; -- mid point
	local posObjective = TransformBus.Event.GetWorldTM(self.entityId):GetTranslation();
	
	local dirToTracked = posObjective - trackerPos;
	dirToTracked.z = 0;
	local dirToTrackedLength = dirToTracked:GetLength();
	dirToTracked = dirToTracked / dirToTrackedLength;
	
	local dirDot = trackerDir:Dot(dirToTracked);
	local rightDot = trackerRight:Dot(dirToTracked);
	local linearAngle = Math.ArcCos(dirDot) / math.pi; -- gives -1 to 1
	
	trackerValue = (linearAngle + 1.0) * 0.5 -- gives 0 to 1
	if (rightDot >= 0) then
	else
		trackerValue = (trackerValue * -1.0) + 1.0;	-- flips because im on the wrong side
	end
	
	-- compensate for visible region on map
	trackerValue = self:Clamp(((trackerValue - 0.5) * (360 / angle)) + 0.5, 0.0, 1.0);
	--Debug.Log("     ObjectiveDirection: " .. tostring(self.entityId) .. " : " .. tostring(self.trackerId) .. " : " .. tostring(trackerValue));
	
	GameplayNotificationBus.Event.OnEventBegin(self.trackerUpdateId, trackerValue);
end

function uiplayerhud:OnEventBegin(value)
	--Debug.Log("Recieved message");
	if (GameplayNotificationBus.GetCurrentBusId() == self.healthChangedEventId) then
		self:SetHealth(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.energyChangedEventId) then
		self:SetEnergy(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.healthMaxChangedEventId) then
		self:SetHealthMax(value);		
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.energyMaxChangedEventId) then
		self:SetEnergyMax(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.setBlueKeyVisibleEventId) then
		local fadeValue = 0;
		if (value) then
			fadeValue = 1;
		end
		UiFaderBus.Event.Fade(self.Properties.ScreenHookup.Objectives.BlueKeyVisibleID, fadeValue, 1.0);	
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.setBlueKeyCollectedEventId) then
		local fadeValue = 0;
		if (value) then
			fadeValue = 1;
		end
		UiFaderBus.Event.Fade(self.Properties.ScreenHookup.Objectives.BlueKeyCollectedID, fadeValue, 1.0);	
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.setRedKeyVisibleEventId) then
		local fadeValue = 0;
		if (value) then
			fadeValue = 1;
		end
		UiFaderBus.Event.Fade(self.Properties.ScreenHookup.Objectives.RedKeyVisibleID, fadeValue, 1.0);	
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.setRedKeyCollectedEventId) then
		local fadeValue = 0;
		if (value) then
			fadeValue = 1;
		end
		UiFaderBus.Event.Fade(self.Properties.ScreenHookup.Objectives.RedKeyCollectedID, fadeValue, 1.0);	
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.AddTrackerEventId) then
		self:CreateTracker(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.RemoveTrackerEventId) then
		self:RemoveTracker(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.ShowTrackerEventId) then
		self:ShowTracker(value, true);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.HideTrackerEventId) then
		self:ShowTracker(value, false);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.HighlightTrackerEventId) then
		self:HighlightTracker(value, true);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.UnHighlightTrackerEventId) then
		self:HighlightTracker(value, false);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.DestroyedTrackerEventId) then
		self:DestroyTracker(value);
	end
end

return uiplayerhud;
