local StateMachine = require "scripts/common/StateMachine"
local utilities = require "scripts/common/utilities"
local uiinteract =
{
	Properties =
	{
		InitiallyInteractable = { default = true, description = "can i be interacted with when created" },
		DisableOnInteract = { default = true, description = "Once i have been interacted with disable me from further interaction?" },
		PlayerTag = {default = "PlayerCharacter", description = "The tag for the player entity that will interact with us" },

		InteractRange = { default = 3, description = "The horizontal distance that the player has to be within to interact" },
		InteractHeight = { default = 2, description = "The vertical distance that the player's feet have to be within to interact" },
		InteractAngle = { default = 120, description = "The angle the player has to be infront of in order to interact (infront of me)." },
		ShowRange = { default = 10, description = "The distance that the player has to be within to show that i am an interactable" },
		DisableRange = { default = 20, description = "The distance that i will disableMy UI entirely" }, -- this is to stop loads of UI's being active at the same time
		
		InteractGraphicLocation  = { default = EntityId(), description = "If specified this will be the location of the graphic for the interact" },
		
		InteractedSound = {default = "Play_HUD_commsarray_enterHUD", description = "The sound to play when interaction happens" },
		
		DebugStateMachine = {default = false, description = "debug the interact state machine" },
		
		Messages = {
			HideUIEventIn = { default = "HideUIEvent", description = "The event used to set the whole UI visible / invisible" },
			
			EnableInteract = { default = "EnableInteract", description = "The event used to enable/disable the interaction" },
			InteractPressed = { default = "InteractPressed", description = "The interact Button Has Been Pressed" },			
			
			OnInteractComplete = { default = "Interacted", description = "The message to fire when i am interacted with" },
		},
		
		ScreenAnimation = {
			Button = {
				CycleTime  = { default = 1, description = "The time for the active anim cycle" },
				ScaleAmount = { default = 0.75, description = "The amount that the active ring scales down by" },
			},
			InteracedShownTime = { default = 0.25, description = "The amount that interacted state is shown" },
			TransitionTime = { default = 0.5, description = "The amount of time spent transitioning (mainly graphical)" },
		},
		
		ScreenHookup = {
			AllUIFaderID = { default = 6, description = "The ID of the All UI fader" },
			InteractMover = { default = 8, description = "The ID of object that we move to place it in the correct place" },
			DistanceFader = { default = 3, description = "The ID of the fader to show when you are close enough to see the interact" },
			IdleFader = { default = 21, description = "The ID of the fader to show when you are close enough to see the interact" },
			CanInteractFader = { default = 5, description = "The ID of the fader to show when you are actually close enough to interact" },
			AnimatedRing = { default = 17, description = "The ID of the animated interact ring" },
			PressedFader = { default = 20, description = "The ID of fader to show when i have been pressed." },
		},
	},
	
	States = 
    {
        Inactive = -- screen not loaded, mainly here to make sure i dont load hundreds of interactable screens at the same time
        {      
            OnEnter = function(self, sm)
				sm.UserData:EnableHud(false);
            end,
            OnExit = function(self, sm)				
				sm.UserData:EnableHud(true);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.IdleFader, 0.0, 0);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.DistanceFader, 0.0, 0);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.CanInteractFader, 0.0, 0);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.PressedFader, 0.0, 0);
			end,            
            OnUpdate = function(self, sm, deltaTime)
            end,
            Transitions =
            {
				Active = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData:InsideEnabledRange() and sm.UserData.interactEnabled;
                    end	
				}
            },
        },
        Active = -- all things loaded
        {      
            OnEnter = function(self, sm)
				local transitionTime = sm.UserData.Properties.ScreenAnimation.TransitionTime;
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.IdleFader, 0.0, transitionTime);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.DistanceFader, 0.0, transitionTime);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.CanInteractFader, 0.0, transitionTime);
 				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.PressedFader, 0.0, transitionTime);
			end,
            OnExit = function(self, sm)				
			end,            
            OnUpdate = function(self, sm, deltaTime)

            end,
            Transitions =
            {
				Inactive = 
				{
                    Evaluate = function(state, sm)
						return (not sm.UserData:InsideEnabledRange());
                    end	
				},
				Shown = 
				{
                    Evaluate = function(state, sm)
						return (sm.UserData:InsideShowRange() and sm.UserData.onScreen);
                    end	
				}
            },
        },
        Shown = -- i can see that there is an interactable
        {      
            OnEnter = function(self, sm)
				local transitionTime = sm.UserData.Properties.ScreenAnimation.TransitionTime;
				InteracedShownTime = { default = 0.5, description = "The amount that interacted state is shown" },
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.IdleFader, 1.0, transitionTime);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.DistanceFader, 1.0, transitionTime);
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.CanInteractFader, 0.0, transitionTime);
            end,
            OnExit = function(self, sm)		
				local transitionTime = sm.UserData.Properties.ScreenAnimation.TransitionTime;
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.IdleFader, 0.0, transitionTime);			
			end,            
            OnUpdate = function(self, sm, deltaTime)
				StarterGameUIUtility.UIElementMover(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.InteractMover, sm.UserData.posOnScreen);
            end,
            Transitions =
            {
				Active = 
				{
                    Evaluate = function(state, sm)
						return ((not sm.UserData:InsideShowRange()) or (not sm.UserData.onScreen));
                    end	
				},
				Interactable = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData:InsideInteractRange();
                    end	
				}
            },
        },
        Interactable = -- i can interact with it
        {      
            OnEnter = function(self, sm)
				local transitionTime = sm.UserData.Properties.ScreenAnimation.TransitionTime;
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.CanInteractFader, 1.0, transitionTime);
				self.timer = 0;
				sm.UserData.interactable = true;
            end,
            OnExit = function(self, sm)	
				local transitionTime = sm.UserData.Properties.ScreenAnimation.TransitionTime;
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.CanInteractFader, 0.0, transitionTime);			
				sm.UserData.interactable = false;
			end,
			OnUpdate = function(self, sm, deltaTime)
				StarterGameUIUtility.UIElementMover(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.InteractMover, sm.UserData.posOnScreen);
				
				local bt = sm.UserData.Properties.ScreenAnimation.Button.CycleTime;
				self.timer = self.timer + deltaTime;
				if (self.timer >= bt) then
					self.timer = self.timer - bt;
				end
				local circleScale = utilities.Clamp((self.timer / bt), 0.0, 1.0)
				local circleFade = 1 - circleScale;
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.AnimatedRing, circleFade, 0);
				local scaleVar = utilities.Lerp(1, sm.UserData.Properties.ScreenAnimation.Button.ScaleAmount, circleScale);
				local scaleVec = Vector2(scaleVar, scaleVar);
				StarterGameUIUtility.UIElementScaler(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.AnimatedRing, scaleVec);
            end,
            Transitions =
            {
				Active = 
				{
                    Evaluate = function(state, sm)
						return ((not sm.UserData:InsideShowRange()) or (not sm.UserData.onScreen));
                    end	
				},
				Interacted = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.interacted;
                    end	
				},
				Shown = 
				{
                    Evaluate = function(state, sm)
						return (not sm.UserData:InsideInteractRange());
                    end	
				}
            },
        },
        Interacted = -- i have interacted, show that fact
        {      
            OnEnter = function(self, sm)
				local transitionTime = sm.UserData.Properties.ScreenAnimation.InteracedShownTime;
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.PressedFader, 1.0, transitionTime);	
				self.timer = 0;
				sm.UserData.interacted = false;
				if (sm.UserData.Properties.DisableOnInteract) then
					sm.UserData.interactEnabled = false;
				end
            end,
            OnExit = function(self, sm)
				local transitionTime = sm.UserData.Properties.ScreenAnimation.InteracedShownTime;
				StarterGameUIUtility.UIFaderControl(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.PressedFader, 0.0, transitionTime);	
				if(sm.UserData.Properties.DisableOnInteract) then
					sm.UserData:SetInteractEnabled(false);
				end				
				
				-- i have interacted, send the message
				local eventId = GameplayNotificationId(sm.UserData.entityId, sm.UserData.Properties.Messages.OnInteractComplete, "float");
				GameplayNotificationBus.Event.OnEventBegin(eventId, sm.UserData.entityId);
			end,            
            OnUpdate = function(self, sm, deltaTime)
				StarterGameUIUtility.UIElementMover(sm.UserData.canvasEntityId, sm.UserData.Properties.ScreenHookup.InteractMover, sm.UserData.posOnScreen);
				self.timer = self.timer + deltaTime;
            end,
            Transitions =
            {
				Interactable = 
				{
                    Evaluate = function(state, sm)
						return ((state.timer >= sm.UserData.Properties.ScreenAnimation.InteracedShownTime) and (not sm.UserData.Properties.DisableOnInteract));
                    end	
				},
				Inactive = 
				{
                    Evaluate = function(state, sm)
						return ((state.timer >= sm.UserData.Properties.ScreenAnimation.InteracedShownTime) and sm.UserData.Properties.DisableOnInteract);
                    end	
				}
            },
        },
	},
}

