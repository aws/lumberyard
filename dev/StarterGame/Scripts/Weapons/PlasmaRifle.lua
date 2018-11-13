require "scripts/Weapons/Weapon"
local utilities = require "scripts/common/utilities"

local plasmarifle =
{
	Properties = {
	}
}

PopulateProperties(plasmarifle.Properties);

plasmarifle.Properties.Firepower.Range = { default = 1000.0, description = "The range that the bullets can hit something.", suffix = " m" };
plasmarifle.Properties.Firepower.Damage = { default = 10.0, description = "The amount of damage a single shot does." };
plasmarifle.Properties.Firepower.ForceMultiplier = { default = 400.0, description = "The strength of the impulse." };
plasmarifle.Properties.Laser =
	{
		StartEffect = { default = "SpawnLaserStart" },
		EndEffect = { default = "SpawnLaserEnd" },
	};
plasmarifle.Properties.Events.GotShot = "GotShotEvent";
plasmarifle.Properties.Events.DealDamage = "HealthDeltaValueEvent";
plasmarifle.Properties.EndOfBarrel = { default = EntityId() };
plasmarifle.Properties.Events.Owner = "EventOwner";
plasmarifle.Properties.SoundRange.GunShot = 15.0;
plasmarifle.Properties.SoundRange.ShotHit = { default = 7.5, description = "The distance at which a shot hitting an object can be heard.", suffix = " m" };
plasmarifle.Properties.SoundRange.WhizzPast = { default = 4.0, description = "The distance at which a shot going past is noticable.", suffix = " m" };

function plasmarifle:SendEventToParticle(event, from, to, targetToFollow)

	local params = ParticleSpawnerParams();
	params.transform = MathUtils.CreateLookAt(from, to, AxisType.YPositive);
	params.event = event;
	params.targetId = targetToFollow;
	params.attachToEntity = targetToFollow:IsValid();
	
	GameplayNotificationBus.Event.OnEventBegin(self.particleSpawnEventId, params);

end

function plasmarifle:DoFirstUpdate()
	local particleMan = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("ParticleManager"));
	self.particleSpawnEventId = GameplayNotificationId(particleMan, "SpawnParticleEvent", "float");
end

