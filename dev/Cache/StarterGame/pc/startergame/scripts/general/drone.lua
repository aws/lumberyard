local utilities = require "scripts/common/utilities"
local StateMachine = require "scripts/common/StateMachine"

local drone =
{
	Properties =
	{
		InitiallyEnabled = { default = true, description = "Is this drone enabled when the game starts." },
		DebugStateMachine = { default = false, description = "Output debug information about the drone's states." },
		
		OffsetFollowing = { default = Vector3(-0.5, 0.2, 1.7), description = "Offset from player position when following." },
		DistanceBeforeFollowing = { default = 0.5, description = "How far the player must go before the drone starts moving to follow.", suffix = " m" },
		
		OffsetTalking = { default = Vector3(1.5, 2.0, 2.0), description = "Offset from player position when talking." },
		LingerAfterTalking = { default = 1.0, description = "How long to continue hovering after speech has ended.", suffix = " s" },
		
		SmoothingTargetFactor = { default = 1.0, description = "Linear interpolation factor on each OnTick to change the desired position from the previous to the current.", min = 0, max = 1},
		
		CameraEntityId = { default = EntityId() },
		PlayerEntityId = { default = EntityId() },
		
		Movement =
		{
			MaxSpeed = { default = 6.0, description = "Maximum movement speed", suffix = " m/s" },
			Acceleration = { default = 12.0, description = "Acceleration", suffix = " m/s/s" },
			--Deceleration = { default = 4.0, description = "Deceleration", suffix = " m/s/s" },
		},
		Rotation =
		{
			DebugLookAtTarget = { default = false, description = "Positions a sphere at where the drone is meant to looking." },
			LookAtTarget = { default = EntityId(), description = "The target entity that the drone will always be looking at." },
			
			MaxSpeed = { default = 180.0, description = "Maximum rotation speed", suffix = " deg/s" },
			Acceleration = { default = 90.0, description = "Acceleration", suffix = " deg/s/s" },
			--Deceleration = { default = 60.0, description = "Deceleration", suffix = " deg/s/s" },
		},
		
		Talking =
		{
			MaxSpeed = { default = 20.0, description = "Maximum movement speed", suffix = " m/s" },
			Acceleration = { default = 8.0, description = "Acceleration", suffix = " m/s/s" },
			--Deceleration = { default = 4.0, description = "Deceleration", suffix = " m/s/s" },
			
			Glow =
			{
				Speed = { default = 0.5, description = "Speed of 1.0 changes the emissive at a rate of 0 to 255 in one second." },
				Min = { default = 20.0, description = "Minimum emissive value [0 .. 255]" },
				Max = { default = 200.0, description = "Maximum emissive value [0 .. 255]" },
			},
		},
		
		Wobble =
		{
			PhaseOffset = { default = Vector3(0.0, 1.0, 2.0) },
			Speed = { default = Vector3(0.45773, 0.612345, 1.1241) },
			ScaleMin = { default = 0.1 },
			ScaleMax = { default = 0.02 },
			
			ResolveLookAtAfterWobble = { default = true, description = "If false, wobble will not affect look direction." },
		},
		
		Events =
		{
			Speak = { default = "DroneSpeak", description = "The payload should be the voice line to say." },
			Finished = { default = "DroneSpeakingFinished", description = "This event is broadcast when the drone finishes talking; the payload is the audio event." },
			Enable = { default = "DroneEnable", description = "Enable/Disable the drone." },
		},
	},
	
	EmissiveName = "Emissive_Intensity";
	EmissiveMat = 0;
	
	--------------------------------------------------
	-- A.I. State Machine Definition
	-- States:
	--		Inactivate	- does nothing
	--		Following	- moving to follow the player or idling
	--		Talking		- playing audio
	--		
	-- Transitions:
	--		Inactivate	<-  Following
	--					<-  Talking
	--		Idling		<-> Inactivate
	--					 -> Following
	--					 -> Talking
	--		Following	 -> Inactivate
	--					 -> Talking
	--		Talking		 -> Inactivate
	--					<-  Following
    --------------------------------------------------
	States = 
	{
		Inactive = 
		{
            OnEnter = function(self, sm)
				MeshComponentRequestBus.Event.SetVisibility(sm.UserData.entityId, false);
				
				-- Stop the movement audio.
				--sm.UserData.talkingRefCount = sm.UserData.talkingRefCount - 1;
				AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.entityId, "Stop_drone_movement");
            end,
			
            OnExit = function(self, sm)
				MeshComponentRequestBus.Event.SetVisibility(sm.UserData.entityId, true);
				
				-- Start the movement audio.
				AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.entityId, "Play_drone_movement");
				
				-- We'll probably want it to fly in or something.
				-- For now, just snap to the correct position.
				local playerTm = TransformBus.Event.GetWorldTM(sm.UserData.playerId);
				local offset = sm.UserData.Properties.OffsetFollowing;
				local targetPos = playerTm:GetTranslation() + (playerTm:GetColumn(0) * offset.x) + (playerTm:GetColumn(1) * offset.y) + (playerTm:GetColumn(2) * offset.z);
				
				sm.UserData.position = targetPos;
				--Debug.Log("Target Pos: " .. tostring(targetPos));
				--local tm = TransformBus.Event.GetWorldTM(sm.UserData.entityId);
				--tm:SetTranslation(targetPos);
				--TransformBus.Event.SetWorldTM(sm.UserData.entityId, tm);
				
				sm.UserData.lastPlayerPosReactedTo = playerTm;
            end,
			
            OnUpdate = function(self, sm, deltaTime)
            end,
			
            Transitions =
            {
				Following =
				{
                    Evaluate = function(state, sm)
						return sm.UserData.isEnabled;
                    end	
				},
            },
		},
		
		Following = 
		{
            OnEnter = function(self, sm)
				sm.UserData.Movement = sm.UserData.Properties.Movement;
				self.reachedTargetPos = false;
            end,
			
            OnExit = function(self, sm)
            end,
			
            OnUpdate = function(self, sm, deltaTime)
				local playerTm = TransformBus.Event.GetWorldTM(sm.UserData.playerId);
				local forward = playerTm:GetColumn(1);
				local offset = sm.UserData.Properties.OffsetFollowing;
				if (sm.UserData.isStrafing) then
					local camTm = TransformBus.Event.GetWorldTM(sm.UserData.cameraId);
					forward = camTm:GetColumn(1);
					playerTm = MathUtils.CreateLookAt(playerTm:GetTranslation(), playerTm:GetTranslation() + forward, AxisType.YPositive);
				end
				local targetPos = playerTm:GetTranslation() + (playerTm:GetColumn(0) * offset.x) + (playerTm:GetColumn(1) * offset.y) + (playerTm:GetColumn(2) * offset.z);
				local targetLookPos = targetPos + forward;
				sm.UserData:SetTargetPosition(targetPos, targetLookPos);
				
				-- The result should be true if we've arrived at the target location.
				self.reachedTargetPos = sm.UserData:UpdateMovement(deltaTime);
				sm.UserData:UpdateWobble(deltaTime);
				sm.UserData:UpdateRotation(deltaTime);
				sm.UserData:UpdateTransform(deltaTime);
				
				sm.UserData.lastPlayerPosReactedTo = playerTm;
            end,
			
            Transitions =
            {
				Talking = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData:IsTalking(); -- talking starts
                    end	
				},
				Inactive = 
				{
                    Evaluate = function(state, sm)
						return not sm.UserData.isEnabled;
                    end	
				},
            },
		},
		
		Talking = 
		{
            OnEnter = function(self, sm)
				sm.UserData.Movement = sm.UserData.Properties.Talking;
            end,
			
            OnExit = function(self, sm)
            end,
			
            OnUpdate = function(self, sm, deltaTime)
				local playerTm = TransformBus.Event.GetWorldTM(sm.UserData.playerId);
				local offset = sm.UserData.Properties.OffsetTalking;
				local targetPos = playerTm:GetTranslation() + (playerTm:GetColumn(0) * offset.x) + (playerTm:GetColumn(1) * offset.y) + (playerTm:GetColumn(2) * offset.z);
				local targetLookPos = StarterGameUtility.GetJointWorldTM(sm.UserData.playerId, utilities.Joint_Head):GetTranslation();
				sm.UserData:SetTargetPosition(targetPos, targetLookPos);
				
				sm.UserData:UpdateMovement(deltaTime);
				sm.UserData:UpdateWobble(deltaTime);
				sm.UserData:UpdateRotation(deltaTime);
				sm.UserData:UpdateTalking(deltaTime);
				sm.UserData:UpdateTransform(deltaTime);
			end,
			
            Transitions =
            {
				Following =
				{
                    Evaluate = function(state, sm)
						return sm.UserData:HasFinishedTalking(); -- talking complete
                    end	
				},
				Inactive =
				{
                    Evaluate = function(state, sm)
						return not sm.UserData.isEnabled;
                    end	
				},
			},
		},
	},
}


