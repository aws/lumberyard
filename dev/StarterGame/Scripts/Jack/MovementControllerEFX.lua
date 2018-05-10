local utilities = require "scripts/common/utilities"
local StateMachine = require "scripts/common/StateMachine"
local AnimParamUpdateFlags = require "scripts/jack/AnimParamUpdateFlags"

local movementcontroller = 
{
	Properties = 
	{
		MoveSpeed = { default = 5.6, description = "How fast the player moves.", suffix = " m/s" },
		RotationSpeed = { default = 155.0, description = "How fast (in degrees per second) the player can turn.", suffix = " deg/sec"},
		Radius = { default = 0.4, description = "Character radius.", suffix = " m"},
		CrouchingHalfHeight = { default = 0.24, description = "Height of capsule when crouching.", suffix = " m"},
		StandingHalfHeight = { default = 0.4, description = "Height of capsule when standing.", suffix = " m"},
		Camera = {default = EntityId()},
		InitialState = "Idle",
		DebugStateMachine = false,
		DebugAnimStateMachine = true,
		CrouchToggle = false,
		Jumping =
		{
			JumpForce = { default = 8.0, description = "The force to jump with." },
			JumpDoubleForce = { default = 8.0, description = "The force of the double jump." },
			LandingHeight = { default = 0.5, description = "Height at which the character will start landing.", suffix = " m" },
		},
		
		Falling =
		{
			FallingHeight = { default = 0.8, description = "Distance above ground that will cause a transition to falling.", suffix = " m" },
			FallingMedium = { default = 0.3, description = "How long the fall must be to trigger a medium landing.", suffix = " s" },
			FallingHard = { default = 0.5, description = "How long the fall must be to trigger a hard landing.", suffix = " s" },
			
			FallingEscape = { default = 8.0, description = "How long to fall for before allowing the player to jump again.", suffix = " s" },
			Radius = { default = 1.0, description = "The radius of the character's capsule for escaping infinite falling.", suffix = " m" },
			
			SlopeAngle = { default = 73.0, description = "If the ground is too steep then falling will continue [0, 90].", suffix = " deg" },
			Nudging =
			{
				Acceleration = { default = 6.0, description = "The acceleration of the character's nudging speed." },
				MaxSpeed = { default = 2.0, description = "The character's maximum nudging speed." },
				
				CollisionTimer = { default = 0.15, description = "How long to disable nudging for when a collision occurs." },

			},
		},
		
		UIMessages = 
		{
			SetHideUIMessage = { default = "HideUIEvent", description = "The event used to set the whole UI visible / invisible" },
		},
		
		Events =
		{
			DiedMessage = { default = "HealthEmpty", description = "The event recieved when the player has died" }, 
						
			ControlsEnabled = { default = "EnableControlsEvent", description = "If passed '1.0' it will enable controls, otherwise it will disable them." },
			 
			GotShot = { default = "GotShotEvent", description = "Indicates the player was shot by something." },
		},
	},
	
	InputValues = 
	{
		NavForwardBack = 0.0,
		NavLeftRight = 0.0,
	},


	
	--------------------------------------------------
	States = 
	{
		-- Locomotion is the base state. All other states should transition back to Locomotion
		-- when complete.
		Idle2Move = 
		{
			-- Updates the character's rotation.
			updateRotation = function(deltaTime, angleDelta, movementController)
				local tm = TransformBus.Event.GetWorldTM(movementController.entityId);
				if (angleDelta ~= 0.0) then
					local rotationRate = movementController.Properties.RotationSpeed;
					local thisFrame = rotationRate * deltaTime;
					local absAngleDelta = movementController:Abs(angleDelta);
					if (absAngleDelta > FloatEpsilon) then
						thisFrame = movementController:Clamp(angleDelta, -thisFrame, thisFrame);
						local rotationTm = Transform.CreateRotationZ(thisFrame);
						tm = tm * rotationTm;
						tm:Orthogonalize();
						TransformBus.Event.SetWorldTM(movementController.entityId, tm);			
					end
				end  
			end,
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData:SetCanFire(true);
			end,
			OnExit = function(self, sm)
			end,			
			OnUpdate = function(self, sm, deltaTime)
				local angleDelta = sm.UserData:GetAngleDelta();
				self.updateRotation(deltaTime, angleDelta, sm.UserData);
			end,
			Transitions =
			{
			},
		},
		Move = 
		{
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData.animParamUpdateFlags.TurnAngle.show = true;
				sm.UserData:SetCanFire(true);
			end,
			OnExit = function(self, sm)
			end,			
			OnUpdate = function(self, sm, deltaTime)
			end,
			
			Transitions =
			{
			},
		},
		Idle = 
		{
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData.animParamUpdateFlags.TurnAngle.show = true;
				sm.UserData.animParamUpdateFlags.Speed.show = true;
				sm.UserData.animParamUpdateFlags.WasHit.show = true;
				sm.UserData:SetCanFire(true);
			end,
			OnExit = function(self, sm)
			end,			
			OnUpdate = function(self, sm, deltaTime)
			end,
			
			Transitions =
			{
			},
		},
		SlowStrafe = 
		{
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData.animParamUpdateFlags.WantsToStrafe.show = true;
				sm.UserData.animParamUpdateFlags.Speed.show = true;
				sm.UserData:SetCanFire(true);
			end,
			OnExit = function(self, sm)
			end,			
			OnUpdate = function(self, sm, deltaTime)
			end,
			
			Transitions =
			{
			},
		},
		SlowStrafeIdle = 
		{
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData.animParamUpdateFlags.WantsToStrafe.show = true;
				sm.UserData.animParamUpdateFlags.Speed.show = true;
				sm.UserData:StrafeAlign();
				sm.UserData:EnableStrafeCamera();
				sm.UserData:SetCanFire(true);
			end,
			OnExit = function(self, sm)
				sm.UserData:DisableStrafeCamera();
			end,			
			OnUpdate = function(self, sm, deltaTime)
				sm.UserData:StrafeAlign();
			end,
			Transitions =
			{
			},
		},
		SlowStrafeMove = 
		{
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData.animParamUpdateFlags.WantsToStrafe.show = true;
				sm.UserData.animParamUpdateFlags.Speed.show = true;
				sm.UserData:EnableStrafeCamera();
				sm.UserData:SetCanFire(true);
			end,
			OnExit = function(self, sm)
				sm.UserData:DisableStrafeCamera();
			end,			
			OnUpdate = function(self, sm, deltaTime)
				sm.UserData:StrafeAlign();
			end,
			Transitions =
			{
			},
		},
		FastStrafe = 
		{
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData.animParamUpdateFlags.WantsToStrafe.show = true;
				sm.UserData.animParamUpdateFlags.Speed.show = true;
				sm.UserData:SetCanFire(true);
			end,
			OnExit = function(self, sm)
			end,			
			OnUpdate = function(self, sm, deltaTime)
			end,
			
			Transitions =
			{
			},
		},
		FastStrafeIdle = 
		{
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData.animParamUpdateFlags.WantsToStrafe.show = true;
				sm.UserData.animParamUpdateFlags.Speed.show = true;
				sm.UserData:StrafeAlign();
				sm.UserData:SetCanFire(true);
			end,
			OnExit = function(self, sm)
			end,			
			OnUpdate = function(self, sm, deltaTime)
				sm.UserData:StrafeAlign();
			end,
			Transitions =
			{
			},
		},
		FastStrafeMove = 
		{
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData.animParamUpdateFlags.WantsToStrafe.show = true;
				sm.UserData.animParamUpdateFlags.Speed.show = true;
				sm.UserData:SetCanFire(true);
			end,
			OnExit = function(self, sm)
			end,			
			OnUpdate = function(self, sm, deltaTime)
				sm.UserData:StrafeAlign();
			end,
			Transitions =
			{
			},
		},
		Crouch = 
		{
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData.animParamUpdateFlags.WantsToCrouch.show = true;
				sm.UserData:SetCanFire(false);
			end,
			OnExit = function(self, sm)
			end,			
			OnUpdate = function(self, sm, deltaTime)
			end,
			Transitions =
			{
			},
		},
		CrouchIdle = 
		{
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				StarterGameEntityUtility.SetCharacterHalfHeight(sm.EntityId, sm.UserData.Properties.CrouchingHalfHeight);
				sm.UserData:SetCanFire(false);
			end,
			OnExit = function(self, sm)
				StarterGameEntityUtility.SetCharacterHalfHeight(sm.EntityId, sm.UserData.Properties.StandingHalfHeight);
			end,			
			OnUpdate = function(self, sm, deltaTime)
			end,
			Transitions =
			{
			},
		},
		CrouchMove = 
		{
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData.animParamUpdateFlags.Speed.show = true;
				StarterGameEntityUtility.SetCharacterHalfHeight(sm.EntityId, sm.UserData.Properties.CrouchingHalfHeight);
				sm.UserData:SetCanFire(false);
			end,
			OnExit = function(self, sm)
				StarterGameEntityUtility.SetCharacterHalfHeight(sm.EntityId, sm.UserData.Properties.StandingHalfHeight);
			end,			
			OnUpdate = function(self, sm, deltaTime)
			end,
			Transitions =
			{
			},
		},
		CrouchIdleTurn = 
		{
			OnEmotionAnimationEvent = function(self, event, sm)
				if(event.eventTypeName == "IdleTurnTransition") then
					sm.UserData:SetNamedAnimTagParameter("IdleTurnTransition", true);
				end
			end,
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData.animParamUpdateFlags.TurnAngle.update = false;
				sm.UserData.animParamUpdateFlags.TurnAngle.show = true;
				sm.UserData:SetNamedAnimTagParameter("IdleTurnTransition", false);
				StarterGameEntityUtility.SetCharacterHalfHeight(sm.EntityId, sm.UserData.Properties.CrouchingHalfHeight);
				sm.UserData:SetCanFire(false);
			end,
			OnExit = function(self, sm)
				StarterGameEntityUtility.SetCharacterHalfHeight(sm.EntityId, sm.UserData.Properties.StandingHalfHeight);
			end,			
			OnUpdate = function(self, sm, deltaTime)
			end,
			Transitions =
			{
			},
		},
		JumpingIdle = 
		{
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData:SetCanFire(false);
			end,
			OnExit = function(self, sm)
			end,			
			OnUpdate = function(self, sm, deltaTime)
			end,
			
			Transitions =
			{
			},
		},
		MotionTurn = 
		{
			OnEmotionAnimationEvent = function(self, event, sm)
				if(event.eventTypeName == "TransitionReached") then
					sm.UserData:SetNamedAnimTagParameter("MotionTurnTransitionReached", event.isEventStart);
				end
				if(event.eventTypeName == "EarlyOut") then
					sm.UserData:SetNamedAnimTagParameter("MotionTurnEarlyOut", event.isEventStart);
				end
				if(event.eventTypeName == "MotionTurn") then
					sm.UserData:SetNamedAnimTagParameter("MotionTurnMotionTurnTag", event.isEventStart);
				end
			end,
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData.animParamUpdateFlags.TurnAngle.update = false;
				sm.UserData.animParamUpdateFlags.Speed.update = false;
				sm.UserData.animParamUpdateFlags.Speed.show = true;
				sm.UserData.animParamUpdateFlags.TurnAngle.show = true;
				sm.UserData.animParamUpdateFlags.IsFalling.show = true;
				sm.UserData.animParamUpdateFlags.MotionTurnTransitionReached.show = true;
				sm.UserData.animParamUpdateFlags.MotionTurnEarlyOut.show = true;
				sm.UserData.animParamUpdateFlags.MotionTurnMotionTurnTag.show = true;
				sm.UserData:SetNamedAnimTagParameter("MotionTurnTransitionReached", false);
				sm.UserData:SetNamedAnimTagParameter("MotionTurnEarlyOut", false);
				sm.UserData:SetNamedAnimTagParameter("MotionTurnMotionTurnTag", false);
				sm.UserData:SetCanFire(true);
			end,
			OnExit = function(self, sm)
			end,			
			OnUpdate = function(self, sm, deltaTime)
			end,
			
			Transitions =
			{
			},
		},
		Jump = 
		{
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData.animParamUpdateFlags.LandType.update = true;
				self.force = sm.UserData.Properties.Jumping.JumpForce;

				local maxSpeed = sm.UserData.Properties.MoveSpeed;
				local forward = TransformBus.Event.GetWorldTM(sm.UserData.entityId):GetColumn(1):GetNormalized();
				forward = forward * sm.UserData.movementDirection:GetLength();
				self.JumpingVector = forward * maxSpeed;
				
				PhysicsComponentRequestBus.Event.EnablePhysics(sm.UserData.entityId);
				
				-- Push the character upwards.
				local moveMag = (self.JumpingVector) + Vector3(0.0, 0.0, self.force);
				PhysicsComponentRequestBus.Event.SetVelocity(sm.UserData.entityId, moveMag);	
				sm.UserData.fallingDuration = 0.0;			
				sm.UserData:SetCanFire(false);
				sm.UserData.fallngPrevTm = nil;
				sm.UserData.nudgingCollisionTimer = 0.0;
			end,
			OnExit = function(self, sm)
			end,			
			OnUpdate = function(self, sm, deltaTime)
				sm.UserData:UpdateFallingNudging(deltaTime, self.JumpingVector);
			end,
			
			Transitions =
			{
			},
		},
		DoubleJump = 
		{
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData.animParamUpdateFlags.LandType.update = true;
				self.force = sm.UserData.Properties.Jumping.JumpDoubleForce;

				local maxSpeed = sm.UserData.Properties.MoveSpeed;
				local forward = TransformBus.Event.GetWorldTM(sm.UserData.entityId):GetColumn(1):GetNormalized();
				forward = forward * sm.UserData.movementDirection:GetLength();
				self.JumpingVector = forward * maxSpeed;
				
				-- Push the character upwards.
				local moveMag = (self.JumpingVector) + Vector3(0.0, 0.0, self.force);
				PhysicsComponentRequestBus.Event.SetVelocity(sm.UserData.entityId, moveMag);	
				sm.UserData.fallingDuration = 0.0;		
				-- Signal that we're double-jumping.
				-- This is primarily for the thruster particle effects because they can't be
				-- done through the animation event system.
				local eventId = GameplayNotificationId(sm.UserData.entityId, "DoubleJumped", "float");
				GameplayNotificationBus.Event.OnEventBegin(eventId, sm.UserData.entityId);
				AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.entityId, "Play_pla_foley_jump_doublejump");
				sm.UserData:SetCanFire(false);
				sm.UserData.fallngPrevTm = nil;
			end,
			OnExit = function(self, sm)
			end,			
			OnUpdate = function(self, sm, deltaTime)
				sm.UserData:UpdateFallingNudging(deltaTime, self.JumpingVector);
			end,
			
			Transitions =
			{
			},
		},
		Falling = 
		{
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData.animParamUpdateFlags.LandType.update = true;
				sm.UserData.fallingDuration = 0.0;
				sm.UserData:SetCanFire(false);
				-- If we walk off a cliff instead of jumping then we'll enter this state with a
				-- zero jumping vector.
				if (sm.UserData.fallingPrevTm == nil) then
					local maxSpeed = sm.UserData.Properties.MoveSpeed;
					local forward = TransformBus.Event.GetWorldTM(sm.UserData.entityId):GetColumn(1):GetNormalized();
					-- If 'GetLength()' is a zero length vector then it means we've started falling
					-- because the ground has moved rather than us walking/jumping off something.
					local movDirLen = sm.UserData.movementDirection:GetLength();
					if (movDirLen <= 0.001) then
						forward = Vector3(0.0, 0.0, -1.0);
					else
						forward = forward * movDirLen;
					end
					sm.UserData.States.Jump.JumpingVector = forward * maxSpeed;
				end
			end,
			OnExit = function(self, sm)
			end,			
			OnUpdate = function(self, sm, deltaTime)
				sm.UserData.fallingDuration = sm.UserData.fallingDuration + deltaTime;
				sm.UserData:UpdateFallingNudging(deltaTime, sm.UserData.States.Jump.JumpingVector);
			end,
			Transitions =
			{
			},
		},
		Landing = 
		{
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData.animParamUpdateFlags.LandType.show = true
				sm.UserData:SetCanFire(false);
				sm.UserData.fallingPrevTm = nil;
			end,
			OnExit = function(self, sm)
			end,			
			OnUpdate = function(self, sm, deltaTime)
			end,
			
			Transitions =
			{
			},
		},
		IdleTurn = 
		{
			OnEmotionAnimationEvent = function(self, event, sm)
				if(event.eventTypeName == "IdleTurnTransition") then
					sm.UserData:SetNamedAnimTagParameter("IdleTurnTransition", true);
				end
			end,
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData.animParamUpdateFlags.TurnAngle.update = false;
				sm.UserData.animParamUpdateFlags.TurnAngle.show = true;
				sm.UserData.animParamUpdateFlags.Speed.show = true;
				sm.UserData:SetNamedAnimTagParameter("IdleTurnTransition", false);
				sm.UserData:SetCanFire(true);
			end,
			OnExit = function(self, sm)
			end,			
			OnUpdate = function(self, sm, deltaTime)
				sm.UserData:UpdateAnimationParams(deltaTime, true);
			end,
			
			Transitions =
			{
			},
		},
		Interact =
		{
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				-- zero velocity
				CryCharacterPhysicsRequestBus.Event.RequestVelocity(sm.UserData.entityId, Vector3(0,0,0), 0);
				PhysicsComponentRequestBus.Event.SetVelocity(sm.UserData.entityId, Vector3(0,0,0));
				PhysicsComponentRequestBus.Event.SetAngularVelocity(sm.UserData.entityId, Vector3(0,0,0));
				-- position to entity (find ground)
				local tm = TransformBus.Event.GetWorldTM(sm.UserData.InteractPosEntity);
				local rayCastConfig = RayCastConfiguration();
				rayCastConfig.origin = tm:GetTranslation() + Vector3(0.0, 0.0, 1.5);
				rayCastConfig.direction =  Vector3(0.0, 0.0, -1.0);
				rayCastConfig.maxDistance = 4.0;
				rayCastConfig.maxHits = 1;
				rayCastConfig.physicalEntityTypes = PhysicalEntityTypes.Static;		
				local hits = PhysicsSystemRequestBus.Broadcast.RayCast(rayCastConfig);
				if (#hits > 0) then
					tm:SetTranslation(hits[1].position);
				end
				TransformBus.Event.SetWorldTM(sm.EntityId, tm);
				-- select camera
				local camMan = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("CameraManager"));
				local camEventId = GameplayNotificationId(camMan, "ActivateCameraEvent", "float");
				GameplayNotificationBus.Event.OnEventBegin(camEventId, sm.UserData.InteractCamEntity);
				-- disable controls
				local playerCam = TagGlobalRequestBus.Event.RequestTaggedEntities(sm.UserData.cameraTag)
				local camControlsEventId = GameplayNotificationId(playerCam, "EnableControlsEvent", "float");
				GameplayNotificationBus.Event.OnEventBegin(camControlsEventId, 0);
				local playerControlsEventId = GameplayNotificationId(sm.UserData.entityId, "EnableControlsEvent", "float");
				GameplayNotificationBus.Event.OnEventBegin(playerControlsEventId, 0);
				sm.UserData:SetCanFire(false);
			end,
			OnExit = function(self, sm)
			end,
			OnUpdate = function(self, sm, deltaTime)
			end,
			Transitions =
			{
			},
		},
		InteractIdle =
		{
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData:SetCanFire(false);
			end,
			OnExit = function(self, sm)
			end,
			OnUpdate = function(self, sm, deltaTime)
			end,
			Transitions =
			{
			},
		},
		InteractEnd =
		{
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData:SetCanFire(false);
			end,
			OnExit = function(self, sm)
				-- select player camera
				local camMan = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("CameraManager"));
				local camEventId = GameplayNotificationId(camMan, "ActivateCameraEvent", "float");
				local playerCam = TagGlobalRequestBus.Event.RequestTaggedEntities(sm.UserData.cameraTag)
				GameplayNotificationBus.Event.OnEventBegin(camEventId, playerCam);

				-- enable controls
				local camControlsEventId = GameplayNotificationId(playerCam, "EnableControlsEvent", "float");
				GameplayNotificationBus.Event.OnEventBegin(camControlsEventId, 1);
				local playerControlsEventId = GameplayNotificationId(sm.UserData.entityId, "EnableControlsEvent", "float");
				GameplayNotificationBus.Event.OnEventBegin(playerControlsEventId, 1);
			end,
			OnUpdate = function(self, sm, deltaTime)
			end,
			Transitions =
			{
			},
		},
		Death =
		{
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData.animParamUpdateFlags.DeathState.show = true;
				CharacterAnimationRequestBus.Event.SetAnimationDrivenMotion(sm.EntityId, true);
				sm.UserData:SetControlsEnabled(false);
				
				-- Announce to the entity that it's died.
				GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(sm.EntityId, "OnDeath", "float"), 1.0);
				PhysicsComponentRequestBus.Event.DisablePhysics(sm.EntityId);
				sm.UserData:SetCanFire(false);
			end,
			
			OnUpdate = function(self, sm, deltaTime)
			end,
			
			Transitions =
			{
			},
		},
		DeathStateFront =
		{
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData.animParamUpdateFlags.DeathState.show = true;
				sm.UserData:SetCanFire(false);
			end,
			
			OnUpdate = function(self, sm, deltaTime)
				sm.UserData:RotateCharacterToSlope();
			end,
			
			Transitions =
			{
			},
		},
		DeathStateBack =
		{
			OnEnter = function(self, sm)
				sm.UserData.animParamUpdateFlags:reset();
				sm.UserData.animParamUpdateFlags.DeathState.show = true;
				sm.UserData:SetCanFire(false);
			end,
			
			OnUpdate = function(self, sm, deltaTime)
				sm.UserData:RotateCharacterToSlope();
			end,
			
			Transitions =
			{
			},
		},
	},
	NoTurnIdleAngle = 0.78, -- The player won't play a turn idle animation if the move in a direction less than this angle from the facing direction
	DeathStateFront = 1.0,
	DeathStateBack = 2.0,
}

