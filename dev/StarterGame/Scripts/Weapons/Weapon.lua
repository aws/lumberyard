function PopulateProperties(Properties)
	Properties.Owner = { default = EntityId() };
	Properties.MaxJitterAngle = { default = 0.0, description = "Max jitter angle when firing in degrees."};
	Properties.Firepower =
	{
		ShotDelay = { default = 0.2, description = "Time between shots.", suffix = " s" },
		ShotEnergyCost = { default = 2.5, description = "The energy cost per shot." },			
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

	local playerId = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"));
	if (not self.Properties.Owner:IsValid()) then
		self.Properties.Owner = playerId;
	end
	self.playerOwned = self.Properties.Owner == playerId;

	self.FireSuccessEventId = GameplayNotificationId(self.Properties.Owner, self.Properties.Events.FireSuccess, "float");
	self.FireFailEventId = GameplayNotificationId(self.Properties.Owner, self.Properties.Events.FireFail, "float");
	self.hideCrosshairEventId = GameplayNotificationId(self.entityId, self.Properties.Events.HideCrosshair, "float");

	self.tickHandler = TickBus.Connect(self);
	self.performedFirstUpdate = false;
	self.timeBeforeNextShot = 0.0;
	
	self.varWpnFireSound = self.Properties.Audio.WpnFireSound;
	self.varWpnFireNoNRGSound = self.Properties.Audio.WpnFireNoNRGSound;
	--self.varWpnSwitchSound = self.Properties.Audio.WpnSwitchSound;
	
	self.UseRifleForwardForAiming = false;
	self.maxJitterAngle = Math.DegToRad(self.Properties.MaxJitterAngle);
end

function weapon:OnDeactivate()

	self.activateHandler:Disconnect();
	self.activateHandler = nil;
	self.fireHandler:Disconnect();
	self.fireHandler = nil;
	self.useWeaponForwardForAimingHandler:Disconnect();
	self.useWeaponForwardForAimingHandler = nil;
	self.reloadHandler:Disconnect();
	self.reloadHandler = nil;
	
	self.tickHandler:Disconnect();
	self.tickHandler = nil;

end

function weapon:OnTick(deltaTime, timePoint)

	if (self.performedFirstUpdate == false) then
		self.parent:DoFirstUpdate();
		self.performedFirstUpdate = true;
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
		local phi = StarterGameUtility.randomF(0.0, self.maxJitterAngle);
		local theta = StarterGameUtility.randomF(0.0, 6.28318530718);
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
function weapon:Fire(value)

	if (not self:CanShoot()) then
		return;
	end
	
	self.timeBeforeNextShot = self.Properties.Firepower.ShotDelay;
		if (value <	self.Properties.Firepower.ShotEnergyCost) then
			--Debug.Log("Weapon Fire failure");
			GameplayNotificationBus.Event.OnEventBegin(self.FireFailEventId, self.Properties.Firepower.ShotEnergyCost);
			AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.varWpnFireNoNRGSound);
			return;
		end
		--Debug.Log("Weapon Fire success");
		GameplayNotificationBus.Event.OnEventBegin(self.FireSuccessEventId, self.Properties.Firepower.ShotEnergyCost);
	
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

function weapon:OnEventBegin(value)

	-- If this is the active weapon then catch any events that have been sent to it.
	if (self.IsActive) then
		if (GameplayNotificationBus.GetCurrentBusId() == self.fireEventId) then
			self:Fire(value);
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.reloadEventId) then
			-- Placeholder.
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.useWeaponForwardForAimingEventId) then
			self:UseWeaponForwardForAiming(value);
		end
	end

	-- Check if the event is actually to toggle activation/deactivation.
	if (GameplayNotificationBus.GetCurrentBusId() == self.activateEventId) then
		self.IsActive = (value == 1);
		--Debug.Log("weapon:OnEventBegin: activateEventId [" .. tostring(self.entityId) .. "] " .. tostring(self.IsActive));
		--if(self.IsActive) then
		--	AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.varWpnSwitchSound);
		--end
		
		-- send the crosshair show message 
		local crosshairFaderValue = 0.0;
		if(self.IsActive) then
			crosshairFaderValue = 1;
		end
		GameplayNotificationBus.Event.OnEventBegin(self.hideCrosshairEventId, crosshairFaderValue);
	end
end
