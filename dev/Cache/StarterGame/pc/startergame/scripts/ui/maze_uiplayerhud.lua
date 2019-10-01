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

local uiplayerhud =
{
	Properties =
	{
		--CodeEntity = { default = EntityId() }, -- to be removed
		
		Messages = {
			EnableUIEvent = { default = "EnableUIEvent", description = "The event used to enable/disable the crosshair (1 to enable; anything else disables)" },
			HideUIEventIn = { default = "HideUIEvent", description = "The event used to set the whole UI visible / invisible" },
			
			EnableCrosshairEvent = { default = "EnableCrosshairEvent", description = "The event used to enable/disable the crosshair (1 to enable; anything else disables)" },
			HealthChangedEventIn = { default = "HealthChanged", description = "The event used to set the health percent (0%-100%)" },
			SetEnergyEventIn = { default = "SetEnergyEvent", description = "The event used to set the Energy percent (0%-100%)" },
			SetMapValueEventIn = { default = "SetMapValueEvent", description = "set the position of the map" },
			SetCrosshairEventIn = { default = "SetCrosshairEvent", description = "set the visible crosshair" },
			SetCrosshairTargetEventIn = { default = "SetCrosshairTargetEvent", description = "set the visible crosshair Type" },
		},
		
		ScreenHookup = {
			AllUIFaderID = { default = -1, description = "The ID of the All UI fader" },
			StatusBars = {
				HealthBarID = { default = -1, description = "The ID of the health Bar slider" },
				EnergyBarID = { default = -1, description = "The ID of the energy Bar slider" },
				
				HealthFlasherID = { default = -1, description = "The ID of the Health Bar full fader" },
				HealthLowFlasherID = { default = -1, description = "The ID of the Health Bar low fader" },
				EnergyFlasherID = { default = -1, description = "The ID of the Energy Bar full fader" },
				EnergyLowFlasherID = { default = -1, description = "The ID of the Energy Bar low fader" },
			},
			MapBar = {
				SliderBackgroundAID = { default = -1, description = "ID the fader for the map Background A" },
				SliderBackgroundBID = { default = -1, description = "ID the fader for the map Background B" },
			},
			CrossHairs = {
				FaderAID = { default = -1, description = "ID the fader for the crosshair A" },
				FaderBID = { default = -1, description = "ID the fader for the crosshair B" },
				FaderANormalID = { default = -1, description = "ID the fader for the crosshair A for danger targets" },
				FaderBNormalID = { default = -1, description = "ID the fader for the crosshair B for danger targets" },
				FaderATargetID = { default = -1, description = "ID the fader for the crosshair A for danger targets" },
				FaderBTargetID = { default = -1, description = "ID the fader for the crosshair B for danger targets" },
			},
		},
	},
}

function uiplayerhud:OnActivate()

	self.enabled = false;
	
	--Debug.Log("uiplayerhud:OnActivate()");
	
	self.playerID = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"));
	
	-- Listen for enable/disable events.
	self.activateUIEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.EnableUIEvent, "float");
	self.activateUIHandler = GameplayNotificationBus.Connect(self, self.activateUIEventId);
	self.HideUIEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.HideUIEventIn, "float");
	self.HideUIHandler = GameplayNotificationBus.Connect(self, self.HideUIEventId);
	self.activateCrosshairEventId = GameplayNotificationId(self.entityId, self.Properties.Messages.EnableCrosshairEvent, "float");
	self.activateCrosshairHandler = GameplayNotificationBus.Connect(self, self.activateCrosshairEventId);
	
	-- Listen for value set events.
	self.HealthChangedEventId = GameplayNotificationId(self.playerID, self.Properties.Messages.HealthChangedEventIn, "float");
	self.setHealthHandler = GameplayNotificationBus.Connect(self, self.HealthChangedEventId);
	self.setEnergyEventId = GameplayNotificationId(self.entityId, self.Properties.Messages.SetEnergyEventIn, "float");
	self.setEnergyHandler = GameplayNotificationBus.Connect(self, self.setEnergyEventId);
	self.setMapValueEventId = GameplayNotificationId(self.entityId, self.Properties.Messages.SetMapValueEventIn, "float");
	self.setMapValueHandler = GameplayNotificationBus.Connect(self, self.setMapValueEventId);
	self.setCrosshairEventId = GameplayNotificationId(self.entityId, self.Properties.Messages.SetCrosshairEventIn, "float");
	self.setCrosshairHandler = GameplayNotificationBus.Connect(self, self.setCrosshairEventId);
	self.setCrosshairTargetEventId = GameplayNotificationId(self.entityId, self.Properties.Messages.SetCrosshairTargetEventIn, "float");
	self.setCrosshairTargetHandler = GameplayNotificationBus.Connect(self, self.setCrosshairTargetEventId);
		
	self.EnergyLastValue = 100;
	self.EnergyFadeTime = 0;
	self.EnergyLowFadeTime = 0;
	self.HealthLastValue = 100;
	self.HealthFadeTime = 0;
	self.HealthLowFadeTime = 0;
	self.ObjectiveFaderTime1 = 0;
	self.ObjectiveFaderTime2 = 0;
	self.ObjectiveFaderTime3 = 0;
	
	self:EnableHud(true);
	self:SetEnergy(100);
	
	--Debug.Log("UIPlayerHUD ID: " .. tostring(self.entityId));
	
	self.tickHandler = TickBus.Connect(self);