--------------------------------------------------
-- Component behavior
--------------------------------------------------
--function movementcontroller:OnStateEntering(state)
function movementcontroller:OnStateTransitionStart(previousState, nextState)
	if(self.Properties.DebugAnimStateMachine == true) then
		Debug.Log(">>>>>>>>>>>>>>>>>>>>>"..nextState.."<<<<<<<<<<<<<<<<<<<<<<<");
	end
	self.animState = nextState;
	self.StateMachine:GotoState(nextState);
end
function movementcontroller:OnMotionEvent(motionEvent)
	if(self.Properties.DebugAnimStateMachine == true) then
		Debug.Log("Animation tag >>>>>>>>>>>>>> "..motionEvent.eventTypeName.." time: "..motionEvent.time.." param: "..motionEvent.parameter);
	end
	self.StateMachine:OnEmotionAnimationEvent(motionEvent);
end
function movementcontroller:OnActivate()	

	self.animParamUpdateFlags = AnimParamUpdateFlags:create();
	-- Play the specified Audio Trigger (wwise event) on this component
	AudioTriggerComponentRequestBus.Event.Play(self.entityId);

	self.Properties.RotationSpeed = Math.DegToRad(self.Properties.RotationSpeed);
	self.Fragments = {};
	
	-- Enable and disable events
	self.enableEventId = GameplayNotificationId(self.entityId, "Enable", "float");
	self.enableHandler = GameplayNotificationBus.Connect(self, self.enableEventId);
	self.disableEventId = GameplayNotificationId(self.entityId, "Disable", "float");
	self.disableHandler = GameplayNotificationBus.Connect(self, self.disableEventId);
	
	self.enableFiringEventId = GameplayNotificationId(self.entityId, "EnableFiring", "float");
	self.disableFiringEventId = GameplayNotificationId(self.entityId, "DisableFiring", "float");
	
	-- Input listeners (events).
	self.getDiedEventId = GameplayNotificationId(self.entityId, self.Properties.Events.DiedMessage, "float");
	self.getDiedHandler = GameplayNotificationBus.Connect(self, self.getDiedEventId);
	
	self.gotShotEventId = GameplayNotificationId(self.entityId, self.Properties.Events.GotShot, "float");
	self.gotShotHandler = GameplayNotificationBus.Connect(self, self.gotShotEventId);

	-- Tick needed to detect aim timeout
	self.tickBusHandler = TickBus.Connect(self);
	self.performedFirstUpdate = false;
	
	-- Subscribe to collision notifications.
	self.physicsNotificationHandler = PhysicsComponentNotificationBus.Connect(self, self.entityId);
	self.nudgingCollisionTimer = 0.0;

	-- Create and start our state machine.
	self.StateMachine = {}
	setmetatable(self.StateMachine, StateMachine);
	self.StateMachine:Start("Animation", self.entityId, self, self.States, false, self.Properties.InitialState, self.Properties.DebugStateMachine)
	self.controlsDisabledCount = 0;
	self.controlsEnabled = true;
	self.controlsEnabledEventId = GameplayNotificationId(self.entityId, self.Properties.Events.ControlsEnabled, "float");
	self.controlsEnabledHandler = GameplayNotificationBus.Connect(self, self.controlsEnabledEventId);
	
	self.WalkEnabled = false;
	
	self.shouldImmediatelyRagdoll = false;
	
	
	-- Use this to get the camera information when firing. It saves making an entity property
	-- and linking the weapon to a specific camera entity.
	-- Note: this only returns the LAST entity with this tag, so make sure there's only one
	-- entity with the "PlayerCamera" tag otherwise weird stuff might happen.
	self.cameraTag = Crc32("PlayerCamera");
	self.wasHit = false;
	self.wasHitLastFrame = false;
	
	self.setMovementDirectionId = GameplayNotificationId(self.entityId, "SetMovementDirection", "float");
	self.setMovementDirectionHandler = GameplayNotificationBus.Connect(self, self.setMovementDirectionId);
	self.movementDirection = Vector3(0,0,0);
	
	self.setStrafeFacingId = GameplayNotificationId(self.entityId, "SetStrafeFacing", "float");
	self.setStrafeFacingHandler = GameplayNotificationBus.Connect(self, self.setStrafeFacingId);
	self.strafeFacing = Vector3(0,0,0);

	
	self.jumpEventId = GameplayNotificationId(self.entityId, "Jump", "float");
	self.jumpHandler = GameplayNotificationBus.Connect(self, self.jumpEventId);
	self.wantsToJump = false;
	self.wantedToJumpLastFrame = false;
	self.landedBecauseSupported = false;

	self.strafeEventId = GameplayNotificationId(self.entityId, "Strafe", "float");
	self.strafeHandler = GameplayNotificationBus.Connect(self, self.strafeEventId);
	self.crouchEventId = GameplayNotificationId(self.entityId, "Crouch", "float");
	self.crouchHandler = GameplayNotificationBus.Connect(self, self.crouchEventId);
	self.wantsToStrafe = false;
	self.wantsToCrouch = false;

	self.camAimSettingsEventArgs = CameraSettingsEventArgs();
	self.camAimSettingsEventArgs.name = "Aim";
	self.camAimSettingsEventArgs.entityId = self.entityId;
			
	self.startInteractEventId = GameplayNotificationId(self.entityId, "StartInteract", "float");
	self.startInteractHandler = GameplayNotificationBus.Connect(self, self.startInteractEventId);
	self.endInteractEventId = GameplayNotificationId(self.entityId, "EndInteract", "float");
	self.endInteractHandler = GameplayNotificationBus.Connect(self, self.endInteractEventId);
	
	self.Properties.Falling.SlopeAngle = self.Properties.Falling.SlopeAngle * math.pi / 180.0;
	self.teleportToEventId = GameplayNotificationId(self.entityId, "TeleportTo", "float");
	self.teleportToHandler = GameplayNotificationBus.Connect(self, self.teleportToEventId);
	
	self.previousSpeed = 0;
	self.actorNotificationHandler = ActorNotificationBus.Connect(self, self.entityId);
	self.turningLastFrame = false;
	self.hitParam = 0;
	StarterGameEntityUtility.SetCharacterHalfHeight(self.entityId, self.Properties.StandingHalfHeight);
	self:OnStateTransitionStart(nil, self.Properties.InitialState);
	self.fallingPrevTm = nil;
