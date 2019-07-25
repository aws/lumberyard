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

local StateMachine = require "scripts/common/StateMachine"
local utilities = require "scripts/common/utilities"
local UIDiegeticInfoConsole =
{
	Properties =
	{
		TitleText = { default = "Cave Terminal Title", description = "The title text on the screen" },
		BodyText1 = { default = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed quis semper enim. Aliquam libero enim, convallis id convallis ac, tempor et tortor. Quisque aliquam, nisl quis rutrum aliquet, orci nisl tempor diam, vel aliquam ex augue a mauris. Vestibulum rutrum tincidunt bibendum. Phasellus mi ligula, tincidunt non tellus sed, semper pellentesque justo. Fusce tincidunt pulvinar convallis. Suspendisse mollis pellentesque convallis. Suspendisse nec imperdiet tortor. Cras ac tincidunt massa, eu convallis ex. Proin ut nisl gravida magna sollicitudin mattis nec eget augue. Nulla mollis pretium efficitur. Curabitur placerat placerat fermentum. Sed sapien magna, congue et auctor in, lacinia vitae eros.", description = "The body text on the screen" },
		AudioDescription1 = { default = "", description = "The sound event to fire for the voice description, will wait untill this finishes before close is allowed." },
		AudioStop = { default = "Stop_Vox_ShipAI_StopVOXCave", description = "The sound event to fire for stopping the VO." },
		ActionID = { default = 0, description = "The action ID for the button (from StarterGameUIActionType), a non \"invalid\" means show." },
		ActionDescription = { default = "TheAction", description = "The name for the action of the action button." },
		
		MessagesIn = {
			ShowMenu = { default = "ShowDiegeticMenu", description = "The event used to enable/disable the crosshair (0 is just data, 1 is for choices)" },
			MenuContinue = { default = "Continue", description = "The event used to enable/disable the crosshair (0 is just data, 1 is for choices)" },
			SetParent = { default = "UIAssetRefLinkParent", description = "The event to send to the screen with the parent / loader as the param." },
			SetInteractPosition = { default = "SetInteractPosition", description = "The event to set the interact position param for positioning the player when interacting." },
			SetInteractCamera = { default = "SetInteractCamera", description = "The event to set the interact Camera param for positioning the camera when interacting." },
			SoundFinished = { default = "DroneSpeakingFinished", description = "This event is broadcast when the drone finishes talking; the payload is the audio event." },		
			SetTitleText = { default = "SetTitleText", description = "This event to change the text title." },		
			SetBodyText = { default = "SetBodyText", description = "This event to change the text Body, page number appended to the end of the message name e.g. =\"SetBodyText1\", values from 1 to 6." },		
			SetAudioDescription = { default = "SetAudioDescription", description = "This event to change the audio, page number appended to the end of the message name e.g. =\"SetAudioDescription1\", values from 1 to 6." },		
			SetActionID = { default = "SetActionID", description = "This is the event id taken from the \"StarterGameUIActionType\" enum, 0 for hidden / no action. e.g. 6 for \"ActionType_Confirm\"" },		
			SetActionDescription = { default = "SetActionDescription", description = "This event to change the audio, page number appended to the end of the message name e.g. =\"SetAudioDescription1\", values from 1 to 6." },		
		},
		MessagesOut = {
			MenuFinished = { default = "MenuComplete", description = "The event sent when the menu is closed" },
			ControlCharacter = { default = "ControlCharacter", description = "sent when we grab control of the player for the menu" },
			ReleaseCharacter = { default = "ReleaseCharacter", description = "sent when we grab control of the player for the menu" },
			EnableControlsEvent = { default = "EnableControlsEvent", description = "This is the message we send to disable / enable player control" },
			ParticleEffectEnableEvent = { default = "ParticleEffectEnableEvent", description = "This is the message we send to disable / enable our particle effect" },
			SetButtonStateEvent = { default = "SetState", description = "This is the message we send to change the button state" },
			SetButtonActionEvent = { default = "SetAction", description = "This is the message we send to change the button state" },
			SoundStart = { default = "DroneSpeak", description = "The payload should be the voice line to say." },
		},
		
		ScreenAnimation = {
			FadeTime = { default = 0.25, description = "The time taken to fade up and down the UI" },
		},
		
		DebugStateMachine =  { default = false, description = "display debugging onif0ormation for the state machine." },
		
		Sounds = {
			Open = { default = "Play_HUD_CommsArray_InMenu", description = "The sound event to fire when the menu opens." },
			Close = { default = "Stop_HUD_CommsArray_InMenu", description = "The sound event to fire when the menu opens." },
			
		},
		
		ScreenHookup = { 
			-- note, the assumption here is that all the screens are taken from a common master base, so that the IDs will not change on different versions
			ContentFaderID = { default = EntityId(), description = "All non generic items" },
			ButtonNew = { default = EntityId(), description = "The the base button" },
			Title = { default = EntityId(), description = "The title text object" },
			Body = { default = EntityId(), description = "The body text object" },
			Action = { default = EntityId(), description = "The action object fader" },
			ActionText = { default = EntityId(), description = "The action text object" },
			ActionButton = { default = EntityId(), description = "The action Button object" },
		},
	},
	
	States = 
    {
        Closed = 
        {      
            OnEnter = function(self, sm)
				-- set all the things to be hidden
				self.WantsToShow = false;
				sm.UserData:EnableHud(false);
				--ParticleComponentRequestBus.Event.Enable(sm.UserData.Properties.ParticleEffect, false);

				if(sm.UserData.parentEntity ~= nil) then
					GameplayNotificationBus.Event.OnEventBegin( GameplayNotificationId(sm.UserData.parentEntity, sm.UserData.Properties.MessagesOut.ParticleEffectEnableEvent, "float"), false);
					
					if(sm.UserData.modelOriginalLocalT ~= nil) then
						TransformBus.Event.SetLocalTM(sm.UserData.parentEntity, sm.UserData.modelOriginalLocalT);
					end
				end
				
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.buttonStateSetEventId, "Inactive");
           end,
            OnExit = function(self, sm)				
				-- prescale this down to avoid a full flash
				if (sm.UserData.modelOriginalLocalT == nil) then
					sm.UserData.modelOriginalLocalT = TransformBus.Event.GetLocalTM(sm.UserData.parentEntity);
				end
				local trans = sm.UserData.modelOriginalLocalT * Transform.CreateScale(Vector3(1, 1, 0));
				TransformBus.Event.SetLocalTM(sm.UserData.parentEntity, trans);

				sm.UserData:EnableHud(true);
				sm.UserData.WantsToShow = false;
				
				--ParticleComponentRequestBus.Event.Enable(sm.UserData.Properties.ParticleEffect, true);
				GameplayNotificationBus.Event.OnEventBegin( GameplayNotificationId(sm.UserData.parentEntity, sm.UserData.Properties.MessagesOut.ParticleEffectEnableEvent, "float"), true);
			end,            
            OnUpdate = function(self, sm, deltaTime)
            end,
            Transitions =
            {
				Opening = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.WantsToShow;
                    end	
				}
            },
        },
        Opening = 
        {      
            OnEnter = function(self, sm)
				self.aimTimer = sm.UserData.Properties.ScreenAnimation.FadeTime;
				
				--Debug.Log("UIDiegeticInfoConsole:Opening fading up over " .. tostring(self.aimTimer));
				UiFaderBus.Event.Fade(sm.UserData.entityId, 1, 1.0 / self.aimTimer);
				
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.controlCharacterEventId, true);
				
				-- send event to player to begin interaction (change camera, move character, animate)
				local eventArgs = InteractParams();
				
				-- get the entities via their tags
				eventArgs.positionEntity = sm.UserData.InteractPositionEntity;
				eventArgs.cameraEntity = sm.UserData.InteractCameraEntity;
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.startInteractEventId, eventArgs);
				
				if(sm.UserData.Properties.Sounds.Open ~= "") then
					AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.parentEntity, sm.UserData.Properties.Sounds.Open);
				end
				
				--sm.UserData.CurrentPage = 1;
				sm.UserData:SetPage(1);
				
				UiTextBus.Event.SetText(sm.UserData.Properties.ScreenHookup.Body, sm.UserData.BodyTextCurrent);
				
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.buttonStateSetEventId, "Inactive");
            end,
            OnExit = function(self, sm)
            end,            
            OnUpdate = function(self, sm, deltaTime)
				self.aimTimer = self.aimTimer - deltaTime;
				if(self.aimTimer < 0) then
					self.aimTimer = 0;
				end
				
				local scaleValue = 1 - (self.aimTimer / sm.UserData.Properties.ScreenAnimation.FadeTime);
				--Debug.Log("UIDiegeticInfoConsole:Opening " .. tostring(scaleValue));
				local trans = sm.UserData.modelOriginalLocalT * Transform.CreateScale(Vector3(1, 1, scaleValue));
				TransformBus.Event.SetLocalTM(sm.UserData.parentEntity, trans);
            end,           
            Transitions =
            {
				Speaking = 
				{
                    Evaluate = function(state, sm)
						return state.aimTimer <= 0;
                    end	
				}
            },
        },
		Speaking = 
		{
			OnEnter = function(self, sm)			
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.buttonStateSetEventId, "Active");			
				sm.UserData.SoundFinished = false;
				if(sm.UserData.AudioDescriptionCurrent ~= "") then
					-- NOTE this might need to be different, need to consult ed
					GameplayNotificationBus.Event.OnEventBegin( GameplayNotificationId(EntityId(), sm.UserData.Properties.MessagesOut.SoundStart, "float"), sm.UserData.AudioDescriptionCurrent);
					sm.UserData.SoundStarted = true;
				end
			end,
            OnExit = function(self, sm)
				sm.UserData.SoundStarted = false;
				sm.UserData.SoundFinished = false;
			end,            
            OnUpdate = function(self, sm, deltaTime)
			end,
            Transitions =
            {
				AwaitingClose = 
				{
                    Evaluate = function(state, sm)
						return ((sm.UserData.SoundFinished == true) or (sm.UserData.AudioDescriptionCurrent == ""));
					end	
				}
            },
		},
		AwaitingClose = 
        {      
            OnEnter = function(self, sm)
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.buttonStateSetEventId, "Active");
				sm.UserData.WaitingForContinue = true;
            end,
            OnExit = function(self, sm)
				sm.UserData.WaitingForContinue = false;	
				sm.UserData.ContinuePressed = false;
			end,            
            OnUpdate = function(self, sm, deltaTime)
				
            end,
            Transitions =
            {
				Closing = 
				{
                    Evaluate = function(state, sm)
						return (sm.UserData.ContinuePressed == true) and (sm.UserData.CurrentPage == 0);
	                end	
				},
 				PageTransition = 
				{
                    Evaluate = function(state, sm)
						return (sm.UserData.ContinuePressed == true) and (sm.UserData.CurrentPage ~= 0);
	                end	
				},
           },
        },
        PageTransition = 
        {
            OnEnter = function(self, sm)
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.buttonStateSetEventId, "Pressed");
			
				self.aimTimer = sm.UserData.Properties.ScreenAnimation.FadeTime;
				
				--Debug.Log("UIDiegeticInfoConsole:Opening fading up over " .. tostring(self.aimTimer));
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Body, 0, 1.0 / self.aimTimer);
				--UiTextBus.Event.SetText(sm.UserData.Properties.ScreenHookup.Body, sm.UserData.BodyTextCurrent);
				
				sm.UserData:SetPage(sm.UserData.CurrentPage + 1);
				
				if (sm.UserData.CurrentPage ~= 0) then
					GameplayNotificationBus.Event.OnEventBegin(sm.UserData.buttonStateSetEventId, "Inactive");
				end
				sm.UserData.ContinuePressed = false;
            end,
            OnExit = function(self, sm)
				UiTextBus.Event.SetText(sm.UserData.Properties.ScreenHookup.Body, sm.UserData.BodyTextCurrent);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Body, 1, 1.0 / self.aimTimer);
            end,
            OnUpdate = function(self, sm, deltaTime)
				self.aimTimer = self.aimTimer - deltaTime;
				if(self.aimTimer < 0) then
					self.aimTimer = 0;
				end
            end,
            Transitions =
            {
				Speaking = 
				{
                    Evaluate = function(state, sm)
						return (state.aimTimer <= 0) and (sm.UserData.CurrentPage ~= 0);
                    end	
				},
				Closing = 
				{
                    Evaluate = function(state, sm)
						return (sm.UserData.CurrentPage == 0);
                    end	
				}
            },
        },		
		Closing = 
        {      
            OnEnter = function(self, sm)
				self.aimTimer = sm.UserData.Properties.ScreenAnimation.FadeTime;
				
				UiFaderBus.Event.Fade(sm.UserData.entityId, 0, 1.0 / self.aimTimer);
				
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.buttonStateSetEventId, "Pressed");
			end,
            OnExit = function(self, sm)
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.releaseCharacterEventId, true);
				
				-- send event to player to end interaction (enable controls, select player camera)
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.endInteractEventId, nil);
				GameplayNotificationBus.Event.OnEventBegin( GameplayNotificationId(sm.UserData.parentEntity, sm.UserData.Properties.MessagesOut.MenuFinished, "float"), true);
				sm.UserData.WantsToShow = false;
				
				if(sm.UserData.Properties.Sounds.Close ~= "") then
					AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.parentEntity, sm.UserData.Properties.Sounds.Close);
				end
				
				self.WantsToShow = false;
            end,
            OnUpdate = function(self, sm, deltaTime)
				self.aimTimer = self.aimTimer - deltaTime;
				if (self.aimTimer < 0) then
					self.aimTimer = 0;
				end
				
				local scaleValue = (self.aimTimer / sm.UserData.Properties.ScreenAnimation.FadeTime);
				--Debug.Log("UIDiegeticInfoConsole:ClosingData " .. tostring(scaleValue));
				local trans = sm.UserData.modelOriginalLocalT * Transform.CreateScale(Vector3(1, 1, scaleValue));
				TransformBus.Event.SetLocalTM(sm.UserData.parentEntity, trans);
            end,
            Transitions =
            {
				Closed = 
				{
                    Evaluate = function(state, sm)
						return (state.aimTimer <= 0);
	                end	
				},
            },
        },
	},
}

