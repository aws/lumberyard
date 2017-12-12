local UIMapTrackable =
{
	Properties =
	{		
		Messages = {
			EnableUIEvent = { default = "EnableUIEvent", description = "The event used to enable the whole UI" }, -- maybe not neccessary?
			HideUIEvent = { default = "HideUIEvent", description = "The event used to set the whole UI visible / invisible" },
			ShowMarkerEvent = { default = "ShowMarkerEvent", description = "The event used to enable/disable the marker (1 to enable; anything else disables)" },
			SetTrackerEvent = { default = "SetTrackerEvent", description = "set the object doing the tracking" },
		},
		InitiallyEnabled = { default = true, description = "Initial state of this marker" },
		Tracker = { default = EntityId(), description = "The entity that is doing the tracking of this entity" },
		TrackerTags = { default = "", description = "If an entity is not specified then search for one that matches these tags" },
		
		ScreenHookup = {
			File = { default = "UI/Canvases/UIMapTrackable.uicanvas", description = "The file path in the projects directory of the scrren to use"},
			AllUIFaderID = { default = -1, description = "The ID of the All UI fader" },
			FaderID = { default = -1, description = "ID the fader for the objective tracker" },
			ScrollID = { default = -1, description = "ID the scroll bar for the objective tracker" },
			TrackerAngle = { default = 157.5, description = "The Visible angle on the tracker, degrees" },
		},
	},
}

function UIMapTrackable:OnActivate()
	--Debug.Log("UIMapTrackable:OnActivate()");

	self.enabled = false;
	
	-- Listen for enable/disable events.
	self.EnableUIEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.EnableUIEvent, "float");
	self.EnableUIHandler = GameplayNotificationBus.Connect(self, self.EnableUIEventId);
	self.HideUIEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.HideUIEvent, "float");
	self.HideUIHandler = GameplayNotificationBus.Connect(self, self.HideUIEventId);
	self.ShowMarkerEventId = GameplayNotificationId(self.entityId, self.Properties.Messages.ShowMarkerEvent, "float");
	self.ShowMarkerHandler = GameplayNotificationBus.Connect(self, self.ShowMarkerEventId);
	self.SetTrackerEventId = GameplayNotificationId(self.entityId, self.Properties.Messages.SetTrackerEvent, "float");
	self.SetTrackerHandler = GameplayNotificationBus.Connect(self, self.SetTrackerEventId);
	
	--Debug.Log("UIPlayerHUD ID: " .. tostring(self.entityId));
	self:EnableHud(true);
	
	local show = 0;
	if (self.Properties.InitiallyEnabled) then
		show = 1;
	end
	StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.FaderID, show, 0.0);
end

function UIMapTrackable:OnDeactivate()
	--Debug.Log("UIMapTrackable:OnDeactivate()");

	self:EnableHud(false);

	self.EnableUIHandler:Disconnect();
	self.EnableUIHandler = nil;
	self.HideUIHandler:Disconnect();
	self.HideUIHandler = nil;
	self.ShowMarkerHandler:Disconnect();
	self.ShowMarkerHandler = nil;
	self.SetTrackerHandler:Disconnect();
	self.SetTrackerHandler = nil;
end

function UIMapTrackable:Clamp(_n, _min, _max)
	if (_n > _max) then _n = _max; end
	if (_n < _min) then _n = _min; end
	return _n;
end