function drone:OnActivate()
	
	self.Properties.Rotation.MaxSpeed = Math.DegToRad(self.Properties.Rotation.MaxSpeed);
	self.Properties.Rotation.Acceleration = Math.DegToRad(self.Properties.Rotation.Acceleration);
	--self.Properties.Rotation.Deceleration = Math.DegToRad(self.Properties.Rotation.Deceleration);
	
	self.Properties.Talking.Glow.Min = utilities.Clamp(self.Properties.Talking.Glow.Min, 0.0, 255.0);
	self.Properties.Talking.Glow.Max = utilities.Clamp(self.Properties.Talking.Glow.Max, 0.0, 255.0);
	self.Properties.Talking.Glow.Speed = self.Properties.Talking.Glow.Speed * 255.0;
	
	self.setVisibleEventId = GameplayNotificationId(self.entityId, "SetVisible", "float");
	self.setVisibleHandler = GameplayNotificationBus.Connect(self, self.setVisibleEventId);
	
	self.teleportToEventId = GameplayNotificationId(self.entityId, "TeleportTo", "float");
	self.teleportToHandler = GameplayNotificationBus.Connect(self, self.teleportToEventId);
	
    self.StateMachine = {}
    setmetatable(self.StateMachine, StateMachine)
	
	self.isEnabled = self.Properties.InitiallyEnabled;
	self.talkingRefCount = 0;
	self.talkingDelay = self.Properties.LingerAfterTalking;
	self.talkingEventNames = {};
	self.emissiveTarget = 0;
	self.materialCloned = false;
	
	self.targetPosition = Vector3(0.0, 0.0, 0.0);
	self.lastPlayerPosReactedTo = Vector3(0.0, 0.0, 0.0);
	self.speed = 0.0;
	self.rotationSpeed = 0.0;
	self.lookAtTargetPos = Vector3(0.0, 0.0, 0.0);
	
	-- This is done in OnTick to accomodate 'Play From Here'.
	--self.position = TransformBus.Event.GetWorldTM(self.entityId):GetTranslation();
	--self.rotation = Transform.CreateIdentity();
	
	self.wobbleTime = 0.0;
	self.wobbleOffset = Vector3(0.0, 0.0, 0.0);
	self.wobble = Vector3(0.0, 0.0, 0.0);
	self.wobbleScale = 0.1;
	
	self.maxSpeed = math.max(self.Properties.Movement.MaxSpeed, self.Properties.Talking.MaxSpeed);
		
	self.cameraPosition = TransformBus.Event.GetWorldTM(self.Properties.CameraEntityId):GetTranslation();
	
	self.currentAnimationState = "";
	self.cameraJitterCompensationFactor = 0.0;
	
	self.tickBusHandler = TickBus.Connect(self);
	self.motionEventHandler = ActorNotificationBus.Connect(self, self.Properties.PlayerEntityId);
	self.transformBusHandler = TransformNotificationBus.Connect(self, self.Properties.CameraEntityId);
