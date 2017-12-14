
local uimissionsuccess =
{
	Properties =
	{
		AutoFinish = { default = false },
		InputDelay = { default = 3, description = "The delay before somebody can push through the screen." },
		FaderID = { default = -1, description = "The ID of the fader component in the canvas." },
		
		CameraToUse = {default = EntityId()},
		
		EnableControlsEvent = { default = "EnableControlsEvent", description = "The event used to enable/disable the player controls." },
		
		ShowScreenEvent = { default = "ShowMissionSuccessEvent", description = "The event used to show this screen." },
		
		MissionSuccessSndEvent = "",
		
		--ShowScreenSndEvent = "",
		--CloseScreenSndEvent = "",
	},
}



function uimissionsuccess:OnActivate()
	--Debug.Log("uimissionsuccess:OnActivate");
	self.ShowScreenEventId = GameplayNotificationId(EntityId(), self.Properties.ShowScreenEvent, "float");
	self.ShowScreenHandler = GameplayNotificationBus.Connect(self, self.ShowScreenEventId);	
	self.FirstUpdate = true;
	-- AUDIO
	varMissionSuccessSndEvent = self.Properties.MissionSuccessSndEvent;
end

function uimissionsuccess:OnDeactivate()
	--Debug.Log("uimissionsuccess:OnDeactivate");
	if (self.canvasEntityId ~= nil) then
		UiCanvasManagerBus.Broadcast.UnloadCanvas(self.canvasEntityId);
		self.canvasEntityId = nil;
	end

	if (self.uiCanvasNotificationLuaBusHandler ~= nil) then
		self.uiCanvasNotificationLuaBusHandler:Disconnect();
		self.uiCanvasNotificationLuaBusHandler = nil;
	end

	if (self.anyButtonHandler) then
		self.anyButtonHandler:Disconnect();
		self.anyButtonHandler = nil;
	end
	if (self.tickHandler ~= nil) then
		self.tickHandler:Disconnect();
		self.tickHandler = nil;
	end
end

function uimissionsuccess:ShowScreen()
	--Debug.Log("uimissionsuccess:ShowScreen");
	
	-- Audio
	--AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.Properties.ShowScreenSndEvent);	

	-- IMPORTANT: The 'canvas ID' is different to 'self.entityId'.
	self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("UI/Canvases/UIMissionSuccess.uicanvas");

	
	-- Listen for action strings broadcast by the canvas.
	self.uiCanvasNotificationLuaBusHandler = UiCanvasNotificationLuaBus.Connect(self, self.canvasEntityId);
		
	-- Display the mouse cursor.
	LyShineLua.ShowMouseCursor(true);
	
	self.tickHandler = TickBus.Connect(self);
	self.ignoredFirst = false;
	
	self.timer = self.Properties.InputDelay;

end

function uimissionsuccess:CloseScreen()
	--Debug.Log("uimissionsuccess - CloseScreen!");	
	
	-- Audio
	--AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.Properties.CloseScreenSndEvent);
	
	-- Remove the U.I.
	if (self.canvasEntityId ~= nil) then
		UiCanvasManagerBus.Broadcast.UnloadCanvas(self.canvasEntityId);
		self.canvasEntityId = nil;
	end
	-- Hide the mouse cursor.
	LyShineLua.ShowMouseCursor(false);
	
	-- camera cleanup
	local camMan = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("CameraManager"));
	local playerCam = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCamera"));
	local camEventId = GameplayNotificationId(camMan, "ActivateCameraEvent", "float");
	GameplayNotificationBus.Event.OnEventBegin(camEventId, playerCam);
	
	-- Enable the player controls.
	self:SetControls("PlayerCharacter", self.Properties.EnableControlsEvent, 1.0);
	self:SetControls("PlayerCamera", self.Properties.EnableControlsEvent, 1.0);
	
	-- Enable the crosshair.
	local ch = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("Crosshair"));
	local chEventId = GameplayNotificationId(ch, "EnableCrosshairEvent", "float");
	GameplayNotificationBus.Event.OnEventBegin(chEventId, 1.0);
	
	if (self.tickHandler ~= nil) then
		self.tickHandler:Disconnect();
		self.tickHandler = nil;
	end
	if (self.anyButtonHandler) then
		self.anyButtonHandler:Disconnect();
		self.anyButtonHandler = nil;
	end

	StarterGameUtility.RestartLevel(true);		
end

function uimissionsuccess:OnTick(deltaTime, timePoint)
	--Debug.Log("uimissionsuccess:OnTick");

	-- The camera manager uses the 'OnTick()' call to set the initial camera (so all cameras
	-- can be set up in the 'OnActivate()' call. This means that we need to ignore the first
	-- tick otherwise we'll be competing with the camera manager.
	if (self.ignoredFirst == false) then
		self.ignoredFirst = true;
		return;
	end
	
	self.timer = self.timer - deltaTime;
	
	
	-- perhaps do some camera things in gere, slow rotate of payer etc?
	
	if (self.FirstUpdate) then
		self.FirstUpdate = false;
		-- Hide the mouse cursor.
		LyShineLua.ShowMouseCursor(false);
	
		StarterGameUIUtility.UIFaderControl(self.canvasEntityId, self.Properties.FaderID, 1.0, 0.25);
	
		-- Disable the player controls so that they can be re-enabled once the player has started.
		self:SetControls("PlayerCharacter", self.Properties.EnableControlsEvent, 0.0);
		self:SetControls("PlayerCamera", self.Properties.EnableControlsEvent, 0.0);
	
		--Debug.Log("uimissionsuccess:OnTick - CameraToUse: " .. tostring(self.Properties.CameraToUse));
		if(self.Properties.CameraToUse ~= nil) then
			local camMan = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("CameraManager"));
			local camEventId = GameplayNotificationId(camMan, "ActivateCameraEvent", "float");
			GameplayNotificationBus.Event.OnEventBegin(camEventId, self.Properties.CameraToUse);
		end
		
	end	
				
	-- Check if we should skip the start screen.
	if(self.timer < 0) then
		if (self.Properties.AutoFinish == true) then
			--Debug.Log("skipping start screen");
			self:CloseScreen();
		else
			self.anyButtonEventId = GameplayNotificationId(self.entityId, "AnyKey", "float");
			self.anyButtonHandler = GameplayNotificationBus.Connect(self, self.anyButtonEventId);
			
			-- Disable the tick bus because we don't care about updating anymore.
			self.tickHandler:Disconnect();
			self.tickHandler = nil;
		end
	end
end

function uimissionsuccess:SetControls(tag, event, enabled)
	--Debug.Log("uimissionsuccess:SetControls: " .. tostring(tag) .. ":" .. tostring(event) .. ":".. tostring(enabled));

	local entity = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32(tag));
	local controlsEventId = GameplayNotificationId(entity, event, "float");
	GameplayNotificationBus.Event.OnEventBegin(controlsEventId, enabled);		-- 0.0 to disable

end

function uimissionsuccess:OnAction(entityId, actionName)

	if (actionName == "CloseScreen") then
		self.CloseScreen();
	end
		
end

function uimissionsuccess:OnEventBegin(value)
	--Debug.Log("Recieved message - uimissionsuccess");
	if (GameplayNotificationBus.GetCurrentBusId() == self.anyButtonEventId) then
		--Debug.Log("uimissionsuccess - ANY KEY!");
		self:CloseScreen();
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.ShowScreenEventId) then
		--Debug.Log("Show screen");
		self:ShowScreen();
		-- Audio
		AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, varMissionSuccessSndEvent);	
	end

end



return uimissionsuccess;