end
function movementcontroller:paramToStringIntro(name)
    paramIndex = AnimGraphComponentRequestBus.Event.FindParameterIndex(self.entityId, name);
	return name.."["..tostring(paramIndex).."]: ";
end
function movementcontroller:floatParamToString(name, show)
	if(show) then
		return self:paramToStringIntro(name)..tostring(AnimGraphComponentRequestBus.Event.GetNamedParameterFloat(self.entityId, name))..", ";
	else
		return "";
	end
end
function movementcontroller:boolParamToString(name, show)
	if(show) then
		return self:paramToStringIntro(name)..tostring(AnimGraphComponentRequestBus.Event.GetNamedParameterBool(self.entityId, name))..", ";
	else
		return "";
	end
end
function movementcontroller:printAnimState()
	local output = self.animState.."::";
	output = output..self:floatParamToString("Speed", self.animParamUpdateFlags.Speed.show);
	output = output..self:floatParamToString("SpeedDBuffer", self.animParamUpdateFlags.Speed.show and not self.animParamUpdateFlags.Speed.update);
	output = output..self:floatParamToString("TurnSpeed", self.animParamUpdateFlags.TurnSpeed.show);
	output = output..self:floatParamToString("TurnAngle", self.animParamUpdateFlags.TurnAngle.show);
	output = output..self:floatParamToString("TurnAngleDBuffer", self.animParamUpdateFlags.TurnAngle.show and not self.animParamUpdateFlags.TurnAngle.update);
	output = output..self:floatParamToString("PreviousSpeed", self.animParamUpdateFlags.PreviousSpeed.show);
	output = output..self:floatParamToString("PreviousTurning", self.animParamUpdateFlags.PreviousTurning.show);
	output = output..self:floatParamToString("SlopeAngle", self.animParamUpdateFlags.SlopeAngle.show);
	output = output..self:boolParamToString("IsFalling", self.animParamUpdateFlags.IsFalling.show);
	output = output..self:boolParamToString("WantsToJump", self.animParamUpdateFlags.WantsToJump.show);
	output = output..self:boolParamToString("ShouldLand", self.animParamUpdateFlags.ShouldLand.show);
	output = output..self:floatParamToString("LandType", self.animParamUpdateFlags.LandType.show);
	output = output..self:boolParamToString("WantsToStrafe", self.animParamUpdateFlags.WantsToStrafe.show);
	output = output..self:boolParamToString("MotionTurnTransitionReached", self.animParamUpdateFlags.MotionTurnTransitionReached.show);
	output = output..self:boolParamToString("MotionTurnEarlyOut", self.animParamUpdateFlags.MotionTurnEarlyOut.show);
	output = output..self:boolParamToString("MotionTurnMotionTurnTag", self.animParamUpdateFlags.MotionTurnMotionTurnTag.show);
	output = output..self:boolParamToString("WantsToCrouch", self.animParamUpdateFlags.WantsToCrouch.show);
	output = output..self:boolParamToString("WantsToInteract", self.animParamUpdateFlags.WantsToInteract.show);
	output = output..self:floatParamToString("DeathState", self.animParamUpdateFlags.DeathState.show);
	output = output..self:boolParamToString("IdleTurnTransition", self.animParamUpdateFlags.IdleTurnTransition.show);
	output = output..self:boolParamToString("WasHit", self.animParamUpdateFlags.WasHit.show);
	output = output..self:floatParamToString("HitDirection", self.animParamUpdateFlags.HitDirection.show);
	output = output..self:floatParamToString("Aiming", self.animParamUpdateFlags.Aiming.show);
	Debug.Log(output);