function uiinteract:OnActivate()
	--Debug.Log("uiinteract:OnActivate()");
	self.enabled = false;			-- ui is enabled
	self.interactEnabled = false 	-- i am active for interaction
	self.interactable = false;		-- they are close enough to actually interact with me
	self.interacted = false;		-- have interacted
	self.onScreen = false;			-- i am on screen
	self.posOnScreen = Vector2(0.0, 0.0);	-- cached screen pos
	self.interactAngleCos = Math.Cos(Math.DegToRad(self.Properties.InteractAngle / 2));
	
	-- Listen for enable/disable events.
	self.HideUIEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.HideUIEventIn, "float");
	self.HideUIHandler = GameplayNotificationBus.Connect(self, self.HideUIEventId);
	self.enableInteractEventId = GameplayNotificationId(self.entityId, self.Properties.Messages.EnableInteract, "float");
	self.enableInteractHandler = GameplayNotificationBus.Connect(self, self.enableInteractEventId);
	
	self.performedFirstUpdate = false;
	self.tickBusHandler = TickBus.Connect(self);
	
	-- state machine hookup
    self.StateMachine = {}
    setmetatable(self.StateMachine, StateMachine);
    self.StateMachine:Start("UIinteract", self.entityId, self, self.States, false, "Inactive", self.Properties.DebugStateMachine);