end

function drone:OnStateEntered(stateName)
	self.currentAnimationState = stateName;
end

function drone:OnTransformChanged(localTm, worldTm)
	local newCameraPos = worldTm:GetTranslation();
	local deltaPos = newCameraPos - self.cameraPosition;

	if (self.position ~= nil) then
		self.position = self.position + deltaPos * self.cameraJitterCompensationFactor;
	end
	self.cameraPosition = newCameraPos;
end

function drone:OnDeactivate()
	if (self.materialCloned) then
		StarterGameMaterialUtility.RestoreOriginalMaterial(self.entityId);
	end
	
	if (self.strafingHandler ~= nil) then
		self.strafingHandler:Disconnect();
		self.strafingHandler = nil;
	end
	if (self.audioHandler ~= nil) then
		self.audioHandler:Disconnect();
		self.audioHandler = nil;
	end
	if (self.audioNotificationHandler ~= nil) then
		self.audioNotificationHandler:Disconnect();
		self.audioNotificationHandler = nil;
	end
	if (self.tickBusHandler ~= nil) then
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end
	if (self.teleportToHandler ~= nil) then
		self.teleportToHandler:Disconnect();
		self.teleportToHandler = nil;
	end
	if (self.setVisibleHandler ~= nil) then
		self.setVisibleHandler:Disconnect();
		self.setVisibleHandler = nil;
	end
	
	if (self.motionEventHandler ~= nil) then
		self.motionEventHandler:Disconnect();
		self.motionEventHandler = nil;
	end
	
	if (self.transformBusHandler ~= nil) then
		self.transformBusHandler:Disconnect();
		self.transformBusHandler = nil;
	end
