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

local weaponswitcher = 
{
    Properties = 
    {
		Events =
		{
			ControlsEnabled = { default = "EnableControlsEvent", description = "If passed '1.0' it will enable controls, oherwise it will disable them." },
			EnableWeapon = { default = "EventEnableWeapon", description = "Sent with the name of the weapon in ofder to switch the weapon selection on." },
		},
		
		-- The weapons to be used.
		Weapons =
		{
			Pistol =
			{
				Weapon = {default = EntityId()},
				Enabled = { default = true, description = "am i available to the player?" },
			},
			Grenade =
			{
				Weapon = {default = EntityId()},
				Enabled = { default = false, description = "am i available to the player?" },
			},
		},
		WeaponSetup = 
		{
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
	self.setVisibleEventId = GameplayNotificationId(self.entityId, "SetVisible", "float");
	self.setVisibleHandler = GameplayNotificationBus.Connect(self, self.setVisibleEventId);

	-- Input listeners (weapon).
	self.weaponSwitchEventId = GameplayNotificationId(self.entityId, "WeaponSwitch", "float");
	self.weaponSwitchHandler = GameplayNotificationBus.Connect(self, self.weaponSwitchEventId);
	
	self.enableWeaponEventId = GameplayNotificationId(self.entityId, self.Properties.Events.EnableWeapon, "float");
	self.enableWeaponHandler = GameplayNotificationBus.Connect(self, self.enableWeaponEventId);

	-- Set the weapon information.
	Debug.Assert(self.Properties.Weapons.Pistol.Weapon ~= 0 and self.Properties.Weapons.Pistol.Weapon);
	Debug.Assert(self.Properties.Weapons.Grenade.Weapon ~= 0 and self.Properties.Weapons.Grenade.Weapon);
	
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
	self.enableWeaponHandler:Disconnect();
	self.enableWeaponHandler = nil;
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
	if (self.setVisibleHandler ~= nil) then
		self.setVisibleHandler:Disconnect();
		self.setVisibleHandler = nil;
	end
end

function weaponswitcher:ChangeWeapon()
	if (self.activeWeapon == nil) then
		self.activeWeapon = self.Properties.Weapons[1];
		self:SetupNewWeapon();
	else
		local foundExisting = false;
		local foundNext = false;
		local newWeapon = nil;
		
		for key,value in pairs(self.Properties.Weapons) do
			if (value == self.activeWeapon) then
				foundExisting = true;
			elseif (foundExisting and (not foundNext) and value.Enabled) then
				foundNext = true;
				newWeapon = value;
				break;
			end
		end
		-- if i have not found the next one to use do a search from the beginning till our current to wrap our search
		if (foundExisting and (not foundNext)) then
			for key,value in pairs(self.Properties.Weapons) do
				if (value == self.activeWeapon) then
					-- i have done a full loop, so bail
					break;
				elseif (value.Enabled) then
					foundNext = true;
					newWeapon = value;
					break;
				end
			end
		end
		
		if(foundExisting and foundNext) then
			-- Detach the old weapon.
			AttachmentComponentRequestBus.Event.Detach(self.activeWeapon.Weapon);
			MeshComponentRequestBus.Event.SetVisibility(self.activeWeapon.Weapon, false);
			
			-- fire an event off to tell the current weapon that we are not using it anymore
			GameplayNotificationBus.Event.OnEventBegin(self.activeWeaponActivateEventId, 0);

			AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.varWpnSwitchSound);
			
			self.activeWeapon = newWeapon;
			
			self:SetupNewWeapon();
		end
	end
	
	-- if (self.activeWeapon == self.Properties.Weapons.Pistol) then
		-- self.activeWeapon = self.Properties.Weapons.Grenade;
	-- elseif (self.activeWeapon == self.Properties.Weapons.Grenade) then
		-- self.activeWeapon = self.Properties.Weapons.Pistol;
	-- else
		-- self.activeWeapon = self.Properties.Weapons.Pistol;
	-- end
end

function weaponswitcher:SetupNewWeapon()
	Debug.Assert(self.activeWeapon ~= 0 and self.activeWeapon);

	local tm = TransformBus.Event.GetWorldTM(self.activeWeapon.Weapon);
	local scale = tm:ExtractScale();
	tm = Transform:CreateIdentity();
	tm:MultiplyByScale(scale);
	MeshComponentRequestBus.Event.SetVisibility(self.activeWeapon.Weapon, true);
	AttachmentComponentRequestBus.Event.Attach(self.activeWeapon.Weapon, self.entityId, self.Properties.WeaponSetup.Bone, tm);

	self.activeWeaponActivateEventId = GameplayNotificationId(self.activeWeapon.Weapon, self.Properties.WeaponSetup.GenericEvents.Activate, "float");

	GameplayNotificationBus.Event.OnEventBegin(self.activeWeaponActivateEventId, 1);
	
	local onSetupNewWeaponEventId = GameplayNotificationId(self.entityId, "OnSetupNewWeapon", "float");
	GameplayNotificationBus.Event.OnEventBegin(onSetupNewWeaponEventId, self.activeWeapon.Weapon);
end

function weaponswitcher:OnTick(deltaTime, timePoint)
	-- First update initialisation
	self.activeWeapon = self.Properties.Weapons.Pistol;
	self:SetupNewWeapon();
	self.tickBusHandler:Disconnect();
	self.tickBusHandler = nil;
end

function weaponswitcher:SetControlsEnabled(newControlsEnabled)
	self.controlsEnabled = newControlsEnabled;
end

function weaponswitcher:EnableWeapon(weapon)
	for key,value in pairs(self.Properties.Weapons) do
		if (key == weapon) then
			value.Enabled = true;
			return;
		end
	end
end	

function weaponswitcher:OnEventBegin(value)
	local busId = GameplayNotificationBus.GetCurrentBusId();
	--Debug.Log("weaponswitcher:OnEventBegin " .. tostring(busId) .. " " .. tostring(value));
	if (busId == self.controlsEnabledEventId) then
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
		if ((busId == self.weaponSwitchEventId) and (self.enabled)) then
			self:ChangeWeapon();
		end
	end
	
	if (busId == self.enableEventId) then
		self:OnEnable();
	elseif (busId == self.disableEventId) then
		self:OnDisable();
	elseif (busId == self.setVisibleEventId) then
		MeshComponentRequestBus.Event.SetVisibility(self.activeWeapon.Weapon, value);
	elseif (busId == self.enableWeaponEventId) then
		self:EnableWeapon(value);
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
