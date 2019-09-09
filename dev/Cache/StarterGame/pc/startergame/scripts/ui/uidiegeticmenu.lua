local StateMachine = require "scripts/common/StateMachine"
local utilities = require "scripts/common/utilities"
local uidiegeticmenu =
{
	Properties =
	{
		--CodeEntity = { default = EntityId() }, -- to be removed
		
		MessagesIn = {
			ShowMenu = { default = "ShowDiegeticMenu", description = "The event used to enable/disable the crosshair (0 is just data, 1 is for choices)" },
			MenuRight = { default = "Right", description = "The from the controller to move right on the menu" },
			MenuLeft = { default = "Left", description = "The from the controller to move left on the menu" },
			MenuContinue = { default = "Continue", description = "The event used to enable/disable the crosshair (0 is just data, 1 is for choices)" },
		},
		MessagesOut = {
			MenuFinished = { default = "MenuComplete", description = "The event sent when the menu is closed" },
			OptionSelected = { default = "OptionSelected", description = "Represenst the selected option in payload, 0 for left, 1 for right" },
			ControlCharacter = { default = "ControlCharacter", description = "sent when we grab control of the player for the menu" },
			ReleaseCharacter = { default = "ReleaseCharacter", description = "sent when we grab control of the player for the menu" },
			EnableControlsEvent = { default = "EnableControlsEvent", description = "This is the message we send to disable / enable player control" },
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
				CompleteTime = { default = 3, description = "The time taken to show complete" },
			},
			ChoiceScreen = {
				SideTransitionTime = { default = 0.25, description = "The time taken to transition sides" },
				ArrowPieceCycleTime = { default = 1, description = "The time for the upgrade arrow peice cycle" },
				ArrowPieceOffsetTime = { default = 0.25, description = "The time for the upgrade arrow peice offset" },
				ChosenHighlightTime = { default = 2, description = "The time to show in a highlight state before transitioning off" },
			},
			Button = {
				CycleTime  = { default = 1, description = "The time for the active anim cycle" },
				ScaleAmount = { default = 0.75, description = "The amount that the active ring scales down by" },
			},
		},
		
		DebugStateMachine =  { default = false, description = "display debugging onif0ormation for the state machine." },
		DisplayUpgrade =  { default = false, description = "Should i display the upgrade." },
		ParticleEffect = { default = EntityId(), description = "The object with the display particle effect." },
		
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
			Model = { default = EntityId(), description = "The model that the menu is displayed upon." },
			Camera = { default = EntityId(), description = "The camera to use while in the menu." },
			PositionEntity = { default = EntityId(), description = "The entity to place the player on while in the menu." },
			-- note, the assumption here is that all the screens are taken from a common master base, so that the IDs will not change on different versions
			AllFaderID = { default = 100, description = "The ID of the health Bar slider" },
			Layers = {
				--HealthBarID = { default = -1, description = "The ID of the health Bar slider" },
				Layer1 = {
					WorkingFaderID = { default = 94, description = "The ID of all working items" },
					CompleteFaderID = { default = 95, description = "The ID of all complete items" },
					ContentFaderID = { default = 104, description = "The ID of all non generic items" },
					WorkingRotatorID = { default = 14, description = "The ID of a rotating element" },
					WorkingBarID = { default = 202, description = "The ID of a bar element" },
					UpgradeFaderID = { default = 34, description = "The ID of all choice items" },
					DataFaderID = { default = 10, description = "The ID of all choice items" },
					LeftSelectedID = { default = 105, description = "The ID of selected items on the left" },
					RightSelectedID = { default = 106, description = "The ID of selected items on the right" },
					LeftItems = {
						Arrow1ID = { default = 37, description = "The ID of selected items on the right" },
						Arrow2ID = { default = 38, description = "The ID of selected items on the right" },
						Arrow3ID = { default = 47, description = "The ID of selected items on the right" },
						Arrow4ID = { default = 48, description = "The ID of selected items on the right" },
						BarCurrentID = { default = 171, description = "The ID of selected items on the right" },
						BarMaxID = { default = 167, description = "The ID of the bar for proposed health" },
						SelectionConfirmedFlasherID = { default = 33, description = "The ID of an item for all of the highlights of the bar to flash them" },
					},
					RightItems = {
						Arrow1ID = { default = 50, description = "The ID of selected items on the right" },
						Arrow2ID = { default = 51, description = "The ID of selected items on the right" },
						Arrow3ID = { default = 52, description = "The ID of selected items on the right" },
						Arrow4ID = { default = 53, description = "The ID of selected items on the right" },
						BarCurrentID = { default = 181, description = "The ID of the bar for current energy" },
						BarMaxID = { default = 177, description = "The ID of the bar for proposed energy" },
						SelectionConfirmedFlasherID = { default = 32, description = "The ID of an item for all of the highlights of the bar to flash them" },
					},
					Button = {
						IdleID = { default = 7, description = "The ID of the base button" },
						ActiveID = { default = 165, description = "The ID of the button for when it can be pushed" },
						HighlightID = { default = 109, description = "The ID of ring highlight" },
						ConfirmedID = { default = 166, description = "The ID of pressed button" },
					},
				},
				Layer2 = {
					WorkingFaderID = { default = 30, description = "The ID of all working items" },
					CompleteFaderID = { default = 31, description = "The ID of all complete items" },
					ContentFaderID = { default = 103, description = "The ID of all non generic items" },
					WorkingRotatorID = { default = 30, description = "The ID of a rotating element" },
					UpgradeFaderID = { default = 28, description = "The ID of all choice items" },
					DataFaderID = { default = 27, description = "The ID of all choice items" },
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
				ParticleComponentRequestBus.Event.Enable(sm.UserData.Properties.ParticleEffect, false);
            end,
            OnExit = function(self, sm)				
				-- prescale this down to avoid a full flash
				if (sm.UserData.modelOriginalLocalT == nil) then
					sm.UserData.modelOriginalLocalT = TransformBus.Event.GetLocalTM(sm.UserData.Properties.ScreenHookup.Model);
				end
				local trans = sm.UserData.modelOriginalLocalT * Transform.CreateScale(Vector3(1, 1, 0));
				TransformBus.Event.SetLocalTM(sm.UserData.Properties.ScreenHookup.Model, trans);

				sm.UserData:EnableHud(true);
				sm.UserData.WantsToShow = false;
				
				-- try ton ensure that the screen has the correct things visible
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.DataFaderID, 1, 0);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer2.DataFaderID, 1, 0);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.UpgradeFaderID, 0, 0);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer2.UpgradeFaderID, 0, 0);								
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.WorkingFaderID, 1, 0);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer2.WorkingFaderID, 1, 0);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.CompleteFaderID, 0, 0);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer2.CompleteFaderID, 0, 0);
				
				-- get the information as to the stats for the player to populate the menu
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.healthMaxGetEventId, sm.UserData.entityId);
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.energyMaxGetEventId, sm.UserData.entityId);
				
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.healthValueSetEventId, sm.UserData.playerHealthMax);
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.energyValueSetEventId, sm.UserData.playerEnergyMax);
				
				ParticleComponentRequestBus.Event.Enable(sm.UserData.Properties.ParticleEffect, true);
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
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.AllFaderID, 1, self.aimTimer);
				
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.controlCharacterEventId, true);
				
				-- send event to player to begin interaction (change camera, move character, animate)
				local eventArgs = InteractParams();
				eventArgs.positionEntity = sm.UserData.Properties.ScreenHookup.PositionEntity;
				eventArgs.cameraEntity = sm.UserData.Properties.ScreenHookup.Camera;
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.startInteractEventId, eventArgs);
				
				if(sm.UserData.Properties.Sounds.Open ~= "") then
					AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.entityId, sm.UserData.Properties.Sounds.Open);
				end
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
				TransformBus.Event.SetLocalTM(sm.UserData.Properties.ScreenHookup.Model, trans);
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

				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.WorkingFaderID, 1, sm.UserData.Properties.ScreenAnimation.Layers.WorkingToCompleteFadeTime);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer2.WorkingFaderID, 1, sm.UserData.Properties.ScreenAnimation.Layers.WorkingToCompleteFadeTime);
				StarterGameUIUtility.UISliderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.WorkingBarID, 0);
				
				if(sm.UserData.Properties.Sounds.DownloadStart ~= "") then
					AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.entityId, sm.UserData.Properties.Sounds.DownloadStart);
				end
			end,
            OnExit = function(self, sm)
				-- should always end on a full rotation, so zero it out
				StarterGameUIUtility.UIElementRotator(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.WorkingRotatorID, 0);
				StarterGameUIUtility.UIElementRotator(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer2.WorkingRotatorID, 0);

				--Debug.Log("uidiegeticmenu:DataWorking exit " .. tostring(sm.UserData.Properties.ScreenAnimation.Layers.WorkingToCompleteFadeTime));
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.WorkingFaderID, 0, sm.UserData.Properties.ScreenAnimation.Layers.WorkingToCompleteFadeTime);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer2.WorkingFaderID, 0, sm.UserData.Properties.ScreenAnimation.Layers.WorkingToCompleteFadeTime);
 
				if(sm.UserData.Properties.Sounds.DownloadStop ~= "") then
					AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.entityId, sm.UserData.Properties.Sounds.DownloadStop);
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
			
				StarterGameUIUtility.UIElementRotator(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.WorkingRotatorID, rotateVar);
				StarterGameUIUtility.UIElementRotator(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer2.WorkingRotatorID, rotateVar);
				StarterGameUIUtility.UISliderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.WorkingBarID, (1 - linearVar) * 100);	
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
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.CompleteFaderID, 1, 0);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer2.CompleteFaderID, 1, 0);
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
				
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.AllFaderID, 0, self.aimTimer);
								
				if(sm.UserData.Properties.Sounds.NoOptions ~= "") then
					AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.entityId, sm.UserData.Properties.Sounds.NoOptions);
				end
			end,
            OnExit = function(self, sm)
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.releaseCharacterEventId, true);
				
				-- send event to player to end interaction (enable controls, select player camera)
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.endInteractEventId, nil);
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.menuFinishedEventId, true);
				sm.UserData.WantsToShow = false;
				
				if(sm.UserData.Properties.Sounds.Close ~= "") then
					AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.entityId, sm.UserData.Properties.Sounds.Close);
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
				TransformBus.Event.SetLocalTM(sm.UserData.Properties.ScreenHookup.Model, trans);
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
					AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.entityId, sm.UserData.Properties.Sounds.ToOptions);
				end
            end,
            OnExit = function(self, sm)
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.DataFaderID, 0, 0);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer2.DataFaderID, 0, 0);
            end,            
            OnUpdate = function(self, sm, deltaTime)
				self.aimTimer = self.aimTimer - deltaTime;
				if(self.aimTimer < 0) then
					self.aimTimer = 0;
				end

				local scaleVar = Vector2(1, (self.aimTimer / sm.UserData.Properties.ScreenAnimation.FadeTime));
				-- scale all the layers down
				StarterGameUIUtility.UIElementScaler(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.ContentFaderID, scaleVar);
				StarterGameUIUtility.UIElementScaler(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer2.ContentFaderID, scaleVar);
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

				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.UpgradeFaderID, 1, 0);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer2.UpgradeFaderID, 1, 0);

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
				StarterGameUIUtility.UISliderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.BarCurrentID, sm.UserData.playerHealthMax);	
				StarterGameUIUtility.UISliderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.BarMaxID, sm.UserData.healthBoostValue );	
				StarterGameUIUtility.UISliderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.BarCurrentID, sm.UserData.playerEnergyMax);	
				StarterGameUIUtility.UISliderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.BarMaxID, sm.UserData.energyBoostValue);
			end,
            OnExit = function(self, sm)
				local st = sm.UserData.Properties.ScreenAnimation.ChoiceScreen.SideTransitionTime;
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.Button.ActiveID, 1, st);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.Button.HighlightID, 1, st);
            end,            
            OnUpdate = function(self, sm, deltaTime)
				self.aimTimer = self.aimTimer - deltaTime;
				if(self.aimTimer < 0) then
					self.aimTimer = 0;
				end

				local scaleVar = Vector2(1, 1 - (self.aimTimer / sm.UserData.Properties.ScreenAnimation.FadeTime));
				-- scale all the layers down
				StarterGameUIUtility.UIElementScaler(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.ContentFaderID, scaleVar);
				StarterGameUIUtility.UIElementScaler(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer2.ContentFaderID, scaleVar);
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
				local st = sm.UserData.Properties.ScreenAnimation.ChoiceScreen.SideTransitionTime;
				self.arrow1Timer = -st - 0;
				self.arrow2Timer = -st - (sm.UserData.Properties.ScreenAnimation.ChoiceScreen.ArrowPieceOffsetTime * 1);
				self.arrow3Timer = -st - (sm.UserData.Properties.ScreenAnimation.ChoiceScreen.ArrowPieceOffsetTime * 2);
				self.arrow4Timer = -st - (sm.UserData.Properties.ScreenAnimation.ChoiceScreen.ArrowPieceOffsetTime * 3);
				self.barTimer = 0;
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftSelectedID, 1, st);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightSelectedID, 0, st);
			end,
            OnExit = function(self, sm)
				if(sm.UserData.ContinuePressed) then
					-- if i have hit this case then i need to change my params / update 
					local ft = sm.UserData.Properties.ScreenAnimation.FadeTime;
					local ct = sm.UserData.Properties.ScreenAnimation.ChoiceScreen.ArrowPieceCycleTime;
					StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.BarMaxID, 1, ft);
					StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.SelectionConfirmedFlasherID, 1, ft);
					StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.Arrow1ID, 1, ct);
					StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.Arrow2ID, 1, ct);
					StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.Arrow3ID, 1, ct);
					StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.Arrow4ID, 1, ct);		

					-- change my health
					GameplayNotificationBus.Event.OnEventBegin(sm.UserData.healthMaxSetEventId, sm.UserData.healthBoostValue);
					GameplayNotificationBus.Event.OnEventBegin(sm.UserData.healthValueSetEventId, sm.UserData.healthBoostValue);
					
					if(sm.UserData.Properties.Sounds.SelectHealth ~= "") then
						AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.entityId, sm.UserData.Properties.Sounds.SelectHealth);
					end
				else				
					if(sm.UserData.Properties.Sounds.OptionChange ~= "") then
						AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.entityId, sm.UserData.Properties.Sounds.OptionChange);
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
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.Arrow1ID, fade1, 0);
				
				self.arrow2Timer = self.arrow2Timer + deltaTime;
				if (self.arrow2Timer >= ct) then
					self.arrow2Timer = self.arrow2Timer - ct;
				end
				local fade2 = 1 - utilities.Clamp(math.abs(((self.arrow2Timer / ct) * 2) - 1), 0.0, 1.0);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.Arrow2ID, fade2, 0);
				
				self.arrow3Timer = self.arrow3Timer + deltaTime;
				if (self.arrow3Timer >= ct) then
					self.arrow3Timer = self.arrow3Timer - ct;
				end
				local fade3 = 1 - utilities.Clamp(math.abs(((self.arrow3Timer / ct) * 2) - 1), 0.0, 1.0);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.Arrow3ID, fade3, 0);
				
				self.arrow4Timer = self.arrow4Timer + deltaTime;
				if (self.arrow4Timer >= ct) then
					self.arrow4Timer = self.arrow4Timer - ct;
				end
				local fade4 = 1 - utilities.Clamp(math.abs(((self.arrow4Timer / ct) * 2) - 1), 0.0, 1.0);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.Arrow4ID, fade4, 0);

				-- flash the highlight bar
				self.barTimer = self.barTimer + deltaTime;
				if (self.barTimer >= ct) then
					self.barTimer = self.barTimer - ct;
				end
				local barfade = 1 - utilities.Clamp(math.abs(((self.barTimer / ct) * 2) - 1), 0.0, 1.0);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftItems.BarMaxID, barfade, 0);
				
				local bt = sm.UserData.Properties.ScreenAnimation.Button.CycleTime;
				sm.UserData.buttonTimer = sm.UserData.buttonTimer + deltaTime;
				if (sm.UserData.buttonTimer >= bt) then
					sm.UserData.buttonTimer = sm.UserData.buttonTimer - bt;
				end
				local circleScale = utilities.Clamp((sm.UserData.buttonTimer / bt), 0.0, 1.0)
				local circleFade = 1 - circleScale;
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.Button.HighlightID, circleFade, 0);
				local scaleVar = utilities.Lerp(1, sm.UserData.Properties.ScreenAnimation.Button.ScaleAmount, circleScale);
				local scaleVec = Vector2(scaleVar, scaleVar);
				StarterGameUIUtility.UIElementScaler(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.Button.HighlightID, scaleVec);
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
				local st = sm.UserData.Properties.ScreenAnimation.ChoiceScreen.SideTransitionTime;
				self.arrow1Timer = -st - 0;
				self.arrow2Timer = -st - (sm.UserData.Properties.ScreenAnimation.ChoiceScreen.ArrowPieceOffsetTime * 1);
				self.arrow3Timer = -st - (sm.UserData.Properties.ScreenAnimation.ChoiceScreen.ArrowPieceOffsetTime * 2);
				self.arrow4Timer = -st - (sm.UserData.Properties.ScreenAnimation.ChoiceScreen.ArrowPieceOffsetTime * 3);
				self.barTimer = 0;
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightSelectedID, 1, st);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.LeftSelectedID, 0, st);
			end,
			OnExit = function(self, sm)
				if(sm.UserData.ContinuePressed) then
					-- if i have hit this case then i need to change my params / update 
					local ft = sm.UserData.Properties.ScreenAnimation.FadeTime;
					local ct = sm.UserData.Properties.ScreenAnimation.ChoiceScreen.ArrowPieceCycleTime;
					StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.BarMaxID, 1, ft);
					StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.SelectionConfirmedFlasherID, 1, ft);
					StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.Arrow1ID, 1, ct);
					StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.Arrow2ID, 1, ct);
					StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.Arrow3ID, 1, ct);
					StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.Arrow4ID, 1, ct);	
					
					-- change my energy
					GameplayNotificationBus.Event.OnEventBegin(sm.UserData.energyMaxSetEventId, sm.UserData.energyBoostValue);					
					GameplayNotificationBus.Event.OnEventBegin(sm.UserData.energyValueSetEventId, sm.UserData.energyBoostValue);					
					
					if(sm.UserData.Properties.Sounds.SelectEnergy ~= "") then
						AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.entityId, sm.UserData.Properties.Sounds.SelectEnergy);
					end
				else				
					if(sm.UserData.Properties.Sounds.OptionChange ~= "") then
						AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.entityId, sm.UserData.Properties.Sounds.OptionChange);
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
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.Arrow1ID, fade1, 0);
				
				self.arrow2Timer = self.arrow2Timer + deltaTime;
				if (self.arrow2Timer >= ct) then
					self.arrow2Timer = self.arrow2Timer - ct;
				end
				local fade2 = 1 - utilities.Clamp(math.abs(((self.arrow2Timer / ct) * 2) - 1), 0.0, 1.0);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.Arrow2ID, fade2, 0);
				
				self.arrow3Timer = self.arrow3Timer + deltaTime;
				if (self.arrow3Timer >= ct) then
					self.arrow3Timer = self.arrow3Timer - ct;
				end
				local fade3 = 1 - utilities.Clamp(math.abs(((self.arrow3Timer / ct) * 2) - 1), 0.0, 1.0);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.Arrow3ID, fade3, 0);
				
				self.arrow4Timer = self.arrow4Timer + deltaTime;
				if (self.arrow4Timer >= ct) then
					self.arrow4Timer = self.arrow4Timer - ct;
				end
				local fade4 = 1 - utilities.Clamp(math.abs(((self.arrow4Timer / ct) * 2) - 1), 0.0, 1.0);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.Arrow4ID, fade4, 0);

				-- flash the highlight bar
				self.barTimer = self.barTimer + deltaTime;
				if (self.barTimer >= ct) then
					self.barTimer = self.barTimer - ct;
				end
				local barfade = 1 - utilities.Clamp(math.abs(((self.barTimer / ct) * 2) - 1), 0.0, 1.0);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.RightItems.BarMaxID, barfade, 0);
				
				local bt = sm.UserData.Properties.ScreenAnimation.Button.CycleTime;
				sm.UserData.buttonTimer = sm.UserData.buttonTimer + deltaTime;
				if (sm.UserData.buttonTimer >= bt) then
					sm.UserData.buttonTimer = sm.UserData.buttonTimer - bt;
				end
				local circleScale = utilities.Clamp((sm.UserData.buttonTimer / bt), 0.0, 1.0)
				local circleFade = 1 - circleScale;
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.Button.HighlightID, circleFade, 0);
				local scaleVar = utilities.Lerp(1, sm.UserData.Properties.ScreenAnimation.Button.ScaleAmount, circleScale);
				local scaleVec = Vector2(scaleVar, scaleVar);
				StarterGameUIUtility.UIElementScaler(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.Button.HighlightID, scaleVec);
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
				
				local ct = sm.UserData.Properties.ScreenAnimation.ChoiceScreen.ArrowPieceCycleTime;
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.Button.ActiveID, 0, ct);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.Button.HighlightID, 0, ct);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.Layers.Layer1.Button.ConfirmedID, 1, ct);
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
				
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.AllFaderID, 0, self.aimTimer);

				if(sm.UserData.Properties.Sounds.OptionClosing ~= "") then
					AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.entityId, sm.UserData.Properties.Sounds.OptionClosing);
				end
			end,
            OnExit = function(self, sm)
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.releaseCharacterEventId, true);
				
				-- send event to player to end interaction (enable controls, select player camera)
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.endInteractEventId, nil);
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.menuFinishedEventId, true);
				sm.UserData.WantsToShow = false;
				
				if(sm.UserData.Properties.Sounds.Close ~= "") then
					AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.entityId, sm.UserData.Properties.Sounds.Close);
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
				TransformBus.Event.SetLocalTM(sm.UserData.Properties.ScreenHookup.Model, trans);		
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
	
	self.playerID = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"));
	
	-- Listen for enable/disable events.
	self.showMenuEventId = GameplayNotificationId(self.entityId, self.Properties.MessagesIn.ShowMenu, "float");
	self.showMenuHandler = GameplayNotificationBus.Connect(self, self.showMenuEventId);
	self.menuRightEventId = GameplayNotificationId(self.entityId, self.Properties.MessagesIn.MenuRight, "float");
	self.menuRightHandler = GameplayNotificationBus.Connect(self, self.menuRightEventId);
	self.menuLeftEventId = GameplayNotificationId(self.entityId, self.Properties.MessagesIn.MenuLeft, "float");
	self.menuLeftHandler = GameplayNotificationBus.Connect(self, self.menuLeftEventId);
	self.menuContinueEventId = GameplayNotificationId(self.entityId, self.Properties.MessagesIn.MenuContinue, "float");
	self.menuContinueHandler = GameplayNotificationBus.Connect(self, self.menuContinueEventId);
	
	self.menuFinishedEventId = GameplayNotificationId(self.entityId, self.Properties.MessagesOut.MenuFinished, "float");
	self.optionSelectedEventId = GameplayNotificationId(self.entityId, self.Properties.MessagesOut.OptionSelected, "float");
	self.controlCharacterEventId = GameplayNotificationId(self.entityId, self.Properties.MessagesOut.ControlCharacter, "float");
	self.releaseCharacterEventId = GameplayNotificationId(self.entityId, self.Properties.MessagesOut.ReleaseCharacter, "float");
	
	self.healthMaxGetEventId = GameplayNotificationId(self.playerID, self.Properties.PlayerChanges.Health.GetMaxMessage, "float");
	self.healthMaxSetEventId = GameplayNotificationId(self.playerID, self.Properties.PlayerChanges.Health.SetMaxMessage, "float");
	self.healthValueSetEventId = GameplayNotificationId(self.playerID, self.Properties.PlayerChanges.Health.SetValueMessage, "float");
	self.healthMaxCBEventId = GameplayNotificationId(self.entityId, self.Properties.PlayerChanges.Health.GetMaxMessageCallback, "float");
	self.healthMaxCBHandler = GameplayNotificationBus.Connect(self, self.healthMaxCBEventId);
	self.energyMaxGetEventId = GameplayNotificationId(self.playerID, self.Properties.PlayerChanges.Energy.GetMaxMessage, "float");
	self.energyMaxSetEventId = GameplayNotificationId(self.playerID, self.Properties.PlayerChanges.Energy.SetMaxMessage, "float");
	self.energyValueSetEventId = GameplayNotificationId(self.playerID, self.Properties.PlayerChanges.Energy.SetValueMessage, "float");
	self.energyMaxCBEventId = GameplayNotificationId(self.entityId, self.Properties.PlayerChanges.Energy.GetMaxMessageCallback, "float");
	self.energyMaxCBHandler = GameplayNotificationBus.Connect(self, self.energyMaxCBEventId);
	
	self.playerHealthMax = nil;
	self.playerEnergyMax = nil;
	
	self.startInteractEventId = GameplayNotificationId(self.playerID, "StartInteract", "float");
	self.endInteractEventId = GameplayNotificationId(self.playerID, "EndInteract", "float");
	
	self.WantsToShow = false;
	self.WantsToShowChoice = self.Properties.DisplayUpgrade;
	self.menuShowing = false;
	self.buttonPressed = false;
		
	self.modelOriginalLocalT = TransformBus.Event.GetLocalTM(self.Properties.ScreenHookup.Model);
	
	-- state machine hookup
    self.StateMachine = {}
    setmetatable(self.StateMachine, StateMachine);
    self.StateMachine:Start("UIDigeticMenu", self.entityId, self, self.States, false, "Closed", self.Properties.DebugStateMachine)
	
	--Debug.Log("uidiegeticmenu ID: " .. tostring(self.entityId));
	
	self.tickHandler = TickBus.Connect(self);