end
function movementcontroller:GetLandType()
	local landType = 0;
	if(self.fallingDuration == nil) then Debug.Log("self.fallingDuration nil") end
	if(self.Properties.Falling.FallingHard == nil) then Debug.Log("self.Properties.Falling.FallingHard nil") end
	if(self.fallingDuration >= self.Properties.Falling.FallingHard) then
		landType = 2;
	else 
		if(self.fallingDuration >= self.Properties.Falling.FallingMedium) then
			landType = 1;
		end
	end
	return landType;
end
function movementcontroller:OnDeactivate()

	-- Terminate our state machine.
	self.StateMachine:Stop();
	self.tickBusHandler:Disconnect();
	self.tickBusHandler = nil;

	self.getDiedHandler:Disconnect();
	self.getDiedHandler = nil;
	
	self.gotShotHandler:Disconnect();
	self.gotShotHandler = nil;
	
	if (self.enableHandler ~= nil) then
		self.enableHandler:Disconnect();
		self.enableHandler = nil;
	end
	if (self.disableHandler ~= nil) then
		self.disableHandler:Disconnect();
		self.disableHandler = nil;	
	end
	
	if (self.physicsNotificationHandler ~= nil) then
		self.physicsNotificationHandler:Disconnect();
		self.physicsNotificationHandler = nil;
	end
	
	if (self.jumpHandler ~= nil) then
		self.jumpHandler:Disconnect();
		self.jumpHandler = nil;
	end
	if(self.actorNotificationHandler ~= nil) then
		self.actorNotificationHandler:Disconnect();
	end
	if (self.strafeHandler ~= nil) then
		self.strafeHandler:Disconnect();
		self.strafeHandler = nil;
	end
	if (self.crouchHandler ~= nil) then
		self.crouchHandler:Disconnect();
		self.crouchHandler = nil;
	end
	if (self.strafeFacingHandler ~= nil) then
		self.strafeFacingHandler:Disconnect();
		self.strafeFacingHandler = nil;
	end
	if (self.teleportToHandler ~= nil) then
		self.teleportToHandler:Disconnect();
		self.teleportToHandler = nil;	
	end

