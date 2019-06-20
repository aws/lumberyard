local StateMachine = require "scripts/common/StateMachine"
local utilities = require "scripts/common/utilities"
local AnimParamUpdateFlags = require "scripts/Jack/AnimParamUpdateFlags"

local util = 
{
	LerpAimingAnim = function(weaponcontroller, timeLeft, targetValue, deltaTime)
		local targetDiff = targetValue - weaponcontroller.shootingParamValue;
		if(deltaTime >= timeLeft) then
			deltaTime = timeLeft;
		end
		local timeFrac = deltaTime / timeLeft;
		weaponcontroller.shootingParamValue = weaponcontroller.shootingParamValue + (timeFrac * targetDiff);
		return timeLeft - deltaTime;
	end
}
local weaponcontroller = 
{
    Properties = 
    {
		UIMessages = 
		{
			SetWeaponStatusMessage = { default = "SetWeaponStatusEvent", description = "set the visible crosshair type" },
		},
		
		Events =
		{
			ControlsEnabled = { default = "EnableControlsEvent", description = "If passed '1.0' it will enable controls, oherwise it will disable them." },
		},
		
		-- The weapons to be used.
		Weapons =
		{

			GenericEvents =
			{
				Fire = "EventFire";
				Reload = "EventReload";
				UseWeaponForwardForAiming = "EventUseWeaponForwardForAiming";
				OwnerDied = "HealthEmpty";
			},
			-- The bone that the weapons will attach to.
			-- This may need to become weapon-specific if we end up having weird weapons (e.g. shoulder-mounted).
			AimDelay = { default = 2.5, description = "Time that the player should keep aiming after firing" },
			BeginAimingLerpTime = {default = 0.1, description = "Time that the character takes to start aiming at target after pressing fire" },
			EndAimingLerpTime = {default = 0.3, description = "Time that the character takes to holster the waapon after AimDelay elapses" },
			MaxUpAimAngle = {default = 55, description = "Maximum angle that the character can aim upwards in degrees."},
			MaxDownAimAngle = {default = 35, description = "Maximum angle that the character can aim downwards in degrees."},
			MaxHorizontalAimAngle = {default = 155, description = "Maximum angle in degrees that the player can aim behind them relative to the forward direction."},
		},
        InitialState = "Down",
        DebugStateMachine = false,
    },
    States = 
    {
		-- Weapon is down
        Down = 
        {      
            OnEnter = function(self, sm)
				sm.UserData.shootingParamValue = 0.0;
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData.isAiming = false;
            end,
            OnExit = function(self, sm)
				sm.UserData.isAiming = true;
            end,            
            OnUpdate = function(self, sm, deltaTime)
            end,
            Transitions =
            {
				Rising = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.WantsToShoot;
                    end	
				},
				Dead = 
				{
					Evaluate = function(state, sm)
						return sm.UserData.IsOwnerDead;
					end
				}
            },
        },
		-- Weapon is coming up after fire was pressed
        Rising = 
        {      
            OnEnter = function(self, sm)
				self.aimTimer = sm.UserData.Properties.Weapons.AimDelay;
				self.aimLerpTime = sm.UserData.Properties.Weapons.BeginAimingLerpTime;
				sm.UserData.animParamUpdateFlags:reset();
				GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(sm.UserData.entityId, "OnStartAiming", "float"), 0);
				self.aimLerpTarget = 1.0;
				self.aimTimer = 0.0;
            end,
            OnExit = function(self, sm)

            end,            
            OnUpdate = function(self, sm, deltaTime)
				if(self.aimLerpTime > 0) then
					self.aimLerpTime = util.LerpAimingAnim(sm.UserData, self.aimLerpTime, self.aimLerpTarget, deltaTime, nil);
				end
            end,           
            Transitions =
            {
				RisingInBuffer = 
				{
                    Evaluate = function(state, sm)
						return state.aimLerpTime == 0;
                    end	
				},
				Dead = 
				{
					Evaluate = function(state, sm)
						return sm.UserData.IsOwnerDead;
					end
				},
				Down =
				{
					Priority = 1,
                    Evaluate = function(state, sm)
						return sm.UserData.canFire == false;
                    end	
				}
            },
        },
		-- Small buffer after raising gun
		RisingInBuffer = 
		{
			OnEnter = function(self, sm)
				self.bufferTimeLeft = 0.1;
				sm.UserData.animParamUpdateFlags:reset();
            end,
            OnExit = function(self, sm)
				sm.UserData:TellWeaponToFire();
            end,            
            OnUpdate = function(self, sm, deltaTime)
				self.bufferTimeLeft = self.bufferTimeLeft - deltaTime;
			end,
            Transitions =
            {
				UpFireRequested = 
				{
                    Evaluate = function(state, sm)
						return state.bufferTimeLeft <= 0 and sm.UserData.WantsToShoot == true;
                end	
				},
				UpFireNotRequested = 
				{
                    Evaluate = function(state, sm)
						return state.bufferTimeLeft <= 0 and sm.UserData.WantsToShoot == false;
	                end	
				},
				Dead = 
				{
					Priority = 2,
					Evaluate = function(state, sm)
						return sm.UserData.IsOwnerDead;
					end
				},
				Down =
				{
					Priority = 1,
                    Evaluate = function(state, sm)
						return sm.UserData.canFire == false;
                    end	
				}
            },
		},
		-- Weapon up and ready to fire
        UpFireRequested = 
        {      
            OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
            end,
            OnExit = function(self, sm)
            end,            
            OnUpdate = function(self, sm, deltaTime)
				sm.UserData:TellWeaponToFire();
            end,
            Transitions =
            {
				UpFireNotRequested = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.WantsToShoot == false;
                    end	
				},
				Dead = 
				{
					Priority = 2,
					Evaluate = function(state, sm)
						return sm.UserData.IsOwnerDead;
					end
				},
				Down =
				{
					Priority = 1,
                    Evaluate = function(state, sm)
						return sm.UserData.canFire == false;
                    end	
				}
            },
        },
		-- Weapon up and ready to fire
        UpFireNotRequested = 
        {      
            OnEnter = function(self, sm)
				self.aimTimeLeft = sm.UserData.Properties.Weapons.AimDelay;
				sm.UserData.animParamUpdateFlags:reset();
            end,
            OnExit = function(self, sm)
            end,            
            OnUpdate = function(self, sm, deltaTime)
				self.aimTimeLeft = self.aimTimeLeft - deltaTime;
            end,
            Transitions =
            {
				UpFireRequested = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.WantsToShoot == true;
	                end	
				},
				Lowering = 
				{
                    Evaluate = function(state, sm)
						return state.aimTimeLeft <= 0;
                    end	
				},
				Dead = 
				{
					Priority = 2,
					Evaluate = function(state, sm)
						return sm.UserData.IsOwnerDead;
					end
				},
				Down =
				{
					Priority = 1,
                    Evaluate = function(state, sm)
						return sm.UserData.canFire == false;
                    end	
				}
            },
        },
		-- Weapon is lowering
        Lowering = 
        {      
            OnEnter = function(self, sm)
				self.aimLerpTime = sm.UserData.Properties.Weapons.EndAimingLerpTime;
				self.aimLerpTarget = 0;
				self.aimLerpComplete = nil;
				sm.UserData.isAiming = false;
				sm.UserData.animParamUpdateFlags:reset();
            end,
            OnExit = function(self, sm)
            end,            
            OnUpdate = function(self, sm, deltaTime)
				if(self.aimLerpTime > 0) then
					self.aimLerpTime = util.LerpAimingAnim(sm.UserData, self.aimLerpTime, self.aimLerpTarget, deltaTime, self.aimLerpComplete);
				end
            end,
            Transitions =
            {
				Down = 
				{
					Priority = 1,
                    Evaluate = function(state, sm)
						return state.aimLerpTime == 0 or sm.UserData.canFire == false;
					end	
				},
				Dead = 
				{
					Priority = 2,
					Evaluate = function(state, sm)
						return sm.UserData.IsOwnerDead;
					end
				}

            },
        },	
		Dead =
		{
			OnAnimationEvent = function(self, evName, startTime)
				if(evName == "SwitchRagdoll") then
					self.switchRagdoll = true;
				end
			end,
			OnEnter = function(self, sm)
				self.switchRagdoll = false;
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData.WantsToShoot = false;
				self.weaponTransform = TransformBus.Event.GetWorldTM(sm.UserData.weapon);
				-- Stop aiming
				self.aimLerpTime = sm.UserData.Properties.Weapons.EndAimingLerpTime;
				self.aimLerpTarget = 0;
				self.aimLerpComplete = nil;
				sm.UserData.isAiming = false;
			end,
            OnUpdate = function(self, sm, deltaTime)
				if(self.switchRagdoll == true) then
					self.switchRagdoll = false;
					AttachmentComponentRequestBus.Event.Detach(sm.UserData.weapon);
					local currentWeaponTransform = TransformBus.Event.GetWorldTM(sm.UserData.weapon);
					local prevRotation = Quaternion.CreateFromTransform(self.weaponTransform);
					local currRotation = Quaternion.CreateFromTransform(currentWeaponTransform);
					local prevPosition = self.weaponTransform:GetTranslation();
					local currPosition = currentWeaponTransform:GetTranslation();
					local offset = currPosition - prevPosition;
					local velocity = offset / deltaTime;
					PhysicsComponentRequestBus.Event.EnablePhysics(sm.UserData.weapon);
					PhysicsComponentRequestBus.Event.SetVelocity(sm.UserData.weapon, velocity);
				end
				if(self.aimLerpTime > 0) then
					self.aimLerpTime = util.LerpAimingAnim(sm.UserData, self.aimLerpTime, self.aimLerpTarget, deltaTime, self.aimLerpComplete);
				end
				self.weaponTransform = TransformBus.Event.GetWorldTM(sm.UserData.weapon);				
            end,
            Transitions =
            {
            },
		},
	},
	MaxAimRange = 1000.0, -- Range to do ray cast when detecting aim location
}

