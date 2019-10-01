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

local utilities = require "scripts/common/utilities"
local StateMachine = require "scripts/common/StateMachine"

local teleporter =
{
	Properties =
	{
		Destination =
		{
			Location = { default = EntityId(), description = "Location to teleport to." },
			Direction = { default = EntityId(), description = "Entity describing the direction the player should be facing." },
		},
		
		FadeEvents =
		{
			Show = { default = "TeleportShowUIEvent", description = "The event to show the black screen." },
			Hide = { default = "TeleportHideUIEvent", description = "The event to hide the black screen." },
			Duration = { default = "TeleportFadeDurationUIEvent", description = "The event to set the duration of the fade." },
		},
		
		States =
		{
			DebugStateMachine = { default = false, description = "Output debug information about the teleporter's states." },
			
			Idle =
			{
				StartEvent = { default = "StartTeleporting", description = "Initiates the teleport." },
				StopEvent = { default = "StopTeleporting", description = "Cancels the teleport." },
			},
			StationaryCheck =
			{
				Duration = { default = 1.0, description = "How long the player must stand still for." },
			},
			Energizing =
			{
				Duration = { default = 1.0, description = "How long after the teleport particles have started before the player gets locked into the teleport." },
				
				Particles =
				{
					Location = { default = EntityId(), description = "The entity that has the particles on it." },
					Event = { default = "TeleporterEnergizing", description = "The event to trigger the particles." },
				},
			},
			TeleportingOut =
			{
				DurationBeforeTeleport = { default = 1.0, description = "How long before we actually teleport the player." },
				DurationBeforeFade = { default = 1.0, description = "How long to wait after the teleport before we start the fade." },
				
				Particles =
				{
					Location = { default = EntityId(), description = "The entity that has the particles on it." },
					Event = { default = "TeleporterTeleportingOut", description = "The event to trigger the particles." },
				},
				
				Camera = { default = EntityId(), description = "The camera to use when teleporting the player away." },
			},
			FadeToBlack =
			{
				Duration = { default = 1.0, description = "How long the fade should take." },
			},
			StayBlack =
			{
				Duration = { default = 0.2, description = "How long to remain on a black screen." },
				
				CutsceneCamera = { default = EntityId(), description = "The camera to use when watching the player teleport in." },
			},
			FadeFromBlack =
			{
				Duration = { default = 0.5, description = "How long the fade should take." },
			},
			TeleportingIn =
			{
				DurationBeforeTeleport = { default = 1.0, description = "How long before we actually teleport the player in." },
				DurationBeforeReturnControl = { default = 1.5, description = "How long to wait after the teleport occurs." },
				
				Particles =
				{
					Location = { default = EntityId(), description = "The entity that has the particles on it." },
					Event = { default = "TeleporterTeleportingIn", description = "The event to trigger the particles." },
				},
			},
			ReturnControl =
			{
				Duration = { default = 1.0, description = "How long it should take to move the camera into position." },
				
				FocusPlayer = { default = true, description = "If true, the cutscene camera will attempt to keep the player in view when moving." },
			},
		},
	},
	
	
	--------------------------------------------------
	-- Teleporter State Machine Definition
	-- States:
	--		Idle
	--		StationaryCheck
	--		Energizing
	--		TeleportingOut
	--		FadeToBlack
	--		StayBlack
	--		FadeFromBlack
	--		TeleportingIn
	--		ReturnControl
	--
	-- Transitions:
	--		Idle			 -> StationaryCheck
	--						<-  ReturnControl
	--		StationaryCheck	<-> Idle
	--						 -> Energizing
	--		Energizing		 -> Idle
	--						<-  StationaryCheck
	--						 -> TeleportingOut
	--		TeleportingOut	<-  Energizing
	--						 -> FadeToBlack
	--		FadeToBlack		<-  TeleportingOut
	--						 -> StayBlack
	--		StayBlack		<-  FadeToBlack
	--						 -> FadeFromBlack
	--		FadeFromBlack 	<-  StayBlack
	--						 -> TeleportingIn
	--		TeleportingIn	<-  FadeFromBlack
	--						 -> ReturnControl
	--		ReturnControl	<-  TeleportingIn
	--						 -> Idle
	--------------------------------------------------
	States =
	{
		Idle =
		{
			OnEnter = function(self, sm)
				sm.UserData.payload = nil;
				sm.UserData.teleportee = nil;
			end,
			
			OnExit = function(self, sm)
				sm.UserData.teleportee = sm.UserData.payload;
			end,
			
			OnUpdate = function(self, sm, deltaTime)
			end,
			
			Transitions =
			{
				StationaryCheck =
				{
					Evaluate = function(state, sm)
						return sm.UserData.payload ~= nil;
					end,
				},
			},
		},
		
		StationaryCheck =
		{
			OnEnter = function(self, sm)
				self.timeRemaining = sm.UserData.Properties.States.StationaryCheck.Duration;
				self.prevPos = TransformBus.Event.GetWorldTM(sm.UserData.teleportee):GetTranslation();
			end,
			
			OnExit = function(self, sm)
			end,
			
			OnUpdate = function(self, sm, deltaTime)
				if (sm.UserData:ObjectHasMoved(sm.UserData.teleportee, self.prevPos)) then
					self.timeRemaining = sm.UserData.Properties.States.StationaryCheck.Duration;
					self.prevPos = TransformBus.Event.GetWorldTM(sm.UserData.teleportee):GetTranslation();
				else
					self.timeRemaining = self.timeRemaining - deltaTime;
				end
			end,
			
			Transitions =
			{
				Idle =
				{
					Evaluate = function(state, sm)
						return sm.UserData.payload == nil;
					end,
				},
				Energizing =
				{
					Evaluate = function(state, sm)
						return state.timeRemaining <= 0.0;
					end,
				},
			},
		},
		
		Energizing =
		{
			OnEnter = function(self, sm)
				self.timeRemaining = sm.UserData.Properties.States.Energizing.Duration;
				self.prevPos = TransformBus.Event.GetWorldTM(sm.UserData.teleportee):GetTranslation();
				
				-- Trigger the particles.
				local eventId = GameplayNotificationId(sm.UserData.Properties.States.Energizing.Particles.Location, sm.UserData.Properties.States.Energizing.Particles.Event, "float");
				GameplayNotificationBus.Event.OnEventBegin(eventId, 0);
			end,
			
			OnExit = function(self, sm)
			end,
			
			OnUpdate = function(self, sm, deltaTime)
				self.timeRemaining = self.timeRemaining - deltaTime;
			end,
			
			Transitions =
			{
				Idle =
				{
					Evaluate = function(state, sm)
						return sm.UserData.payload == nil;
					end,
				},
				StationaryCheck =
				{
					Evaluate = function(state, sm)
						return sm.UserData:ObjectHasMoved(sm.UserData.teleportee, state.prevPos);
					end,
				},
				TeleportingOut =
				{
					Evaluate = function(state, sm)
						return state.timeRemaining <= 0.0;
					end,
				},
			},
		},
		
		TeleportingOut =
		{
			OnEnter = function(self, sm)
				self.timeUntilTeleport = sm.UserData.Properties.States.TeleportingOut.DurationBeforeTeleport;
				self.timeUntilFade = sm.UserData.Properties.States.TeleportingOut.DurationBeforeFade;
				self.updatedAtleastOnce = false;
				
				-- Trigger the particles.
				local eventId = GameplayNotificationId(sm.UserData.Properties.States.TeleportingOut.Particles.Location, sm.UserData.Properties.States.TeleportingOut.Particles.Event, "float");
				GameplayNotificationBus.Event.OnEventBegin(eventId, 0);
				
				-- Disable player controls.
				sm.UserData:EnablePlayerControls(0);
				
				-- Take the properties from the player's camera and apply them so there's no
				-- obvious change (i.e. F.o.V., clip distances, etc.).
				local newCam = sm.UserData.Properties.States.TeleportingOut.Camera;
				local playerCam = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCamera"));
				sm.UserData:DuplicateCameraProps(playerCam, newCam);
				
				-- Move the new camera to where the player's camera was.
				TransformBus.Event.SetWorldTM(newCam, TransformBus.Event.GetWorldTM(playerCam));
				
				-- Switch camera so we can move the player without having the camera move.
				local camMan = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("CameraManager"));
				local eventId = GameplayNotificationId(camMan, "ActivateCameraEvent", "float");
				GameplayNotificationBus.Event.OnEventBegin(eventId, newCam);
			end,
			
			OnExit = function(self, sm)
			end,
			
			OnUpdate = function(self, sm, deltaTime)
				self.updatedAtleastOnce = true;
				
				if (self.timeUntilTeleport > 0.0) then
					self.timeUntilTeleport = self.timeUntilTeleport - deltaTime;
				elseif (self.timeUntilFade > 0.0) then
					self.timeUntilFade = self.timeUntilFade - deltaTime;
				end
				
				-- At this point, move the player to the new location.
				if (self.timeUntilTeleport <= 0.0 and self.timeUntilFade == sm.UserData.Properties.States.TeleportingOut.DurationBeforeFade) then
					sm.UserData:TeleportObjectAway();
				end
			end,
			
			Transitions =
			{
				FadeToBlack =
				{
					Evaluate = function(state, sm)
						return state.updatedAtleastOnce and state.timeUntilTeleport <= 0.0 and state.timeUntilFade <= 0.0;
					end,
				},
			},
		},
		
		FadeToBlack =
		{
			OnEnter = function(self, sm)
				self.timeRemaining = sm.UserData.Properties.States.FadeToBlack.Duration;
				
				-- Fade the screen.
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.durationEventId, self.timeRemaining);
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.hideEventId, true);
			end,
			
			OnExit = function(self, sm)
			end,
			
			OnUpdate = function(self, sm, deltaTime)
				self.timeRemaining = self.timeRemaining - deltaTime;
			end,
			
			Transitions =
			{
				StayBlack =
				{
					Evaluate = function(state, sm)
						return state.timeRemaining <= 0.0;
					end,
				},
			},
		},
		
		StayBlack =
		{
			OnEnter = function(self, sm)
				self.timeRemaining = sm.UserData.Properties.States.StayBlack.Duration;
				
				-- Switch cameras so we now look at the destination (where the player will appear).
				local newCam = sm.UserData.Properties.States.StayBlack.CutsceneCamera;
				local camMan = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("CameraManager"));
				local eventId = GameplayNotificationId(camMan, "ActivateCameraEvent", "float");
				GameplayNotificationBus.Event.OnEventBegin(eventId, newCam);
				
				-- Take the properties from the player's camera and apply them so there's no
				-- obvious change (i.e. F.o.V., clip distances, etc.).
				local playerCam = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCamera"));
				sm.UserData:DuplicateCameraProps(playerCam, newCam);
			end,
			
			OnExit = function(self, sm)
			end,
			
			OnUpdate = function(self, sm, deltaTime)
				self.timeRemaining = self.timeRemaining - deltaTime;
			end,
			
			Transitions =
			{
				FadeFromBlack =
				{
					Evaluate = function(state, sm)
						return state.timeRemaining <= 0.0;
					end,
				},
			},
		},
		
		FadeFromBlack =
		{
			OnEnter = function(self, sm)
				self.timeRemaining = sm.UserData.Properties.States.FadeFromBlack.Duration;
				
				-- Fade the screen.
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.durationEventId, self.timeRemaining);
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.showEventId, true);
				-- Orientate the player.
				-- In theory, I could just use the player's location and the destination's
				-- direction but I want to make sure that the direction is valid (i.e. not zero
				-- length).
				local playerTM = TransformBus.Event.GetWorldTM(sm.UserData.teleportee);
				local newDir = TransformBus.Event.GetWorldTM(sm.UserData.Properties.Destination.Direction):GetTranslation() - playerTM:GetTranslation();
				newDir.z = 0.0;
				if (newDir:GetLengthSq() < 0.01) then
					newDir = Vector3(0.0, 1.0, 0.0);
				else
					newDir:Normalize();
				end
				TransformBus.Event.SetWorldTM(sm.UserData.teleportee, MathUtils.CreateLookAt(playerTM:GetTranslation(), playerTM:GetTranslation() + newDir, AxisType.YPositive));
				
				-- Also reset the camera's orientation so it has time to perform any collision
				-- detection resolutions.
				local playerCam = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCamera"));
				local resetEvent = GameplayNotificationId(playerCam, "ResetOrientationEvent", "float");
				GameplayNotificationBus.Event.OnEventBegin(resetEvent, true);	-- payload is irrelevant

			end,
			
			OnExit = function(self, sm)
			end,
			
			OnUpdate = function(self, sm, deltaTime)
				self.timeRemaining = self.timeRemaining - deltaTime;
			end,
			
			Transitions =
			{
				TeleportingIn =
				{
					Evaluate = function(state, sm)
						return state.timeRemaining <= 0.0;
					end,
				},
			},
		},
		
		TeleportingIn =
		{
			OnEnter = function(self, sm)
				self.timeUntilTeleport = sm.UserData.Properties.States.TeleportingIn.DurationBeforeTeleport;
				self.timeUntilReturnControl = sm.UserData.Properties.States.TeleportingIn.DurationBeforeReturnControl;
				
				-- Trigger the particles.
				local eventId = GameplayNotificationId(sm.UserData.Properties.States.TeleportingIn.Particles.Location, sm.UserData.Properties.States.TeleportingIn.Particles.Event, "float");
				GameplayNotificationBus.Event.OnEventBegin(eventId, 0);
			end,
			
			OnExit = function(self, sm)
			end,
			
			OnUpdate = function(self, sm, deltaTime)
				if (self.timeUntilTeleport > 0.0) then
					self.timeUntilTeleport = self.timeUntilTeleport - deltaTime;
				elseif (self.timeUntilReturnControl > 0.0) then
					self.timeUntilReturnControl = self.timeUntilReturnControl - deltaTime;
				end
				
				-- At this point, move the player to the new location.
				if (self.timeUntilTeleport <= 0.0 and self.timeUntilReturnControl == sm.UserData.Properties.States.TeleportingIn.DurationBeforeReturnControl) then
					sm.UserData:TeleportObjectIn();
				end
			end,
			
			Transitions =
			{
				ReturnControl =
				{
					Evaluate = function(state, sm)
						return state.timeUntilReturnControl <= 0.0;
					end,
				},
			},
		},
		
		ReturnControl =
		{
			OnEnter = function(self, sm)
				self.focusPlayer = sm.UserData.Properties.States.ReturnControl.FocusPlayer;	-- alias
				self.duration = sm.UserData.Properties.States.ReturnControl.Duration;	-- alias
				self.timeRemaining = self.duration;
				
				self.camMan = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("CameraManager"));
				self.playerCam = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCamera"));
				self.cutsceneCam = sm.UserData.Properties.States.StayBlack.CutsceneCamera;
				
				self.playerCamOriginalTM = TransformBus.Event.GetWorldTM(self.playerCam);
				
				-- We need to store the original transform of the cutscene camera so we can reset
				-- it once the teleportation has completed (otherwise the next time the
				-- teleporter's used it will start in the position of the player's camera).
				self.cutsceneCamOriginalTM = TransformBus.Event.GetWorldTM(self.cutsceneCam);
				
				if (self.focusPlayer) then
					local targetPos = StarterGameUtility.GetJointWorldTM(sm.UserData.teleportee, utilities.Joint_Torso_Upper):GetTranslation();
					local dir = targetPos - self.playerCamOriginalTM:GetTranslation();
					local dist = dir:GetLength();
					self.playerCamLookAt = self.playerCamOriginalTM:GetTranslation() + (self.playerCamOriginalTM:GetColumn(1) * dist);
					
					dir = targetPos - self.cutsceneCamOriginalTM:GetTranslation();
					dist = dir:GetLength();
					self.cutsceneCamLookAt = self.cutsceneCamOriginalTM:GetTranslation() + (self.cutsceneCamOriginalTM:GetColumn(1) * dist);
				else
					self.playerCamForward = self.playerCamOriginalTM:GetColumn(1);
					self.cutsceneCamForward = self.cutsceneCamOriginalTM:GetColumn(1);
				end
			end,
			
			OnExit = function(self, sm)
				-- Switch camera as the camera should now be in the same place as the player camera.
				local eventId = GameplayNotificationId(self.camMan, "ActivateCameraEvent", "float");
				GameplayNotificationBus.Event.OnEventBegin(eventId, self.playerCam);
				
				-- Enable player controls.
				sm.UserData:EnablePlayerControls(1);
				
				-- Move the cutscene camera back to its original position in preparation for next
				-- teleportation.
				TransformBus.Event.SetWorldTM(self.cutsceneCam, self.cutsceneCamOriginalTM);
				
				-- Tell the teleporter to clear the teleport-specific data (such as the payload).
				sm.UserData:TeleportComplete();
				
				--Debug.Log("Player: " .. tostring(TransformBus.Event.GetWorldTM(self.playerCam):GetTranslation()));
				--Debug.Log("Cutscene: " .. tostring(TransformBus.Event.GetWorldTM(self.cutsceneCam):GetTranslation()));
			end,
			
			OnUpdate = function(self, sm, deltaTime)
				self.timeRemaining = self.timeRemaining - deltaTime;
				
				local newPos = nil;
				local newLookAt = nil;
				local delta = utilities.Clamp(self.timeRemaining / self.duration, 0.0, 1.0);
				delta = math.abs((delta - 1.0));		-- this is done to have it START at 0
				local dir = self.playerCamOriginalTM:GetTranslation() - self.cutsceneCamOriginalTM:GetTranslation();
				local dist = dir:GetLength() * delta;
				newPos = self.cutsceneCamOriginalTM:GetTranslation() + (dir:GetNormalized() * dist);
				if (self.focusPlayer) then
					dir = self.playerCamLookAt - self.cutsceneCamLookAt;
					dist = dir:GetLength() * delta;
					newLookAt = self.cutsceneCamLookAt + (dir:GetNormalized() * dist);
				else
					local newForward = Vector3.Slerp(self.cutsceneCamForward, self.playerCamForward, delta);
					newLookAt = newPos + newForward;
				end
				
				local newTm = MathUtils.CreateLookAt(newPos, newLookAt, AxisType.YPositive);
				TransformBus.Event.SetWorldTM(self.cutsceneCam, newTm);
			end,
			
			Transitions =
			{
				Idle =
				{
					Evaluate = function(state, sm)
						return state.timeRemaining <= 0.0;
					end,
				},
			},
		},
	},
}