end

function uiplayerhud:OnDeactivate()

	self.activateUIHandler:Disconnect();
	self.activateUIHandler = nil;
	self.HideUIHandler:Disconnect();
	self.HideUIHandler = nil;
	self.activateCrosshairHandler:Disconnect();
	self.activateCrosshairHandler = nil;
	self.setHealthHandler:Disconnect();
	self.setHealthHandler = nil;
	self.setEnergyHandler:Disconnect();
	self.setEnergyHandler = nil;
	self.setMapValueHandler:Disconnect();
	self.setMapValueHandler = nil;
	self.setCrosshairHandler:Disconnect();
	self.setCrosshairHandler = nil;
	self.setCrosshairTargetHandler:Disconnect();
	self.setCrosshairTargetHandler = nil;
	
	self.tickHandler:Disconnect();
	self.tickHandler = nil;

	self:EnableHud(false);
end

function uiplayerhud:OnTick(deltaTime, timePoint)
		
	local fadeValue = 0;
	
	-- flash health Full
	if (self.HealthFadeTime > 0.0) then
		self.HealthFadeTime = self.HealthFadeTime - deltaTime;
		if (self.HealthFadeTime <= 0.0) then
			fadeValue = 0;
		elseif (self.HealthFadeTime <= 0.5) then
			fadeValue = self.HealthFadeTime * 2.0;
		else
			fadeValue = (1.0 - self.HealthFadeTime) * 2.0;
		end
		StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.StatusBars.HealthFlasherID, fadeValue, 0);
	end

	-- flash health low
	self.HealthLowFadeTime = self.HealthLowFadeTime - deltaTime;
	if (self.HealthLowFadeTime < 0) then
		if (self.HealthLastValue < 25.0) then
			self.HealthLowFadeTime = self.HealthLowFadeTime + 1.0;
		else
			self.HealthLowFadeTime = 0;
		end
	else
		self.HealthLowFadeTime = 0;
	end
	if (self.HealthLowFadeTime <= 0.0) then
		fadeValue = 0;
	elseif (self.HealthLowFadeTime <= 0.5) then
		fadeValue = self.HealthLowFadeTime * 2.0;
	else
		fadeValue = (1.0 - self.HealthLowFadeTime) * 2.0;
	end
	StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.StatusBars.HealthLowFlasherID, fadeValue, 0);

	-- flash energy Full
	if (self.EnergyFadeTime > 0.0) then
		self.EnergyFadeTime = self.EnergyFadeTime - deltaTime;
		if (self.EnergyFadeTime <= 0.0) then
			fadeValue = 0;
		elseif (self.EnergyFadeTime <= 0.5) then
			fadeValue = self.EnergyFadeTime * 2.0;
		else
			fadeValue = (1.0 - self.EnergyFadeTime) * 2.0;
		end
		StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.StatusBars.EnergyFlasherID, fadeValue, 0);
	end

	-- flash energy low
	self.EnergyLowFadeTime = self.EnergyLowFadeTime - deltaTime;
	if (self.EnergyLowFadeTime <= 0) then
		if (self.EnergyLastValue < 25.0) then
			self.EnergyLowFadeTime = self.EnergyLowFadeTime + 1.0;
		else
			self.EnergyLowFadeTime = 0;
		end
	else
		self.EnergyLowFadeTime = 0;
	end
	if (self.EnergyLowFadeTime <= 0.0) then
		fadeValue = 0;
	elseif (self.EnergyLowFadeTime <= 0.5) then
		fadeValue = self.EnergyLowFadeTime * 2.0;
	else
		fadeValue = (1.0 - self.EnergyLowFadeTime) * 2.0;
	end
	StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.StatusBars.EnergyLowFlasherID, fadeValue, 0);
		
end

