
local uistartscreen =
{
	Properties =
	{
		SkipStartScreen = { default = false },
		
		EnableControlsEvent = { default = "EnableControlsEvent", description = "The event used to enable/disable the player controls." },
		
		PressStartSndEvent = "",
	},
}

function uistartscreen:OnActivate()

	-- IMPORTANT: The 'canvas ID' is different to 'self.entityId'.
	self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("UI/Canvases/Ui_Maze_StartScreen.uicanvas");
	
	-- Listen for action strings broadcast by the canvas.
	self.uiCanvasNotificationLuaBusHandler = UiCanvasNotificationLuaBus.Connect(self, self.canvasEntityId);
	
	self.anyButtonEventId = GameplayNotificationId(self.entityId, "AnyKey", "float");
	self.anyButtonHandler = GameplayNotificationBus.Connect(self, self.anyButtonEventId);
	
	-- Display the mouse cursor.
	LyShineLua.ShowMouseCursor(true);
	
	self.tickHandler = TickBus.Connect(self);
	self.ignoredFirst = false;
	
	self.gameStarted = false;

	-- AUDIO
	varPressStartSndEvent = self.Properties.PressStartSndEvent;
end

function uistartscreen:OnDeactivate()

	self.uiCanvasNotificationLuaBusHandler:Disconnect();
	self.uiCanvasNotificationLuaBusHandler = nil;

	if (self.anyButtonHandler) then
		self.anyButtonHandler:Disconnect();
		self.anyButtonHandler = nil;
	end
end

function uistartscreen:OnTick(deltaTime, timePoint)

	

	-- The camera manager uses the 'OnTick()' call to set the initial camera (so all cameras
	-- can be set up in the 'OnActivate()' call. This means that we need to ignore the first
	-- tick otherwise we'll be competing with the camera manager.
	if (self.ignoredFirst == false) then
		self.ignoredFirst = true;
		return;
	end
	
	Debug.Log("GameStarted: " .. tostring(StarterGameUtility.IsGameStarted()));
	
	if((not self.gameStarted) and StarterGameUtility.IsGameStarted()) then
		self.gameStarted = true;
		
		-- fade from black
	end
	
	if (self.tickHandler ~= nil) then
		-- Disable the player controls so that they can be re-enabled once the player has started.
		self:SetControls("PlayerCharacter", self.Properties.EnableControlsEvent, 0.0);
		self:SetControls("PlayerCamera", self.Properties.EnableControlsEvent, 0.0);
	
		-- Disable the tick bus because we don't care about updating anymore.
		self.tickHandler:Disconnect();
		self.tickHandler = nil;
	end	
		
	-- Check if we should skip the start screen.
	if (self.Properties.SkipStartScreen == true) then
		--Debug.Log("skipping start screen");
		self:OnAction(self.entityId, "StartGame");
	end
end

function uistartscreen:SetControls(tag, event, enabled)

	local entity = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32(tag));
	local controlsEventId = GameplayNotificationId(entity, event, "float");
	GameplayNotificationBus.Event.OnEventBegin(controlsEventId, enabled);		-- 0.0 to disable

end

function uistartscreen:OnAction(entityId, actionName)

	if (actionName == "StartGame") then
		-- Audio
		AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, varPressStartSndEvent);
		Debug.Log("Starting the game...");
		-- Remove the U.I.
		UiCanvasManagerBus.Broadcast.UnloadCanvas(self.canvasEntityId);
		-- Hide the mouse cursor.
		LyShineLua.ShowMouseCursor(false);
		
		-- Jump to the player camera.
		-- TODO: Expose the destination camera as a property?
		local camMan = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("CameraManager"));
		local playerCam = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCamera"));
		local camEventId = GameplayNotificationId(camMan, "ActivateCameraEvent", "float");
		GameplayNotificationBus.Event.OnEventBegin(camEventId, playerCam);
		
		-- Enable the player controls.
		self:SetControls("PlayerCharacter", self.Properties.EnableControlsEvent, 1.0);
		self:SetControls("PlayerCamera", self.Properties.EnableControlsEvent, 1.0);
		
		if (self.tickHandler ~= nil) then
			self.tickHandler:Disconnect();
			self.tickHandler = nil;
		end
	end
		
end

function uistartscreen:OnEventBegin(value)

	--Debug.Log("Recieved message - uistartscreen");
	if (GameplayNotificationBus.GetCurrentBusId() == self.anyButtonEventId) then
		--Debug.Log("ANY KEY!");
		self:OnAction(self.entityId, "StartGame");
		self.anyButtonHandler:Disconnect();
		self.anyButtonHandler = nil;	
	end

end



return uistartscreen;