function teleporter:OnActivate()
	
	self.startEventId = GameplayNotificationId(self.entityId, self.Properties.States.Idle.StartEvent, "float");
	self.startHandler = GameplayNotificationBus.Connect(self, self.startEventId);
	self.stopEventId = GameplayNotificationId(self.entityId, self.Properties.States.Idle.StopEvent, "float");
	self.stopHandler = GameplayNotificationBus.Connect(self, self.stopEventId);
	
	self.showEventId = GameplayNotificationId(EntityId(), self.Properties.FadeEvents.Show, "float");
	self.hideEventId = GameplayNotificationId(EntityId(), self.Properties.FadeEvents.Hide, "float");
	self.durationEventId = GameplayNotificationId(EntityId(), self.Properties.FadeEvents.Duration, "float");
	
	self.StateMachine = {}
	setmetatable(self.StateMachine, StateMachine);
	self.StateMachine:Start("Teleporter", self.entityId, self, self.States, false, "Idle", self.Properties.States.DebugStateMachine);
	self.StateMachine:Stop();
	self.payload = nil;
	self.teleportee = nil;
	
end

function teleporter:OnDeactivate()
	
	if (self.StateMachine ~= nil) then
		self.StateMachine:Stop();
		self.StateMachine = nil;
	end
	if (self.stopHandler ~= nil) then
		self.stopHandler:Disconnect();
		self.stopHandler = nil;
	end
	if (self.startHandler ~= nil) then
		self.startHandler:Disconnect();
		self.startHandler = nil;
	end
	