--------------------------------------------------
-- Component behavior
--------------------------------------------------

function weaponcontroller:OnActivate()   

	self.animParamUpdateFlags = AnimParamUpdateFlags:create();
 
	AudioTriggerComponentRequestBus.Event.Play(self.entityId);
	
	-- Enable and disable events
	self.enableEventId = GameplayNotificationId(self.entityId, "Enable", "float");
	self.enableHandler = GameplayNotificationBus.Connect(self, self.enableEventId);
	self.disableEventId = GameplayNotificationId(self.entityId, "Disable", "float");
	self.disableHandler = GameplayNotificationBus.Connect(self, self.disableEventId);
	
	
	self.enableFiringEventId = GameplayNotificationId(self.entityId, "EnableFiring", "float");
	self.enableFiringHandler = GameplayNotificationBus.Connect(self, self.enableFiringEventId);
	self.disableFiringEventId = GameplayNotificationId(self.entityId, "DisableFiring", "float");
	self.disableFiringHandler = GameplayNotificationBus.Connect(self, self.disableFiringEventId);
	self.canFire = true;
	self.startedShootingWhenDisabled = false;

	-- Input listeners (weapon).
	self.weaponFirePressedEventId = GameplayNotificationId(self.entityId, "WeaponFirePressed", "float");
	self.weaponFirePressedHandler = GameplayNotificationBus.Connect(self, self.weaponFirePressedEventId);
	self.weaponFireReleasedEventId = GameplayNotificationId(self.entityId, "WeaponFireReleased", "float");
	self.weaponFireReleasedHandler = GameplayNotificationBus.Connect(self, self.weaponFireReleasedEventId);
	self.ownerDiedEventId = GameplayNotificationId(self.entityId, self.Properties.Weapons.GenericEvents.OwnerDied, "float");
	self.ownerDiedHandler = GameplayNotificationBus.Connect(self, self.ownerDiedEventId);

	self.jumpEventId = GameplayNotificationId(self.entityId, "Jump", "float");
	self.jumpHandler = GameplayNotificationBus.Connect(self, self.jumpEventId);
	self.WantsToJump = false;

	-- Tick needed to detect aim timeout
	self.tickBusHandler = TickBus.Connect(self);
	self.performedFirstUpdate = false;
	self.setupComplete = false;
	
	self.WantsToShoot = false;
	self.IsTargetingEnemy = false;
	self.isAiming = false;
	self.IsOwnerDead = false;

	self.EnergyRegenTimer = 0;
	self.minHorizontalAimDot = Math.Cos(Math.DegToRad(self.Properties.Weapons.MaxHorizontalAimAngle));
	self.maxVerticalAimDot = Math.Sin(Math.DegToRad(self.Properties.Weapons.MaxUpAimAngle));
	self.minVerticalAimDot = -Math.Sin(Math.DegToRad(self.Properties.Weapons.MaxDownAimAngle));

	-- weapon response listeners.
	self.weaponFireSuccessEventId = GameplayNotificationId(self.entityId, "WeaponFireSuccess", "float");
	self.weaponFireSuccessHandler = GameplayNotificationBus.Connect(self, self.weaponFireSuccessEventId);
	self.weaponFireFailEventId = GameplayNotificationId(self.entityId, "WeaponFireFail", "float");
	self.weaponFireFailHandler = GameplayNotificationBus.Connect(self, self.weaponFireFailEventId);

	self.onSetupNewWeaponEventId = GameplayNotificationId(self.entityId, "OnSetupNewWeapon", "float");
	self.onSetupNewWeaponHandler = GameplayNotificationBus.Connect(self, self.onSetupNewWeaponEventId);
	self.shootingParamValue = 0;
	
    -- Create and start our state machine.
    self.StateMachine = {}
    setmetatable(self.StateMachine, StateMachine);
    self.StateMachine:Start("Weapon", self.entityId, self, self.States, false, self.Properties.InitialState, self.Properties.DebugStateMachine)
	-- Direction from aim origin to do raycast to find aim target
	self.aimDirection = Vector3(0.0, 0.0, 1.0); 
	-- Origin of raycast to find aim target
	self.aimOrigin = Vector3(0,0,0);
	self.setAimDirectionEventId = GameplayNotificationId(self.entityId, "SetAimDirection", "float");
	self.setAimDirectionHandler = GameplayNotificationBus.Connect(self, self.setAimDirectionEventId);
	self.setAimOriginEventId = GameplayNotificationId(self.entityId, "SetAimOrigin", "float");
	self.setAimOriginHandler = GameplayNotificationBus.Connect(self, self.setAimOriginEventId);
	
	self.controlsDisabledCount = 0;
	self.controlsEnabled = true;
	self.controlsEnabledEventId = GameplayNotificationId(self.entityId, self.Properties.Events.ControlsEnabled, "float");
	self.controlsEnabledHandler = GameplayNotificationBus.Connect(self, self.controlsEnabledEventId);
	
	self.requestAimUpdateEventId = GameplayNotificationId(self.entityId, "RequestAimUpdate", "float");
	
	self.debugFireMessage = false;
	self.debugFireMessageEventId = GameplayNotificationId(self.entityId, "DebugFireMessage", "float");
	self.debugFireMessageHandler = GameplayNotificationBus.Connect(self, self.debugFireMessageEventId);

   
	self.previousShot = false;
	self.shot = false;

	-- Pitch angle and target vector
	self.pitchAngle = 0.0;
    AnimGraphComponentRequestBus.Event.SetNamedParameterFloat(self.entityId, "TargetPitchAngle", self.pitchAngle);
	self.targetVector = Vector3(0,0,0);
	
	-- Notify calculated target position
	self.setTargetPosId = GameplayNotificationId(self.entityId, "SetTargetPos", "float");
