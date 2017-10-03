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
		
		ScreenAnimation = {
			FadeTime = { default = 1, description = "The time taken to fade up and down the UI" },
			DataScreen = {
				WorkingTime = { default = 3, description = "The time taken to perform working" },
				SpinAmount = { default = 5, description = "The time amount of revolutions on the rotating elements" },
				WorkingToCompleteFadeTime = { default = 0.25, description = "The time spent transitioning" },
				CompleteTime = { default = 3, description = "The time taken to show complete" },
			},
			ChoiceScreen = {
				SideTransitionTime = { default = 0.5, description = "The time taken to transition sides" },
				ArrowPieceCycleTime = { default = 3, description = "The time for the upgrade arror cycle" },
				ChosenHighlightTime = { default = 3, description = "The time to show in a highlight state before transitioning off" },
			},
		},
		
		DebugStateMachine =  { default = false, description = "display debugging onif0ormation for the state machine." },
		
		Sounds = {
			Open = { default = "Play_HUD_CommsArray_InMenu", description = "The sound event to fire when the menu opens." },
			Close = { default = "Stop_HUD_CommsArray_InMenu", description = "The sound event to fire when the menu opens." },
		},
		
		ScreenHookup = {
			Model = { default = EntityId(), description = "The model that the menu is displayed upon." },
			Camera = { default = EntityId(), description = "The camera to use while in the menu." },
			PositionEntity = { default = EntityId(), description = "The entity to place the player on while in the menu." },
			-- note, the assumption here is that all the screens are taken from a common master base, so that the IDs will not change on different versions
			AllFaderID = { default = 100, description = "The ID of the health Bar slider" },
			DataScreen = {
				--HealthBarID = { default = -1, description = "The ID of the health Bar slider" },
				Layer1 = {
					LayerFaderID = { default = 5, description = "The ID of the fader for the entire layer for data items" },
					WorkingFaderID = { default = 94, description = "The ID of all working items" },
					CompleteFaderID = { default = 95, description = "The ID of all complete items" },
					ContentFaderID = { default = 104, description = "The ID of all non generic items" },
					WorkingRotatorID = { default = 14, description = "The ID of a rotating element" },
					WorkingBarID = { default = 18, description = "The ID of a bar element" },
				},
				Layer2 = {
					LayerFaderID = { default = 25, description = "The ID of the fader for the entire layer for data items" },
					WorkingFaderID = { default = 30, description = "The ID of all working items" },
					CompleteFaderID = { default = 31, description = "The ID of all complete items" },
					ContentFaderID = { default = 103, description = "The ID of all non generic items" },
					WorkingRotatorID = { default = 30, description = "The ID of a rotating element" },
				},
				Layer3 = {
					LayerFaderID = { default = 87, description = "The ID of the fader for the entire layer for data items" },
					WorkingFaderID = { default = 97, description = "The ID of all working items" },
					CompleteFaderID = { default = 98, description = "The ID of all complete items" },
					ContentFaderID = { default = 88, description = "The ID of all non generic items" },
					WorkingRotatorID = { default = 91, description = "The ID of a rotating element" },
				},
			},
			UpgradeScreen = {
			-- this has soo many things, ignoring it for now
				--HealthBarID = { default = -1, description = "The ID of the health Bar slider" },
				Layer1 = {
					HealthBarID = { default = -1, description = "The ID of the health Bar slider" },
				},
				Layer2 = {
					SliderBackgroundAID = { default = -1, description = "ID the fader for the map Background A" },
				},
				--Layer3 = {
				--	FaderAID = { default = -1, description = "ID the fader for the crosshair A" },
				--},
			},
			GenericScreen = {
				--HealthBarID = { default = -1, description = "The ID of the health Bar slider" },
				Layer1 = {
					LayerFaderID = { default = 9, description = "The ID of the fader for the entire layer for generic items" },
					ButtonID = { default = 7, description = "The ID of the button" },
				},
				Layer2 = {
					LayerFaderID = { default = 26, description = "The ID of the fader for the entire layer for generic items" },
				},
				Layer3 = {
					LayerFaderID = { default = 101, description = "The ID of the fader for the entire layer for generic items" },
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
            end,
            OnExit = function(self, sm)
				sm.UserData:EnableHud(false);

				-- prescale this down to avoid a full flash
				if (sm.UserData.modelOriginalLocalT == nil) then
					sm.UserData.modelOriginalLocalT = TransformBus.Event.GetLocalTM(sm.UserData.Properties.ScreenHookup.Model);
				end
				local trans = sm.UserData.modelOriginalLocalT * Transform.CreateScale(Vector3(1, 1, 0));

				sm.UserData:EnableHud(true);
				sm.UserData.WantsToShow = false;
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
				StarterGameUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.AllFaderID, 1, self.aimTimer);
				
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
				self.aimTimer = sm.UserData.Properties.ScreenAnimation.DataScreen.WorkingTime;

				StarterGameUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.DataScreen.Layer1.WorkingFaderID, 1, sm.UserData.Properties.ScreenAnimation.DataScreen.WorkingToCompleteFadeTime);
				StarterGameUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.DataScreen.Layer2.WorkingFaderID, 1, sm.UserData.Properties.ScreenAnimation.DataScreen.WorkingToCompleteFadeTime);
				StarterGameUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.DataScreen.Layer3.WorkingFaderID, 1, sm.UserData.Properties.ScreenAnimation.DataScreen.WorkingToCompleteFadeTime);				
			end,
            OnExit = function(self, sm)
				-- should always end on a full rotation, so zero it out
				StarterGameUtility.UIElementRotator(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.DataScreen.Layer1.WorkingRotatorID, 0);
				StarterGameUtility.UIElementRotator(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.DataScreen.Layer2.WorkingRotatorID, 0);
				StarterGameUtility.UIElementRotator(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.DataScreen.Layer3.WorkingRotatorID, 0);

				--Debug.Log("uidiegeticmenu:DataWorking exit " .. tostring(sm.UserData.Properties.ScreenAnimation.DataScreen.WorkingToCompleteFadeTime));
				StarterGameUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.DataScreen.Layer1.WorkingFaderID, 0, sm.UserData.Properties.ScreenAnimation.DataScreen.WorkingToCompleteFadeTime);
				StarterGameUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.DataScreen.Layer2.WorkingFaderID, 0, sm.UserData.Properties.ScreenAnimation.DataScreen.WorkingToCompleteFadeTime);
				StarterGameUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.DataScreen.Layer3.WorkingFaderID, 0, sm.UserData.Properties.ScreenAnimation.DataScreen.WorkingToCompleteFadeTime);				
            end,            
            OnUpdate = function(self, sm, deltaTime)
				self.aimTimer = self.aimTimer - deltaTime;
				if(self.aimTimer < 0) then
					self.aimTimer = 0;
				end
				
				-- note that the lear var goes from 1-0
				local linearVar = (self.aimTimer / sm.UserData.Properties.ScreenAnimation.DataScreen.WorkingTime);
				local rotateVar = (1 + Math.Cos(linearVar * math.pi)) * 0.5 * sm.UserData.Properties.ScreenAnimation.DataScreen.SpinAmount * 360;
			
				StarterGameUtility.UIElementRotator(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.DataScreen.Layer1.WorkingRotatorID, rotateVar);
				StarterGameUtility.UIElementRotator(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.DataScreen.Layer2.WorkingRotatorID, rotateVar);
				StarterGameUtility.UIElementRotator(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.DataScreen.Layer3.WorkingRotatorID, rotateVar);
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
				self.aimTimer = sm.UserData.Properties.ScreenAnimation.DataScreen.CompleteTime;
				--Debug.Log("uidiegeticmenu:DataComplete enter " .. tostring(sm.UserData.Properties.ScreenAnimation.DataScreen.WorkingToCompleteFadeTime));
				StarterGameUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.DataScreen.Layer1.CompleteFaderID, 1, 0);
				StarterGameUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.DataScreen.Layer2.CompleteFaderID, 1, 0);
				StarterGameUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.DataScreen.Layer3.CompleteFaderID, 1, 0);
            end,
            OnExit = function(self, sm)
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
				
				StarterGameUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.AllFaderID, 0, self.aimTimer);
								
				if(sm.UserData.Properties.Sounds.Close ~= "") then
					AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.entityId, sm.UserData.Properties.Sounds.Close);
				end
			end,
            OnExit = function(self, sm)
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.releaseCharacterEventId, true);
				
				-- send event to player to end interaction (enable controls, select player camera)
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.endInteractEventId, nil);
				
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.menuFinishedEventId, true);
            end,            
            OnUpdate = function(self, sm, deltaTime)
				self.aimTimer = self.aimTimer - deltaTime;
				if(self.aimTimer < 0) then
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
            end,
            OnExit = function(self, sm)
            end,            
            OnUpdate = function(self, sm, deltaTime)
				self.aimTimer = self.aimTimer - deltaTime;
				if(self.aimTimer < 0) then
					self.aimTimer = 0;
				end

				local scaleVar = Vector2(1, (self.aimTimer / sm.UserData.Properties.ScreenAnimation.FadeTime));
				-- scale all the layers down
				StarterGameUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.DataScreen.Layer1.ContentFaderID, scaleVar);
				StarterGameUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.DataScreen.Layer2.ContentFaderID, scaleVar);
				StarterGameUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.DataScreen.Layer3.ContentFaderID, scaleVar);
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
            end,
            OnExit = function(self, sm)
            end,            
            OnUpdate = function(self, sm, deltaTime)
            end,
            Transitions =
            {
				ChoiceIdle = 
				{
                    Evaluate = function(state, sm)
						return state.aimTimer == 0;
					end	
				}
            },
        },	
		ChoiceIdle =
		{
			OnEnter = function(self, sm)
			end,
            OnUpdate = function(self, sm, deltaTime)
            end,
            Transitions =
            {
 				ChoiceSelected = 
				{
                    Evaluate = function(state, sm)
						return state.aimTimer == 0;
					end	
				}
           },
		},
		ChoiceSelected =
		{
			OnEnter = function(self, sm)

			end,
            OnUpdate = function(self, sm, deltaTime)
				
            end,
            Transitions =
            {
 				ChoiceClosing = 
				{
                    Evaluate = function(state, sm)
						return state.aimLerpTime == 0;
					end	
				} 
            },
		},
		ChoiceClosing =
		{
			OnEnter = function(self, sm)
				if(sm.UserData.Properties.Sounds.Close ~= "") then
					AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.entityId, sm.UserData.Properties.Sounds.Close);
				end
			end,
            OnExit = function(self, sm)
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.releaseCharacterEventId, true);
				
				-- send event to player to end interaction (enable controls, select player camera)
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.endInteractEventId, nil);
				
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.menuFinishedEventId, true);
            end,            
			OnUpdate = function(self, sm, deltaTime)			
            end,
            Transitions =
            {
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
	
	local entityPlayer = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"));
	self.startInteractEventId = GameplayNotificationId(entityPlayer, "StartInteract", "float");
	self.endInteractEventId = GameplayNotificationId(entityPlayer, "EndInteract", "float");
	
	
	self.WantsToShow = false;
	self.WantsToShowChoice = false;
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
	
	self.tickHandler:Disconnect();
	self.tickHandler = nil;

	self:EnableHud(false);
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
		self.WantsToShowChoice = (value == 1);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.menuRightEventId) then
		self.RightOption = true;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.menuLeftEventId) then
		self.RightOption = false;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.menuContinueEventId) then
		self.ContinuePressed = true;
	end
end

return uidiegeticmenu;