end

function teleporter:ObjectHasMoved(entity, prevPos)
	local pos = TransformBus.Event.GetWorldTM(entity):GetTranslation();
	local dist = (pos - prevPos):GetLengthSq();
	return dist > 0.01;
end

function teleporter:TeleportComplete()
	self.payload = nil;
end

function teleporter:TeleportObjectAway()
	local teleportEvent = GameplayNotificationId(self.teleportee, "TeleportTo", "float");
	GameplayNotificationBus.Event.OnEventBegin(teleportEvent, TransformBus.Event.GetWorldTM(self.Properties.Destination.Location):GetTranslation());
	
	-- Set the player to be invisible (so we don't initially see them when we start looking at the
	-- destination).
	local visEvent = GameplayNotificationId(self.teleportee, "SetVisible", "float");
	GameplayNotificationBus.Event.OnEventBegin(visEvent, false);
end

function teleporter:TeleportObjectIn()
	-- Set the player to be visible.
	local visEvent = GameplayNotificationId(self.teleportee, "SetVisible", "float");
	GameplayNotificationBus.Event.OnEventBegin(visEvent, true);
end

function teleporter:EnablePlayerControls(enable)
	local eventName = "EnableControlsEvent";
	self:SetControls("PlayerCharacter", eventName, enable);
	self:SetControls("PlayerCamera", eventName, enable);
end

function teleporter:SetControls(tag, event, enabled)
	local entity = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32(tag));
	local controlsEventId = GameplayNotificationId(entity, event, "float");
	GameplayNotificationBus.Event.OnEventBegin(controlsEventId, enabled);		-- 0.0 to disable
end

function teleporter:DuplicateCameraProps(srcCam, destCam)
	local farClip	= CameraRequestBus.Event.GetFarClipDistance(srcCam);
	local nearClip	= CameraRequestBus.Event.GetNearClipDistance(srcCam);
	local fov		= CameraRequestBus.Event.GetFov(srcCam);
	CameraRequestBus.Event.SetFarClipDistance(destCam, farClip);
	CameraRequestBus.Event.SetNearClipDistance(destCam, nearClip);
	CameraRequestBus.Event.SetFovDegrees(destCam, fov);
end

function teleporter:OnEventBegin(value)
	local busId = GameplayNotificationBus.GetCurrentBusId();
	if (busId == self.startEventId) then
		if (self.payload == nil) then
			self.payload = value;
			self.StateMachine:Resume();
		end
	elseif (busId == self.stopEventId) then
		self.payload = nil;
	end
end

return teleporter;