end

function weaponcontroller:OnDeactivate()
	self.weaponFirePressedHandler:Disconnect();
	self.weaponFirePressedHandler  = nil;
	self.weaponFireReleasedHandler:Disconnect();
	self.weaponFireReleasedHandler = nil;
	self.jumpHandler:Disconnect();
	self.jumpHandler  = nil;
    
	self.onSetupNewWeaponHandler:Disconnect();
	self.onSetupNewWeaponHandler = nil;
	self.tickBusHandler:Disconnect();
	self.tickBusHandler = nil;
	
	if (self.disableFiringHandler ~= nil) then
		self.disableFiringHandler:Disconnect();
		self.disableFiringHandler = nil;
	end
	if (self.enableFiringHandler ~= nil) then
		self.enableFiringHandler:Disconnect();
		self.enableFiringHandler = nil;
	end
	
	if (self.disableHandler ~= nil) then
		self.disableHandler:Disconnect();
		self.disableHandler = nil;
	end
	if (self.enableHandler ~= nil) then
		self.enableHandler:Disconnect();
		self.enableHandler = nil;
	end
end


function weaponcontroller:CheckCanHitTarget(target)
	if(self.weapon == nil) then
		return false;
	else
		local weaponTm = TransformBus.Event.GetWorldTM(self.weapon);
		local weaponForward = weaponTm:GetColumn(2):GetNormalized();
		local canHitTargetVertical = (weaponForward.z <= self.maxVerticalAimDot) and (weaponForward.z >= self.minVerticalAimDot);
		local tm = TransformBus.Event.GetWorldTM(self.entityId);
		weaponForward.z = 0;
		weaponForward:Normalize();
		local facing = tm:GetColumn(1):GetNormalized();
		local dot = facing:Dot(weaponForward);
		local canHitTargetHorizontal = dot > self.minHorizontalAimDot;
		return canHitTargetHorizontal and canHitTargetVertical;
	end
