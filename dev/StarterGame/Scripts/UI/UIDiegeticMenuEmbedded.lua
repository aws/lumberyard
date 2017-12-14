local StateMachine = require "scripts/common/StateMachine"
local utilities = require "scripts/common/utilities"
local uidiegeticmenu =
{
	Properties =
	{
		MessagesIn = {
			ShowMenu = { default = "ShowDiegeticMenu", description = "The event used to enable/disable the crosshair (0 is just data, 1 is for choices)" },
			MenuRight = { default = "Right", description = "The from the controller to move right on the menu" },
			MenuLeft = { default = "Left", description = "The from the controller to move left on the menu" },
			MenuContinue = { default = "Continue", description = "The event used to enable/disable the crosshair (0 is just data, 1 is for choices)" },
			SetParent = { default = "UIAssetRefLinkParent", description = "The event to send to the screen with the parent / loader as the param." },
			SetInteractPosition = { default = "SetInteractPosition", description = "The event to set the interact position param for positioning the player when interacting." },
			SetInteractCamera = { default = "SetInteractCamera", description = "The event to set the interact Camera param for positioning the camera when interacting." },
		},
		MessagesOut = {
			MenuFinished = { default = "MenuComplete", description = "The event sent when the menu is closed" },
			OptionSelected = { default = "OptionSelected", description = "Represenst the selected option in payload, 0 for left, 1 for right" },
			ControlCharacter = { default = "ControlCharacter", description = "sent when we grab control of the player for the menu" },
			ReleaseCharacter = { default = "ReleaseCharacter", description = "sent when we grab control of the player for the menu" },
			EnableControlsEvent = { default = "EnableControlsEvent", description = "This is the message we send to disable / enable player control" },
			ParticleEffectEnableEvent = { default = "ParticleEffectEnableEvent", description = "This is the message we send to disable / enable our particle effect" },
			SetButtonStateEvent = { default = "SetState", description = "This is the message we send to change the button state" },
		},
		
		PlayerChanges = {
			PlayerTag = { default = "PlayerCharacter", description = "This tag of the plyer cahracter so that i can get access to him to alter his stats" },
			Health = {
				ValueIncrease = { default = 10, description = "This is the amount that i will increase if chosen" },
				ValueAbsoluteMax = { default = 100, description = "This is the amount that i will never increase over." },
				GetMaxMessage = { default = "HealthGetMaxEvent", description = "Send this message to the player to request his current health max" },
				GetMaxMessageCallback = { default = "HealthSendMaxEvent", description = "Send this message i get back from the player that specifies his current max" },
				SetMaxMessage = { default = "HealthSetMaxEvent", description = "Send this message to the player to set his health max" },
				SetValueMessage = { default = "HealthSetValueEvent", description = "Send this message to the player to set his health" },
			},
			Energy = {
				ValueIncrease = { default = 10, description = "This is the amount that i will increase if chosen" },
				ValueAbsoluteMax = { default = 100, description = "This is the amount that i will never increase over." },
				GetMaxMessage = { default = "EnergyGetMaxEvent", description = "Send this message to the player to request his current energy max" },
				GetMaxMessageCallback = { default = "EnergySendMaxEvent", description = "Send this message i get back from the player that specifies his current max" },
				SetMaxMessage = { default = "EnergySetMaxEvent", description = "Send this message to the player to set his energy max" },
				SetValueMessage = { default = "EnergySetValueEvent", description = "Send this message to the player to set his energy" },
			},
		},
		
		ScreenAnimation = {
			FadeTime = { default = 0.25, description = "The time taken to fade up and down the UI" },
			Layers = {
				WorkingTime = { default = 3, description = "The time taken to perform working" },
				SpinAmount = { default = 4, description = "The time amount of revolutions on the rotating elements" },
				WorkingToCompleteFadeTime = { default = 0.25, description = "The time spent transitioning" },
				CompleteTime = { default = 2, description = "The time taken to show complete" },
			},
			ChoiceScreen = {
				SideTransitionTime = { default = 0.25, description = "The time taken to transition sides" },
				ArrowPieceCycleTime = { default = 1, description = "The time for the upgrade arrow peice cycle" },
				ArrowPieceOffsetTime = { default = 0.25, description = "The time for the upgrade arrow peice offset" },
				ChosenHighlightTime = { default = 2, description = "The time to show in a highlight state before transitioning off" },
			},
		},
		
		DebugStateMachine =  { default = false, description = "display debugging onif0ormation for the state machine." },
		
		Sounds = {
			Open = { default = "Play_HUD_CommsArray_InMenu", description = "The sound event to fire when the menu opens." },
			Close = { default = "Stop_HUD_CommsArray_InMenu", description = "The sound event to fire when the menu opens." },
			DownloadStart = { default = "Play_HUD_commsarray_rotationdownload", description = "The sound event to start when we are downloading." },
			DownloadStop = { default = "Stop_HUD_commsarray_rotationdownload", description = "The sound event to stop when downloading is finished." },
			NoOptions = { default = "Play_HUD_commsarray_datatransfer_complete", description = "The sound event to fire when closing and no options on this screen." },
			ToOptions = { default = "Play_HUD_commsarray_openupgrades", description = "The sound event to fire when opening the options screen." },
			OptionChange = { default = "Play_HUD_commsarray_scroll", description = "The sound event to fire when changing options." },			
			SelectEnergy = { default = "Play_HUD_commsarray_select_upgradeenergy", description = "The sound event to fire when i highlight energy." },
			SelectHealth = { default = "Play_HUD_commsarray_select_upgradehealth", description = "The sound event to fire when i highlight health." },
			OptionClosing = { default = "Play_HUD_commsarray_select_upgradecomplete_Stop_upgrade", description = "The sound event to fire when i selected an option and are closing." },
		},
		
		ScreenHookup = { 
			-- note, the assumption here is that all the screens are taken from a common master base, so that the IDs will not change on different versions
			Layers = {
				--HealthBarID = { default = -1, description = "The the health Bar slider" },
				Layer1 = {
					WorkingFaderID = { default = EntityId(), description = "All working items" },
					CompleteFaderID = { default = EntityId(), description = "All complete items" },
					ContentFaderID = { default = EntityId(), description = "All non generic items" },
					WorkingRotatorID = { default = EntityId(), description = "The rotating element" },
					WorkingBarID = { default = EntityId(), description = "The bar element" },
					UpgradeFaderID = { default = EntityId(), description = "All choice items" },
					DataFaderID = { default = EntityId(), description = "All choice items" },
					LeftSelectedID = { default = EntityId(), description = "The selected items on the left" },
					RightSelectedID = { default = EntityId(), description = "The selected items on the right" },
					ButtonNew = { default = EntityId(), description = "The the base button" },
					LeftItems = {
						Arrow1ID = { default = EntityId(), description = "The selected items on the right" },
						Arrow2ID = { default = EntityId(), description = "The selected items on the right" },
						Arrow3ID = { default = EntityId(), description = "The selected items on the right" },
						Arrow4ID = { default = EntityId(), description = "The selected items on the right" },
						BarCurrentID = { default = EntityId(), description = "The selected items on the right" },
						BarMaxID = { default = EntityId(), description = "The the bar for proposed health" },
						SelectionConfirmedFlasherID = { default = EntityId(), description = "Then item for all of the highlights of the bar to flash them" },
					},
					RightItems = {
						Arrow1ID = { default = EntityId(), description = "The selected items on the right" },
						Arrow2ID = { default = EntityId(), description = "The selected items on the right" },
						Arrow3ID = { default = EntityId(), description = "The selected items on the right" },
						Arrow4ID = { default = EntityId(), description = "The selected items on the right" },
						BarCurrentID = { default = EntityId(), description = "The the bar for current energy" },
						BarMaxID = { default = EntityId(), description = "The the bar for proposed energy" },
						SelectionConfirmedFlasherID = { default = EntityId(), description = "Then item for all of the highlights of the bar to flash them" },
					},
				},
				Layer2 = {
					WorkingFaderID = { default = EntityId(), description = "All working items" },
					CompleteFaderID = { default = EntityId(), description = "All complete items" },
					ContentFaderID = { default = EntityId(), description = "All non generic items" },
					WorkingRotatorID = { default = EntityId(), description = "The rotating element" },
					UpgradeFaderID = { default = EntityId(), description = "All choice items" },
					DataFaderID = { default = EntityId(), description = "All choice items" },
				},
			},
		},
		
	},
	
	States = 
    {
        Closed = 
        {      
            OnEnter = function(self, sm)
				-- set all the things to be hidden
				sm.UserData:EnableHud(false);
				--ParticleComponentRequestBus.Event.Enable(sm.UserData.Properties.ParticleEffect, false);

				if(sm.UserData.parentEntity ~= nil) then
					GameplayNotificationBus.Event.OnEventBegin( GameplayNotificationId(sm.UserData.parentEntity, sm.UserData.Properties.MessagesOut.ParticleEffectEnableEvent, "float"), false);
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
				
				-- try ton ensure that the screen has the correct things visible
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.DataFaderID, 1, 0);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer2.DataFaderID, 1, 0);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.UpgradeFaderID, 0, 0);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer2.UpgradeFaderID, 0, 0);								
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.WorkingFaderID, 1, 0);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer2.WorkingFaderID, 1, 0);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.CompleteFaderID, 0, 0);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer2.CompleteFaderID, 0, 0);
				
				-- get the information as to the stats for the player to populate the menu
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.healthMaxGetEventId, sm.UserData.entityId);
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.energyMaxGetEventId, sm.UserData.entityId);
				
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.healthValueSetEventId, sm.UserData.playerHealthMax);
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.energyValueSetEventId, sm.UserData.playerEnergyMax);
				
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
				
				--Debug.Log("uidiegeticmenu:Opening fading up over " .. tostring(self.aimTimer));
				UiFaderBus.Event.Fade(sm.UserData.entityId, 1, self.aimTimer);
				
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
				--Debug.Log("uidiegeticmenu:Opening " .. tostring(scaleValue));
				local trans = sm.UserData.modelOriginalLocalT * Transform.CreateScale(Vector3(1, 1, scaleValue));
				TransformBus.Event.SetLocalTM(sm.UserData.parentEntity, trans);
            end,           
            Transitions =
            {
				DataWorking = 
				{
                    Evaluate = function(state, sm)
						return state.aimTimer <= 0;
                    end	
				}
            },
        },
		DataWorking = 
		{
			OnEnter = function(self, sm)
				self.aimTimer = sm.UserData.Properties.ScreenAnimation.Layers.WorkingTime;

				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.WorkingFaderID, 1, sm.UserData.Properties.ScreenAnimation.Layers.WorkingToCompleteFadeTime);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer2.WorkingFaderID, 1, sm.UserData.Properties.ScreenAnimation.Layers.WorkingToCompleteFadeTime);
				UiSliderBus.Event.SetValue(sm.UserData.Properties.ScreenHookup.Layers.Layer1.WorkingBarID, 0);
				
				if(sm.UserData.Properties.Sounds.DownloadStart ~= "") then
					AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.parentEntity, sm.UserData.Properties.Sounds.DownloadStart);
				end
			end,
            OnExit = function(self, sm)
				-- should always end on a full rotation, so zero it out
				UiTransformBus.Event.SetZRotation(sm.UserData.Properties.ScreenHookup.Layers.Layer1.WorkingRotatorID, 0);
				UiTransformBus.Event.SetZRotation(sm.UserData.Properties.ScreenHookup.Layers.Layer2.WorkingRotatorID, 0);
				
				
				--Debug.Log("uidiegeticmenu:DataWorking exit " .. tostring(sm.UserData.Properties.ScreenAnimation.Layers.WorkingToCompleteFadeTime));
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.WorkingFaderID, 0, sm.UserData.Properties.ScreenAnimation.Layers.WorkingToCompleteFadeTime);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer2.WorkingFaderID, 0, sm.UserData.Properties.ScreenAnimation.Layers.WorkingToCompleteFadeTime);
 
				if(sm.UserData.Properties.Sounds.DownloadStop ~= "") then
					AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.parentEntity, sm.UserData.Properties.Sounds.DownloadStop);
				end
			end,            
            OnUpdate = function(self, sm, deltaTime)
				self.aimTimer = self.aimTimer - deltaTime;
				if(self.aimTimer < 0) then
					self.aimTimer = 0;
				end
				
				-- note that the lear var goes from 1-0
				local linearVar = (self.aimTimer / sm.UserData.Properties.ScreenAnimation.Layers.WorkingTime);
				local rotateVar = (1 + Math.Cos(linearVar * math.pi)) * 0.5 * sm.UserData.Properties.ScreenAnimation.Layers.SpinAmount * 360;
			
				UiTransformBus.Event.SetZRotation(sm.UserData.Properties.ScreenHookup.Layers.Layer1.WorkingRotatorID, rotateVar);
				UiTransformBus.Event.SetZRotation(sm.UserData.Properties.ScreenHookup.Layers.Layer2.WorkingRotatorID, rotateVar);
				UiSliderBus.Event.SetValue(sm.UserData.Properties.ScreenHookup.Layers.Layer1.WorkingBarID, (1 - linearVar) * 100);	
			end,
            Transitions =
            {
				DataComplete = 
				{
                    Evaluate = function(state, sm)
						return state.aimTimer <= 0;
					end	
				}
            },
		},
		DataComplete = 
        {      
            OnEnter = function(self, sm)
				self.aimTimer = sm.UserData.Properties.ScreenAnimation.Layers.CompleteTime;
				--Debug.Log("uidiegeticmenu:DataComplete enter " .. tostring(sm.UserData.Properties.ScreenAnimation.Layers.WorkingToCompleteFadeTime));
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.CompleteFaderID, 1, 0);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer2.CompleteFaderID, 1, 0);
            end,
            OnExit = function(self, sm)
				if (sm.UserData.WantsToShowChoice == false) then
				else
				end           
			end,            
            OnUpdate = function(self, sm, deltaTime)
				self.aimTimer = self.aimTimer - deltaTime;
				if(self.aimTimer < 0) then
					self.aimTimer = 0;
				end
            end,
            Transitions =
            {
				ClosingData = 
				{
                    Evaluate = function(state, sm)
						return ((sm.UserData.WantsToShowChoice == false) and (state.aimTimer <= 0));
	                end	
				},
				DataHide = 
				{
                    Evaluate = function(state, sm)
						return ((sm.UserData.WantsToShowChoice == true) and (state.aimTimer <= 0));
                    end	
				}
            },
        },
		ClosingData = 
        {      
            OnEnter = function(self, sm)
				self.aimTimer = sm.UserData.Properties.ScreenAnimation.FadeTime;
				
				UiFaderBus.Event.Fade(sm.UserData.entityId, 0, self.aimTimer);
								
				if(sm.UserData.Properties.Sounds.NoOptions ~= "") then
					AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.parentEntity, sm.UserData.Properties.Sounds.NoOptions);
				end
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
            end,
            OnUpdate = function(self, sm, deltaTime)
				self.aimTimer = self.aimTimer - deltaTime;
				if (self.aimTimer < 0) then
					self.aimTimer = 0;
				end
				
				local scaleValue = (self.aimTimer / sm.UserData.Properties.ScreenAnimation.FadeTime);
				--Debug.Log("uidiegeticmenu:ClosingData " .. tostring(scaleValue));
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
		DataHide = 
        {      
            OnEnter = function(self, sm)
				self.aimTimer = sm.UserData.Properties.ScreenAnimation.FadeTime;
				
				if(sm.UserData.Properties.Sounds.ToOptions ~= "") then
					AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.parentEntity, sm.UserData.Properties.Sounds.ToOptions);
				end
            end,
            OnExit = function(self, sm)
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.DataFaderID, 0, 0);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer2.DataFaderID, 0, 0);
            end,            
            OnUpdate = function(self, sm, deltaTime)
				self.aimTimer = self.aimTimer - deltaTime;
				if(self.aimTimer < 0) then
					self.aimTimer = 0;
				end

				local scaleVar = Vector2(1, (self.aimTimer / sm.UserData.Properties.ScreenAnimation.FadeTime));
				-- scale all the layers down
				UiTransformBus.Event.SetScale(sm.UserData.Properties.ScreenHookup.Layers.Layer1.ContentFaderID, scaleVar);
				UiTransformBus.Event.SetScale(sm.UserData.Properties.ScreenHookup.Layers.Layer2.ContentFaderID, scaleVar);
            end,
            Transitions =
            {
				ChoiceShow = 
				{
                    Evaluate = function(state, sm)
						return (state.aimTimer <= 0);
                    end	
				}
            },
        },
		ChoiceShow = 
        {      
            OnEnter = function(self, sm)
				self.aimTimer = sm.UserData.Properties.ScreenAnimation.FadeTime;
				sm.UserData.buttonTimer = 0;

				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.UpgradeFaderID, 1, 0);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer2.UpgradeFaderID, 1, 0);

				-- set the highlighted health and energy sections since i wont need them to change
				sm.UserData.CanSelectLeft = true;
				if ((sm.UserData.playerHealthMax == nil) or (sm.UserData.playerHealthMax >= sm.UserData.Properties.PlayerChanges.Health.ValueAbsoluteMax)) then
					sm.UserData.CanSelectLeft = false;
				end
				sm.UserData.CanSelectRight = true;
				if ((sm.UserData.playerEnergyMax == nil) or (sm.UserData.playerEnergyMax >= sm.UserData.Properties.PlayerChanges.Energy.ValueAbsoluteMax)) then
					sm.UserData.CanSelectRight = false;
				end
				
				if(sm.UserData.CanSelectLeft) then
					sm.UserData.RightOption = false;
				else
					sm.UserData.RightOption = true;
				end
				
				sm.UserData.healthBoostValue = (sm.UserData.playerHealthMax + sm.UserData.Properties.PlayerChanges.Health.ValueIncrease);
				if (sm.UserData.healthBoostValue > sm.UserData.Properties.PlayerChanges.Health.ValueAbsoluteMax) then
					sm.UserData.healthBoostValue = sm.UserData.Properties.PlayerChanges.Health.ValueAbsoluteMax;
				end
				--Debug.Log("uidiegeticmenu: healthBoostValue  " .. tostring(sm.UserData.healthBoostValue));
				sm.UserData.energyBoostValue = (sm.UserData.playerEnergyMax + sm.UserData.Properties.PlayerChanges.Energy.ValueIncrease);
				if (sm.UserData.energyBoostValue > sm.UserData.Properties.PlayerChanges.Energy.ValueAbsoluteMax) then
					sm.UserData.energyBoostValue = sm.UserData.Properties.PlayerChanges.Energy.ValueAbsoluteMax;
				end
				--Debug.Log("uidiegeticmenu: energyBoostValue  " .. tostring(sm.UserData.energyBoostValue));
				UiSliderBus.Event.SetValue(sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.BarCurrentID, sm.UserData.playerHealthMax);	
				UiSliderBus.Event.SetValue(sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.BarMaxID, sm.UserData.healthBoostValue );	
				UiSliderBus.Event.SetValue(sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.BarCurrentID, sm.UserData.playerEnergyMax);	
				UiSliderBus.Event.SetValue(sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.BarMaxID, sm.UserData.energyBoostValue);
			end,
            OnExit = function(self, sm) 				
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.buttonStateSetEventId, "Active");
			end,            
            OnUpdate = function(self, sm, deltaTime)
				self.aimTimer = self.aimTimer - deltaTime;
				if(self.aimTimer < 0) then
					self.aimTimer = 0;
				end

				local scaleVar = Vector2(1, 1 - (self.aimTimer / sm.UserData.Properties.ScreenAnimation.FadeTime));
				-- scale all the layers down
				UiTransformBus.Event.SetScale(sm.UserData.Properties.ScreenHookup.Layers.Layer1.ContentFaderID, scaleVar);
				UiTransformBus.Event.SetScale(sm.UserData.Properties.ScreenHookup.Layers.Layer2.ContentFaderID, scaleVar);
            end,
            Transitions =
            {
				ChoiceIdleLeft = 
				{
                    Evaluate = function(state, sm)
						return (state.aimTimer <= 0) and (not sm.UserData.RightOption);
					end	
				},
				ChoiceIdleRight = 
				{
                    Evaluate = function(state, sm)
						return (state.aimTimer <= 0) and (sm.UserData.RightOption);
					end	
				}
            },
        },	
		ChoiceIdleLeft =
		{
			OnEnter = function(self, sm)
				sm.UserData.ContinuePressed = false;
				local st =  sm.UserData.Properties.ScreenAnimation.ChoiceScreen.SideTransitionTime;
				self.arrow1Timer = -st - 0;
				self.arrow2Timer = -st - (sm.UserData.Properties.ScreenAnimation.ChoiceScreen.ArrowPieceOffsetTime * 1);
				self.arrow3Timer = -st - (sm.UserData.Properties.ScreenAnimation.ChoiceScreen.ArrowPieceOffsetTime * 2);
				self.arrow4Timer = -st - (sm.UserData.Properties.ScreenAnimation.ChoiceScreen.ArrowPieceOffsetTime * 3);
				self.barTimer = 0;
				st =  1.0 / st;
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftSelectedID, 1, st);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightSelectedID, 0, st);
			end,
            OnExit = function(self, sm)
				if(sm.UserData.ContinuePressed) then
					-- if i have hit this case then i need to change my params / update 
					local ft = 1.0 / sm.UserData.Properties.ScreenAnimation.FadeTime;
					local ct = 1.0 / sm.UserData.Properties.ScreenAnimation.ChoiceScreen.ArrowPieceCycleTime;
					UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.BarMaxID, 1, ft);
					UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.SelectionConfirmedFlasherID, 1, ft);
					UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.Arrow1ID, 1, ct);
					UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.Arrow2ID, 1, ct);
					UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.Arrow3ID, 1, ct);
					UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.Arrow4ID, 1, ct);		

					-- change my health
					GameplayNotificationBus.Event.OnEventBegin(sm.UserData.healthMaxSetEventId, sm.UserData.healthBoostValue);
					GameplayNotificationBus.Event.OnEventBegin(sm.UserData.healthValueSetEventId, sm.UserData.healthBoostValue);
					
					if(sm.UserData.Properties.Sounds.SelectHealth ~= "") then
						AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.parentEntity, sm.UserData.Properties.Sounds.SelectHealth);
					end
				else				
					if(sm.UserData.Properties.Sounds.OptionChange ~= "") then
						AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.parentEntity, sm.UserData.Properties.Sounds.OptionChange);
					end
				end
            end,            
            OnUpdate = function(self, sm, deltaTime)
				-- cycle the upgrade arrow
				local ct = sm.UserData.Properties.ScreenAnimation.ChoiceScreen.ArrowPieceCycleTime;
				self.arrow1Timer = self.arrow1Timer + deltaTime;
				if (self.arrow1Timer >= ct) then
					self.arrow1Timer = self.arrow1Timer - ct;
				end
				local fade1 = 1 - utilities.Clamp(math.abs(((self.arrow1Timer / ct) * 2) - 1), 0.0, 1.0);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.Arrow1ID, fade1, 0);
				
				self.arrow2Timer = self.arrow2Timer + deltaTime;
				if (self.arrow2Timer >= ct) then
					self.arrow2Timer = self.arrow2Timer - ct;
				end
				local fade2 = 1 - utilities.Clamp(math.abs(((self.arrow2Timer / ct) * 2) - 1), 0.0, 1.0);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.Arrow2ID, fade2, 0);
				
				self.arrow3Timer = self.arrow3Timer + deltaTime;
				if (self.arrow3Timer >= ct) then
					self.arrow3Timer = self.arrow3Timer - ct;
				end
				local fade3 = 1 - utilities.Clamp(math.abs(((self.arrow3Timer / ct) * 2) - 1), 0.0, 1.0);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.Arrow3ID, fade3, 0);
				
				self.arrow4Timer = self.arrow4Timer + deltaTime;
				if (self.arrow4Timer >= ct) then
					self.arrow4Timer = self.arrow4Timer - ct;
				end
				local fade4 = 1 - utilities.Clamp(math.abs(((self.arrow4Timer / ct) * 2) - 1), 0.0, 1.0);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.Arrow4ID, fade4, 0);

				-- flash the highlight bar
				self.barTimer = self.barTimer + deltaTime;
				if (self.barTimer >= ct) then
					self.barTimer = self.barTimer - ct;
				end
				local barfade = 1 - utilities.Clamp(math.abs(((self.barTimer / ct) * 2) - 1), 0.0, 1.0);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.BarMaxID, barfade, 0);
            end,
            Transitions =
            {
 				ChoiceSelected = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.ContinuePressed;
					end	
				},
 				ChoiceIdleRight = 
				{
                    Evaluate = function(state, sm)
						return (sm.UserData.RightOption and sm.UserData.CanSelectRight and (not sm.UserData.ContinuePressed));
					end	
				}
          },
		},
		ChoiceIdleRight =
		{
			OnEnter = function(self, sm)
				sm.UserData.ContinuePressed = false;
				local st =  sm.UserData.Properties.ScreenAnimation.ChoiceScreen.SideTransitionTime;
				self.arrow1Timer = -st - 0;
				self.arrow2Timer = -st - (sm.UserData.Properties.ScreenAnimation.ChoiceScreen.ArrowPieceOffsetTime * 1);
				self.arrow3Timer = -st - (sm.UserData.Properties.ScreenAnimation.ChoiceScreen.ArrowPieceOffsetTime * 2);
				self.arrow4Timer = -st - (sm.UserData.Properties.ScreenAnimation.ChoiceScreen.ArrowPieceOffsetTime * 3);
				self.barTimer = 0;
				st =  1.0 / st;
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightSelectedID, 1, st);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftSelectedID, 0, st);
			end,
			OnExit = function(self, sm)
				if(sm.UserData.ContinuePressed) then
					-- if i have hit this case then i need to change my params / update 
					local ft = 1.0 / sm.UserData.Properties.ScreenAnimation.FadeTime;
					local ct = 1.0 / sm.UserData.Properties.ScreenAnimation.ChoiceScreen.ArrowPieceCycleTime;
					UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.BarMaxID, 1, ft);
					UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.SelectionConfirmedFlasherID, 1, ft);
					UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.Arrow1ID, 1, ct);
					UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.Arrow2ID, 1, ct);
					UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.Arrow3ID, 1, ct);
					UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.Arrow4ID, 1, ct);	
					
					-- change my energy
					GameplayNotificationBus.Event.OnEventBegin(sm.UserData.energyMaxSetEventId, sm.UserData.energyBoostValue);					
					GameplayNotificationBus.Event.OnEventBegin(sm.UserData.energyValueSetEventId, sm.UserData.energyBoostValue);					
					
					if(sm.UserData.Properties.Sounds.SelectEnergy ~= "") then
						AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.parentEntity, sm.UserData.Properties.Sounds.SelectEnergy);
					end
				else				
					if(sm.UserData.Properties.Sounds.OptionChange ~= "") then
						AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.parentEntity, sm.UserData.Properties.Sounds.OptionChange);
					end
				end
            end,
            OnUpdate = function(self, sm, deltaTime)
				-- cycle the upgrade arrow
				local ct = sm.UserData.Properties.ScreenAnimation.ChoiceScreen.ArrowPieceCycleTime;
				self.arrow1Timer = self.arrow1Timer + deltaTime;
				if (self.arrow1Timer >= ct) then
					self.arrow1Timer = self.arrow1Timer - ct;
				end
				local fade1 = 1 - utilities.Clamp(math.abs(((self.arrow1Timer / ct) * 2) - 1), 0.0, 1.0);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.Arrow1ID, fade1, 0);
				
				self.arrow2Timer = self.arrow2Timer + deltaTime;
				if (self.arrow2Timer >= ct) then
					self.arrow2Timer = self.arrow2Timer - ct;
				end
				local fade2 = 1 - utilities.Clamp(math.abs(((self.arrow2Timer / ct) * 2) - 1), 0.0, 1.0);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.Arrow2ID, fade2, 0);
				
				self.arrow3Timer = self.arrow3Timer + deltaTime;
				if (self.arrow3Timer >= ct) then
					self.arrow3Timer = self.arrow3Timer - ct;
				end
				local fade3 = 1 - utilities.Clamp(math.abs(((self.arrow3Timer / ct) * 2) - 1), 0.0, 1.0);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.Arrow3ID, fade3, 0);
				
				self.arrow4Timer = self.arrow4Timer + deltaTime;
				if (self.arrow4Timer >= ct) then
					self.arrow4Timer = self.arrow4Timer - ct;
				end
				local fade4 = 1 - utilities.Clamp(math.abs(((self.arrow4Timer / ct) * 2) - 1), 0.0, 1.0);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.Arrow4ID, fade4, 0);

				-- flash the highlight bar
				self.barTimer = self.barTimer + deltaTime;
				if (self.barTimer >= ct) then
					self.barTimer = self.barTimer - ct;
				end
				local barfade = 1 - utilities.Clamp(math.abs(((self.barTimer / ct) * 2) - 1), 0.0, 1.0);
				UiFaderBus.Event.Fade(sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.BarMaxID, barfade, 0);
			end,
            Transitions =
            {
 				ChoiceSelected = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.ContinuePressed;
					end	
				},
				ChoiceIdleLeft = 
				{
                    Evaluate = function(state, sm)
						return ((not sm.UserData.RightOption) and sm.UserData.CanSelectLeft and (not sm.UserData.ContinuePressed));
					end	
				}
			},
		},
		ChoiceSelected =
		{
		-- basically hold for some time before dissappearing
			OnEnter = function(self, sm)
				sm.UserData.ContinuePressed = false;
				self.aimTimer = sm.UserData.Properties.ScreenAnimation.ChoiceScreen.ChosenHighlightTime;
				
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.buttonStateSetEventId, "Pressed");
			end,
            OnUpdate = function(self, sm, deltaTime)
				self.aimTimer = self.aimTimer - deltaTime;
            end,
            Transitions =
            {
 				ChoiceClosing = 
				{
                    Evaluate = function(state, sm)
						return state.aimTimer <= 0;
					end	
				} 
            },
		},
		ChoiceClosing =
		{
			OnEnter = function(self, sm)
				self.aimTimer = sm.UserData.Properties.ScreenAnimation.FadeTime;
				
				UiFaderBus.Event.Fade(sm.UserData.entityId, 0, self.aimTimer);

				if(sm.UserData.Properties.Sounds.OptionClosing ~= "") then
					AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.parentEntity, sm.UserData.Properties.Sounds.OptionClosing);
				end
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
            end,            
			OnUpdate = function(self, sm, deltaTime)	
				self.aimTimer = self.aimTimer - deltaTime;
				if (self.aimTimer < 0) then
					self.aimTimer = 0;
				end
				
				local scaleValue = (self.aimTimer / sm.UserData.Properties.ScreenAnimation.FadeTime);
				--Debug.Log("uidiegeticmenu:ClosingData " .. tostring(scaleValue));
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
				} 
            },
		},
	},
}

