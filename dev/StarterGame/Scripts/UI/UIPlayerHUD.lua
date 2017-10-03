local StateMachine = require "scripts/common/StateMachine"

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
			--SetMapValueEventIn = { default = "SetMapValueEvent", description = "set the position of the map" },
			SetBlueKeyVisible   = { default = "SetBlueKeyVisibleEvent", 	description = "Set the Blue Key icon is Visible" },
			SetBlueKeyCollected = { default = "SetBlueKeyCollectedEvent", 	description = "Set the Blue Key icon is Collected" },
			SetRedKeyVisible   = { default = "SetRedKeyVisibleEvent", 	description = "Set the Red Key icon is Visible" },
			SetRedKeyCollected = { default = "SetRedKeyCollectedEvent", description = "Set the Red Key icon is Collected" },
			SetObjectiveScreenIDOut = { default = "SetObjectiveScreenIDEvent", description = "Set the Objective Screen Element" }, -- this is for external objects to get our screen ID
		},
		
		MapEntityTag = { default = "PlayerCamera", description = "The Tag of the entity for the map to be hooked off" },
		
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
			Objectives = {
				BlueKeyVisibleID = { default = -1, description = "The ID of the Blue key active" },
				BlueKeyCollectedID = { default = -1, description = "The ID of the Blue key highlight" },
				RedKeyVisibleID = { default = -1, description = "The ID of the Red key active" },
				RedKeyCollectedID = { default = -1, description = "The ID of the Red key highlight" },
			},
		},
	},
}

function uiplayerhud:OnActivate()

	self.firstUpdate = true;
	self.enabled = false;
	
	--Debug.Log("uiplayerhud:OnActivate()");
	
	self.playerID = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"));
	self.mapEntityID = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32(self.Properties.MapEntityTag));
	
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
	--self.setMapValueEventId = GameplayNotificationId(self.entityId, self.Properties.Messages.SetMapValueEventIn, "float");
	--self.setMapValueHandler = GameplayNotificationBus.Connect(self, self.setMapValueEventId);
	
	self.setBlueKeyVisibleEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.SetBlueKeyVisible, "float");
	self.setBlueKeyVisibleHandler = GameplayNotificationBus.Connect(self, self.setBlueKeyVisibleEventId);
	self.setBlueKeyCollectedEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.SetBlueKeyCollected, "float");
	self.setBlueKeyCollectedHandler = GameplayNotificationBus.Connect(self, self.setBlueKeyCollectedEventId);
	self.setRedKeyVisibleEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.SetRedKeyVisible, "float");
	self.setRedKeyVisibleHandler = GameplayNotificationBus.Connect(self, self.setRedKeyVisibleEventId);
	self.setRedKeyCollectedEventId = GameplayNotificationId(EntityId(), self.Properties.Messages.SetRedKeyCollected, "float");
	self.setRedKeyCollectedHandler = GameplayNotificationBus.Connect(self, self.setRedKeyCollectedEventId);
	
	self.setObjectiveScreenIDEventId = GameplayNotificationId(self.entityId, self.Properties.Messages.SetObjectiveScreenIDOut, "float");
		
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
	--self.setMapValueHandler:Disconnect();
	--self.setMapValueHandler = nil;
	
	self.setBlueKeyVisibleHandler:Disconnect();
	self.setBlueKeyVisibleHandler = nil;
	self.setBlueKeyCollectedHandler:Disconnect();
	self.setBlueKeyCollectedHandler = nil;
	self.setRedKeyVisibleHandler:Disconnect();
	self.setRedKeyVisibleHandler = nil;
	self.setRedKeyCollectedHandler:Disconnect();
	self.setRedKeyCollectedHandler = nil;
	
	self.tickHandler:Disconnect();
	self.tickHandler = nil;
	
	self:EnableHud(false);
end

function uiplayerhud:OnTick(deltaTime, timePoint)
		
	if(self.firstUpdate == true)then
		self.firstUpdate = false;
		-- tell other things that are listening about our screen ID
		GameplayNotificationBus.Event.OnEventBegin(self.setObjectiveScreenIDEventId, self.canvasEntityId);
		-- this is necessary because of the auto enable can miss some entities because they dont exist yet
		self.mapEntityID = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32(self.Properties.MapEntityTag));
	end
	
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
		
	-- update map
	self.mapEntityID = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32(self.Properties.MapEntityTag));
	local camTm = TransformBus.Event.GetWorldTM(self.mapEntityID);
	local posCam = camTm:GetTranslation();
	local dirCam = camTm:GetColumn(1):GetNormalized();
	dirCam.z = 0;
	dirCam = dirCam:GetNormalized();
	local rightCam = camTm:GetColumn(0);
	rightCam.z = 0;
	rightCam = rightCam:GetNormalized();
	
	-- map start
	local mapValue = 0.5;
	
	local dirNorth = Vector3(0,1,0);
	
	local dirDot = dirCam:Dot(dirNorth);
	local rightDot = rightCam:Dot(dirNorth);
	local linearAngle = Math.ArcCos(dirDot) / 3.1415926535897932384626433832795;
	
	mapValue = (linearAngle + 1.0) * 0.5
	if(rightDot >= 0) then
	else
		mapValue = (mapValue * -1.0) + 1.0;
	end
	
	-- value is currenlty "0->1" for the full 360
	-- making it only show the foward 90
	--mapValue = self:Clamp(((mapValue - 0.5) * 4.0) + 0.5, 0.0, 1.0); 
	self:SetMapValue(mapValue);
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
		
		-- tell other things that are listening about our screen ID
		GameplayNotificationBus.Event.OnEventBegin(self.setObjectiveScreenIDEventId, self.canvasEntityId);
		--Debug.Log("uiplayerhud:EnableHud() setup");
	else
		-- tell other things that are listening about screen ID dying
		GameplayNotificationBus.Event.OnEventBegin(self.setObjectiveScreenIDEventId, nil);

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
	--elseif (GameplayNotificationBus.GetCurrentBusId() == self.setMapValueEventId) then
	--	self:SetMapValue(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.setBlueKeyVisibleEventId) then
		local fadeValue = 0;
		if (value) then
			fadeValue = 1;
		end
		StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.Objectives.BlueKeyVisibleID, fadeValue, 1.0);	
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.setBlueKeyCollectedEventId) then
		local fadeValue = 0;
		if (value) then
			fadeValue = 1;
		end
		StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.Objectives.BlueKeyCollectedID, fadeValue, 1.0);	
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.setRedKeyVisibleEventId) then
		local fadeValue = 0;
		if (value) then
			fadeValue = 1;
		end
		StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.Objectives.RedKeyVisibleID, fadeValue, 1.0);	
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.setRedKeyCollectedEventId) then
		local fadeValue = 0;
		if (value) then
			fadeValue = 1;
		end
		StarterGameUtility.UIFaderControl(self.canvasEntityId, self.Properties.ScreenHookup.Objectives.RedKeyCollectedID, fadeValue, 1.0);	
	end
end

return uiplayerhud;
