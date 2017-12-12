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
		
		InteractSound = {default = "", description = "The sound to play when interaction happens" },
		
		Messages = {
			HideUIEventIn = { default = "HideUIEvent", description = "The event used to set the whole UI visible / invisible" },
			
			EnableInteract = { default = "EnableInteract", description = "The event used to enable/disable the interaction" },
			InteractPressed = { default = "InteractPressed", description = "The interact Button Has Been Pressed" },			
			
			OnInteractComplete = { default = "Interacted", description = "The message to fire when i am interacted with" },
		},
		
		ScreenHookup = {
			AllUIFaderID = { default = 6, description = "The ID of the All UI fader" },
			CloseFader = { default = 5, description = "The ID of the fader to show when you are actually close enough to interact" },
			InteractFader = { default = 3, description = "The ID of the fader to show when you are pressing to interact" },
			InteractMover = { default = 8, description = "The ID of the fader to show when you are pressing to interact" },
		},
	},
}

function uiinteract:OnActivate()
	--Debug.Log("uiinteract:OnActivate()");
	
	self.enabled = false;			-- ui is enabled
	self.interactEnabled = false 	-- i am active for interaction
	self.showing = false;			-- i am showing myself, they are close renough to see that i exist
	self.interactable = false;		-- they are close enough to actually interact with me
	self.interactAngleCos = Math.Cos(Math.DegToRad(self.Properties.InteractAngle / 2));
	
	self.playerID = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32(self.Properties.PlayerTag));
	
	-- Listen for enable/disable events.
	self.HideUIEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.HideUIEventIn, "float");
	self.HideUIHandler = GameplayNotificationBus.Connect(self, self.HideUIEventId);
	self.enableInteractEventId = GameplayNotificationId(self.entityId, self.Properties.Messages.EnableInteract, "float");
	self.enableInteractHandler = GameplayNotificationBus.Connect(self, self.enableInteractEventId);
	
	self:SetInteractEnabled(self.Properties.InitiallyInteractable);
end

-- this meas that i can be interacted with, turns on and off interaction of an object
function uiinteract:SetInteractEnabled(enabled)
	if(self.interactEnabled == enabled) then
		return;
	end
	--Debug.Log("uiinteract:SetInteractEnabled() enabled:" .. tostring(self.interactEnabled));
	
	self.interactEnabled = enabled;
	
	if(enabled) then
		self.tickBusHandler = TickBus.Connect(self);
		self.interactPressedEventId = GameplayNotificationId(self.playerID, self.Properties.Messages.InteractPressed, "float");
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
		
end

function uiinteract:OnTick(deltaTime, timePoint)
	self:UpdateScreenPrompt(false);
end