function uidiegeticmenu:OnActivate()
	--Debug.Log("uidiegeticmenu:OnActivate() .. " .. tostring(self.entityId));
	
	-- Listen for enable/disable events.
	self.menuRightEventId = GameplayNotificationId(self.entityId, self.Properties.MessagesIn.MenuRight, "float");
	self.menuRightHandler = GameplayNotificationBus.Connect(self, self.menuRightEventId);
	self.menuLeftEventId = GameplayNotificationId(self.entityId, self.Properties.MessagesIn.MenuLeft, "float");
	self.menuLeftHandler = GameplayNotificationBus.Connect(self, self.menuLeftEventId);
	self.menuContinueEventId = GameplayNotificationId(self.entityId, self.Properties.MessagesIn.MenuContinue, "float");
	self.menuContinueHandler = GameplayNotificationBus.Connect(self, self.menuContinueEventId);
	
	self.optionSelectedEventId = GameplayNotificationId(self.entityId, self.Properties.MessagesOut.OptionSelected, "float");
	self.controlCharacterEventId = GameplayNotificationId(self.entityId, self.Properties.MessagesOut.ControlCharacter, "float");
	self.releaseCharacterEventId = GameplayNotificationId(self.entityId, self.Properties.MessagesOut.ReleaseCharacter, "float");
	self.buttonStateSetEventId = GameplayNotificationId(self.Properties.ScreenHookup.Layers.Layer1.ButtonNew, self.Properties.MessagesOut.SetButtonStateEvent, "float");
	
	self.healthMaxCBEventId = GameplayNotificationId(self.entityId, self.Properties.PlayerChanges.Health.GetMaxMessageCallback, "float");
	self.healthMaxCBHandler = GameplayNotificationBus.Connect(self, self.healthMaxCBEventId);
	self.energyMaxCBEventId = GameplayNotificationId(self.entityId, self.Properties.PlayerChanges.Energy.GetMaxMessageCallback, "float");
	self.energyMaxCBHandler = GameplayNotificationBus.Connect(self, self.energyMaxCBEventId);
				
		
	self.playerHealthMax = nil;
	self.playerEnergyMax = nil;
	self.InteractPositionEntity = nil;
	self.InteractCameraEntity = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCamera"));
	
	self.WantsToShow = false;
	self.WantsToShowChoice = false; --self.Properties.DisplayUpgrade;
	self.menuShowing = false;
	self.buttonPressed = false;
		
	-- self.showMenuEventId = GameplayNotificationId(self.parentEntity, self.Properties.MessagesIn.ShowMenu, "float");
	-- self.showMenuHandler = GameplayNotificationBus.Connect(self, self.showMenuEventId);
	-- self.modelOriginalLocalT = TransformBus.Event.GetLocalTM(self.parentEntity);
	
	-- state machine hookup
    self.StateMachine = {}
    setmetatable(self.StateMachine, StateMachine);
    self.StateMachine:Start("UIDigeticMenu", self.entityId, self, self.States, false, "Closed", self.Properties.DebugStateMachine)
	
	--Debug.Log("uidiegeticmenu ID: " .. tostring(self.entityId));
	
	self.tickHandler = TickBus.Connect(self);
