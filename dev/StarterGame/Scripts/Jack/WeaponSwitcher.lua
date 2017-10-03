local weaponswitcher = 
{
    Properties = 
    {
		UIMessages = 
		{
			SetCrosshairMessage = { default = "SetCrosshairEvent", description = "set the visible crosshair" },
		},

		Events =
		{
			ControlsEnabled = { default = "EnableControlsEvent", description = "If passed '1.0' it will enable controls, oherwise it will disable them." },
		},
		
		-- The weapons to be used.
		Weapons =
		{
			Ray =
			{
				Weapon = {default = EntityId()},
			},
			Flame =
			{
				Weapon = {default = EntityId()},
			},
			GenericEvents =
			{
				Activate = "EventActivate";
			},
			-- The bone that the weapons will attach to.
			-- This may need to become weapon-specific if we end up having weird weapons (e.g. shoulder-mounted).
			Bone = { default = "r_handProp", description = "The bone to attach the weapon to." },
		},
		
		Audio =
		{
			WpnSwitchSound = { default = "Play_HUD_wpn_switch", description = "Weapon Switch Sound" },
		},
		
    },
}

--------------------------------------------------
-- Component behavior
--------------------------------------------------

function weaponswitcher:OnActivate()    
	--Debug.Log("weaponswitcher entityId: ".. tostring(self.entityId))
	
	-- Play the specified Audio Trigger (wwise event) on this component
	AudioTriggerComponentRequestBus.Event.Play(self.entityId);

	-- Enable and disable events
	self.enableEventId = GameplayNotificationId(self.entityId, "Enable", "float");
	self.enableHandler = GameplayNotificationBus.Connect(self, self.enableEventId);
	self.disableEventId = GameplayNotificationId(self.entityId, "Disable", "float");
	self.disableHandler = GameplayNotificationBus.Connect(self, self.disableEventId);

	-- Input listeners (weapon).
	self.weaponSwitchEventId = GameplayNotificationId(self.entityId, "WeaponSwitch", "float");
	self.weaponSwitchHandler = GameplayNotificationBus.Connect(self, self.weaponSwitchEventId);

	-- Set the weapon information.
	Debug.Assert(self.Properties.Weapons.Ray.Weapon ~= 0 and self.Properties.Weapons.Ray.Weapon);
	Debug.Assert(self.Properties.Weapons.Flame.Weapon ~= 0 and self.Properties.Weapons.Flame.Weapon);
	
	self.varWpnSwitchSound = self.Properties.Audio.WpnSwitchSound;

	-- Tick needed to detect aim timeout
    self.tickBusHandler = TickBus.Connect(self);
	
	self.enabled = true;
	
	self.controlsDisabledCount = 0;
	self.controlsEnabled = true;
	self.controlsEnabledEventId = GameplayNotificationId(self.entityId, self.Properties.Events.ControlsEnabled, "float");
	self.controlsEnabledHandler = GameplayNotificationBus.Connect(self, self.controlsEnabledEventId);
	--Debug.Log("weaponswitcher:OnActivate " .. tostring(self.controlsEnabledEventId));
end

function weaponswitcher:OnDeactivate()
	self.weaponSwitchHandler:Disconnect();
	self.weaponSwitchHandler = nil;
	if(self.tickBusHandler ~= nil) then
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;		
	end
	if (self.enableHandler ~= nil) then
		self.enableHandler:Disconnect();
		self.enableHandler = nil;
	end
	if (self.disableHandler ~= nil) then
		self.disableHandler:Disconnect();
		self.disableHandler = nil;	
	end
end

function weaponswitcher:ChangeWeapon()

	-- Detach the old weapon.
	AttachmentComponentRequestBus.Event.Detach(self.activeWeapon.Weapon);
	
	MeshComponentRequestBus.Event.SetVisibility(self.activeWeapon.Weapon, false);
	
	-- fire an event off to tell the current weapon that we are not using it anymore
	GameplayNotificationBus.Event.OnEventBegin(self.activeWeaponActivateEventId, 0);

	AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.varWpnSwitchSound);

	-- This could probably be done better... maybe adding all the potential weapons to an
	-- array or table in 'OnActivate' then searching to find the current one and moving
	-- on to the next one?
	if (self.activeWeapon == self.Properties.Weapons.Ray) then
		self.activeWeapon = self.Properties.Weapons.Flame;
	elseif (self.activeWeapon == self.Properties.Weapons.Flame) then
		self.activeWeapon = self.Properties.Weapons.Ray;
	else
		self.activeWeapon = self.Properties.Weapons.Ray;
	end

	self:SetupNewWeapon();