end

function weaponcontroller:UpdateAim()
	
	if (self.setupComplete) then
	
		local rayCastConfig = RayCastConfiguration();
		rayCastConfig.origin = self.aimOrigin;
		rayCastConfig.origin.z = 0;
		rayCastConfig.direction =  self.aimDirection;
		--if(self.isPlayerWeapon == false) then
			--Debug.Log("RCD: "..rayCastConfig.direction.x..", "..rayCastConfig.direction.y..", "..rayCastConfig.direction.z);
			--Debug.Log("AO: "..rayCastConfig.origin.x..", "..rayCastConfig.origin.y..", "..rayCastConfig.origin.z);
		--end
		rayCastConfig.maxDistance = self.MaxAimRange;
		rayCastConfig.maxHits = 10;
		rayCastConfig.physicalEntityTypes = PhysicalEntityTypes.All;
		rayCastConfig.piercesSurfacesGreaterThan = 13;
		local hits = PhysicsSystemRequestBus.Broadcast.RayCast(rayCastConfig);
		
		local target;
		local targetingEnemy = false;
		if(hits:HasBlockingHit()) then
			local hit = hits:GetBlockingHit();
			if (hit.entityId == self.entityId) then
				target = self.aimOrigin + self.aimDirection * self.MaxAimRange;
	
			else
				target = hit.position;
			end
			
			-- if im looking at an enemy, then i need to change the cursor
			if (hit.entityId:IsValid() and StarterGameEntityUtility.EntityHasTag(hit.entityId, "AICharacter")) then
				--Debug.Log("Looking At enemy");
				targetingEnemy = true;
				if (self.debugFireMessage) then
					GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(hit.entityId, "ShouldLog"), 0, "float");
				end
			end
		else
			target = self.aimOrigin + self.aimDirection * self.MaxAimRange;
		end
		
		--if(self.IsTargetingEnemy ~= targetingEnemy) then
			self.IsTargetingEnemy = targetingEnemy;
			--Debug.Log("TargetChanged : " .. tostring(targetingEnemy));
			local value = Vector2(0, 0);
			if (targetingEnemy) then
				local distToTarget = (target - self.aimOrigin):GetLength();
				value = Vector2(1, distToTarget);				
			end
			
			GameplayNotificationBus.Event.OnEventBegin(self.SetWeaponStatusEventId, value);
		--end
		if(self.animParamUpdateFlags.TargetPos.update == true) then
    		AnimGraphComponentRequestBus.Event.SetNamedParameterVector3(self.entityId, "TargetPos", target);
			GameplayNotificationBus.Event.OnEventBegin(self.setTargetPosId, target);

			-- Calculate the pitch angle based on the target position
			local playerTm = TransformBus.Event.GetWorldTM(self.entityId);
			local targetPlayerSpace = playerTm:GetInverseFull()*target;
			targetPlayerSpace.z = 0;
			targetPlayerSpace:Normalize();
			
			local shoulderPostion = Vector3(0,0,1.5);
			targetPlayerSpace = playerTm:GetInverseFull()*target;
			local shoulderToTarget = targetPlayerSpace - shoulderPostion;
			shoulderToTarget.x = 0;
			shoulderToTarget:Normalize();
			pitchAngle = Math.RadToDeg(Math.ArcCos(Vector3(0,1,0):Dot(shoulderToTarget)))
			pitchAngle = pitchAngle * Math.Sign(shoulderToTarget.z);
							
			AnimGraphComponentRequestBus.Event.SetNamedParameterFloat(self.entityId, "TargetPitchAngle", pitchAngle);
		end
	
		-- player always fires directly at reticule
		local canHitTarget = self.isPlayerWeapon or self:CheckCanHitTarget(target);
		local useWeaponForwardForAiming = 1.0;
		if(canHitTarget == true) then
			useWeaponForwardForAiming = 0.0;
		end
		GameplayNotificationBus.Event.OnEventBegin(self.activeWeaponUseWeaponForwardForAimingEventId, useWeaponForwardForAiming);
	end