function UIDiegeticInfoConsole:OnActivate()
	--Debug.Log("UIDiegeticInfoConsole:OnActivate() .. " .. tostring(self.entityId));
	
	-- Listen for enable/disable events.
	self.menuContinueEventId = GameplayNotificationId(self.entityId, self.Properties.MessagesIn.MenuContinue, "float");
	self.menuContinueHandler = GameplayNotificationBus.Connect(self, self.menuContinueEventId);
	self.SoundFinishedEventId = GameplayNotificationId(EntityId(), self.Properties.MessagesIn.SoundFinished, "float");
	self.SoundFinishedHandler = GameplayNotificationBus.Connect(self, self.SoundFinishedEventId);
	
	self.controlCharacterEventId = GameplayNotificationId(self.entityId, self.Properties.MessagesOut.ControlCharacter, "float");
	self.releaseCharacterEventId = GameplayNotificationId(self.entityId, self.Properties.MessagesOut.ReleaseCharacter, "float");
	self.buttonStateSetEventId = GameplayNotificationId(self.Properties.ScreenHookup.ButtonNew, self.Properties.MessagesOut.SetButtonStateEvent, "float");
	self.buttonActionSetEventId = GameplayNotificationId(self.Properties.ScreenHookup.ActionButton, self.Properties.MessagesOut.SetButtonActionEvent, "float");
					
	self.InteractPositionEntity = nil;
	self.InteractCameraEntity = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCamera"));
	
	self.WantsToShow = false;
	self.menuShowing = false;
	self.buttonPressed = false;
		
	-- self.showMenuEventId = GameplayNotificationId(self.parentEntity, self.Properties.MessagesIn.ShowMenu, "float");
	-- self.showMenuHandler = GameplayNotificationBus.Connect(self, self.showMenuEventId);
	-- self.modelOriginalLocalT = TransformBus.Event.GetLocalTM(self.parentEntity);
	
	-- state machine hookup
    self.StateMachine = {}
    setmetatable(self.StateMachine, StateMachine);
    self.StateMachine:Start("UIDigeticMenu", self.entityId, self, self.States, false, "Closed", self.Properties.DebugStateMachine)
	
	--Debug.Log("UIDiegeticInfoConsole ID: " .. tostring(self.entityId));
	
	self.tickHandler = TickBus.Connect(self);