function uiplayerhud:EnableHud(enabled)

	if (enabled == self.enabled) then
		return;
	end
	
	--Debug.Log("uiplayerhud:EnableHud()" .. tostring(enabled));
	
	if (enabled) then
		-- IMPORTANT: The 'canvas ID' is different to 'self.entityId'.
		self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("UI/Canvases/UIPlayerHUD.uicanvas");

		-- Listen for action strings broadcast by the canvas.
		self.uiCanvasNotificationLuaBusHandler = UiCanvasNotificationLuaBus.Connect(self, self.canvasEntityId);
		
		--Debug.Log("uiplayerhud:EnableHud() setup");
	else
		UiCanvasManagerBus.Broadcast.UnloadCanvas(self.canvasEntityId);

		self.uiCanvasNotificationLuaBusHandler:Disconnect();
		self.uiCanvasNotificationLuaBusHandler = nil;
	end
	self.enabled = enabled;

end

function uiplayerhud:HideHud(enabled)
	if (self.enabled == false) then
		return;
	end
	--Debug.Log("uiplayerhud:HideHud(): " .. tostring(enabled));
	
	local fadeValue = 0;
	if(not enabled) then
		fadeValue = 1;
	end
	StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.AllUIFaderID, fadeValue, 1.0);	
end

function uiplayerhud:SetHealth(value)
	if (self.enabled == false) then
		return;
	end
	
	if (self.HealthLastValue ~= value) then
		--Debug.Log("SetHealth " .. value);
		if ((value == 100.0) and (self.HealthLastValue ~= 100.0)) then
			-- hit max health, do that flash
			self.HealthFadeTime = 1.0;
		end
		self.HealthLastValue = value;
		StarterGameUtility.UISliderControl(self.canvasEntityId, self.Properties.ScreenHookup.StatusBars.HealthBarID, value);	
	end
end

function uiplayerhud:SetEnergy(value)
	if (self.enabled == false) then
		return;
	end
	
	if(value ~= self.EnergyLastValue) then
		--Debug.Log("SetEnergy " .. value);
		if ((value == 100.0) and (self.EnergyLastValue ~= 100.0)) then
			-- hit max health, do that flash
			self.EnergyFadeTime = 1.0;
		end
		self.EnergyLastValue = value;
		StarterGameUtility.UISliderControl(self.canvasEntityId, self.Properties.ScreenHookup.StatusBars.EnergyBarID, value);	
	end
end

function uiplayerhud:SetMapValue(value)
	
	if (self.enabled == false) then
		return;
	end
	--Debug.Log("SetMapValue " .. value);
		
	StarterGameUtility.UISliderControl(self.canvasEntityId, self.Properties.ScreenHookup.MapBar.SliderBackgroundAID, value);
	StarterGameUtility.UISliderControl(self.canvasEntityId, self.Properties.ScreenHookup.MapBar.SliderBackgroundBID, 1.0 - value);
end


function uiplayerhud:SetCrosshair(value)
	
	if (self.enabled == false) then
		return;
	end
	--Debug.Log("SetCrosshair " .. value);
		
	local faderValue = 0;
	-- the normal gun crosshair
	if (value == 0) then
		faderValue = 1.0;
	end
	StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.CrossHairs.FaderAID, faderValue, 0.0);	

	-- the Grenade gun crosshair
	if (value == 1) then
		faderValue = 1.0;
	else
		faderValue = 0.0;		
	end
	StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.CrossHairs.FaderBID, faderValue, 0.0);	
end

function uiplayerhud:SetCrosshairTarget(value)
	
	if (self.enabled == false) then
		return;
	end
	--Debug.Log("SetCrosshairTarget " .. value);
	
	local faderValueNormal = 0.0;
	local faderValueTarget = 1.0;
	if (not value) then
		faderValueNormal = 1.0;
		faderValueTarget = 0.0;
	end
	
	// the normal gun crosshair
	StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.CrossHairs.FaderANormalID, faderValueNormal, 0.0);	
	StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.CrossHairs.FaderATargetID, faderValueTarget, 0.0);	

	// the Grenade gun crosshair
	StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.CrossHairs.FaderBNormalID, faderValueNormal, 0.0);	
	StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.CrossHairs.FaderBTargetID, faderValueTarget, 0.0);	
end

function uiplayerhud:OnEventBegin(value)

	--Debug.Log("Recieved message");
	if (GameplayNotificationBus.GetCurrentBusId() == self.activateEventId) then
		self:EnableHud(value == 1);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.HideUIEventId) then
		self:HideHud(value == 1);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.HealthChangedEventId) then
		-- set health value to be "value"
		self:SetHealth(value);		
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.setEnergyEventId) then
		-- set Energy value to be "value"
		self:SetEnergy(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.setMapValueEventId) then
		self:SetMapValue(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.setCrosshairEventId) then
		self:SetCrosshair(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.setCrosshairTargetEventId) then
		self:SetCrosshairTarget(value);
	end
end

return uiplayerhud;