end

function weaponcontroller:OnSetupNewWeapon(weapon)
	self.weapon = weapon;
	self.activeWeaponFireEventId = GameplayNotificationId(weapon, self.Properties.Weapons.GenericEvents.Fire, "float");
 	self.activeWeaponUseWeaponForwardForAimingEventId = GameplayNotificationId(weapon, self.Properties.Weapons.GenericEvents.UseWeaponForwardForAiming, "float");
 	local isLegacyCharacterEventId = GameplayNotificationId(weapon, "IsLegacyCharacter", "float");
	GameplayNotificationBus.Event.OnEventBegin(isLegacyCharacterEventId, 0.0);

	self.activeWeaponReloadEventId = GameplayNotificationId(weapon, self.Properties.Weapons.GenericEvents.Reload, "float");
	self.SetWeaponStatusEventId = GameplayNotificationId(weapon, self.Properties.UIMessages.SetWeaponStatusMessage, "float");
	self.setupComplete = true;
end

function weaponcontroller:TellWeaponToFire()
	GameplayNotificationBus.Event.OnEventBegin(self.activeWeaponFireEventId, 0);
end

function weaponcontroller:WeaponFireSuccess(value)
	self.FirstShotFired = true;
	self.shot = true;
end

function weaponcontroller:WeaponFireFail(value)
	self.FirstShotFired = true;
	
	-- FOR SOUND - depleated
	AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, "Play_HUD_energy_depleted");

