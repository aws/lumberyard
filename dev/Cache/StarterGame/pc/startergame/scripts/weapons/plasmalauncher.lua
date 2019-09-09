require "scripts/Weapons/Weapon"

local plasmalauncher =
{
	Properties =
	{
		MuzzleEffect =
		{
			Spawner = { default = EntityId() },
			SpawnEvent = { default = "SpawnEffect" },
		},
	}
}

PopulateProperties(plasmalauncher.Properties);

function plasmalauncher:OnActivate()
	self.weapon = weapon:new();
	self.weapon:OnActivate(self, self.entityId, self.Properties);
end

function plasmalauncher:OnDeactivate()
	self.weapon:OnDeactivate();
end

function plasmalauncher:DoFirstUpdate()
end

function plasmalauncher:DoFire(useForwardForAiming, playerOwned)
	-- Spawn a bullet.
	local particleManager = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("ParticleManager"));
	if (particleManager == nil or particleManager:IsValid() == false) then
		Debug.Assert("Invalid particle manager.");
	end
	local params = ParticleSpawnerParams();
	params.transform = TransformBus.Event.GetWorldTM(self.entityId); -- TODO: use a locator for where to spawn the projectile
	params.event = "SpawnLauncherBullet";
	params.ownerId = self.Properties.Owner;
	params.attachToEntity = false;
	if(useForwardForAiming == true) then
		params.impulse = self.weapon:GetWeaponForward():GetNormalized();
	else
		local camTm = TransformBus.Event.GetWorldTM(TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCamera")));
		local cameraPosition = camTm:GetTranslation();
		local weaponPosition = TransformBus.Event.GetWorldTM(self.entityId):GetTranslation();
		local weaponOffset = cameraPosition - weaponPosition;
		local cameraDirection = camTm:GetColumn(1);
		local cameraUp = camTm:GetColumn(2);
		-- Offset the aim direction to compensate for the gun being below and to the left of the camera
		-- The offset is greatest when aiming up and lowest when aiming down
		local upFactor = (cameraDirection.z / 2.0) + 0.5;
		local upAngleMax = 0.045;
		local upAngleMin = 0;
		local upAngleRange = upAngleMax - upAngleMin;
		local upAngle = upAngleMin + upFactor * upAngleRange;
		local aimDirection = cameraDirection * Math.Cos(upAngle) + cameraUp * Math.Sin(upAngle);
		local cameraRight = camTm:GetColumn(0);
		local rightAngle = 0.017
		aimDirection = aimDirection * Math.Cos(rightAngle) + cameraRight * Math.Sin(rightAngle);
		params.impulse = aimDirection;
		
	end
	local pmEventId = GameplayNotificationId(particleManager, "SpawnParticleEvent", "float");
	GameplayNotificationBus.Event.OnEventBegin(pmEventId, params);
	
	-- Spawn a muzzle flash.
	local from = TransformBus.Event.GetWorldTM(self.entityId):GetTranslation();
	local to = from + self.weapon:GetWeaponForward():GetNormalized();
	local tm = MathUtils.CreateLookAt(from, to, AxisType.YPositive);
	TransformBus.Event.SetWorldTM(self.Properties.MuzzleEffect.Spawner, tm);
	
	local eventId = GameplayNotificationId(self.Properties.MuzzleEffect.Spawner, self.Properties.MuzzleEffect.SpawnEvent, "float");
	GameplayNotificationBus.Event.OnEventBegin(eventId, self.entityId);
	
	-- Only the player's rifle should emit sounds.
	if (playerOwned) then
		-- Broadcast the gunshot sound event to nearby entities via the sound manager.
		local propsGunShot = SoundProperties();
		propsGunShot.origin = from;
		propsGunShot.range = self.Properties.SoundRange.GunShot;
		propsGunShot.type = SoundTypes.GunShot;
		AISoundManagerSystemRequestBus.Broadcast.BroadcastSound(propsGunShot);
	end
end

return plasmalauncher;