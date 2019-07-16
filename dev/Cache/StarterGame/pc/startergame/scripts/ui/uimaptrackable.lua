local UIMapTrackable =
{
	Properties =
	{		
		MessagesIn = {
			ShowMarkerEvent = { default = "ShowMarkerEvent", description = "The event used to enable/disable the marker (1 to enable; anything else disables)" },
			HighlightMarkerEvent = { default = "HighlightMarkerEvent", description = "The event used to make the marker flash" },
		},
		MessagesOut = {
			AddTracker = { default = "AddTrackerEvent", description = "Adds a tracker tracking the chosen element" },
			RemoveTracker = { default = "RemoveTrackerEvent", description = "Removes a tracker tracking the chosen element" },
			ShowTracker = { default = "ShowTrackerEvent", description = "Makes the tracker for the chosen element visible." },
			HideTracker = { default = "HideTrackerEvent", description = "Makes the tracker for the chosen element hidden." },
			HighlightTracker = { default = "HighlightTrackerEvent", description = "The event used to make the marker flash" },
			UnHighlightTracker = { default = "UnHighlightTrackerEvent", description = "The event used to make the marker flash" },
		},
		
		InitiallyEnabled = { default = true, description = "Initial state of this marker" },
		ImageOverride = { default = "", description = "The path of the graphic to use for the indicator"},
	},
}

function UIMapTrackable:OnActivate()
	--Debug.Log("UIMapTrackable:OnActivate()");

	self.hasTracker = false;
	self.highlightTracker = false;
	
	-- Listen for enable/disable events.
	self.ShowMarkerEventId = GameplayNotificationId(self.entityId, self.Properties.MessagesIn.ShowMarkerEvent, "float");
	self.ShowMarkerHandler = GameplayNotificationBus.Connect(self, self.ShowMarkerEventId);
	self.HighlightMarkerEventId = GameplayNotificationId(self.entityId, self.Properties.MessagesIn.HighlightMarkerEvent, "float");
	self.HighlightMarkerHandler = GameplayNotificationBus.Connect(self, self.HighlightMarkerEventId);
	
	self.AddTrackerEventId = GameplayNotificationId(EntityId(), self.Properties.MessagesOut.AddTracker, "float");
	self.RemoveTrackerEventId = GameplayNotificationId(EntityId(), self.Properties.MessagesOut.RemoveTracker, "float");
	self.ShowTrackerEventId = GameplayNotificationId(EntityId(), self.Properties.MessagesOut.ShowTracker, "float");
	self.HideTrackerEventId = GameplayNotificationId(EntityId(), self.Properties.MessagesOut.HideTracker, "float");
	self.HighlightTrackerEventId = GameplayNotificationId(EntityId(), self.Properties.MessagesOut.HighlightTracker, "float");
	self.UnHighlightTrackerEventId = GameplayNotificationId(EntityId(), self.Properties.MessagesOut.UnHighlightTracker, "float");

	if (self.Properties.InitiallyEnabled) then
		self:ShowTracker(true);
	end
end

function UIMapTrackable:OnDeactivate()
	--Debug.Log("UIMapTrackable:OnDeactivate()");
	self:ShowTracker(false);

	self.ShowMarkerHandler:Disconnect();
	self.ShowMarkerHandler = nil;
	self.HighlightMarkerHandler:Disconnect();
	self.HighlightMarkerHandler = nil;
end

function UIMapTrackable:ShowTracker(value)
	if (self.hasTracker == value) then
		return;
	end
	
	--Debug.Log("UIMapTrackable:ShowMarker " .. tostring(value));
	if (value) then
		local params = TrackerParams();
		params.trackedItem = self.entityId;
		params.graphic = self.Properties.ImageOverride;
		params.visible = true;
		
		GameplayNotificationBus.Event.OnEventBegin(self.AddTrackerEventId, params);
	else
		GameplayNotificationBus.Event.OnEventBegin(self.RemoveTrackerEventId, self.entityId);
	end
	
	self.hasTracker = value;
end

function UIMapTrackable:HighlightTracker(value)
	if (self.highlightTracker == value) then
		return;
	end
	
	--Debug.Log("UIMapTrackable:ShowMarker " .. value);
	if (value) then		
		GameplayNotificationBus.Event.OnEventBegin(self.HighlightTrackerEventId, self.entityId);
	else
		GameplayNotificationBus.Event.OnEventBegin(self.UnHighlightTrackerEventId, self.entityId);
	end
	
	self.highlightTracker = value;
end

function UIMapTrackable:OnEventBegin(value)
	--Debug.Log("Recieved message");
	if (GameplayNotificationBus.GetCurrentBusId() == self.ShowMarkerEventId) then
		self:ShowTracker(value == 1); 
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.HighlightMarkerEventId) then
		self:HighlightTracker(value); 
	end
end

return UIMapTrackable;