end

function uidiegeticmenu:OnDeactivate()
	--Debug.Log("uidiegeticmenu:OnDeactivate() " .. tostring(self.entityId));
	if (self.showMenuHandler ~= nil) then
		self.showMenuHandler:Disconnect();
		self.showMenuHandler = nil;
	end
	if (self.SetParentHandler ~= nil) then
		self.SetParentHandler:Disconnect();
		self.SetParentHandler = nil;
	end
	if (self.SetInteractPositionHandler ~= nil) then
		self.SetInteractPositionHandler:Disconnect();
		self.SetInteractPositionHandler = nil;
	end
	if (self.SetInteractCameraHandler ~= nil) then
		self.SetInteractCameraHandler:Disconnect();
		self.SetInteractCameraHandler = nil;
	end
	
	self.menuRightHandler:Disconnect();
	self.menuRightHandler = nil;
	self.menuLeftHandler:Disconnect();
	self.menuLeftHandler = nil;
	self.menuContinueHandler:Disconnect();
	self.menuContinueHandler = nil;
	self.healthMaxCBHandler:Disconnect();
	self.healthMaxCBHandler = nil;
	self.energyMaxCBHandler:Disconnect();
	self.energyMaxCBHandler = nil;
	
	self.tickHandler:Disconnect();
	self.tickHandler = nil;

	self:EnableHud(false);
	
	self.StateMachine:Stop();
