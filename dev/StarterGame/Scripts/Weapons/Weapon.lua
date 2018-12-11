local utilities = require "scripts/common/utilities"

function PopulateProperties(Properties)
	Properties.Owner = { default = EntityId() };
	Properties.MaxJitterAngle = { default = 0.0, description = "Max jitter angle when firing in degrees."};
	Properties.Firepower =
	{
		Range = { default = 1000.0, description = "The range that the weapon can hit something.", suffix = " m" };
		ShotDelay = { default = 0.2, description = "Time between shots.", suffix = " s" },
		StatAmmo = { default = "Energy", description = "The stat of our owner to use for firing the weapon." },
		ShotCost = { default = 2.5, description = "The stat cost per shot." },
	};
	Properties.Events =
	{
		Activate = "EventActivate";
		Fire = "EventFire";
		UseWeaponForwardForAiming = "EventUseWeaponForwardForAiming";
		Reload = "EventReload";
		FireSuccess = "WeaponFireSuccess";
		FireFail = "WeaponFireFail";
		HideCrosshair = "SetCrosshairHiddenEvent",
		SetCrosshairStatus = { default = "SetCrosshairStatusEvent", description = "supplies information from the weapon controller" },
		SetWeaponStatus = { default = "SetWeaponStatusEvent", description = "supplies information from the weapon controller" },
	};
	Properties.Audio =
	{
		WpnFireSound = { default = "Play_pla_wpn_plasma_pistol_fire", description = "Weapon Fire Sound" },
		WpnFireNoNRGSound = { default = "Play_pla_wpn_plasma_pistol_fire_no_NRG", description = "Weapon Fire No NRG Sound" },
		--WpnSwitchSound = { default = "Play_HUD_wpn_switch", description = "Weapon Switch Sound" },
	};
	Properties.SoundRange =
	{
		GunShot = { default = 10.0, description = "The distance at which the gunshot can be heard.", suffix = " m" },
	};
end

weapon = {
}

function weapon:new(o)
	o = o or {}
	setmetatable(o, self)
	self.__index = self
	return o;
end

function weapon:OnActivate(parent, entityId, properties)
	self.parent = parent;
	self.entityId = entityId;
	self.Properties = properties;
	self.activateEventId = GameplayNotificationId(self.entityId, self.Properties.Events.Activate, "float");
	self.activateHandler = GameplayNotificationBus.Connect(self, self.activateEventId);
	self.fireEventId = GameplayNotificationId(self.entityId, self.Properties.Events.Fire, "float");
	self.fireHandler = GameplayNotificationBus.Connect(self, self.fireEventId);
	self.reloadEventId = GameplayNotificationId(self.entityId, self.Properties.Events.Reload, "float");
	self.reloadHandler = GameplayNotificationBus.Connect(self, self.reloadEventId);
	self.useWeaponForwardForAimingEventId = GameplayNotificationId(self.entityId, self.Properties.Events.UseWeaponForwardForAiming, "float");
	self.useWeaponForwardForAimingHandler = GameplayNotificationBus.Connect(self, self.useWeaponForwardForAimingEventId);	
	self.setWeaponStatusEventId = GameplayNotificationId(self.entityId, self.Properties.Events.SetWeaponStatus, "float");
	self.setWeaponStatusHandler = GameplayNotificationBus.Connect(self, self.setWeaponStatusEventId);	
	self.isLegacyEventId = GameplayNotificationId(self.entityId, "IsLegacyCharacter", "float");
	self.isLegacyHandler = GameplayNotificationBus.Connect(self, self.isLegacyEventId);	
	
	self.ammo = 100;


	self.FireSuccessEventId = GameplayNotificationId(self.Properties.Owner, self.Properties.Events.FireSuccess, "float");
	self.FireFailEventId = GameplayNotificationId(self.Properties.Owner, self.Properties.Events.FireFail, "float");
	self.AmmoRequestEventId = GameplayNotificationId(self.Properties.Owner, (self.Properties.Firepower.StatAmmo .. "GetValueEvent"), "float");
	self.AmmoValueEventId = GameplayNotificationId(self.entityId, (self.Properties.Firepower.StatAmmo .. "ValueEvent"), "float");
	self.AmmoValueHandler = GameplayNotificationBus.Connect(self, self.AmmoValueEventId);	
	self.AmmoUseEventId = GameplayNotificationId(self.Properties.Owner, (self.Properties.Firepower.StatAmmo .. "DeltaValueEvent"), "float");
	self.AmmoResetRegenEventId = GameplayNotificationId(self.Properties.Owner, (self.Properties.Firepower.StatAmmo .. "RegenResetEvent"), "float");

	self.tickHandler = TickBus.Connect(self);
	self.performedFirstUpdate = false;
	self.timeBeforeNextShot = 0.0;
	
	self.varWpnFireSound = self.Properties.Audio.WpnFireSound;
	self.varWpnFireNoNRGSound = self.Properties.Audio.WpnFireNoNRGSound;
	--self.varWpnSwitchSound = self.Properties.Audio.WpnSwitchSound;
	
	self.UseRifleForwardForAiming = false;
	self.maxJitterAngle = Math.DegToRad(self.Properties.MaxJitterAngle);