end

function UIDiegeticInfoConsole:OnDeactivate()
	--Debug.Log("UIDiegeticInfoConsole:OnDeactivate() " .. tostring(self.entityId));
	if(self.showMenuHandler ~= nil) then
		self.showMenuHandler:Disconnect();
		self.showMenuHandler = nil;
	end
	if (self.SetInteractPositionHandler ~= nil) then
		self.SetInteractPositionHandler:Disconnect();
		self.SetInteractPositionHandler = nil;
	end
	if (self.SetInteractCameraHandler ~= nil) then
		self.SetInteractCameraHandler:Disconnect();
		self.SetInteractCameraHandler = nil;
	end
	if(self.SetTitleTextHandler ~= nil) then
		self.SetTitleTextHandler:Disconnect();
		self.SetTitleTextHandler = nil;
	end
	if(self.SetBodyTextHandler1 ~= nil) then
		self.SetBodyTextHandler1:Disconnect();
		self.SetBodyTextHandler1 = nil;
	end
	if(self.SetBodyTextHandler2 ~= nil) then
		self.SetBodyTextHandler2:Disconnect();
		self.SetBodyTextHandler2 = nil;
	end
	if(self.SetBodyTextHandler3 ~= nil) then
		self.SetBodyTextHandler3:Disconnect();
		self.SetBodyTextHandler3 = nil;
	end
	if(self.SetBodyTextHandler4 ~= nil) then
		self.SetBodyTextHandler4:Disconnect();
		self.SetBodyTextHandler4 = nil;
	end
	if(self.SetBodyTextHandler5 ~= nil) then
		self.SetBodyTextHandler5:Disconnect();
		self.SetBodyTextHandler5 = nil;
	end
	if(self.SetBodyTextHandler6 ~= nil) then
		self.SetBodyTextHandler6:Disconnect();
		self.SetBodyTextHandler6 = nil;
	end
	if(self.SetAudioDescriptionHandler1 ~= nil) then
		self.SetAudioDescriptionHandler1:Disconnect();
		self.SetAudioDescriptionHandler1 = nil;
	end
	if(self.SetAudioDescriptionHandler2 ~= nil) then
		self.SetAudioDescriptionHandler2:Disconnect();
		self.SetAudioDescriptionHandler2= nil;
	end
	if(self.SetAudioDescriptionHandler3 ~= nil) then
		self.SetAudioDescriptionHandler3:Disconnect();
		self.SetAudioDescriptionHandler3 = nil;
	end
	if(self.SetAudioDescriptionHandler4 ~= nil) then
		self.SetAudioDescriptionHandler4:Disconnect();
		self.SetAudioDescriptionHandler4 = nil;
	end
	if(self.SetAudioDescriptionHandler5 ~= nil) then
		self.SetAudioDescriptionHandler5:Disconnect();
		self.SetAudioDescriptionHandler5 = nil;
	end
	if(self.SetAudioDescriptionHandler6 ~= nil) then
		self.SetAudioDescriptionHandler6:Disconnect();
		self.SetAudioDescriptionHandler6 = nil;
	end
	if(self.SetActionIDHandler ~= nil) then
		self.SetActionIDHandler:Disconnect();
		self.SetActionIDHandler = nil;
	end
	if(self.SetActionDescriptionHandler ~= nil) then
		self.SetActionDescriptionHandler:Disconnect();
		self.SetActionDescriptionHandler = nil;
	end

	
	self.menuContinueHandler:Disconnect();
	self.menuContinueHandler = nil;
	self.SoundFinishedHandler:Disconnect();
	self.SoundFinishedHandler = nil;
	
	self.StateMachine:Stop();
	
	self.tickHandler:Disconnect();
	self.tickHandler = nil;

	self:EnableHud(false);