end

function uidiegeticmenu:OnTick(deltaTime, timePoint)
	if(self.firsttickHandeled ~= true) then
		self.canvas = UiElementBus.Event.GetCanvas(self.entityId);
		self.SetParentEventId = GameplayNotificationId(self.canvas, self.Properties.MessagesIn.SetParent, "float");
		self.SetParentHandler = GameplayNotificationBus.Connect(self, self.SetParentEventId);

		self.playerID = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"));
		self.healthMaxGetEventId = GameplayNotificationId(self.playerID, self.Properties.PlayerChanges.Health.GetMaxMessage, "float");
		self.healthMaxSetEventId = GameplayNotificationId(self.playerID, self.Properties.PlayerChanges.Health.SetMaxMessage, "float");
		self.healthValueSetEventId = GameplayNotificationId(self.playerID, self.Properties.PlayerChanges.Health.SetValueMessage, "float");
		self.energyMaxGetEventId = GameplayNotificationId(self.playerID, self.Properties.PlayerChanges.Energy.GetMaxMessage, "float");
		self.energyMaxSetEventId = GameplayNotificationId(self.playerID, self.Properties.PlayerChanges.Energy.SetMaxMessage, "float");
		self.energyValueSetEventId = GameplayNotificationId(self.playerID, self.Properties.PlayerChanges.Energy.SetValueMessage, "float");
		self.startInteractEventId = GameplayNotificationId(self.playerID, "StartInteract", "float");
		self.endInteractEventId = GameplayNotificationId(self.playerID, "EndInteract", "float");
		self.firsttickHandeled = true;
	end