end

function weapon:GetWeaponForward()
	return TransformBus.Event.GetWorldTM(self.entityId):GetColumn(self.WeaponForwardColumn);
end

function weapon:OnDeactivate()
	self.activateHandler:Disconnect();
	self.activateHandler = nil;
	self.fireHandler:Disconnect();
	self.fireHandler = nil;
	self.useWeaponForwardForAimingHandler:Disconnect();
	self.useWeaponForwardForAimingHandler = nil;
	self.isLegacyHandler:Disconnect();
	self.isLegacyHandler = nil;
	self.reloadHandler:Disconnect();
	self.reloadHandler = nil;
	self.AmmoValueHandler:Disconnect();
	self.AmmoValueHandler = nil;
	self.setWeaponStatusHandler:Disconnect();
	self.setWeaponStatusHandler = nil;
	
	self.tickHandler:Disconnect();
	self.tickHandler = nil;
end

function weapon:OnTick(deltaTime, timePoint)

	if (self.performedFirstUpdate == false) then
		self.parent:DoFirstUpdate();
		self.performedFirstUpdate = true;

		local playerId = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"));
		if (not self.Properties.Owner:IsValid()) then
			self.Properties.Owner = playerId;
		end
		self.playerOwned = self.Properties.Owner == playerId;
	end

	if (not self:CanShoot()) then
		self.timeBeforeNextShot = self.timeBeforeNextShot - deltaTime;
	end
	
end

function weapon:UseWeaponForwardForAiming(value)
	if(value == 0.0) then
		self.UseRifleForwardForAiming = false;
	else
		self.UseRifleForwardForAiming = true;
	end
end

function weapon:GetJitteredDirection(direction)
	if (self.maxJitterAngle ~= 0.0) then
		local phi = math.random() * self.maxJitterAngle;
		local theta = math.random() * 6.28318530718;
		local sinPhi = Math.Sin(phi);
		local cosPhi = Math.Cos(phi);
		local sinTheta = Math.Sin(theta);
		local cosTheta = Math.Cos(theta);
		local x = sinPhi * cosTheta;
		local y = sinPhi * sinTheta;
		local z = cosPhi;
		local yAxis = Vector3(0, -direction.z, direction.y);
		yAxis:Normalize();
		local xAxis = Vector3.Cross(direction, yAxis);
		local jitteredDirection = (xAxis * x) + (yAxis * y) + (direction * z);
		return jitteredDirection;
	else
		return direction;
	end
end