end

function UIDiegeticInfoConsole:OnTick(deltaTime, timePoint)
	if(self.firsttickHandeled ~= true) then
		self.canvas = UiElementBus.Event.GetCanvas(self.entityId);
		--Debug.Log("UIDiegeticInfoConsole:OnTick() first frame : " .. tostring(self.canvas));

		self.SetParentEventId = GameplayNotificationId(self.canvas, self.Properties.MessagesIn.SetParent, "float");
		self.SetParentHandler = GameplayNotificationBus.Connect(self, self.SetParentEventId);

		self.playerID = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"));
		self.startInteractEventId = GameplayNotificationId(self.playerID, "StartInteract", "float");
		self.endInteractEventId = GameplayNotificationId(self.playerID, "EndInteract", "float");
		
		-- GameplayNotificationBus.Event.OnEventBegin(self.buttonActionSetEventId, self.Properties.ActionID);
		-- UiTextBus.Event.SetText(self.Properties.ScreenHookup.SetActionDescription, self.Properties.ActionID);	
		-- local visibleAction = (self.Properties.ActionID ~= StarterGameUIUtility.ActionType_Invalid);
		-- local fadeValue = 0;
		-- if (visibleAction) then
			-- fadeValue = 1;
		-- end
		-- UiFaderBus.Event.Fade(self.Properties.ScreenHookup.Action, 0, fadeValue);
		-- UiTextBus.Event.SetText(self.Properties.ScreenHookup.SetActionDescription, self.Properties.ActionDescription);	
		
		self:SetActionButton(self.Properties.ActionID, self.Properties.ActionDescription);
		
		self.firsttickHandeled = true;
	end