end

function weaponcontroller:UpdateAiming(deltaTime)	
	-- The player needs to update aiming every frame (for the crosshair) but A.I. only need
	-- to update when they want to shoot.
	if (self.isAiming or self.isPlayerWeapon) then
		self:UpdateAim();
	end
	if(self.animParamUpdateFlags.Aiming.update == true) then
	     AnimGraphComponentRequestBus.Event.SetNamedParameterFloat(self.entityId, "Aiming", self.shootingParamValue);
	end
end

function weaponcontroller:OnTick(deltaTime, timePoint)
	
	if (not self.performedFirstUpdate) then
		local playerId = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"));
		self.isPlayerWeapon = (self.entityId == playerId);

		-- Make sure we don't do this 'first update' again.
		self.performedFirstUpdate = true;
	end

	GameplayNotificationBus.Event.OnEventBegin(self.requestAimUpdateEventId, nil);

    self:UpdateAiming(deltaTime); 
	
	self.debugFireMessage = false;
	if(self.previousShot == true and self.shot == true) then
		self.shot = false;
	end
	if(self.animParamUpdateFlags.Shot.update == true) then
   		AnimGraphComponentRequestBus.Event.SetNamedParameterBool(self.entityId, "Shot", self.shot);
	end
	self.previousShot = self.shot;
end