end

function drone:OnTick(deltaTime)
	-- material may not be available for cloning immediately, so we keep trying until the clone is successful
	self.materialCloned = StarterGameMaterialUtility.ReplaceMaterialWithClone(self.entityId);
	
	if (self.materialCloned) then
		local initialState = "Inactive";
		if (self.Properties.InitiallyEnabled) then
			initialState = "Following";
			AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, "Play_drone_movement");
		end
		
		self.position = TransformBus.Event.GetWorldTM(self.entityId):GetTranslation();
		self.rotation = Transform.CreateIdentity();
		
		-- Get original emissive value from material.
		self.emissiveOriginal = StarterGameMaterialUtility.GetSubMtlShaderFloat(self.entityId, self.EmissiveName, self.EmissiveMat);
		self.emissiveTarget = self.emissiveOriginal;
		--Debug.Log("Original Emissive: " .. tostring(self.emissiveOriginal));
		
		self.targetLookPosition = TransformBus.Event.GetWorldTM(self.entityId):GetTranslation();
		local lookAtTm = TransformBus.Event.GetWorldTM(self.Properties.Rotation.LookAtTarget);
		lookAtTm:SetTranslation(self.targetLookPosition);
		TransformBus.Event.SetWorldTM(self.Properties.Rotation.LookAtTarget, lookAtTm);
		MeshComponentRequestBus.Event.SetVisibility(self.Properties.Rotation.LookAtTarget, self.Properties.Rotation.DebugLookAtTarget);
		
		self.StateMachine:Start("Drone", self.entityId, self, self.States, false, initialState, self.Properties.DebugStateMachine);
		
		self.playerId = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"));
		self.cameraId = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCamera"));
		
		self.audioNotificationHandler = AudioTriggerComponentNotificationBus.Connect(self, self.entityId);
		
		self.audioEventId = GameplayNotificationId(EntityId(), self.Properties.Events.Speak, "float");
		self.audioHandler = GameplayNotificationBus.Connect(self, self.audioEventId);
		
		self.audioFinishedEventId = GameplayNotificationId(EntityId(), self.Properties.Events.Finished, "float");
		
		self.enableEventId = GameplayNotificationId(EntityId(), self.Properties.Events.Enable, "float");
		self.enableHandler = GameplayNotificationBus.Connect(self, self.enableEventId);
		
		self.strafingEventId = GameplayNotificationId(self.entityId, "IsStrafing", "float");
		self.strafingHandler = GameplayNotificationBus.Connect(self, self.strafingEventId);
		self.justStartedStrafing = false;
		self.isStrafing = false;
		
		-- now that we're initialised, there is no need for any more ticks
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end
end

function drone:SetTargetPosition(targetPos, lookPos)
	self.targetPosition = self.targetPosition + (targetPos - self.targetPosition) * self.Properties.SmoothingTargetFactor;
	self.targetLookPosition = lookPos;
end