end

function movementcontroller:OnCollision(data)
	if (data and data ~= nil) then
		self.nudgingCollisionTimer = self.Properties.Falling.Nudging.CollisionTimer;
	end
end

function movementcontroller:IsMoving()
	return Vector3.GetLengthSq(self.movementDirection) > 0.01;
end

-- Applies small impulses to allow fine-tuned landing.
function movementcontroller:UpdateFallingNudging(deltaTime, jumpingDir)
	-- Update the collision timer.
	if (self.nudgingCollisionTimer ~= 0.0) then
		self.nudgingCollisionTimer = utilities.Clamp(self.nudgingCollisionTimer - deltaTime, 0.0, self.nudgingCollisionTimer);
	end
	
	-- Compare the desired direction with the jumping direction.
	local moveFlatDir = Vector3(self.movementDirection.x, self.movementDirection.y, 0.0);
	local moveFlatDirNorm = Vector3(0.0);
	if (moveFlatDir:GetLength() > 0.001) then
		moveFlatDirNorm = moveFlatDir:GetNormalized();
	end
	--Debug.Log("Movement: " .. tostring(self.movementDirection) .. ", Norm: " .. tostring(moveFlatDirNorm));
	local jumpFlatDir = Vector3(jumpingDir.x, jumpingDir.y, 0.0);
	local jumpFlatDirNorm = Vector3(0.0);
	if (jumpFlatDir:GetLength() > 0.001) then
		jumpFlatDirNorm = jumpFlatDir:GetNormalized();
	end
	local nudgeThisFrame = jumpFlatDirNorm:Dot(moveFlatDirNorm);
	
	local thisTm = TransformBus.Event.GetWorldTM(self.entityId);
	if (self.fallingPrevTm == nil) then
		-- Prep for nudging.
		--	MaxSpeed == pushing in the jumping direction
		--	Â½ MaxSpeed == not pushing or pulling
		--	0.0 == pulling backwards from the jumping direction
		self.nudgingVelocity = self.Properties.Falling.Nudging.MaxSpeed;
	else
		local deltaAcc = self.Properties.Falling.Nudging.Acceleration * deltaTime;
		if (nudgeThisFrame == 0.0) then
			-- If the character isn't trying to be nudged then head towards the 'mid-point'.
			local halfMaxSpeed = self.Properties.Falling.Nudging.MaxSpeed * 0.5;
			local diff = self.nudgingVelocity - halfMaxSpeed;
			
			if (math.abs(diff) <= deltaAcc) then
				deltaAcc = 0.0;
				self.nudgingVelocity = halfMaxSpeed;
			elseif (diff > deltaAcc) then
				deltaAcc = deltaAcc * -1.0;
			elseif (diff < deltaAcc) then
				-- do nothing
			end
		else
			deltaAcc = deltaAcc * nudgeThisFrame;
		end
		
		self.nudgingVelocity = self.nudgingVelocity + deltaAcc;
		self.nudgingVelocity = utilities.Clamp(self.nudgingVelocity, 0, self.Properties.Falling.Nudging.MaxSpeed);
		--Debug.Log("self.nudgingVelocity: " .. tostring(self.nudgingVelocity));

		-- Only modify the velocity if we haven't hit something.
		if (self.nudgingCollisionTimer == 0.0) then
			-- Calculate the new horizontal velocity.
			local normalisedNudgingVel = self.nudgingVelocity / self.Properties.Falling.Nudging.MaxSpeed;
			local hVel = jumpFlatDir * normalisedNudgingVel;
			
			-- Get the vertical velocity.
			local vVel = Vector3(0.0);
			if (not StarterGameEntityUtility.GetEntitysVelocity(self.entityId, vVel)) then
				Debug.Log(tostring(self.entityId) .. " failed to get velocity.");
			--else
			--	Debug.Log(tostring(self.entityId) .. " vel: " .. tostring(vVel));
			end
			
			-- Combine the vertical and horizontal velocities and set it.
			hVel.z = vVel.z;
			PhysicsComponentRequestBus.Event.SetVelocity(self.entityId, hVel);
		end
	end
	self.fallingPrevTm = thisTm;
