--[[
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--]]


local uistartscreen =
{
	Properties =
	{
		SkipStartScreen = { default = false },
		
		StartScreenMusic = { default = "StartScreenMusicEvent", description = "Event to start start sceen music" },
		
		EnableControlsEvent = { default = "EnableControlsEvent", description = "The event used to enable/disable the player controls." },
		CutsceneEndedEvent = { default = "CutsceneHasStopped", description = "The event used to catch the end of the cutscene." },
		PlayCutsceneEvent = { default = "StartCutscene", description = "The event used to start the cutscene." },
		
		PressStartSndEvent = "",
	},
}

function uistartscreen:OnActivate()
	-- IMPORTANT: The 'canvas ID' is different to 'self.entityId'.
	self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("UI/Canvases/uiStartScreen.uicanvas");
	self.playerHUDCanvasEntityId = UiCanvasRefBus.Event.GetCanvas(self.entityId);

	--Debug.Log("uistartscreen:OnActivate - " .. tostring(self.canvasEntityId));
	
	-- Listen for action strings broadcast by the canvas.
	--self.uiCanvasNotificationLuaBusHandler = UiCanvasNotificationLuaBus.Connect(self, self.canvasEntityId);
	
	self.anyButtonEventId = GameplayNotificationId(self.entityId, "AnyKey", "float");
	self.anyButtonHandler = GameplayNotificationBus.Connect(self, self.anyButtonEventId);
	self.endScreenEventId = GameplayNotificationId(self.entityId, self.Properties.CutsceneEndedEvent, "float");
	self.endScreenHandler = GameplayNotificationBus.Connect(self, self.endScreenEventId);
	
	-- Display the mouse cursor.
	LyShineLua.ShowMouseCursor(true);
	
	self.tickHandler = TickBus.Connect(self);
	self.ignoredFirst = false;
	
	self.gameStarted = false;
	self.cutsceneStarted = false;
	self.controlsDisabled = false;
end

function uistartscreen:OnDeactivate()
	--self.uiCanvasNotificationLuaBusHandler:Disconnect();
	--self.uiCanvasNotificationLuaBusHandler = nil;

	if (self.anyButtonHandler) then
		self.anyButtonHandler:Disconnect();
		self.anyButtonHandler = nil;
	end
	if (self.endScreenHandler) then
		self.endScreenHandler:Disconnect();
		self.endScreenHandler = nil;
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
	
	--Debug.Log("GameStarted: " .. tostring(StarterGameUtility.IsGameStarted()));
	
	if((not self.gameStarted) and StarterGameUtility.IsGameStarted()) then
		self.gameStarted = true;
		
		-- fade from black
	end
	
	if (self.tickHandler ~= nil) then
		-- Disable the player controls so that they can be re-enabled once the player has started.
		--Debug.Log("uistartscreen:OnTick - disable controls");
		self:SetControls("PlayerCharacter", self.Properties.EnableControlsEvent, 0);
		self:SetControls("PlayerCamera", self.Properties.EnableControlsEvent, 0);
		self.controlsDisabled = true;
		self:EnablePlayerHUD(false);	
		-- Disable the tick bus because we don't care about updating anymore.
		self.tickHandler:Disconnect();
		self.tickHandler = nil;
	end	
		
	AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.Properties.StartScreenMusic);
		
	-- Check if we should skip the start screen.
	if (self.Properties.SkipStartScreen == true) then
		--Debug.Log("skipping start screen");
		UiCanvasManagerBus.Broadcast.UnloadCanvas(self.canvasEntityId);
		self:OnAction(self.entityId, "StartGame");
		self.cutsceneStarted = true;
	end
end

function uistartscreen:SetControls(tag, event, enabled)

	local entity = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32(tag));
	local controlsEventId = GameplayNotificationId(entity, event, "float");
	GameplayNotificationBus.Event.OnEventBegin(controlsEventId, enabled);		-- 0.0 to disable

end

function uistartscreen:EnablePlayerHUD(enabled)
	UiCanvasBus.Event.SetEnabled(self.playerHUDCanvasEntityId, enabled);
end

function uistartscreen:OnAction(entityId, actionName)

	if (actionName == "StartGame") then
		--Debug.Log("Starting the game...");
		
		-- Jump to the player camera.
		-- TODO: Expose the destination camera as a property?
		local camMan = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("CameraManager"));
		local playerCam = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCamera"));
		local camEventId = GameplayNotificationId(camMan, "ActivateCameraEvent", "float");
		GameplayNotificationBus.Event.OnEventBegin(camEventId, playerCam);
		
		-- Enable the player controls.
		self:SetControls("PlayerCharacter", self.Properties.EnableControlsEvent, 1);
		self:SetControls("PlayerCamera", self.Properties.EnableControlsEvent, 1);
		self.controlsDisabled = false;
		self:EnablePlayerHUD(true);
		if (self.tickHandler ~= nil) then
			self.tickHandler:Disconnect();
			self.tickHandler = nil;
		end
	end
		
end

function uistartscreen:OnEventBegin(value)
	--Debug.Log("Recieved message - uistartscreen");
	if (GameplayNotificationBus.GetCurrentBusId() == self.anyButtonEventId and self.controlsDisabled) then
		if(self.cutsceneStarted  == false) then
			local cutsceneEventId = GameplayNotificationId(self.entityId, self.Properties.PlayCutsceneEvent, "float");
			GameplayNotificationBus.Event.OnEventBegin(cutsceneEventId, 1);
			self.cutsceneStarted = true;
		
			-- Hide the mouse cursor.
			LyShineLua.ShowMouseCursor(false);
		
			self.anyButtonHandler:Disconnect();
			self.anyButtonHandler = nil;	
			
			-- Audio
			AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.Properties.PressStartSndEvent);		
		
			-- Remove the U.I.
			--Debug.Log("uistartscreen:OnAction - remove screen " .. tostring(self.canvasEntityId));
			UiCanvasManagerBus.Broadcast.UnloadCanvas(self.canvasEntityId);
		end
	end
	if (GameplayNotificationBus.GetCurrentBusId() == self.endScreenEventId) then
		self:OnAction(self.entityId, "StartGame");	
	end
end



return uistartscreen;