function drone:UpdateMovement(deltaTime)
	
	if (self.currentAnimationState == "Move") then
		self.cameraJitterCompensationFactor = self.cameraJitterCompensationFactor + 4.0 * deltaTime;
	else
		self.cameraJitterCompensationFactor = self.cameraJitterCompensationFactor - 0.5 * deltaTime;
	end
	
	self.cameraJitterCompensationFactor = utilities.Clamp(self.cameraJitterCompensationFactor, 0.0, 1.0);

	local pos = self.position;
	local dirToTarget = self.targetPosition - pos;
	local distance = dirToTarget:GetLength();
	
	if (distance <= 0.01) then
		-- Reached target position.
		self.speed = 0.0;
		return true;
	end

	local standardAcc = self.Movement.Acceleration;
	local u = self.speed;
	local s = distance;
	local a = math.abs((u*u) / (2.0 * s));
	
	if (a < standardAcc) then
		self.speed = utilities.Clamp(self.speed + (standardAcc * deltaTime), 0.0, self.Movement.MaxSpeed);
	else
		self.speed = utilities.Clamp(self.speed - (a * deltaTime), 0.0, self.Movement.MaxSpeed);
	end
	local newPos = pos + (dirToTarget:GetNormalized() * (self.speed * deltaTime));
	
	self.position = newPos;
	return false;
end

function drone:UpdateWobble(deltaTime)
	--self.wobbleTime = self.wobbleTime + deltaTime;
	--self.wobbleOffset.z = math.sin(self.wobbleTime) * self.wobbleScale;
	
	-- sin wave per axis
	-- -1 .. 1 offset range
	self.wobbleTime = self.wobbleTime + deltaTime;
	local wobbleVec = self.Properties.Wobble.PhaseOffset + (self.Properties.Wobble.Speed * self.wobbleTime);
	self.wobbleOffset = Vector3(Math.Sin(wobbleVec.x), Math.Sin(wobbleVec.y), Math.Sin(wobbleVec.z));
	
	-- adjust scale based on speed
	local targetWobbleScale = self.Properties.Wobble.ScaleMin + ((self.speed / self.Movement.MaxSpeed) * (self.Properties.Wobble.ScaleMax - self.Properties.Wobble.ScaleMin));
	local wobbleScaleDelta = targetWobbleScale - self.wobbleScale;
	local maxWobbleDelta = 0.1 * deltaTime; -- cap rate of change of wobble scale
	self.wobbleScale = self.wobbleScale + utilities.Clamp(wobbleScaleDelta, -maxWobbleDelta, maxWobbleDelta);
	self.wobble = self.wobbleOffset * self.wobbleScale;
end

function drone:UpdateRotation(deltaTime)
	-- Update the positioning of the lookat entity.
	local lookAtTm = TransformBus.Event.GetWorldTM(self.Properties.Rotation.LookAtTarget);
	local dirToTarget = self.targetLookPosition - lookAtTm:GetTranslation();
	local distToTarget = dirToTarget:GetLength();
	local lookAtSpeed = 10.0 * deltaTime;	-- it moves at a (fast) constant speed
	if (distToTarget <= lookAtSpeed) then
		lookAtTm:SetTranslation(self.targetLookPosition);
	else
		lookAtTm:SetTranslation(lookAtTm:GetTranslation() + (dirToTarget:GetNormalized() * lookAtSpeed));
	end
	TransformBus.Event.SetWorldTM(self.Properties.Rotation.LookAtTarget, lookAtTm);
	
	-- Now update the drone's rotation so it looks at the target entity.
	local pos = self.position;
	if (self.Properties.Wobble.ResolveLookAtAfterWobble) then
		pos = pos + self.wobble;
	end
	self.rotation = MathUtils.CreateLookAt(pos, lookAtTm:GetTranslation(), AxisType.YPositive);
	self.rotation:SetTranslation(Vector3(0.0, 0.0, 0.0));
end

function drone:UpdateTalking(deltaTime)
	local isTalking = self:IsTalking();
	if (not isTalking) then
		self.talkingDelay = self.talkingDelay - deltaTime;
	else
		self.talkingDelay = self.Properties.LingerAfterTalking;
	end
	
	local currEmissive = StarterGameMaterialUtility.GetSubMtlShaderFloat(self.entityId, self.EmissiveName, self.EmissiveMat);
	--Debug.Log("Current Emissive: " .. tostring(currEmissive));
	
	-- Raise/Lower emissive.
	local emissiveSpeed = self.Properties.Talking.Glow.Speed * deltaTime;
	local emissiveDiff = self.emissiveTarget - currEmissive;
	emissiveSpeed = math.min(math.abs(emissiveSpeed), math.abs(emissiveDiff));
	if (emissiveDiff < 0.0) then
		emissiveSpeed = -emissiveSpeed;
	end
	
	currEmissive = currEmissive + emissiveSpeed;
	
	if (currEmissive == self.emissiveTarget) then
		if (isTalking) then
			local minEmissive = self.Properties.Talking.Glow.Min;
			local maxEmissive = self.Properties.Talking.Glow.Max;
			self.emissiveTarget = math.random(maxEmissive - minEmissive) + minEmissive;
		end
	end
	
	--Debug.Log("Emissive Speed: " .. tostring(emissiveSpeed) .. ", New Emissive: " .. tostring(currEmissive));
	StarterGameMaterialUtility.SetSubMtlShaderFloat(self.entityId, self.EmissiveName, self.EmissiveMat, currEmissive);