end

function uiinteract:getPlayerID()
	if(self.playerID == nil) then
		self.playerID = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32(self.Properties.PlayerTag));
	end
	return self.playerID;
end
-- this means that i can be interacted with, turns on and off interaction of an object
function uiinteract:SetInteractEnabled(enabled)
	if(self.interactEnabled == enabled) then
		return;
	end
	--Debug.Log("uiinteract:SetInteractEnabled() enabled:" .. tostring(self.interactEnabled));
	
	self.interactEnabled = enabled;
	
	if(enabled) then
		self.tickBusHandler = TickBus.Connect(self);
		self.interactPressedEventId = GameplayNotificationId(self:getPlayerID(), self.Properties.Messages.InteractPressed, "float");
		self.interactPressedHandler = GameplayNotificationBus.Connect(self, self.interactPressedEventId);
	else
		self:EnableHud(false);
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
		self.interactPressedHandler:Disconnect();
		self.interactPressedHandler = nil;
	end
end

function uiinteract:OnDeactivate()
	self:SetInteractEnabled(false);
	self:EnableHud(false);
	
	if(not self.HideUIHandler == nil) then
		self.HideUIHandler:Disconnect();
		self.HideUIHandler = nil;
	end
	if(not self.enableInteractHandler == nil) then
		self.enableInteractHandler:Disconnect();
		self.enableInteractHandler = nil;
	end
	if(not self.interactPressedHandler == nil) then
		self.interactPressedHandler:Disconnect();
		self.interactPressedHandler = nil;
	end
		
	self.StateMachine:Stop();
end

function uiinteract:OnTick(deltaTime, timePoint)
	if (not self.performedFirstUpdate) then
		-- Disconnect first as the 'SetInteractEnabled()' call should be what determines whether
		-- we're connected to the tick bus or not.
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
		
		-- This is done here instead of 'OnActivate()' because we can't enforce the player being
		-- created before this object is activated.
		self:SetInteractEnabled(self.Properties.InitiallyInteractable);
		
		self.performedFirstUpdate = true;
		
		-- If we didn't reconnect then don't perform the rest of the update.
		if (self.tickBusHandler == nil) then
			return;
		end
	end
	
	self.onScreen = self:OnScreen(self.posOnScreen);

	--self:UpdateScreenPrompt(false);
end