end

function UIDiegeticInfoConsole:EnableHud(enabled)
	if (enabled == self.enabled) then
		return;
	end

	if (self.parentEntity ~= nil) then
		MeshComponentRequestBus.Event.SetVisibility(self.parentEntity, enabled);
	end
	
	self.enabled = enabled;
end

function UIDiegeticInfoConsole:OnEventBegin(value)
	--Debug.Log("UIDiegeticInfoConsole:OnEventBegin() " .. tostring(self.entityId) .. " : " .. tostring(value));
	if (GameplayNotificationBus.GetCurrentBusId() == self.showMenuEventId) then
		if(self.InteractPositionEntity == nil or self.InteractCameraEntity == nil) then
			--Debug.Log("UIDiegeticInfoConsole:OnEventBegin() " .. tostring(self.canvas) .. " tried to show but params were " .. tostring(self.InteractPositionEntity) .. " " .. tostring(self.InteractCameraEntity));
		else
			--Debug.Log("UIDiegeticInfoConsole:OnEventBegin() " .. tostring(self.canvas) .. " show, params were " .. tostring(self.InteractPositionEntity) .. " " .. tostring(self.InteractCameraEntity));
			self.WantsToShow = true;
		end
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.menuContinueEventId) then
		--Debug.Log("UIDiegeticInfoConsole:OnEventBegin() set contionue pressed" .. tostring(self.canvas) .. " : " .. tostring(value));
		if(self.SoundStarted and (not self.SoundFinished)) then
			GameplayNotificationBus.Event.OnEventBegin( GameplayNotificationId(EntityId(), self.Properties.MessagesOut.SoundStart, "float"), self.Properties.AudioStop);
			self.ContinuePressed = true;
		end
		if (self.WaitingForContinue) then
			self.ContinuePressed = true;
		end
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetInteractPositionEventId) then
		--Debug.Log("UIDiegeticInfoConsole:OnEventBegin() set InteractPositionEntity" .. tostring(self.canvas) .. " : " .. tostring(value));
		self.InteractPositionEntity = value;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetInteractCameraEventId) then
		--Debug.Log("UIDiegeticInfoConsole:OnEventBegin() set InteractCameraEntity" .. tostring(self.canvas) .. " : " .. tostring(value));
		self.InteractCameraEntity = value;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetParentEventId) then
		-- clear the handlers
		if(self.showMenuHandler ~= nil) then
			self.showMenuHandler:Disconnect();
			self.showMenuHandler = nil;
		end
		if (self.SetInteractPositionHandler ~= nil) then
			self.SetInteractPositionHandler:Disconnect();
			self.SetInteractPositionHandler = nil;
		end
		if (self.SetInteractCameraHandler ~= nil) then
			self.SetInteractCameraHandler:Disconnect();
			self.SetInteractCameraHandler = nil;
		end
		if(self.SetTitleTextHandler ~= nil) then
			self.SetTitleTextHandler:Disconnect();
			self.SetTitleTextHandler = nil;
		end
		if(self.SetBodyTextHandler1 ~= nil) then
			self.SetBodyTextHandler1:Disconnect();
			self.SetBodyTextHandler1 = nil;
		end
		if(self.SetBodyTextHandler2 ~= nil) then
			self.SetBodyTextHandler2:Disconnect();
			self.SetBodyTextHandler2 = nil;
		end
		if(self.SetBodyTextHandler3 ~= nil) then
			self.SetBodyTextHandler3:Disconnect();
			self.SetBodyTextHandler3 = nil;
		end
		if(self.SetBodyTextHandler4 ~= nil) then
			self.SetBodyTextHandler4:Disconnect();
			self.SetBodyTextHandler4 = nil;
		end
		if(self.SetBodyTextHandler5 ~= nil) then
			self.SetBodyTextHandler5:Disconnect();
			self.SetBodyTextHandler5 = nil;
		end
		if(self.SetBodyTextHandler6 ~= nil) then
			self.SetBodyTextHandler6:Disconnect();
			self.SetBodyTextHandler6 = nil;
		end
		if(self.SetAudioDescriptionHandler1 ~= nil) then
			self.SetAudioDescriptionHandler1:Disconnect();
			self.SetAudioDescriptionHandler1 = nil;
		end
		if(self.SetAudioDescriptionHandler2 ~= nil) then
			self.SetAudioDescriptionHandler2:Disconnect();
			self.SetAudioDescriptionHandler2= nil;
		end
		if(self.SetAudioDescriptionHandler3 ~= nil) then
			self.SetAudioDescriptionHandler3:Disconnect();
			self.SetAudioDescriptionHandler3 = nil;
		end
		if(self.SetAudioDescriptionHandler4 ~= nil) then
			self.SetAudioDescriptionHandler4:Disconnect();
			self.SetAudioDescriptionHandler4 = nil;
		end
		if(self.SetAudioDescriptionHandler5 ~= nil) then
			self.SetAudioDescriptionHandler5:Disconnect();
			self.SetAudioDescriptionHandler5 = nil;
		end
		if(self.SetAudioDescriptionHandler6 ~= nil) then
			self.SetAudioDescriptionHandler6:Disconnect();
			self.SetAudioDescriptionHandler6 = nil;
		end
		if(self.SetActionIDHandler ~= nil) then
			self.SetActionIDHandler:Disconnect();
			self.SetActionIDHandler = nil;
		end
		if(self.SetActionDescriptionHandler ~= nil) then
			self.SetActionDescriptionHandler:Disconnect();
			self.SetActionDescriptionHandler = nil;
		end
		
		--Debug.Log("UIDiegeticInfoConsole:OnEventBegin() set parent" .. tostring(self.canvas) .. " : " .. tostring(value) .. " : " .. tostring(self.entityId));
		self.parentEntity = value;
		
		-- re-setup the handlers
		self.showMenuEventId = GameplayNotificationId(self.parentEntity, self.Properties.MessagesIn.ShowMenu, "float");
		self.showMenuHandler = GameplayNotificationBus.Connect(self, self.showMenuEventId);
		self.modelOriginalLocalT = TransformBus.Event.GetLocalTM(self.parentEntity);	
		self.SetTitleTextEventId = GameplayNotificationId(self.parentEntity, self.Properties.MessagesIn.SetTitleText, "float");
		self.SetTitleTextHandler = GameplayNotificationBus.Connect(self, self.SetTitleTextEventId);
		
		self.SetBodyTextEventId1 = GameplayNotificationId(self.parentEntity, self.Properties.MessagesIn.SetBodyText .. "1", "float");
		self.SetBodyTextHandler1 = GameplayNotificationBus.Connect(self, self.SetBodyTextEventId1);
		self.SetBodyTextEventId2 = GameplayNotificationId(self.parentEntity, self.Properties.MessagesIn.SetBodyText .. "2", "float");
		self.SetBodyTextHandler2 = GameplayNotificationBus.Connect(self, self.SetBodyTextEventId2);
		self.SetBodyTextEventId3 = GameplayNotificationId(self.parentEntity, self.Properties.MessagesIn.SetBodyText .. "3", "float");
		self.SetBodyTextHandler3 = GameplayNotificationBus.Connect(self, self.SetBodyTextEventId3);
		self.SetBodyTextEventId4 = GameplayNotificationId(self.parentEntity, self.Properties.MessagesIn.SetBodyText .. "4", "float");
		self.SetBodyTextHandler4 = GameplayNotificationBus.Connect(self, self.SetBodyTextEventId4);
		self.SetBodyTextEventId5 = GameplayNotificationId(self.parentEntity, self.Properties.MessagesIn.SetBodyText .. "5", "float");
		self.SetBodyTextHandler5 = GameplayNotificationBus.Connect(self, self.SetBodyTextEventId5);
		self.SetBodyTextEventId6 = GameplayNotificationId(self.parentEntity, self.Properties.MessagesIn.SetBodyText .. "6", "float");
		self.SetBodyTextHandler6 = GameplayNotificationBus.Connect(self, self.SetBodyTextEventId6);
		
		self.SetAudioDescriptionEventId1 = GameplayNotificationId(self.parentEntity, self.Properties.MessagesIn.SetAudioDescription .. "1", "float");
		self.SetAudioDescriptionHandler1 = GameplayNotificationBus.Connect(self, self.SetAudioDescriptionEventId1);
		self.SetAudioDescriptionEventId2 = GameplayNotificationId(self.parentEntity, self.Properties.MessagesIn.SetAudioDescription .. "2", "float");
		self.SetAudioDescriptionHandler2 = GameplayNotificationBus.Connect(self, self.SetAudioDescriptionEventId2);
		self.SetAudioDescriptionEventId3 = GameplayNotificationId(self.parentEntity, self.Properties.MessagesIn.SetAudioDescription .. "3", "float");
		self.SetAudioDescriptionHandler3 = GameplayNotificationBus.Connect(self, self.SetAudioDescriptionEventId3);
		self.SetAudioDescriptionEventId4 = GameplayNotificationId(self.parentEntity, self.Properties.MessagesIn.SetAudioDescription .. "4", "float");
		self.SetAudioDescriptionHandler4 = GameplayNotificationBus.Connect(self, self.SetAudioDescriptionEventId4);
		self.SetAudioDescriptionEventId5 = GameplayNotificationId(self.parentEntity, self.Properties.MessagesIn.SetAudioDescription .. "5", "float");
		self.SetAudioDescriptionHandler5 = GameplayNotificationBus.Connect(self, self.SetAudioDescriptionEventId5);
		self.SetAudioDescriptionEventId6 = GameplayNotificationId(self.parentEntity, self.Properties.MessagesIn.SetAudioDescription .. "6", "float");
		self.SetAudioDescriptionHandler6 = GameplayNotificationBus.Connect(self, self.SetAudioDescriptionEventId6);
	
		self.SetActionIDEventId = GameplayNotificationId(self.parentEntity, self.Properties.MessagesIn.SetActionID, "float");
		self.SetActionIDHandler = GameplayNotificationBus.Connect(self, self.SetActionIDEventId);
		self.SetActionDescriptionEventId = GameplayNotificationId(self.parentEntity, self.Properties.MessagesIn.SetActionDescription, "float");
		self.SetActionDescriptionHandler = GameplayNotificationBus.Connect(self, self.SetActionDescriptionEventId);
		
		--Debug.Log("UIDiegeticInfoConsole:OnEventBegin() setup position event listener : " .. tostring(self.parentEntity) .. " : " .. tostring(self.Properties.MessagesIn.SetInteractPosition));
		self.SetInteractPositionEventId = GameplayNotificationId(self.parentEntity, self.Properties.MessagesIn.SetInteractPosition, "float");
		self.SetInteractPositionHandler = GameplayNotificationBus.Connect(self, self.SetInteractPositionEventId);
		self.SetInteractCameraEventId = GameplayNotificationId(self.parentEntity, self.Properties.MessagesIn.SetInteractCamera, "float");
		self.SetInteractCameraHandler = GameplayNotificationBus.Connect(self, self.SetInteractCameraEventId);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SoundFinishedEventId) then
		--Debug.Log("UIDiegeticInfoConsole:OnEventBegin() sSoundFinishedEventId : " .. tostring(self.AudioDescriptionCurrent) .. " : " .. tostring(value));
		if(value == self.AudioDescriptionCurrent) then
			self.SoundFinished = true;
		end
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetTitleTextEventId) then
		UiTextBus.Event.SetText(self.Properties.ScreenHookup.Title, value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetBodyTextEventId1) then
		self.Properties.BodyText1 = value;
		--UiTextBus.Event.SetText(self.Properties.ScreenHookup.Body1, value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetBodyTextEventId2) then
		self.Properties.BodyText2 = value;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetBodyTextEventId3) then
		self.Properties.BodyText3 = value;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetBodyTextEventId4) then
		self.Properties.BodyText4 = value;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetBodyTextEventId5) then
		self.Properties.BodyText5 = value;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetBodyTextEventId6) then
		self.Properties.BodyText6 = value;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetAudioDescriptionEventId1) then
		self.Properties.AudioDescription1 = value;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetAudioDescriptionEventId2) then
		self.Properties.AudioDescription2 = value;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetAudioDescriptionEventId3) then
		self.Properties.AudioDescription3 = value;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetAudioDescriptionEventId4) then
		self.Properties.AudioDescription4 = value;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetAudioDescriptionEventId5) then
		self.Properties.AudioDescription5 = value;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetAudioDescriptionEventId6) then
		self.Properties.AudioDescription6 = value;	
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetActionIDEventId) then		
		self:SetActionButton(value, nil);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetActionDescriptionEventId) then
		self:SetActionButton(nil, value);
	end