end

function uidiegeticmenu:EnableHud(enabled)
	if (enabled == self.enabled) then
		return;
	end

	if (self.parentEntity ~= nil) then
		MeshComponentRequestBus.Event.SetVisibility(self.parentEntity, enabled);
	end
	
	self.enabled = enabled;
end

function uidiegeticmenu:OnEventBegin(value)
	--Debug.Log("uidiegeticmenu:OnEventBegin() " .. tostring(self.entityId) .. " : " .. tostring(value));
	if (GameplayNotificationBus.GetCurrentBusId() == self.showMenuEventId) then
		if(self.InteractPositionEntity == nil or self.InteractCameraEntity == nil) then
			--Debug.Log("uidiegeticmenu:OnEventBegin() " .. tostring(self.canvas) .. " tried to show but params were " .. tostring(self.InteractPositionEntity) .. " " .. tostring(self.InteractCameraEntity));
		else	
			--Debug.Log("uidiegeticmenu:OnEventBegin() " .. tostring(self.canvas) .. " show, params were " .. tostring(self.InteractPositionEntity) .. " " .. tostring(self.InteractCameraEntity));
			self.WantsToShow = true;
			self.WantsToShowChoice = value;
		end

	elseif (GameplayNotificationBus.GetCurrentBusId() == self.menuRightEventId and self.CanSelectRight) then
		self.RightOption = true;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.menuLeftEventId and self.CanSelectLeft) then
		self.RightOption = false;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.menuContinueEventId) then
		self.ContinuePressed = true;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.healthMaxCBEventId) then
		self.playerHealthMax = value;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.energyMaxCBEventId) then
		self.playerEnergyMax = value;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetInteractPositionEventId) then
		--Debug.Log("uidiegeticmenu:OnEventBegin() set InteractPositionEntity" .. tostring(self.canvas) .. " : " .. tostring(value));
		self.InteractPositionEntity = value;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetInteractCameraEventId) then
		--Debug.Log("uidiegeticmenu:OnEventBegin() set InteractCameraEntity" .. tostring(self.canvas) .. " : " .. tostring(value));
		self.InteractCameraEntity = value;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.SetParentEventId) then
		if(self.showMenuHandler ~= nil) then
			self.showMenuHandler:Disconnect();
			self.showMenuHandler = nil;
		end
		--Debug.Log("uidiegeticmenu:OnEventBegin() set parent" .. tostring(self.canvas) .. " : " .. tostring(value));
		self.parentEntity = value;
		self.showMenuEventId = GameplayNotificationId(self.parentEntity, self.Properties.MessagesIn.ShowMenu, "float");
		self.showMenuHandler = GameplayNotificationBus.Connect(self, self.showMenuEventId);
		self.modelOriginalLocalT = TransformBus.Event.GetLocalTM(self.parentEntity);	
		
		if (self.SetInteractPositionHandler ~= nil) then
			self.SetInteractPositionHandler:Disconnect();
			self.SetInteractPositionHandler = nil;
		end
		if (self.SetInteractCameraHandler ~= nil) then
			self.SetInteractCameraHandler:Disconnect();
			self.SetInteractCameraHandler = nil;
		end		

		if(self.InteractPositionEntity == nil) then
			--self.InteractPositionEntity = self.parentEntity;
		end
		
		--Debug.Log("uidiegeticmenu:OnEventBegin() setup position event listener : " .. tostring(self.parentEntity) .. " : " .. tostring(self.Properties.MessagesIn.SetInteractPosition));
		self.SetInteractPositionEventId = GameplayNotificationId(self.parentEntity, self.Properties.MessagesIn.SetInteractPosition, "float");
		self.SetInteractPositionHandler = GameplayNotificationBus.Connect(self, self.SetInteractPositionEventId);
		self.SetInteractCameraEventId = GameplayNotificationId(self.parentEntity, self.Properties.MessagesIn.SetInteractCamera, "float");
		self.SetInteractCameraHandler = GameplayNotificationBus.Connect(self, self.SetInteractCameraEventId);
	end
end

return uidiegeticmenu;