end
function movementcontroller:IsFalling()
	local angle, realAngle, dist = utilities.GetSlopeAngle(self);
	local isFalling = false;
	--if (realAngle >= self.Properties.Falling.SlopeAngle) then
	--	return true;
	--end
	
	if (dist < self.Properties.Falling.FallingHeight) then
		-- If the basic falling check has become valid again then reset this variable.
		self.landedBecauseSupported = false;
	else
		if (self.landedBecauseSupported) then
			-- If we landed because we were in the falling state but not actually going downwards
			-- then we need to perform extra raycasts around us to check if we're still supported.
			-- I'd have liked to just use a simple 'have I moved down in the last frame' check but
			-- because this is called in the statemachine and the 'prevTm' would be stored in the
			-- MovementController it means I'd have to rely on a specific update order.
			local pos = TransformBus.Event.GetWorldTM(self.entityId):GetTranslation();
			local radius = self.Properties.Falling.Radius;
			
			-- 'pos' is at the base of the character. The physics uses a capsule so raise up the
			-- base position so account for the curves.
			local range = 0.5;
			pos = pos + Vector3(0.0, 0.0, range);
			range = range + 0.2;
			
			-- If any of these raycasts hit then we don't want to transition into falling because
			-- we should still be being supported.
			if (self:FallingRayCastHitSomething(pos + Vector3(1.0, 0.0, 0.0), range)) then
			elseif (self:FallingRayCastHitSomething(pos + Vector3(-1.0, 0.0, 0.0), range)) then
			elseif (self:FallingRayCastHitSomething(pos + Vector3(0.0, 1.0, 0.0), range)) then
			elseif (self:FallingRayCastHitSomething(pos + Vector3(0.0, -1.0, 0.0), range)) then
			else
				isFalling = true;
			end
		else
			isFalling = true;
		end
	end
	
	return isFalling;
end

function movementcontroller:FallingRayCastHitSomething(origin, dist)
	local rayCastConfig = RayCastConfiguration();
	rayCastConfig.origin = origin;
	rayCastConfig.direction =  Vector3(0.0, 0.0, -1.0);
	rayCastConfig.maxDistance = dist;
	rayCastConfig.maxHits = 1;
	rayCastConfig.physicalEntityTypes = PhysicalEntityTypes.Static + PhysicalEntityTypes.Dynamic + PhysicalEntityTypes.Living;
	local hits = PhysicsSystemRequestBus.Broadcast.RayCast(rayCastConfig);
	return hits:HasBlockingHit();
end

function movementcontroller:ShouldLand()
	local angle, realAngle, dist = utilities.GetSlopeAngle(self);
	if (realAngle < self.Properties.Falling.SlopeAngle) then
		if (dist <= self.Properties.Jumping.LandingHeight) then
			return true;
		end
	end
	
	return false;
end

-- Returns true if the character is facing approximately in the direction of the controller
function movementcontroller:IsFacingTarget()
	local angleDelta = self:GetAngleDelta();
	if (angleDelta < 0) then
		angleDelta = -angleDelta;
	end
	return angleDelta < self.NoTurnIdleAngle;
end

function movementcontroller:WantsToJump()
	return self.wantsToJump;
end

function movementcontroller:Abs(value)
	return value * Math.Sign(value);
end

function movementcontroller:Clamp(_n, _min, _max)
	if (_n > _max) then _n = _max; end
	if (_n < _min) then _n = _min; end
	return _n;
end

function movementcontroller:Rad2Deg(radians)
	return (radians / 6.28318530718) * 360.0;
end

function movementcontroller:UpdateAnimationParams(deltaTime, lockTurnAngle)
	local angleDelta, moveMag = self:GetAngleDelta();
	self:UpdateRotation(deltaTime, angleDelta, lockTurnAngle);

	-- Request movement from character physics.
	local tm = TransformBus.Event.GetWorldTM(self.entityId);
	local maxSpeed = self.Properties.MoveSpeed;
	
	local vel = (tm:GetColumn(1) * moveMag * maxSpeed);
	speed = moveMag * maxSpeed;
	if(self.animParamUpdateFlags.Speed.update == true) then
    	AnimGraphComponentRequestBus.Event.SetNamedParameterFloat(self.entityId, "Speed", speed);
	else
    	AnimGraphComponentRequestBus.Event.SetNamedParameterFloat(self.entityId, "SpeedDBuffer", speed);
	end
	if(self.animParamUpdateFlags.PreviousSpeed.update == true) then
    	AnimGraphComponentRequestBus.Event.SetNamedParameterFloat(self.entityId, "PreviousSpeed", self.previousSpeed);
	end
	local slopeAngle = utilities.GetSlopeAngle(self);
	if(self.animParamUpdateFlags.SlopeAngle.update == true) then
    	AnimGraphComponentRequestBus.Event.SetNamedParameterFloat(self.entityId, "SlopeAngle", slopeAngle);	
	end
	if(self.animParamUpdateFlags.ShouldLand.update == true) then
    	AnimGraphComponentRequestBus.Event.SetNamedParameterBool(self.entityId, "ShouldLand", self:ShouldLand());
	end
	if(self.animParamUpdateFlags.IsFalling.update == true) then
    	AnimGraphComponentRequestBus.Event.SetNamedParameterBool(self.entityId, "IsFalling", self:IsFalling());
	end
	if(self.animParamUpdateFlags.WantsToJump.update == true) then
    	AnimGraphComponentRequestBus.Event.SetNamedParameterBool(self.entityId, "WantsToJump", self:WantsToJump());
	end
	if(self.animParamUpdateFlags.LandType.update == true) then
    	AnimGraphComponentRequestBus.Event.SetNamedParameterFloat(self.entityId, "LandType", self:GetLandType());
	end
	if(self.animParamUpdateFlags.PreviousTurning.update == true) then
    	AnimGraphComponentRequestBus.Event.SetNamedParameterBool(self.entityId, "PreviousTurning", self.turningLastFrame);
	end	
	if(self.animParamUpdateFlags.WantsToStrafe.update == true) then
    	AnimGraphComponentRequestBus.Event.SetNamedParameterBool(self.entityId, "WantsToStrafe", self.wantsToStrafe);
	end	
	if(self.animParamUpdateFlags.WantsToCrouch.update == true) then
    	AnimGraphComponentRequestBus.Event.SetNamedParameterBool(self.entityId, "WantsToCrouch", self.wantsToCrouch);
	end		
	if(self.animParamUpdateFlags.WasHit.update == true) then
    	AnimGraphComponentRequestBus.Event.SetNamedParameterBool(self.entityId, "WasHit", self.wasHit);
	end	
	if(self.animParamUpdateFlags.HitDirection.update == true) then
    	AnimGraphComponentRequestBus.Event.SetNamedParameterFloat(self.entityId, "HitDirection", self.hitParam);
	end		
	self.previousSpeed = speed;
	self.turningLastFrame = self:IsLargeAngleDelta();