end

function UIDiegeticInfoConsole:SetActionButton(buttonID, Text)
	--Debug.Log("UIDiegeticInfoConsole:SetActionButton() : " .. tostring(self.parentEntity) .. " : " .. tostring(buttonID) .. " : " .. tostring(Text));
	if(buttonID ~= nil) then
		self.Properties.ActionID = buttonID;
		GameplayNotificationBus.Event.OnEventBegin(self.buttonActionSetEventId, buttonID);
		local visibleAction = (buttonID ~= StarterGameUIUtility.ActionType_Invalid);
		local fadeValue = 0;
		if(visibleAction) then
			fadeValue = 1;
		end
		UiFaderBus.Event.Fade(self.Properties.ScreenHookup.Action, fadeValue, 0);
	end
	if(Text ~= nil) then
		self.Properties.ActionDescription = Text;
		UiTextBus.Event.SetText(self.Properties.ScreenHookup.ActionText, Text);	
	end
end


function UIDiegeticInfoConsole:SetPage(pageNum)
	if(pageNum == 1) then
		if(self.Properties.BodyText1 ~= nil and self.Properties.BodyText1 ~= "") then
			self.BodyTextCurrent = self.Properties.BodyText1;
			self.AudioDescriptionCurrent = self.Properties.AudioDescription1;
		else
			pageNum = pageNum + 1;
		end
	end
	if (pageNum == 2) then
		if(self.Properties.BodyText2 ~= nil and self.Properties.BodyText2 ~= "") then
			self.BodyTextCurrent = self.Properties.BodyText2;
			self.AudioDescriptionCurrent = self.Properties.AudioDescription2;
		else
			pageNum = pageNum + 1;
		end
	end
	if (pageNum == 3) then
		if(self.Properties.BodyText3 ~= nil and self.Properties.BodyText3 ~= "") then
			self.BodyTextCurrent = self.Properties.BodyText3;
			self.AudioDescriptionCurrent = self.Properties.AudioDescription3;
		else
			pageNum = pageNum + 1;
		end
	end
	if (pageNum == 4) then
		if(self.Properties.BodyText4 ~= nil and self.Properties.BodyText4 ~= "") then
			self.BodyTextCurrent = self.Properties.BodyText4;
			self.AudioDescriptionCurrent = self.Properties.AudioDescription4;
		else
			pageNum = pageNum + 1;
		end
	end
	if (pageNum == 5) then
		if(self.Properties.BodyText5 ~= nil and self.Properties.BodyText5 ~= "") then
			self.BodyTextCurrent = self.Properties.BodyText5;
			self.AudioDescriptionCurrent = self.Properties.AudioDescription5;
		else
			pageNum = pageNum + 1;
		end
	end
	if (pageNum == 6) then
		if(self.Properties.BodyText6 ~= nil and self.Properties.BodyText6 ~= "") then
			self.BodyTextCurrent = self.Properties.BodyText6;
			self.AudioDescriptionCurrent = self.Properties.AudioDescription6;
		else
			pageNum = pageNum + 1;
		end
	end
	if (pageNum > 6) then
		pageNum = 0;
	end
	self.CurrentPage = pageNum;
	--Debug.Log("UIDiegeticInfoConsole:SetPage() " .. tostring(pageNum) .. " : " .. tostring(self.BodyTextCurrent) .. " : " .. tostring(self.AudioDescriptionCurrent));
end


return UIDiegeticInfoConsole;