end

function weaponswitcher:SetupNewWeapon()

	Debug.Assert(self.activeWeapon ~= 0 and self.activeWeapon);

	local tm = TransformBus.Event.GetWorldTM(self.activeWeapon.Weapon);
	local scale = tm:ExtractScale();
	tm = Transform:CreateIdentity();
	tm:MultiplyByScale(scale);
	MeshComponentRequestBus.Event.SetVisibility(self.activeWeapon.Weapon, true);
	AttachmentComponentRequestBus.Event.Attach(self.activeWeapon.Weapon, self.entityId, self.Properties.Weapons.Bone, tm);

	self.activeWeaponActivateEventId = GameplayNotificationId(self.activeWeapon.Weapon, self.Properties.Weapons.GenericEvents.Activate, "float");

	GameplayNotificationBus.Event.OnEventBegin(self.activeWeaponActivateEventId, 1);
	
	local onSetupNewWeaponEventId = GameplayNotificationId(self.entityId, "OnSetupNewWeapon", "float");
	GameplayNotificationBus.Event.OnEventBegin(onSetupNewWeaponEventId, self.activeWeapon.Weapon);
end

function weaponswitcher:OnTick(deltaTime, timePoint)
	-- First update initialisation
	local uiElement = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("UIPlayer"));
	self.SetCrosshairEventId = GameplayNotificationId(uiElement, self.Properties.UIMessages.SetCrosshairMessage, "float");
	self.activeWeapon = self.Properties.Weapons.Ray;
	self:SetupNewWeapon();
	self.tickBusHandler:Disconnect();
	self.tickBusHandler = nil;
end

function weaponswitcher:SetControlsEnabled(newControlsEnabled)
	self.controlsEnabled = newControlsEnabled;
end
	
function weaponswitcher:OnEventBegin(value)
	--Debug.Log("weaponswitcher:OnEventBegin " .. tostring(GameplayNotificationBus.GetCurrentBusId()) .. " " .. tostring(value));
	if (GameplayNotificationBus.GetCurrentBusId() == self.controlsEnabledEventId) then
		--Debug.Log("weaponswitcher:controlsEnabledEventId " .. tostring(value));
		if (value == 1.0) then
			self.controlsDisabledCount = self.controlsDisabledCount - 1;
		else
			self.controlsDisabledCount = self.controlsDisabledCount + 1;
		end
		if (self.controlsDisabledCount < 0) then
			Debug.Log("[Warning] WeaponSwitcher: controls disabled ref count is less than 0.  Probable enable/disable mismatch.");
			self.controlsDisabledCount = 0;
		end		
		local newEnabled = self.controlsDisabledCount == 0;
		if (self.controlsEnabled ~= newEnabled) then
			self:SetControlsEnabled(newEnabled);
		end
	end

	if (self.controlsEnabled) then
		-- Player controls.
		if ((GameplayNotificationBus.GetCurrentBusId() == self.weaponSwitchEventId) and (self.enabled)) then
			self:ChangeWeapon();
		end
	end
	
	if (GameplayNotificationBus.GetCurrentBusId() == self.enableEventId) then
		self:OnEnable();
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.disableEventId) then
		self:OnDisable();
	end
end

function weaponswitcher:OnEnable()
	self.enabled = true;
	MeshComponentRequestBus.Event.SetVisibility(self.activeWeapon.Weapon, true);
	
	-- We don't want to re-join the tick bus because it's only meant to be for setup;
	-- which should have been done when we were disabled. If we perform the tick again
	-- then it may forcibly reset values (such as the active weapon).
	--self.tickBusHandler = TickBus.Connect(self);
end

function weaponswitcher:OnDisable()
	self.enabled = false;

	-- If the active weapon hasn't been assigned yet then it means we haven't ticked but
	-- something else has in order to send us a 'disable' message. Therefore, assume that
	-- enough things have been setup by now for the tick function to process things
	-- correctly.
	if (self.activeWeapon == nil) then
		self:OnTick(0, 0);
	end
	MeshComponentRequestBus.Event.SetVisibility(self.activeWeapon.Weapon, false);
	if (self.tickBusHandler ~= nil) then
		self.tickBusHandler:Disconnect();
	end
end

return weaponswitcher;