end

-- Updates the character's rotation.
function movementcontroller:UpdateRotation(deltaTime, angleDelta, lockTurnAngle)
	local tm = TransformBus.Event.GetWorldTM(self.entityId);
	local thisFrame = 0
	if (angleDelta ~= 0.0) then
		thisFrame = self.Properties.RotationSpeed * deltaTime;
		local absAngleDelta = self:Abs(angleDelta);
		if (absAngleDelta > FloatEpsilon) then
			thisFrame = self:Clamp(angleDelta, -thisFrame, thisFrame);
		end
	end  
	local turnSpeedRad = thisFrame / deltaTime;
	local turnSpeed = self:Rad2Deg(turnSpeedRad);
	local turnAngle = self:Rad2Deg(angleDelta);
	if(self.animParamUpdateFlags.TurnSpeed.update == true) then
    	AnimGraphComponentRequestBus.Event.SetNamedParameterFloat(self.entityId, "TurnSpeed", turnSpeed);
	end
	if(self.animParamUpdateFlags.TurnAngle.update == true) then
    	AnimGraphComponentRequestBus.Event.SetNamedParameterFloat(self.entityId, "TurnAngle", turnAngle);
	else
    	AnimGraphComponentRequestBus.Event.SetNamedParameterFloat(self.entityId, "TurnAngleDBuffer", turnAngle);
	end
end

-- Returns the angle to turn in the requested direction and the length of the input vector
function movementcontroller:GetAngleDelta()
	local movementDirectionMagnitude = self.movementDirection:GetLength();
	if (movementDirectionMagnitude > 0.01) then
		local movementDirectionNormalised = self.movementDirection:GetNormalized();
		local tm = TransformBus.Event.GetWorldTM(self.entityId);
		local facing = tm:GetColumn(1):GetNormalized();
		local dot = facing:Dot(movementDirectionNormalised);
		dot = utilities.Clamp(dot, -1, 1);	-- even with both vectors normalised it can sometimes still be more than 1.0
		local angleDelta = Math.ArcCos(dot);
		local side = Math.Sign(facing:Cross(movementDirectionNormalised).z);
		if (side < 0.0) then
			angleDelta = -angleDelta;
		end
		return angleDelta, movementDirectionMagnitude;
	else
		return 0.0, movementDirectionMagnitude;
	end
end

-- Returns true if the delta angle is >= 90.0 degrees.
function movementcontroller:IsLargeAngleDelta()
	local angle = self:GetAngleDelta();
	return math.abs(angle) > (math.pi * 0.5);
end

function movementcontroller:OnTick(deltaTime, timePoint)
	
	-- Doing anything that requires other entities here because they might not exist
	-- yet in the 'OnActivate()' function.
	
	-- Get Player Height for Audio Environment Sound
	local playerHeight = TransformBus.Event.GetWorldTM(self.entityId):GetTranslation().z;
	--Debug.Log("Player Height = : " .. tostring(playerHeight));
	
	-- Set RTPC "rtpc_playerHeight" to equal "playerHeight"
	AudioRtpcComponentRequestBus.Event.SetValue(self.entityId, playerHeight);
	

	if (not self.performedFirstUpdate) then
	
		local camMan = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("CameraManager"));
		self.camPushNotificationId = GameplayNotificationId(camMan, "PushCameraSettings", "float");
		self.camPopNotificationId = GameplayNotificationId(camMan, "PopCameraSettings", "float");
		
		self.SetHideUIEventId = GameplayNotificationId(EntityId(), self.Properties.UIMessages.SetHideUIMessage, "float");

		-- Make sure we don't do this 'first update' again.
		self.performedFirstUpdate = true;	
	end
	
	self:UpdateAnimationParams(deltaTime);
	if(self.Properties.DebugAnimStateMachine == true) then
		self:printAnimState();
	end
	if (self.wantedToJumpLastFrame) then
		self.wantsToJump = false;
		self.wantedToJumpLastFrame = false;
	elseif (self.wantsToJump) then
		self.wantedToJumpLastFrame = true;
	end
	if (self.wasHitLastFrame) then
		self.wasHit = false;
		self.wasHitLastFrame = false;
	elseif (self.wasHit) then
		self.wasHitLastFrame = true;
	end
end
   
function movementcontroller:Die()
	--Debug.Log("Player killed message");
	if(self.animParamUpdateFlags.DeathState.update == true) then
		local deathState = self.DeathStateFront;
		if ((self.hitParam > 0.25) and (self.hitParam < 0.75)) then
			deathState = self.DeathStateBack;
		end
    	AnimGraphComponentRequestBus.Event.SetNamedParameterFloat(self.entityId, "DeathState", deathState);
	end		
end

function movementcontroller:IsDead()
	local isDead = self.StateMachine.CurrentState == self.States.Dead;
	return isDead;
end


function movementcontroller:Hit(value)
	if(self:IsDead() == false) then
		local tm = TransformBus.Event.GetWorldTM(self.entityId);
		local facing = tm:GetColumn(1):GetNormalized();
		local direction = -value.direction;
		local dot = facing:Dot(direction);
		local angle = Math.ArcCos(dot);
		local side = Math.Sign(facing:Cross(direction).z);
		local PI_2 = 6.28318530718;
		if (side < 0.0) then
			angle = -angle;
		end
		if (self.wasHit) then
			--MannequinRequestsBus.Event.ForceFinishRequest(self.entityId, self.Fragments.Hit);
		end
		self.wasHit = true;
		if(angle > 0) then
			self.hitParam = (angle / PI_2);
		else
			self.hitParam = (angle / PI_2) + 1.0;
		end
		--self.Fragments.Hit = MannequinRequestsBus.Event.QueueFragment(self.entityId, 1, "Hit", "", false);
	end
end
function movementcontroller:StrafeAlign()
	local tm = TransformBus.Event.GetWorldTM(self.entityId);
	local facing = tm:GetColumn(1):GetNormalized();
	local dot = facing:Dot(self.strafeFacing);
	dot = utilities.Clamp(dot, -1, 1);	-- even with both vectors normalised it can sometimes still be more than 1.0
	local angleDelta = Math.ArcCos(dot);
	local side = Math.Sign(facing:Cross(self.strafeFacing).z);
	if (side < 0.0) then
		angleDelta = -angleDelta;
	end
	TransformBus.Event.RotateAroundLocalZ(self.entityId, angleDelta);