function weapon:Fire()
	if (not self:CanShoot()) then
		return;
	end
	
	-- update my ammo value
	--Debug.Log("weapon:OnEventBegin: Fire [" .. tostring(self.entityId) .. "] " .. tostring(self.ammo));
	GameplayNotificationBus.Event.OnEventBegin(self.AmmoRequestEventId, self.entityId);
	
	self.timeBeforeNextShot = self.Properties.Firepower.ShotDelay;
	
	local infiniteAmmo = utilities.GetDebugManagerBool("InfiniteEnergy", true);

	if ((self.ammo < self.Properties.Firepower.ShotCost) and (not infiniteAmmo)) then
		--Debug.Log("Weapon Fire failure");
		GameplayNotificationBus.Event.OnEventBegin(self.FireFailEventId, self.Properties.Firepower.ShotCost);
		AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.varWpnFireNoNRGSound);
		
		--GameplayNotificationBus.Event.OnEventBegin(self.AmmoResetRegenEventId, -(self.Properties.Firepower.ShotCost));
		return;
	end
	
	-- use the ammo
	if (not infiniteAmmo) then
		GameplayNotificationBus.Event.OnEventBegin(self.AmmoUseEventId, -(self.Properties.Firepower.ShotCost));
	end
	
	--Debug.Log("Weapon Fire success");
	GameplayNotificationBus.Event.OnEventBegin(self.FireSuccessEventId, self.Properties.Firepower.ShotCost);
	
	-- Audio Wpn Fire
	AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.varWpnFireSound);
	self.parent:DoFire(self.UseRifleForwardForAiming, self.playerOwned);
end

function weapon:CanShoot()
	-- A.I. can shoot as often as they like... because they cheat, that's why!
	if (not self.playerOwned) then
		return true;
	end

	if (self.timeBeforeNextShot <= 0.0) then
		return true;
	end
	
	return false;
end

function weapon:SetWeaponStatus(value)
	-- send on the weapon to the crosshair

	--Debug.Log("weapon:SetWeaponStatus: [" .. tostring(self.entityId) .. "] " .. tostring(value));
	local hostile = 0;
	if (value.x ~= 0) then
		-- i am poining at a target, make sure that it is within range
		
		if(value.y <= self.Properties.Firepower.Range) then
			hostile = 1;
		end
	end
	
	-- send on the message
	local crosshairScreen = UiCanvasRefBus.Event.GetCanvas(self.entityId);
	if (crosshairScreen == nil) then
		-- this is possible for enemy weapons for example
		--Debug.Log("weapon:OnEventBegin() activateEventId : i dont have a canvas ?!");
	else	
		local setCrosshairStatusEventId = GameplayNotificationId(crosshairScreen, self.Properties.Events.SetCrosshairStatus, "float");
		GameplayNotificationBus.Event.OnEventBegin(setCrosshairStatusEventId, hostile);
	end
end


function weapon:SetWeaponActive(value)
	--Debug.Log("weapon:OnEventBegin: SetWeaponActive [" .. tostring(self.entityId) .. "] " .. tostring(value));
	self.IsActive = (value == 1);
	
	-- send the crosshair show message 
	local crosshairFaderValue = 0.0;
	if(self.IsActive) then
		crosshairFaderValue = 1;
	end

	local crosshairScreen = UiCanvasRefBus.Event.GetCanvas(self.entityId);
	if (crosshairScreen == nil) then
		-- this is possible for enemy weapons for example
		--Debug.Log("weapon:OnEventBegin() activateEventId : i dont have a canvas ?!");
	else	
		local hideCrosshairEventId = GameplayNotificationId(crosshairScreen, self.Properties.Events.HideCrosshair, "float");
		GameplayNotificationBus.Event.OnEventBegin(hideCrosshairEventId, crosshairFaderValue);
	end
	
end

function weapon:OnEventBegin(value)
	-- If this is the active weapon then catch any events that have been sent to it.
	if (self.IsActive) then
		if (GameplayNotificationBus.GetCurrentBusId() == self.fireEventId) then
			self:Fire();
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.reloadEventId) then
			-- Placeholder.
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.useWeaponForwardForAimingEventId) then
			self:UseWeaponForwardForAiming(value);
		end

	end

	-- Check if the event is actually to toggle activation/deactivation.
	if (GameplayNotificationBus.GetCurrentBusId() == self.AmmoValueEventId) then
		--Debug.Log("weapon:OnEventBegin: AmmoValueEventId [" .. tostring(self.entityId) .. "] " .. tostring(value));
		self.ammo = value;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.activateEventId) then
		self:SetWeaponActive(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.setWeaponStatusEventId) then
		--Debug.Log("weapon:OnEventBegin: setWeaponStatusEventId [" .. tostring(self.entityId) .. "] " .. tostring(value));
		self:SetWeaponStatus(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.isLegacyEventId) then
		if(value == 0) then
			self.WeaponForwardColumn = 1
		else
			self.WeaponForwardColumn = 2
		end
	end
end