function plasmarifle:DoFire(useForwardForAiming, playerOwned)
	-- Perform a raycast from the gun into the world. If the ray hits something then apply an
	-- impulse to it. This weapon's bullet won't have physics because it's the raycast that'll
	-- apply the impulse.
	local mask = PhysicalEntityTypes.Static + PhysicalEntityTypes.Dynamic + PhysicalEntityTypes.Living + PhysicalEntityTypes.Terrain;
	local camTm = TransformBus.Event.GetWorldTM(TagGlobalRequestBus.Event.RequestTaggedEntities(self.cameraFinderTag));
	local camPos = camTm:GetTranslation();
	local endOfBarrelPos = TransformBus.Event.GetWorldTM(self.Properties.EndOfBarrel):GetTranslation();
	local dir;
	local hits = nil;
	local rch = nil;
	local hasBlockingHit = false;
	
	local rayCastConfig = RayCastConfiguration();
	rayCastConfig.maxHits = 10;
	rayCastConfig.physicalEntityTypes = mask;
	rayCastConfig.piercesSurfacesGreaterThan = 13;
	rayCastConfig.maxDistance = self.Properties.Firepower.Range;
	
	-- Always use the weapon's direction for A.I.
	if (not playerOwned or useForwardForAiming == true) then
	
		local weaponForward;		
		if (self.targetPos ~= nil) then
			weaponForward = self.targetPos - endOfBarrelPos;
		else
			weaponForward = self.weapon:GetWeaponForward();
		end
		dir = self.weapon:GetJitteredDirection(weaponForward:GetNormalized());
		
		rayCastConfig.origin = endOfBarrelPos;
		rayCastConfig.direction =  dir;
		hits = PhysicsSystemRequestBus.Broadcast.RayCast(rayCastConfig);
		hasBlockingHit = hits:HasBlockingHit();
		rch = hits:GetBlockingHit();
	else
		rayCastConfig.origin = camPos;
		rayCastConfig.direction =  camTm:GetColumn(1);
		hits = PhysicsSystemRequestBus.Broadcast.RayCast(rayCastConfig);	
		hasBlockingHit = hits:HasBlockingHit();
		rch = hits:GetBlockingHit();
		local endPoint = nil;
		if (hasBlockingHit) then
			endPoint = rch.position;
		else
			-- If the camera's ray didn't hit anything then the gun's raycast should be to the
			-- maximum distance.
			endPoint = camPos + (camTm:GetColumn(1) * self.Properties.Firepower.Range);
		end
		
		-- Now do a ray from the gun to see if there's anything in the way.
		local offsetVector = endPoint - endOfBarrelPos;
		local offsetDistance = offsetVector:GetLength();
		dir = self.weapon:GetJitteredDirection(offsetVector / offsetDistance);
		
		rayCastConfig.origin = endOfBarrelPos;
		rayCastConfig.direction = dir;
		rayCastConfig.maxDistance = offsetDistance;
		local gunHits = PhysicsSystemRequestBus.Broadcast.RayCast(rayCastConfig);
		-- If the gun's raycast hit something then it means there's something in the way between the gun and
		-- the camera's target. This means we want to override the rch variable with this new information.
		-- If we didn't hit anything then we want to use the camera's rch information (as it may be that the
		-- gun's raycast fell short of hitting the camera's target because we're using its hit position as
		-- the maximum distance).
		if (gunHits:HasBlockingHit()) then
			hasBlockingHit = true;
			rch = gunHits:GetBlockingHit();
		end
	end
	
	-- Apply an impulse if we hit a physics object.
	if (hasBlockingHit) then
		if (rch.entityId:IsValid() and (StarterGameEntityUtility.EntityHasTag(rch.entityId, "PlayerCharacter") or StarterGameEntityUtility.EntityHasTag(rch.entityId, "AICharacter"))) then
			-- We don't want to apply an impulse to player or A.I. characters.
			local params = GotShotParams();
			params.damage = self.Properties.Firepower.Damage;
			params.direction = dir;
			params.assailant = self.Properties.Owner;
			local eventId = GameplayNotificationId(rch.entityId, self.Properties.Events.GotShot, "float");
			GameplayNotificationBus.Event.OnEventBegin(eventId, params);
		else
			--Debug.Log("Applying impulse to " .. tostring(rch.entityId) .. " with force " .. dir.x .. ", " .. dir.y .. ", " .. dir.z);
			PhysicsComponentRequestBus.Event.AddImpulseAtPoint(rch.entityId, dir * self.Properties.Firepower.ForceMultiplier, rch.position);
		end
		
		-- send damage event, make sure that i cannot hurt myself
		local godMode = utilities.GetDebugManagerBool("GodMode", false);
		if (rch.entityId:IsValid() and ((rch.entityId ~= self.Properties.Owner) and not ((godMode == true) and StarterGameEntityUtility.EntityHasTag(rch.entityId, "PlayerCharacter"))) ) then
			local eventId = GameplayNotificationId(rch.entityId, self.Properties.Events.DealDamage, "float");
			--Debug.Log("Damaging [" .. tostring(rch.entityId) .. "] for " .. self.Properties.Firepower.Damage .. " by [" .. tostring(self.Properties.Owner) .. "] message \"" .. self.Properties.Events.DealDamage .. "\", combinedID == " .. tostring(eventId));
			GameplayNotificationBus.Event.OnEventBegin(eventId, -self.Properties.Firepower.Damage);
		end
	end
	
	-- Render the line.
	local rayEnd;
	if (hasBlockingHit) then
		rayEnd = rch.position;
	else
		-- If the raycast didn't hit anything then put the hit position as the
		-- barrel position + the range.
		rayEnd = endOfBarrelPos + (dir * self.Properties.Firepower.Range);
	end
	local rayStart = endOfBarrelPos;
	LineRendererRequestBus.Event.SetStartAndEnd(self.entityId, rayStart, rayEnd, camPos);
	
	-- Spawn the particle effects at the start and end (if a hit occured) of the line.
	--Debug.Log("Direction: " .. tostring(dir));
	self:SendEventToParticle(self.Properties.Laser.StartEffect, rayStart, rayEnd, self.entityId);
	if (hasBlockingHit) then
		self:SendEventToParticle(self.Properties.Laser.EndEffect, rayEnd, rayStart, EntityId());
		
		local particleManager = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("ParticleManager"));
		if (particleManager == nil or particleManager:IsValid() == false) then
			Debug.Assert("Invalid particle manager.");
		end
		
		local params = DecalSpawnerParams();
		params.event = "SpawnDecalRifleShot";
		params.transform = MathUtils.CreateLookAt(rayEnd, rayEnd + rch.normal, AxisType.ZPositive);
		params.surfaceType = StarterGameUtility.GetSurfaceFromRayCast(rayEnd - (dir * 0.5), dir);
		params.attachToEntity = rch.entityId:IsValid();
		if (params.attachToEntity == true) then
			params.targetId = rch.entityId;
		end
		
		-- Override the attachment if the entity doesn't exist.
		if (params.attachToEntity == true) then
			if (not params.targetId:IsValid()) then
				params.attachToEntity = false;
				Debug.Log("The decal was meant to follow an entity, but it doesn't have one assigned.");
			end
		end
		
		--Debug.Log("Decal surface: " .. tostring(params.surfaceType));
		local pmEventId = GameplayNotificationId(particleManager, "SpawnDecalEvent", "float");
		GameplayNotificationBus.Event.OnEventBegin(pmEventId, params);
	end
	
	-- Only the player's rifle should emit sounds.
	if (playerOwned) then
		-- NOTE: I have to use new variables for each sound because Lua stores them as pointers and
		-- it gets really confused if it tries to re-use the same variable.
		-- Broadcast the gunshot sound event to nearby entities via the sound manager.
		local propsGunShot = SoundProperties();
		propsGunShot.origin = rayStart;
		propsGunShot.range = self.Properties.SoundRange.GunShot;
		propsGunShot.type = SoundTypes.GunShot;
		AISoundManagerSystemRequestBus.Broadcast.BroadcastSound(propsGunShot);
		-- Broadcast the whizzing sound event to nearby entities via the sound manager.
		local propsWhizz = SoundProperties();
		propsWhizz.origin = rayStart;
		propsWhizz.destination = rayEnd;
		propsWhizz.range = self.Properties.SoundRange.WhizzPast;
		propsWhizz.type = SoundTypes.WhizzPast;
		AISoundManagerSystemRequestBus.Broadcast.BroadcastSound(propsWhizz);
		-- Broadcast the ricochet sound event to nearby entities via the sound manager.
		if (hasBlockingHit) then
			local propsShotHit = SoundProperties();
			propsShotHit.origin = rayEnd;
			propsShotHit.range = self.Properties.SoundRange.ShotHit;
			propsShotHit.type = SoundTypes.RifleHit;
			AISoundManagerSystemRequestBus.Broadcast.BroadcastSound(propsShotHit);
		end
	end
end

function plasmarifle:OnActivate()
	-- Use this to get the camera information when firing. It saves making an entity property
	-- and linking the weapon to a specific camera entity.
	-- Note: this only returns the LAST entity with this tag, so make sure there's only one
	-- entity with the "PlayerCamera" tag otherwise weird stuff might happen.
	self.cameraFinderTag = Crc32("PlayerCamera");
	self.weapon = weapon:new();
	self.weapon:OnActivate(self, self.entityId, self.Properties);
	
	if (self.Properties.Owner ~= nil) then
		self.setTargetPosEventId = GameplayNotificationId(self.Properties.Owner, "SetTargetPos", "float");
		self.setTargetPosHandler = GameplayNotificationBus.Connect(self, self.setTargetPosEventId);
	end	
end

function plasmarifle:OnDeactivate()
	self.weapon:OnDeactivate();
	
	if (self.setTargetPosHandler ~= nil) then
		self.setTargetPosHandler:Disconnect();
		self.setTargetPosHandler = nil;
	end	
end

function plasmarifle:OnEventBegin(value)
	local busId = GameplayNotificationBus.GetCurrentBusId();
	
	if (busId == self.setTargetPosEventId) then
		self.targetPos = value;
	end
end

return plasmarifle;