function UIMapTrackable:OnTick(deltaTime, timePoint)

	if (not self.Properties.Tracker:IsValid()) then
		if(self.Properties.TrackerTags ~= "") then
			local trackerProspect = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32(self.Properties.TrackerTags));
			if (trackerProspect:IsValid()) then 
				self.Properties.Tracker = trackerProspect;
			else
				return;
			end
		else
			return;
		end
	end
	--Debug.Log("UIMapTrackable:OnTick()");
	
	-- update objectives		
	local trackerTm = TransformBus.Event.GetWorldTM(self.Properties.Tracker);
	local trackerPos = trackerTm:GetTranslation();
	local trackerDir = trackerTm:GetColumn(1):GetNormalized();
	trackerDir.z = 0;
	trackerDir = trackerDir:GetNormalized();
	local trackerRight = trackerTm:GetColumn(0);
	trackerRight.z = 0;
	trackerRight = trackerRight:GetNormalized();
	
	local pi = 3.1415926535897932384626433832795;

	local trackerValue = 0.5; -- mid point
	local posObjective = TransformBus.Event.GetWorldTM(self.entityId):GetTranslation();	-- will self work ?
	--Debug.Log("ObjectivePosition: " .. tostring(posObjective));		
	
	local dirToTracked = posObjective - trackerPos;
	dirToTracked.z = 0;
	local dirToTrackedLength = dirToTracked:GetLength();
	dirToTracked = dirToTracked / dirToTrackedLength;
	
	local dirDot = trackerDir:Dot(dirToTracked);
	local rightDot = trackerRight:Dot(dirToTracked);
	local linearAngle = Math.ArcCos(dirDot) / pi; -- gives -1 to 1
	
	trackerValue = (linearAngle + 1.0) * 0.5 -- gives 0 to 1
	if(rightDot >= 0) then
	else
		trackerValue = (trackerValue * -1.0) + 1.0;	-- flips because im on the wrong side
	end
	
	-- compensate for visible region on map
	trackerValue = self:Clamp(((trackerValue - 0.5) * (360 / self.Properties.ScreenHookup.TrackerAngle)) + 0.5, 0.0, 1.0);
	--Debug.Log("ObjectiveDirection: " .. tostring(self.entityId) .. " : " .. tostring(trackerValue));
	StarterGameUtility.UIScrollControl(self.canvasEntityId, self.Properties.ScreenHookup.ScrollID, trackerValue);	
end

function UIMapTrackable:EnableHud(enabled)

	if (enabled == self.enabled) then
		return;
	end
	
	--Debug.Log("UIMapTrackable:EnableHud()" .. tostring(enabled));
	
	if (enabled) then
		-- IMPORTANT: The 'canvas ID' is different to 'self.entityId'.
		self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas(self.Properties.ScreenHookup.File);

		--Debug.Log("UIMapTrackable:CanvasLoad(): " .. tostring(self.canvasEntityId));
		
		-- Listen for action strings broadcast by the canvas.
		self.uiCanvasNotificationLuaBusHandler = UiCanvasNotificationLuaBus.Connect(self, self.canvasEntityId);
		
		self.visible = true;
		
	    self.tickBusHandler = TickBus.Connect(self);
	else
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;

		self.visible = false;
		
		UiCanvasManagerBus.Broadcast.UnloadCanvas(self.canvasEntityId);

		self.uiCanvasNotificationLuaBusHandler:Disconnect();
		self.uiCanvasNotificationLuaBusHandler = nil;
	end
	self.enabled = enabled;
end

function UIMapTrackable:HideHud(enabled)
	if (self.enabled == false) then
		return;
	end
	
	--Debug.Log("UIMapTrackable:HideHud(): " .. tostring(enabled));
	
	local fadeValue = 1;
	if(enabled) then
		fadeValue = 0;
	end
	StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.AllUIFaderID, fadeValue, 1.0);	
end

function UIMapTrackable:SetTracker(value)
	if (self.enabled == false) then
		return;
	end
		
	--Debug.Log("SetTracker: " .. value);
	
	self.Properties.Tracker = value;
end

function UIMapTrackable:ShowMarker(value)
	if (self.enabled == false) then
		return;
	end
	
	--Debug.Log("UIMapTrackable:ShowMarker " .. value);

	StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.FaderID, value, 1.0);
end

function UIMapTrackable:OnEventBegin(value)
	--Debug.Log("Recieved message");
	if (GameplayNotificationBus.GetCurrentBusId() == self.EnableUIEventId) then
		self:EnableHud(value == 1);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.HideUIEventId) then
		self:HideHud(value == 1);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetTrackerEventId) then
		self:SetTracker(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.ShowMarkerEventId) then
		self:ShowMarker(value); 
	end
end

return UIMapTrackable;