function weaponcontroller:OnEnable()
	self.StateMachine:Resume();
	self.tickBusHandler = TickBus.Connect(self);
end

function weaponcontroller:OnDisable()
	self.StateMachine:Stop();
	self.tickBusHandler:Disconnect();
end

function weaponcontroller:SetControlsEnabled(newControlsEnabled)
	self.controlsEnabled = newControlsEnabled;
	
	if(not newControlsEnabled) then
		self.WantsToShoot = false;
		self.startedShootingWhenDisabled = false;
	end
end

function weaponcontroller:OnEventBegin(value)
	--Debug.Log("weaponcontroller:OnEventBegin " .. tostring(GameplayNotificationBus.GetCurrentBusId()) .. " " .. tostring(value));
	if (GameplayNotificationBus.GetCurrentBusId() == self.controlsEnabledEventId) then
		--Debug.Log("weaponcontroller:controlsEnabledEventId " .. tostring(value));
		if (value == 1.0) then
			self.controlsDisabledCount = self.controlsDisabledCount - 1;
		else
			self.controlsDisabledCount = self.controlsDisabledCount + 1;
		end
		if (self.controlsDisabledCount < 0) then
			Debug.Log("[Warning] WeaponController: controls disabled ref count is less than 0.  Probable enable/disable mismatch.");
			self.controlsDisabledCount = 0;
		end		
		local newEnabled = self.controlsDisabledCount == 0;
		if (self.controlsEnabled ~= newEnabled) then
			self:SetControlsEnabled(newEnabled);
		end
	end

	if(self.controlsEnabled)then
		if (GameplayNotificationBus.GetCurrentBusId() == self.weaponFirePressedEventId) and self:IsDead() == false then
			if (self.canFire and self.WantsToJump == false) then
				self.WantsToShoot = true;
			else
				self.WantsToJump = false;
				self.startedShootingWhenDisabled = true;
			end
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.weaponFireReleasedEventId) and self:IsDead() == false then
			self.WantsToShoot = false;
			self.startedShootingWhenDisabled = false;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.jumpEventId) and self:IsDead() == false then			
			-- This is to ensure that tapping on the jump does not trigger a weaponfire. 
			self.WantsToShoot = false;
			self.WantsToJump = true;
		end
	end
	
	if (GameplayNotificationBus.GetCurrentBusId() == self.onSetupNewWeaponEventId) then
		self:OnSetupNewWeapon(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.setAimDirectionEventId) then
		self.aimDirection = value;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.setAimOriginEventId) then
		self.aimOrigin = value;
	end
	
	if (GameplayNotificationBus.GetCurrentBusId() == self.weaponFireSuccessEventId) then
		self:WeaponFireSuccess(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.weaponFireFailEventId) then
		self:WeaponFireFail(value);    
    end
	
	if (GameplayNotificationBus.GetCurrentBusId() == self.enableEventId) then
		self:OnEnable();
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.disableEventId) then
		self:OnDisable();
	end
	
	if (GameplayNotificationBus.GetCurrentBusId() == self.enableFiringEventId) then
		if(self.canFire == false) then
			self.canFire = true;
			self.WantsToShoot = self.startedShootingWhenDisabled;
            
            -- If we're enabling shooting, then we need to disable the jump at the same place.
            -- See logic for shooting/jumping further up in this function. WantsToJump is set to true when we receive the jump event.
            -- But, we're setting it to false only when we shoot again after the jump which causes the first shoot event to be ignored.
            self.WantsToJump = false;
		end
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.disableFiringEventId) then
		if(self.canFire == true) then	
			self.canFire = false;
			self.startedShootingWhenDisabled = self.WantsToShoot;
			self.WantsToShoot = false;
		end
	end
	
	if (GameplayNotificationBus.GetCurrentBusId() == self.ownerDiedEventId) then
		self.IsOwnerDead = true;
	end
	
	if (GameplayNotificationBus.GetCurrentBusId() == self.debugFireMessageEventId) then
		self.debugFireMessage = true;
	end
	
end

function weaponcontroller:IsDead()
	local isDead = self.StateMachine.CurrentState == self.States.Dead;
	return isDead;
end

return weaponcontroller;