end

function uidiegeticmenu:OnDeactivate()
	--Debug.Log("uidiegeticmenu:OnDeactivate() " .. tostring(self.entityId));
	self.showMenuHandler:Disconnect();
	self.showMenuHandler = nil;
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
end

function uidiegeticmenu:EnableHud(enabled)
	if (enabled == self.enabled) then
		return;
	end
	
	--Debug.Log("uidiegeticmenu:EnableHud()" .. tostring(enabled));
	MeshComponentRequestBus.Event.SetVisibility(self.Properties.ScreenHookup.Model, enabled);
	
	if (enabled) then
		-- IMPORTANT: The 'canvas ID' is different to 'self.entityId'.
		self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("UI/Canvases/uidiegeticmenumaster.uicanvas");	
		--Debug.Log("uidiegeticmenu:EnableHud() setup");
	else
		if(self.canvasEntityId ~= nil) then
			UiCanvasManagerBus.Broadcast.UnloadCanvas(self.canvasEntityId);
		end
	end
	self.enabled = enabled;
end

function uidiegeticmenu:OnEventBegin(value)
	--Debug.Log("uidiegeticmenu:OnEventBegin() " .. tostring(self.entityId) .. " : " .. tostring(value));
	if (GameplayNotificationBus.GetCurrentBusId() == self.showMenuEventId) then
		self.WantsToShow = true;
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
	end
end

return uidiegeticmenu;
