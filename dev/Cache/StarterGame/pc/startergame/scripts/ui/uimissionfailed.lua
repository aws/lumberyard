
local uimissionfailed =
{
	Properties =
	{
		AutoFinish = { default = false },
		InputDelay = { default = 3, description = "The delay before somebody can push through the screen." },
		FaderID = { default = -1, description = "The ID of the fader component in the canvas." },
		
		CameraTurnRate = { default = 1, description = "The delay before somebody can push through the screen." },
		
		EnableControlsEvent = { default = "EnableControlsEvent", description = "The event used to enable/disable the player controls." },
		
		ShowScreenEvent = { default = "ShowMissionFailedEvent", description = "The event used to show this screen." },
		
		MissionFailSndEvent = "",
		
		--ShowScreenSndEvent = "",
		--CloseScreenSndEvent = "",
	},
}

function uimissionfailed:OnActivate()
	--Debug.Log("uimissionfailed:OnActivate");
	self.ShowScreenEventId = GameplayNotificationId(EntityId(), self.Properties.ShowScreenEvent, "float");
	self.ShowScreenHandler = GameplayNotificationBus.Connect(self, self.ShowScreenEventId);	
	self.FirstUpdate = true;
	-- AUDIO
	varMissionFailSndEvent = self.Properties.MissionFailSndEvent;
end

function uimissionfailed:OnDeactivate()
	--Debug.Log("uimissionfailed:OnDeactivate");
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

function uimissionfailed:ShowScreen()
	--Debug.Log("uimissionfailed:ShowScreen");
	
	-- IMPORTANT: The 'canvas ID' is different to 'self.entityId'.
	self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("UI/Canvases/UIMissionFailed.uicanvas");

	
	-- Listen for action strings broadcast by the canvas.
	self.uiCanvasNotificationLuaBusHandler = UiCanvasNotificationLuaBus.Connect(self, self.canvasEntityId);
		
	-- Display the mouse cursor.
	LyShineLua.ShowMouseCursor(true);
	
	self.tickHandler = TickBus.Connect(self);
	self.ignoredFirst = false;
	
	self.timer = self.Properties.InputDelay;

end

function uimissionfailed:CloseScreen()
	--Debug.Log("uimissionfailed - CloseScreen!");	
	
	-- Audio
	--AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, varPressStartSndEvent);
	
	-- Remove the U.I.
	if (self.canvasEntityId ~= nil) then
		UiCanvasManagerBus.Broadcast.UnloadCanvas(self.canvasEntityId);
		self.canvasEntityId = nil;
	end
	-- Hide the mouse cursor.
	LyShineLua.ShowMouseCursor(false);
	
	-- Enable the player controls.
	--self:SetControls("PlayerCharacter", self.Properties.EnableControlsEvent, 1.0);
	self:SetControls("PlayerCamera", self.Properties.EnableControlsEvent, 1.0);
	
	-- Enable the crosshair.
	--local ch = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("Crosshair"));
	--local chEventId = GameplayNotificationId(ch, "EnableCrosshairEvent", "float");
	--GameplayNotificationBus.Event.OnEventBegin(chEventId, 1.0);
	
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

function uimissionfailed:OnTick(deltaTime, timePoint)
	--Debug.Log("uimissionfailed:OnTick");

	-- The camera manager uses the 'OnTick()' call to set the initial camera (so all cameras
	-- can be set up in the 'OnActivate()' call. This means that we need to ignore the first
	-- tick otherwise we'll be competing with the camera manager.
	if (self.ignoredFirst == false) then
		self.ignoredFirst = true;
		return;
	end
	
	self.timer = self.timer - deltaTime;
	
	if (self.FirstUpdate) then
		self.FirstUpdate = false;
		-- Hide the mouse cursor.
		LyShineLua.ShowMouseCursor(false);
	
		StarterGameUIUtility.UIFaderControl(self.canvasEntityId, self.Properties.FaderID, 1.0, 0.25);
	
		-- Disable the player controls so that they can be re-enabled once the player has started.
		self:SetControls("PlayerCharacter", self.Properties.EnableControlsEvent, 0.0);
		self:SetControls("PlayerCamera", self.Properties.EnableControlsEvent, 0.0);
	end	
		
	local playerCam = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCamera"));
	local turnCamera = GameplayNotificationId(playerCam, "LookLeftRightForced", "float");
	--Debug.Log("Sending Turn: " .. tostring(playerCam) .. " : " .. tostring(turnCamera) .. " : " .. tostring(self.Properties.CameraTurnRate * deltaTime));	
	GameplayNotificationBus.Event.OnEventBegin(turnCamera, self.Properties.CameraTurnRate * deltaTime);		
		
	-- Check if we should skip the start screen.
	if(self.timer < 0) then
		if (self.Properties.AutoFinish == true) then
			--Debug.Log("skipping start screen");
			self:CloseScreen();
		else
			self.anyButtonEventId = GameplayNotificationId(self.entityId, "AnyKey", "float");
			self.anyButtonHandler = GameplayNotificationBus.Connect(self, self.anyButtonEventId);
		end
	end
end

function uimissionfailed:SetControls(tag, event, enabled)
	--Debug.Log("uimissionfailed:SetControls: " .. tostring(tag) .. ":" .. tostring(event) .. ":".. tostring(enabled));
	local entity = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32(tag));
	local controlsEventId = GameplayNotificationId(entity, event, "float");
	GameplayNotificationBus.Event.OnEventBegin(controlsEventId, enabled);		-- 0.0 to disable
end

function uimissionfailed:OnAction(entityId, actionName)
	if (actionName == "CloseScreen") then
		self:CloseScreen();
	end
end

function uimissionfailed:OnEventBegin(value)
	--Debug.Log("Recieved message - uimissionfailed");
	if (GameplayNotificationBus.GetCurrentBusId() == self.anyButtonEventId) then
		--Debug.Log("uimissionfailed - ANY KEY!");
		self:CloseScreen();
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.ShowScreenEventId) then
		--Debug.Log("Show screen");
		self:ShowScreen();
		-- Audio
		AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, varMissionFailSndEvent);			
	end

end



return uimissionfailed;
