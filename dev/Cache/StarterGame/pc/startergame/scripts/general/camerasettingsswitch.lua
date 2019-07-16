local utilities = require "scripts/common/utilities"

camerasettingsswitch =
{
	Properties =
	{
		EventOnEnter = { default = "OnEnter", description = "The name of the event that activates the settings." },
		EventOnExit = { default = "OnExit", description = "The name of the event that deactivates the settings." },
		
		TransitionTimeEnter = { default = 0.25, description = "Time to change to new camera settings when activated"},
		TransitionTimeExit = { default = 0.25, description = "Time to change to new camera settings when deactivated"},
		
		
		CameraSettings = { default = "", description = "The name of the camera settings preset." },
	},
}

function camerasettingsswitch:OnActivate()
	
	self.onEnterEventId = GameplayNotificationId(self.entityId, self.Properties.EventOnEnter, "float");
	self.onEnterHandler = GameplayNotificationBus.Connect(self, self.onEnterEventId);
	self.onExitEventId = GameplayNotificationId(self.entityId, self.Properties.EventOnExit, "float");
	self.onExitHandler = GameplayNotificationBus.Connect(self, self.onExitEventId);
	
	self.tickHandler = TickBus.Connect(self);
	
end

function camerasettingsswitch:OnDeactivate()
	
	if (self.tickHandler ~= nil) then
		self.tickHandler:Disconnect();
		self.tickHandler = nil;
	end
	if (self.onExitHandler ~= nil) then
		self.onExitHandler:Disconnect();
		self.onExitHandler = nil;
	end
	if (self.onEnterHandler ~= nil) then
		self.onEnterHandler:Disconnect();
		self.onEnterHandler = nil;
	end
	
end

function camerasettingsswitch:OnTick(deltaTime, timePoint)
	
	self.camMan = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("CameraManager"));
	self.pushNotificationId = GameplayNotificationId(self.camMan, "PushCameraSettings", "float");
	self.popNotificationId = GameplayNotificationId(self.camMan, "PopCameraSettings", "float");
	
	if (self.tickHandler ~= nil) then
		self.tickHandler:Disconnect();
		self.tickHandler = nil;
	end
	
end

function camerasettingsswitch:OnEventBegin(value)
	
	if (GameplayNotificationBus.GetCurrentBusId() == self.onEnterEventId) then
		local settings = CameraSettingsEventArgs();
		settings.name = self.Properties.CameraSettings;
		settings.entityId = self.entityId;
		settings.transitionTime = self.Properties.TransitionTimeEnter;		
		GameplayNotificationBus.Event.OnEventBegin(self.pushNotificationId, settings);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.onExitEventId) then
		local settings = CameraSettingsEventArgs();
		settings.name = self.Properties.CameraSettings;
		settings.entityId = self.entityId;
		settings.transitionTime = self.Properties.TransitionTimeExit;		
		GameplayNotificationBus.Event.OnEventBegin(self.popNotificationId, settings);
	end
	
end

return camerasettingsswitch;