function uiinteract:UpdateScreenPrompt(hardSet)
	local transitionTime = 1.0;
	if(hardSet) then
		transitionTime = 0.0;
	end
	
	local posOnScreen = Vector2(0.0, 0.0);
	local onscreen = false;
	if (self.Properties.InteractGraphicLocation:IsValid()) then
		onscreen = StarterGameUtility.GetElementPosOnScreen(self.Properties.InteractGraphicLocation, posOnScreen);
	else
		onscreen = StarterGameUtility.GetElementPosOnScreen(self.entityId, posOnScreen);
	end
	if(onscreen) then
		--Debug.Log("uiinteract:UpdateScreenPrompt() pos on screen:" .. tostring(posOnScreen));
		
		-- get the distance from the character
		local playerTm = TransformBus.Event.GetWorldTM(self.playerID);
		local myTm = TransformBus.Event.GetWorldTM(self.entityId);
		local toVector = (myTm:GetTranslation() - playerTm:GetTranslation());
		local range = toVector:GetLength();
		--Debug.Log("uiinteract:UpdateScreenPrompt() distance to player:" .. tostring(range));
		if(range >= self.Properties.DisableRange) then
			self:EnableHud(false);
		else
			local insideShowRange = (range <= self.Properties.ShowRange);
			if(self.showing ~= insideShowRange) then
				self.showing = insideShowRange;
				if(insideShowRange) then
					self:EnableHud(true);
					--Debug.Log("uiinteract:UpdateScreenPrompt() insideShowRange");
					StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.InteractFader, 1.0, transitionTime);	
				elseif (self.enabled) then
					--Debug.Log("uiinteract:UpdateScreenPrompt() insideShowRange NOT");
					StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.InteractFader, 0.0, transitionTime);
				end
			end	
		end
		
		if (self.enabled) then
			StarterGameUtility.UIElementMover(self.canvasEntityId, self.Properties.ScreenHookup.InteractMover, posOnScreen);
		
			local heightDiff = toVector.z;
			-- flatten
			toVector.z = 0.0;
			range = toVector:GetLength();
			-- normalize
			toVector = toVector / range;
			
			local insideHeightRange = ((heightDiff >= 0.0) and (heightDiff <= self.Properties.InteractHeight));
			local insideInteractRange = (range <= self.Properties.InteractRange);
			local angleDot = playerTm:GetColumn(1):GetNormalized():Dot(toVector);
			local insideInteractAngle = (angleDot >= self.interactAngleCos);
			local isInteractable = (insideHeightRange and insideInteractRange and insideInteractAngle);
			--Debug.Log("uiinteract:insideInteractAngle testVar:" .. tostring(self.interactAngleCos) .. " angleDot:" .. tostring(angleDot) .. " hit:" .. tostring(insideInteractAngle) .. " DistCheck:" .. tostring(isInteractable));
			local range = (playerTm:GetTranslation() - myTm:GetTranslation()):GetLength();
			if(self.interactable ~= isInteractable) then
				self.interactable = isInteractable;
				if(isInteractable) then
					--Debug.Log("uiinteract:UpdateScreenPrompt() insideInteractRange");
					StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.CloseFader, 1.0, transitionTime);	
				else
					--Debug.Log("uiinteract:UpdateScreenPrompt() insideInteractRange NOT");
					StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.CloseFader, 0.0, transitionTime);	
				end
			end
		end
	elseif (self.enabled and self.showing) then
		--Debug.Log("uiinteract:UpdateScreenPrompt() offscreen");
		self.showing = false;
		StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.InteractFader, 0.0, transitionTime);	
		self.interactable = false;
		StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.CloseFader, 0.0, transitionTime);	
	end
end

function uiinteract:EnableHud(enabled)
	if (enabled == self.enabled) then
		return;
	end
	
	--Debug.Log("uiinteract:EnableHud()" .. tostring(enabled));
	self.showing = false;
	self.interactable = false;

	
	if (enabled) then
		-- IMPORTANT: The 'canvas ID' is different to 'self.entityId'.
		self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("UI/Canvases/UIInteractIdentifier.uicanvas");
	else
		UiCanvasManagerBus.Broadcast.UnloadCanvas(self.canvasEntityId);
	end
	self.enabled = enabled;

end

function uiinteract:HideHud(enabled)
	if (self.enabled == false) then
		return;
	end
	--Debug.Log("uiinteract:HideHud(): " .. tostring(enabled));
	
	local fadeValue = 0;
	if(not enabled) then
		fadeValue = 1;
	end
	StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.AllUIFaderID, fadeValue, 1.0);	
end

function uiinteract:InteractAttempt()
	--Debug.Log("uiinteract:InteractAttempt() enabled:" .. tostring(self.enabled) .. "  interactable:".. tostring(self.interactable));
	if(self.enabled and self.interactable) then
		-- i have interacted, send the message
		local eventId = GameplayNotificationId(self.entityId, self.Properties.Messages.OnInteractComplete, "float");
		GameplayNotificationBus.Event.OnEventBegin(eventId, self.entityId);
		
		if(self.Properties.InteractSound ~= "") then
			AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.Properties.InteractSound);
		end

		-- disable me if i have the settings to do so
		if(self.Properties.DisableOnInteract) then
			self:SetInteractEnabled(false);
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