function uiinteract:OnScreen(posOnScreen)
	local onscreen = false;
	if (self.Properties.InteractGraphicLocation:IsValid()) then
		onscreen = StarterGameUIUtility.GetElementPosOnScreen(self.Properties.InteractGraphicLocation, posOnScreen);
	else
		onscreen = StarterGameUIUtility.GetElementPosOnScreen(self.entityId, posOnScreen);
	end
	return onscreen;
end

function uiinteract:InsideEnabledRange()
	local playerTm = TransformBus.Event.GetWorldTM(self:getPlayerID());
	local myTm = TransformBus.Event.GetWorldTM(self.entityId);
	local toVector = (myTm:GetTranslation() - playerTm:GetTranslation());
	local range = toVector:GetLength();
	--Debug.Log("uiinteract:InsideEnabledRange() distance to player:" .. tostring(range));
	return (range <= self.Properties.DisableRange);
end

function uiinteract:InsideShowRange()
	local playerTm = TransformBus.Event.GetWorldTM(self:getPlayerID());
	local myTm = TransformBus.Event.GetWorldTM(self.entityId);
	local toVector = (myTm:GetTranslation() - playerTm:GetTranslation());
	local range = toVector:GetLength();
	return (range <= self.Properties.ShowRange);
end

function uiinteract:InsideInteractRange()
	local playerTm = TransformBus.Event.GetWorldTM(self:getPlayerID());
	local myTm = TransformBus.Event.GetWorldTM(self.entityId);
	local toVector = (myTm:GetTranslation() - playerTm:GetTranslation());
	local heightDiff = toVector.z;
	toVector.z = 0.0;
	local range = toVector:GetLength();
	-- normalize
	toVector = toVector / range;
	
	local insideHeightRange = ((heightDiff >= 0.0) and (heightDiff <= self.Properties.InteractHeight));
	local insideInteractRange = (range <= self.Properties.InteractRange);
	local angleDot = playerTm:GetColumn(1):GetNormalized():Dot(toVector);
	local insideInteractAngle = (angleDot >= self.interactAngleCos);
	return (insideHeightRange and insideInteractRange and insideInteractAngle);
end

function uiinteract:EnableHud(enabled)
	if (enabled == self.enabled) then
		return;
	end
	
	--Debug.Log("uiinteract:EnableHud()" .. tostring(enabled));

	if (enabled) then
		-- IMPORTANT: The 'canvas ID' is different to 'self.entityId'.
		self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("UI/Canvases/UIInteractIdentifier.uicanvas");
	elseif (self.canvasEntityId ~= nil) then
		UiCanvasManagerBus.Broadcast.UnloadCanvas(self.canvasEntityId);
		self.canvasEntityId = nil;
	end
	self.enabled = enabled;
end

function uiinteract:HideHud(enabled)
	if (self.enabled == false) then
		return;
	end
	--Debug.Log("uiinteract:HideHud(): " .. tostring(enabled));
	
	local fadeValue = 0;
	if (not enabled) then
		fadeValue = 1;
	end
	StarterGameUIUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.AllUIFaderID, fadeValue, 1.0);	
end

function uiinteract:InteractAttempt()
	--Debug.Log("uiinteract:InteractAttempt() for " .. tostring(self.entityId) .. " enabled:" .. tostring(self.enabled) .. "  interactable:".. tostring(self.interactable));
	if(self.enabled and self.interactable) then
		self.interacted = true;
		if(self.Properties.InteractedSound ~= "") then
			AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.Properties.InteractedSound);
		end
	end
end


function uiinteract:OnEventBegin(value)
	--Debug.Log("uiinteract:OnEventBegin : " .. tostring(value));
	if (GameplayNotificationBus.GetCurrentBusId() == self.activateEventId) then
		--Debug.Log("uiinteract:OnEventBegin EnableHud: " .. tostring(value));
		self:EnableHud(value == 1);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.HideUIEventId) then
		--Debug.Log("uiinteract:OnEventBegin HideHud: " .. tostring(value));
		self:HideHud(value == 1);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.enableInteractEventId) then
		--Debug.Log("uiinteract:OnEventBegin SetInteractEnabled: " .. tostring(value));
		self:SetInteractEnabled(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.interactPressedEventId) then
		--Debug.Log("uiinteract:OnEventBegin InteractAttempt: " .. tostring(value));
		self:InteractAttempt();
	end
end

return uiinteract;