end

function drone:UpdateTransform(deltaTime)
	local prevTm = TransformBus.Event.GetWorldTM(self.entityId);
	local tm = self.rotation;
	tm:SetPosition(self.position + self.wobble);
	TransformBus.Event.SetWorldTM(self.entityId, tm);
	
	-- Update the movement audio's R.T.P.C.
	local dist = (prevTm:GetTranslation() - tm:GetTranslation()):GetLength();
	local speed = (dist / deltaTime) / self.maxSpeed;
	AudioRtpcComponentRequestBus.Event.SetRtpcValue(self.entityId, "rtpc_shipAIdrone_speed", utilities.Clamp(speed, 0.0, 1.0));
	--Debug.Log("Drone speed: " .. tostring(speed));
	
	-- Update the drone's height R.T.P.C.
	local heightChange = tm:GetTranslation().z - prevTm:GetTranslation().z;
	local verticalSpeed = (heightChange / deltaTime) / self.maxSpeed;
	if (heightChange < 0.0) then
		verticalSpeed = -verticalSpeed;
	end
	AudioRtpcComponentRequestBus.Event.SetRtpcValue(self.entityId, "rtpc_shipAIdrone_height", verticalSpeed);
	--Debug.Log("Drone height change: " .. tostring(heightChange) .. ", speed: " .. tostring(verticalSpeed));
	
	-- By this point, everything that should need this should've used it.
	self.justStartedStrafing = false;
end

--function drone:CalculateStandardAcceleration(currSpeed, acc, maxSpeed, distance, deltaTime)
--	local res = 0.0;
--	local a = math.abs((currSpeed * currSpeed) / (2.0 * distance));
--	
--	if (a < acc) then
--		res = utilities.Clamp(currSpeed + (acc * deltaTime), 0.0, maxSpeed);
--	else
--		res = utilities.Clamp(currSpeed - (a * deltaTime), 0.0, maxSpeed);
--	end
--	
--	return res;
--end

function drone:IsTalking()
	return self.talkingRefCount ~= 0;
end

function drone:HasFinishedTalking()
	--Debug.Log("Ref Count: " .. tostring(self.talkingRefCount) .. ", Delay: " .. tostring(self.talkingDelay));
	return not self:IsTalking() and self.talkingDelay <= 0.0;
end

function drone:OnTriggerFinished(triggerId)
	self.talkingRefCount = self.talkingRefCount - 1;
	
	for i,t in ipairs(self.talkingEventNames) do
		local crc = Crc32(t);
		if (crc.value == triggerId) then
			--Debug.Log("Drone finished audio: " .. tostring(t));
			GameplayNotificationBus.Event.OnEventBegin(self.audioFinishedEventId, t);
			table.remove(self.talkingEventNames, i);
			break;
		end
	end
	
	if (not self:IsTalking()) then
		self.emissiveTarget = self.emissiveOriginal;
	end
end

function drone:OnEventBegin(value)
	local busId = GameplayNotificationBus.GetCurrentBusId();
	if (busId == self.audioEventId) then
		if (value ~= nil and value ~= "") then
			AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, value);
			table.insert(self.talkingEventNames, value);
			self.talkingRefCount = self.talkingRefCount + 1;
		else
			Debug.Log("DroneSpeak event received with empty payload. Ignoring.");
		end
	elseif (busId == self.enableEventId) then
		self.isEnabled = value;
	elseif (busId == self.setVisibleEventId) then
		MeshComponentRequestBus.Event.SetVisibility(self.entityId, value);
	elseif (busId == self.teleportToEventId) then
		self.position = value;
	elseif (busId == self.strafingEventId) then
		self.isStrafing = value;
		self.justStartedStrafing = value;
	end
end



return drone;