end
function movementcontroller:EnableStrafeCamera()
	self.camAimSettingsEventArgs.name = "Aim";
	self.camAimSettingsEventArgs.transitionTime = 0.25;
	self.camAimSettingsEventArgs.entityId = self.entityId;
	GameplayNotificationBus.Event.OnEventBegin(self.camPushNotificationId, self.camAimSettingsEventArgs);
end
function movementcontroller:DisableStrafeCamera()
	self.camAimSettingsEventArgs.name = "Aim";
	self.camAimSettingsEventArgs.transitionTime = 0.25;
	self.camAimSettingsEventArgs.entityId = self.entityId;
	GameplayNotificationBus.Event.OnEventBegin(self.camPopNotificationId, self.camAimSettingsEventArgs);
end

function movementcontroller:IsStrafing()
	return self.isStrafing;
end

function movementcontroller:SetCanFire(value)
	if (value) then
		GameplayNotificationBus.Event.OnEventBegin(self.enableFiringEventId, 0);
	else
		GameplayNotificationBus.Event.OnEventBegin(self.disableFiringEventId, 0);
	end
end
	
function movementcontroller:StartInteract(posEntity, camEntity)
	self.InteractPosEntity = posEntity;
	self.InteractCamEntity = camEntity;
	if(self.animParamUpdateFlags.WantsToInteract.update == true) then
    	AnimGraphComponentRequestBus.Event.SetNamedParameterBool(self.entityId, "WantsToInteract", true);
	end		
end

function movementcontroller:EndInteract()
	if(self.animParamUpdateFlags.WantsToInteract.update == true) then
    	AnimGraphComponentRequestBus.Event.SetNamedParameterBool(self.entityId, "WantsToInteract", false);
	end
end
	
function movementcontroller:SetControlsEnabled(newControlsEnabled)
	if ((newControlsEnabled ~= self.controlsEnabled) and (self.SetHideUIEventId ~= nil)) then
		self.controlsEnabled = newControlsEnabled;
		--Debug.Log("controls chaned to: " .. tostring(self.controlsEnabled));
		
		if (self.entityId == TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"))) then
			if(newControlsEnabled) then
				--Debug.Log("ShowUI");
				GameplayNotificationBus.Event.OnEventBegin(self.SetHideUIEventId, 0);
			else
				--Debug.Log("HideUI");
				GameplayNotificationBus.Event.OnEventBegin(self.SetHideUIEventId, 1);
			end
		end
	end 

	if (not newControlsEnabled) then
		self.movementDirection = Vector3(0.0,0.0,0.0);
		self.wantsToJump = false;
		self.wantsToStrafe = false;
	end
end

function movementcontroller:TurnOnLights(enabled)
	local children = TransformBus.Event.GetChildren(self.entityId);
	local numChildren = #children;
	for i=1,numChildren,1 do
		local thischild = children[i];
		LightComponentRequestBus.Event.SetVisible(thischild, enabled);
	end	
end
	
function movementcontroller:OnEnable()
	self.StateMachine:Resume();
	self.tickBusHandler = TickBus.Connect(self);
	PhysicsComponentRequestBus.Event.EnablePhysics(self.entityId);
	MeshComponentRequestBus.Event.SetVisibility(self.entityId, true);
	--MannequinRequestsBus.Event.ResumeAll(self.entityId, 7);
	
	-- Enable the lights on the character
	self:TurnOnLights(true);
end

function movementcontroller:OnDisable()
	--MannequinRequestsBus.Event.PauseAll(self.entityId);
	MeshComponentRequestBus.Event.SetVisibility(self.entityId, false);
	PhysicsComponentRequestBus.Event.DisablePhysics(self.entityId);
	self.StateMachine:Stop();
	self.tickBusHandler:Disconnect();

	-- Enable the lights on the character
	self:TurnOnLights(false);
end
	
function movementcontroller:OnEventBegin(value)
	--Debug.Log("movementcontroller:OnEventBegin( " .. tostring(value) .. " )");
	if (GameplayNotificationBus.GetCurrentBusId() == self.controlsEnabledEventId) then
		--Debug.Log("controlls set: " .. tostring(value));
		if (value == 1.0) then
			self.controlsDisabledCount = self.controlsDisabledCount - 1;
		else
			self.controlsDisabledCount = self.controlsDisabledCount + 1;
		end
		if (self.controlsDisabledCount < 0) then
			Debug.Log("[Warning] MovementController: controls disabled ref count is less than 0.  Probable enable/disable mismatch.");
			self.controlsDisabledCount = 0;
		end		
		local newEnabled = self.controlsDisabledCount == 0;
		if (self.controlsEnabled ~= newEnabled) then
			self:SetControlsEnabled(newEnabled);
		end
	elseif (self.controlsEnabled) then
		if (GameplayNotificationBus.GetCurrentBusId() == self.setMovementDirectionId) then
			--Debug.Log("setMovementDirectionId ( " .. tostring(value) .. " )");
			self.movementDirection = value;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.jumpEventId) then
			self.wantsToJump = true;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.strafeEventId) then
			self.wantsToStrafe = true;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.crouchEventId) then
			self.wantsToCrouch = not self.wantsToCrouch;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.setStrafeFacingId) then
			self.strafeFacing = value;
		end
	end

	if (GameplayNotificationBus.GetCurrentBusId() == self.getDiedEventId) then
		self:Die();
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.gotShotEventId) then
		-- React to being hit (if it wasn't done by themself).
		if (self.entityId ~= value.assailant) then
			self.shouldImmediatelyRagdoll = value.immediatelyRagdoll;
			self:Hit(value);
		end
	end
	
	if (GameplayNotificationBus.GetCurrentBusId() == self.startInteractEventId) then
		self:StartInteract(value.positionEntity, value.cameraEntity);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.endInteractEventId) then
		self:EndInteract();
	end
	
	if (GameplayNotificationBus.GetCurrentBusId() == self.enableEventId) then
		self:OnEnable();
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.disableEventId) then
		self:OnDisable();
	end
	if (GameplayNotificationBus.GetCurrentBusId() == self.teleportToEventId) then
		local tm = TransformBus.Event.GetWorldTM(self.entityId);
		tm:SetTranslation(value);
		TransformBus.Event.SetWorldTM(self.entityId, tm);
	end

end
	
function movementcontroller:OnEventEnd(value)
	if (GameplayNotificationBus.GetCurrentBusId() == self.strafeEventId) then
		self.wantsToStrafe = false;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.crouchEventId and self.Properties.CrouchToggle == false) then
		self.wantsToCrouch = false;
	end
end	

function movementcontroller:RotateCharacterToSlope()
	local tm = TransformBus.Event.GetWorldTM(self.entityId);
	local pos = tm:GetTranslation();
	local up, dist = utilities.GetTerrainIntercept(pos);
	local forward = tm:GetColumn(1):GetNormalized();
	local right = Vector3.Cross(forward, up);
	forward = Vector3.Cross(up, right);
	local worldTm = Transform.CreateFromColumns(right, forward, up, pos);
	TransformBus.Event.SetWorldTM(self.entityId, worldTm);
end
function movementcontroller:SetNamedAnimTagParameter(paramName, state)
    AnimGraphComponentRequestBus.Event.SetNamedParameterBool(self.entityId, paramName, state);
end

return movementcontroller;
