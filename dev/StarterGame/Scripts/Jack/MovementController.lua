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

local utilities = require "scripts/common/utilities"
local StateMachine = require "scripts/common/StateMachine"
		
local movementcontroller = 
{
	Properties = 
	{
		MoveSpeed = { default = 5.6, description = "How fast the chicken moves.", suffix = " m/s" },
		StrafeMoveSpeed = { default = 3.6, description = "How fast the chicken moves.", suffix = " m/s" },
		RotationSpeed = { default = 155.0, description = "How fast (in degrees per second) the chicken can turn.", suffix = " deg/sec"},
		Camera = {default = EntityId()},
		Drone = { default = EntityId() },
		InitialState = "Idle",
		DebugStateMachine = false,
		Move2IdleThreshold = { default = 0.8, description = "Character must be moving faster than this normalised speed when they stop to use a move to idle transition animation." },
		
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
			DiedMessage = { default = "HealthEmpty", description = "The event recieved when you have died" }, 
						
			ControlsEnabled = { default = "EnableControlsEvent", description = "If passed '1.0' it will enable controls, otherwise it will disable them." },
			 
			GotShot = { default = "GotShotEvent", description = "Indicates we were shot by something and will likely get hurt." },
		},
	},
	
	InputValues = 
	{
		NavForwardBack = 0.0,
		NavLeftRight = 0.0,
	},

	--------------------------------------------------
	-- Animation State Machine Definition
	-- States:
	--		Idle
	--		IdleTurn
	--		Idle2Move
	--		Move2Idle
	--		Moving
	--		MovingTurn
	--		JumpingIdle
	--		Jumping
	--		Falling
	--		Landing
	--		InteractStart
	--		InteractIdle
	--		InteractEnd
	--		Dead
	--
	-- Transitions:
	--		Idle		<-> IdleTurn
	--					<-> Idle2Move
	--					<-  Move2Idle
	--					<-  Moving
	--					 -> Jumping
	--					 -> Falling
	--					<-  Landing
	--		IdleTurn	<-> Idle
	--					 -> Idle2Move
	--					 -> Falling
	--		Idle2Move	<-> Idle
	--					<-  IdleTurn
	--					<-> Move2Idle
	--					 -> Moving
	--					 -> MovingTurn
	--					 -> Jumping
	--					 -> Falling
	--					<-  Landing
	--		Move2Idle	 -> Idle
	--					<-> Idle2Move
	--					<-  Moving
	--					<-> MovingTurn
	--					 -> Falling
	--		Moving		 -> Idle
	--					<-  Idle2Move
	--					 -> Move2Idle
	--					<-> MovingTurn
	--					 -> Jumping
	--					 -> Falling
	--		MovingTurn	<-  Idle2Move
	--					<-> Move2Idle
	--					<-> Moving
	--					<-> MovingTurn
	--					 -> Falling
	--		JumpingIdle <-> Idle
	--					 -> IdleToMove
	--		Jumping		<-  Idle
	--					<-  Idle2Move
	--					<-  Moving
	--					<-> Jumping
	--					<-> Falling
	--					 -> Landing
	--		Falling		<-  Idle
	--					<-  IdleTurn
	--					<-  Idle2Move
	--					<-  Move2Idle
	--					<-  Moving
	--					<-  MovingTurn
	--					<-> Jumping
	--					 -> Landing
	--		Landing		 -> Idle
	--					 -> Idle2Move
	--					<-  Jumping
	--					<-  Falling
	--		InteractStart-> InteractIdle
	--		InteractIdle -> InteractEnd
	--		InteractEnd	 -> Idle
	--		Dead
	--------------------------------------------------
	States = 
	{
		-- Idle is the base state. All other states should transition back to idle
		-- when complete.
		Idle = 
		{
			OnAnimationEvent = function(self, evName, startTime)
				if (evName == "IdleAnimEnded") then
					if (not self.IdleAnimLoop) then
						self.transitionReached = true;
					end
				end
			end,
			
			StartNewAnimation = function(self, sm, newAnim)
				self.transitionReached = false;
				self.idleAnimInUse = newAnim;
				sm.UserData.Fragments.Idle = sm:EnsureFragmentPlaying(sm.UserData.Fragments.Idle, 1, self.idleAnimInUse, "", self.IdleAnimLoop);
			end,
			
			OnEnter = function(self, sm)
				--Debug.Log("entering idle")
				-- Trigger idle animation as "persistent". Even if the state isn't running, this guarantees we never t-pose.
				self.StartNewAnimation(self, sm, self.IdleAnim);
				CharacterAnimationRequestBus.Event.SetAnimationDrivenMotion(sm.EntityId, false);
			end,
			
			OnExit = function(self, sm)
				-- Ensure we go back to the normal idle when we next return to Idle.
				if (self.IdleAnim ~= self.DefaultIdleAnim) then
					self.IdleAnim = self.DefaultIdleAnim;
				end
				
				sm.UserData.Fragments.Idle = sm:EnsureFragmentStopped(sm.UserData.Fragments.Idle);
				CryCharacterPhysicsRequestBus.Event.RequestVelocity(sm.EntityId, Vector3(0,0,0), 0);
			end,
			
			OnUpdate = function(self, sm, deltaTime)
				-- If the custom animation has finished, then return to the default idle.
				if (self.transitionReached) then
					self.IdleAnim = self.DefaultIdleAnim;
					self.IdleAnimLoop = self.DefaultIdleAnimLoop;
				end
				
				-- Change the animation we're using if a new one has been set.
				if (self.IdleAnim ~= self.idleAnimInUse) then
					sm.UserData.Fragments.Idle = sm:EnsureFragmentStopped(sm.UserData.Fragments.Idle);
					self.StartNewAnimation(self, sm, self.IdleAnim);
				end
			end,
			
			Transitions =
			{
				IdleTurn =
				{
					Evaluate = function(state, sm)
						return sm.UserData:IsMoving();
					end					
				},
				-- Transition to navigation if we start moving.
				Idle2Move =
				{
					Evaluate = function(state, sm)
						return sm.UserData:IsMoving() and sm.UserData:IsFacingTarget();
					end					
				},
				JumpingIdle =
				{
					Evaluate = function(state, sm)
						return sm.UserData:WantsToJump();
					end,
				},
				Falling =
				{
					Evaluate = function(state, sm)
						return sm.UserData:IsFalling();
					end
				},
			},
			
			IdleAnim = "Idle";
			IdleAnimLoop = true;
			DefaultIdleAnim = "Idle";
			DefaultIdleAnimLoop = true;
		},
		-- Idle turn state transitioning to move
		IdleTurn = 
		{
			OnAnimationEvent = function(self, evName, startTime)
				if(evName == "Transition") then
					self.transitionReached = true;
				else 
					if(evName == "Complete") then
						self.turnComplete = true;
					end
				end
			end,
			GetFacing = function(sm)
				local tm = TransformBus.Event.GetWorldTM(sm.EntityId);
				return tm:GetColumn(1):GetNormalized();
			end,
			OnEnter = function(self, sm)
				CharacterAnimationRequestBus.Event.SetAnimationDrivenMotion(sm.EntityId, true);
				self.blendParam = ((sm.UserData:GetAngleDelta() + math.pi) / (math.pi * 2.0));
				utilities.SetBlendParam(sm, self.blendParam, 7);
				sm.UserData.Fragments.IdleTurn = sm:EnsureFragmentPlaying(sm.UserData.Fragments.IdleTurn, 1, "IdleTurn", "", false);
				self.firstUpdate = true;
				self.transitionReached = false;
				self.turnComplete = false;
				self.startFacing = self.GetFacing(sm);
				
				-- Announce that we've started a turn.
				GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(sm.EntityId, "EventTurnStarted", "float"), 1.0);
			end,
			OnExit = function(self, sm)
				sm.UserData.Fragments.IdleTurn = sm:EnsureFragmentStopped(sm.UserData.Fragments.IdleTurn);
				CryCharacterPhysicsRequestBus.Event.RequestVelocity(sm.EntityId, Vector3(0,0,0), 0);
				
				-- Announce that we've ended a turn.
				GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(sm.EntityId, "EventTurnEnded", "float"), 1.0);
			end,

			-- Update movement logic while in navigation state.
			OnUpdate = function(self, sm, deltaTime)
				if(self.firstUpdate == true) then
					utilities.SetBlendParam(sm, self.blendParam, 7);
					self.firstUpdate = false;
				end				
			end,
			
			Transitions = 
			{
				-- Transition to idle as soon as we stop moving.
				Idle =
				{
					Evaluate = function(state, sm)
						return state.turnComplete == true and ((not sm.UserData:IsMoving()) or (not sm.UserData:IsFacingTarget()));
					end
				},
				Idle2Move =
				{
				   Evaluate = function(state, sm)
						return state.transitionReached == true and sm.UserData:IsMoving() and sm.UserData:IsFacingTarget();
					end
				},
				Falling =
				{
					Evaluate = function(state, sm)
						return sm.UserData:IsFalling();
					end
				},
			},
		},
		Idle2Move = 
		{	  
			OnAnimationEvent = function(self, evName, startTime)
				if(evName == "Transition") then
					self.transitionReached = true;
				end
			end,
			-- Get the angle of the slope the character is standing on in their facing direction
			OnEnter = function(self, sm)
				sm.UserData.Fragments.Idle2Move  = sm:EnsureFragmentPlaying(sm.UserData.Fragments.Idle2Move, 1, "Idle2Move", "", false);
				CharacterAnimationRequestBus.Event.SetAnimationDrivenMotion(sm.EntityId, true);
				self.prevFrameMoveSpeedParam = utilities.GetMoveSpeed(sm.UserData.movementDirection);
				if (sm.UserData:IsStrafing()) then
					self.prevFrameMoveSpeedParam = self.prevFrameMoveSpeedParam * (sm.UserData.Properties.StrafeMoveSpeed / sm.UserData.Properties.MoveSpeed);
				end	
				
				utilities.SetBlendParam(sm, self.prevFrameMoveSpeedParam, utilities.BP_MoveSpeed);
				utilities.SetBlendParam(sm, utilities.GetSlopeAngle(sm.UserData), utilities.BP_SlopeAngle);
				self.transitionReached = false;
			end,
			OnExit = function(self, sm)
				sm.UserData.Fragments.Idle2Move = sm:EnsureFragmentStopped(sm.UserData.Fragments.Idle2Move);
				CryCharacterPhysicsRequestBus.Event.RequestVelocity(sm.EntityId, Vector3(0,0,0), 0);
			end,			
			OnUpdate = function(self, sm, deltaTime)
				local angleDelta = sm.UserData:GetAngleDelta();
				sm.UserData:UpdateRotation(deltaTime, angleDelta);
				
				self.prevFrameMoveSpeedParam = utilities.GetMoveSpeed(sm.UserData.movementDirection);
				if (sm.UserData:IsStrafing()) then
					self.prevFrameMoveSpeedParam = self.prevFrameMoveSpeedParam * (sm.UserData.Properties.StrafeMoveSpeed / sm.UserData.Properties.MoveSpeed);
				end	
				utilities.SetBlendParam(sm, self.prevFrameMoveSpeedParam, utilities.BP_MoveSpeed);
				utilities.SetBlendParam(sm, utilities.GetSlopeAngle(sm.UserData), utilities.BP_SlopeAngle);
			end,
	
			Transitions =
			{
				Idle =
				{
					Evaluate = function(state, sm)
						return (state.prevFrameMoveSpeedParam <= sm.UserData.Properties.Move2IdleThreshold) and not sm.UserData:IsMoving();
					end
				},			
				Move2Idle =
				{
					Evaluate = function(state, sm)
						return not sm.UserData:IsMoving();
					end
				},
				Moving =
				{
				   Evaluate = function(state, sm)
						return state.transitionReached == true;
					end
				},
				MovingTurn =
				{
					Evaluate = function(state, sm)
						return sm.UserData:IsLargeAngleDelta();
					end
				},
				Jumping =
				{
					Evaluate = function(state, sm)
						return sm.UserData:WantsToJump();
					end,
				},
				Falling =
				{
					Evaluate = function(state, sm)
						return sm.UserData:IsFalling();
					end
				},
			},
		},
		Move2Idle =
		{
			OnAnimationEvent = function(self, evName, startTime)
				-- TODO: Add this animevent to the Move2Idle bspace.
				if(evName == "Interruptible") then
					self.interruptible = true;
				end
			end,
			
			OnEnter = function(self, sm)
				sm.UserData.Fragments.Move2Idle = sm:EnsureFragmentPlaying(sm.UserData.Fragments.Move2Idle, 1, "Move2Idle", "", false);
				sm.UserData.Fragments.Idle = sm:EnsureFragmentPlaying(sm.UserData.Fragments.Idle, 1, "Idle", "", false);
				utilities.SetBlendParam(sm, utilities.GetSlopeAngle(sm.UserData), utilities.BP_SlopeAngle);
				CharacterAnimationRequestBus.Event.SetAnimationDrivenMotion(sm.EntityId, true);
				self.transToIdle = false;
				self.interruptible = false;
			end,
			OnExit = function(self, sm)
				sm.UserData.Fragments.Move2Idle = sm:EnsureFragmentStopped(sm.UserData.Fragments.Move2Idle);
				if (not self.transToIdle) then
					sm.UserData.Fragments.Idle = sm:EnsureFragmentStopped(sm.UserData.Fragments.Idle);
				end
			end,
			OnUpdate = function(self, sm, deltaTime)
				utilities.SetBlendParam(sm, utilities.GetSlopeAngle(sm.UserData), utilities.BP_SlopeAngle);
			end,
			Transitions =
			{
				Idle =
				{
					Evaluate = function(state, sm)
						local res = utilities.CheckFragmentPlaying(sm.EntityId, sm.UserData.Fragments.Move2Idle) == false;
						if (res) then
							state.transToIdle = true;
						end
						return res;
					end
				},
				Idle2Move =
				{
					Evaluate = function(state, sm)
						return sm.UserData:IsMoving();
					end,
				},
				MovingTurn =
				{
					Priority = 1,
					Evaluate = function(state, sm)
						return state.interruptible and sm.UserData:IsLargeAngleDelta();
					end,
				},
				Falling =
				{
					Evaluate = function(state, sm)
						return sm.UserData:IsFalling();
					end
				},
			},
		},	
		-- Navigation is the movement state
		Moving = 
		{
			OnEnter = function(self, sm)
				sm.UserData.Fragments.Moving = sm:EnsureFragmentPlaying(sm.UserData.Fragments.Moving, 1, "Moving", "", false);
				CharacterAnimationRequestBus.Event.SetAnimationDrivenMotion(sm.EntityId, false);
				self.firstUpdate = true;
				self.prevFrameMoveMag = 0.0;
				self.movingLastFrame = true;
				self.turningLastFrame = false;
			end,

			OnExit = function(self, sm)
				sm.UserData.Fragments.Moving = sm:EnsureFragmentStopped(sm.UserData.Fragments.Moving);
				CryCharacterPhysicsRequestBus.Event.RequestVelocity(sm.EntityId, Vector3(0,0,0), 0);
			end,

			-- Update movement logic while in navigation state.
			OnUpdate = function(self, sm, deltaTime)
				-- Store the speed of the character. Smooth in case the player took their finger off the stick this frame.
				self.prevFrameMoveMag = 0.8 * self.prevFrameMoveMag + 0.2 * sm.UserData:UpdateMovement(deltaTime);
				
				self.movingLastFrame = sm.UserData:IsMoving();
			end,
			
			Transitions = 
			{
				-- Transition to idle as soon as we stop moving.
				Idle =
				{
					Evaluate = function(state, sm)
						return (state.prevFrameMoveMag <= sm.UserData.Properties.Move2IdleThreshold) and not sm.UserData:IsMoving() and not state.movingLastFrame;
					end,
				},
				Move2Idle =
				{
					Evaluate = function(state, sm)
						return not sm.UserData:IsMoving() and not state.movingLastFrame;
					end,
				},
				MovingTurn =
				{
					Priority = 1,
					Evaluate = function(state, sm)
						local isTurning = sm.UserData:IsLargeAngleDelta();
						local shouldTurn = isTurning and state.turningLastFrame;
						state.turningLastFrame = isTurning;
						sm.UserData.States.MovingTurn.MoveMag = state.prevFrameMoveMag;
						return shouldTurn;
					end,
				},
				Jumping =
				{
					Evaluate = function(state, sm)
						return sm.UserData:WantsToJump();
					end,
				},
				Falling =
				{
					Evaluate = function(state, sm)
						return sm.UserData:IsFalling();
					end
				},
			},
			
			turningLastFrame = false,
		},
		MovingTurn =
		{
			SetBlendParams = function(self, sm)
				utilities.SetBlendParam(sm, self.moveSpeed, utilities.BP_MoveSpeed);
				utilities.SetBlendParam(sm, self.angleDelta, utilities.BP_TurnAngle);
			end,
			
			OnAnimationEvent = function(self, evName, startTime)
				-- This check exists because we can transition from MovingTurn to MovingTurn
				-- which means that the first animation's events may get triggered in the
				-- second animation's state. This could result in the second animation being
				-- unlocked just after it's started which is why we keep track of how long this
				-- animation has been going for and compare it to the event time.
				-- Ideally we'd query the Mannequin to ask how long this animation has been
				-- playing but no such functionality exists which is why we use the deltaTime.
				if (self.timeInState < startTime) then
					return;
				end
					
				if(evName == "Transition") then
					self.transitionReached = true;
				elseif(evName == "UnlockedMotionTurn") then
					self.unlockedMotionTurn = true;
				elseif(evName == "UnlockedEarlyOut") then
					self.unlockedEarlyOut = true;
				elseif(evName == "LockedEarlyOut") then
					self.unlockedEarlyOut = false;
				end
			end,
			
			OnEnter = function(self, sm)
				self.transitionReached = false;
				self.unlockedMotionTurn = false;
				self.unlockedEarlyOut = false;
				
				self.performedFirstUpdate = false;
				self.timeInState = 0.0;
				
				self.angleDelta, self.moveSpeed = sm.UserData:GetAngleDelta();
				if (self.MoveMag > self.moveSpeed) then
					self.moveSpeed = self.MoveMag;
				end
				
				if (sm.UserData:IsStrafing()) then
					self.moveSpeed = self.moveSpeed * (sm.UserData.Properties.StrafeMoveSpeed / sm.UserData.Properties.MoveSpeed);
				end	
				
				-- This is the only reason we need to know the sign; after this 'if' the
				-- sign of the angle will be trashed to get a normalised value we can send
				-- as a blend parameter.
				if (self.angleDelta < 0.0) then
					self.side = "MovingTurnRight";
				else
					self.side = "MovingTurnLeft";
				end
				
				-- Normalise the angle to turn so it's between 0 and 1.
				-- Note that 0 == 90 degrees and 1 == 180 degrees.
				local halfPi = math.pi * 0.5;
				self.angleDelta = (math.abs(self.angleDelta) - halfPi) / halfPi;
				self.angleDelta = utilities.Clamp(self.angleDelta, 0.0, 1.0);
				
				self:SetBlendParams(sm);
				sm.UserData.Fragments.MovingTurn = sm:EnsureFragmentPlaying(sm.UserData.Fragments.MovingTurn, 1, self.side, "", false);
				CharacterAnimationRequestBus.Event.SetAnimationDrivenMotion(sm.EntityId, true);
				
				-- Announce that we've started a turn.
				GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(sm.EntityId, "EventTurnStarted", "float"), 1.0);
			end,
			
			OnExit = function(self, sm)
				sm.UserData.Fragments.MovingTurn = sm:EnsureFragmentStopped(sm.UserData.Fragments.MovingTurn);
				self.MoveMag = 0.0;
				
				-- Announce that we've ended a turn.
				GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(sm.EntityId, "EventTurnEnded", "float"), 1.0);
			end,
			
			OnUpdate = function(self, sm, deltaTime)
				if (not self.performedFirstUpdate) then
					-- This needs to be set on the first frame because only being done in 'OnEnter'
					-- is apparently insufficient.
					self:SetBlendParams(sm);
					
					self.performedFirstUpdate = true;
				end
				
				self.timeInState = self.timeInState + deltaTime;
			end,
			
			Transitions =
			{
				Move2Idle =
				{
					Evaluate = function(state,sm)
						return state.transitionReached and not sm.UserData:IsMoving();
					end
				},
				Moving =
				{
					Evaluate = function(state, sm)
						local isFacing = state.unlockedEarlyOut and math.abs(sm.UserData:GetAngleDelta()) <= 0.05;
						local isMoving = state.transitionReached and sm.UserData:IsMoving();
						return isFacing or isMoving;
					end
				},
				MovingTurn =
				{
					Priority = 1,
					Evaluate = function(state, sm)
						return sm.UserData:IsLargeAngleDelta() and state.unlockedMotionTurn;
					end
				},
				Falling =
				{
					Evaluate = function(state, sm)
						return sm.UserData:IsFalling();
					end
				},
			},
			
			MoveMag = 0.0,
		},
		JumpingIdle =
		{
			OnAnimationEvent = function(self, evName, startTime)
				if(evName == "TransitionToIdle") then
					self.transitionToIdleReached = true;
				elseif(evName == "TransitionToMoving") then
					self.transitionToMovingReached = true;
				end
			end,
			
			OnEnter = function(self, sm)
				self.transitionToIdleReached = false;
				self.transitionToMovingReached = false;
				sm.UserData:SetCanStrafe(false);
				sm.UserData:SetCanFire(false);
				
				sm.UserData.Fragments.JumpingIdle = sm:EnsureFragmentPlaying(sm.UserData.Fragments.JumpingIdle, 1, "JumpIdle", "", false);
				CharacterAnimationRequestBus.Event.SetAnimationDrivenMotion(sm.EntityId, true);
			end,
			
			OnExit = function(self, sm)
				sm.UserData:SetCanStrafe(true);
				sm.UserData:SetCanFire(true);
				sm.UserData.Fragments.JumpingIdle = sm:EnsureFragmentStopped(sm.UserData.Fragments.JumpingIdle);
				CharacterAnimationRequestBus.Event.SetAnimationDrivenMotion(sm.EntityId, false);
			end,
			
			OnUpdate = function(self, sm, deltaTime)
				
			end,
			
			Transitions =
			{
				Idle =
				{
					Evaluate = function(state, sm)
						return state.transitionToIdleReached and not sm.UserData:IsMoving();
					end
				},
				Idle2Move =
				{
					Evaluate = function(state, sm)
						return state.transitionToMovingReached and sm.UserData:IsMoving();
					end
				},
			},
		},
		Jumping =
		{
			OnAnimationEvent = function(self, evName, startTime)
				if (evName == self.transitionName) then
					self.transitionReached = true;
				elseif (evName == "EnableJump") then
					self.canJump = true;
				elseif (evName == "DisableJump") then
					self.canJump = false;
				end
			end,
			
			OnEnter = function(self, sm)
				sm.UserData:SetCanStrafe(false);
				sm.UserData:SetCanFire(false);

				sm.UserData.States.Falling.FallDuration = 0.0;
				sm.UserData.nudgingCollisionTimer = 0.0;
				
				self.launchCompleted = false;
				self.canJump = false;
				self.transitionReached = false;
				self.transitionedToFalling = false;
				
				self.animJump = "JumpRunning";
				self.transitionName = "TransitionJump";
				self.force = sm.UserData.Properties.Jumping.JumpForce;
				if (self.WantsToDoubleJump) then
					self.IsDoubleJumping = true;
					self.animJump = "JumpRunningDouble";
					self.transitionName = "TransitionDouble";
					self.force = sm.UserData.Properties.Jumping.JumpDoubleForce;
					
					-- Signal that we're double-jumping.
					-- This is primarily for the thruster particle effects because they can't be
					-- done through the animation event system.
					local eventId = GameplayNotificationId(sm.UserData.entityId, "DoubleJumped", "float");
					GameplayNotificationBus.Event.OnEventBegin(eventId, sm.UserData.entityId);
				else
					local maxSpeed = sm.UserData.Properties.MoveSpeed;
					if (sm.UserData:IsStrafing()) then
						maxSpeed = sm.UserData.Properties.StrafeMoveSpeed;
					end
					local forward = TransformBus.Event.GetWorldTM(sm.UserData.entityId):GetColumn(1):GetNormalized();
					-- 'GetLength()' shouldn't return an invalid value because we SHOULD always be
					-- moving on the first frame of the jump (otherwise we'd have gone into
					-- 'JumpingIdle').
					forward = forward * sm.UserData.movementDirection:GetLength();
					self.JumpingVector = forward * maxSpeed;
				end
				
				sm.UserData.Fragments.Jumping = sm:EnsureFragmentPlaying(sm.UserData.Fragments.Jumping, 1, self.animJump, "", false);
				
				self.prevTm = TransformBus.Event.GetWorldTM(sm.UserData.entityId);
				PhysicsComponentRequestBus.Event.EnablePhysics(sm.UserData.entityId);
				
				-- Push the character upwards.
				local moveMag = (self.JumpingVector) + Vector3(0.0, 0.0, self.force);
				PhysicsComponentRequestBus.Event.SetVelocity(sm.UserData.entityId, moveMag);
			end,
			
			OnExit = function(self, sm)
				sm.UserData:SetCanStrafe(true);
				sm.UserData:SetCanFire(true);
			
				sm.UserData.Fragments.Jumping = sm:EnsureFragmentStopped(sm.UserData.Fragments.Jumping);
				
				if (not self.transitionedToFalling) then
					sm.UserData.fallingPrevTm = nil;
				end
				
				if (self.IsDoubleJumping) then
					self.WantsToDoubleJump = false;
					self.IsDoubleJumping = false;
				end
			end,
			
			OnUpdate = function(self, sm, deltaTime)
				local thisTm = TransformBus.Event.GetWorldTM(sm.UserData.entityId);
				if (utilities.GetDistanceMovedDown(self.prevTm, thisTm) > 0) then
					sm.UserData.States.Falling.FallDuration = sm.UserData.States.Falling.FallDuration + deltaTime;
				else
					sm.UserData.States.Falling.FallDuration = 0.0;
				end
				sm.UserData:UpdateFallingNudging(deltaTime, self.JumpingVector);
				self.prevTm = thisTm;
			end,
			
			Transitions =
			{
				Jumping =
				{
					Evaluate = function(state, sm)
						if (state.IsDoubleJumping) then
							return false;
						end
						
						local res = state.canJump and sm.UserData:WantsToJump();
						if (res) then
							state.WantsToDoubleJump = true;
						end
						return res;
					end
				},
				
				Falling =
				{
					Evaluate = function(state, sm)
						state.transitionedToFalling = state.transitionReached and not sm.UserData:ShouldLand();
						return state.transitionedToFalling;
					end
				},
				
				Landing =
				{
					Evaluate = function(state, sm)
						return state.transitionReached and sm.UserData:ShouldLand();
					end
				},
			},
			
			WantsToDoubleJump = false;
			IsDoubleJumping = false;
			JumpingVector = Vector3(0.0, 0.0, 0.0);
		},
		Falling =
		{
			OnAnimationEvent = function(self, evName, startTime)
				if(evName == "Transition") then
					self.transitionReached = true;
				end
			end,
			
			OnEnter = function(self, sm)
				sm.UserData:SetCanStrafe(false);
				sm.UserData:SetCanFire(false);

				self.transitionReached = false;
				--self.FallDuration = 0.0;
				
				-- If we walk off a cliff instead of jumping then we'll enter this state with a
				-- zero jumping vector.
				if (sm.UserData.fallingPrevTm == nil) then
					local maxSpeed = sm.UserData.Properties.MoveSpeed;
					if (sm.UserData:IsStrafing()) then
						maxSpeed = sm.UserData.Properties.StrafeMoveSpeed;
					end
					local forward = TransformBus.Event.GetWorldTM(sm.UserData.entityId):GetColumn(1):GetNormalized();
					-- If 'GetLength()' is a zero length vector then it means we've started falling
					-- because the ground has moved rather than us walking/jumping off something.
					local movDirLen = sm.UserData.movementDirection:GetLength();
					if (movDirLen <= 0.001) then
						forward = Vector3(0.0, 0.0, -1.0);
					else
						forward = forward * movDirLen;
					end
					sm.UserData.States.Jumping.JumpingVector = forward * maxSpeed;
				end
				
				sm.UserData.Fragments.Falling = sm:EnsureFragmentPlaying(sm.UserData.Fragments.Falling, 1, "Falling", "", true);
				
				self.prevTm = TransformBus.Event.GetWorldTM(sm.UserData.entityId);
				self.NotReallyFalling = 0;
			end,
			
			OnExit = function(self, sm)
				sm.UserData:SetCanStrafe(true);
				sm.UserData:SetCanFire(true);
				sm.UserData.Fragments.Falling = sm:EnsureFragmentStopped(sm.UserData.Fragments.Falling);
				
				if (self.NotReallyFalling > self.NotReallyFallingMax) then
					sm.UserData.landedBecauseSupported = true;
					sm.UserData.landedBecauseSupportedPrevTm = TransformBus.Event.GetWorldTM(sm.UserData.entityId);
				end
				sm.UserData.fallingPrevTm = nil;
			end,
			
			OnUpdate = function(self, sm, deltaTime)
				self.FallDuration = self.FallDuration + deltaTime;
				
				local thisTm = TransformBus.Event.GetWorldTM(sm.UserData.entityId);
				local diff = math.abs(utilities.GetDistanceMovedDown(self.prevTm, thisTm));
				if (diff < 0.001) then
					self.NotReallyFalling = self.NotReallyFalling + 1;
				end
				self.prevTm = thisTm;
				sm.UserData:UpdateFallingNudging(deltaTime, sm.UserData.States.Jumping.JumpingVector);
			end,
			
			Transitions =
			{
				-- The player shouldn't really be allowed to jump when falling, but we can
				-- occasionally get stuck in falling for a few reasons in various circumstances.
				-- Therefore, if the player is falling for a long time then allow them to jump out.
				Jumping =
				{
					Evaluate = function(state, sm)
						return state.FallDuration >= sm.UserData.Properties.Falling.FallingEscape and sm.UserData:WantsToJump();
					end
				},
				
				Landing =
				{
					Evaluate = function(state, sm)
						return sm.UserData:ShouldLand() or state.NotReallyFalling > state.NotReallyFallingMax;
					end
				},
			},
			
			FallDuration = 0.0;
			NotReallyFalling = 0;
			NotReallyFallingMax = 3;	-- how many frames we can 'not really fall' before we land
		},
		Landing =
		{
			OnAnimationEvent = function(self, evName, startTime)
				if(evName == "TransitionLandingToIdle") then
					self.transitionToIdleReached = true;
				elseif(evName == "TransitionLandingToMoving") then
					self.transitionToMovingReached = true;
				end
			end,
			
			OnEnter = function(self, sm)
				sm.UserData:SetCanStrafe(false);
				sm.UserData:SetCanFire(false);
				
				self.transitionToIdleReached = false;
				self.transitionToMovingReached = false;
				self.useRootMotion = false;
				
				self.anim = "LandingSoft";
				if (sm.UserData.States.Falling.FallDuration >= sm.UserData.Properties.Falling.FallingHard) then
					self.anim = "LandingHard";
					self.useRootMotion = true;
				elseif (sm.UserData.States.Falling.FallDuration >= sm.UserData.Properties.Falling.FallingMedium) then
					self.anim = "LandingMedium";
					self.useRootMotion = true;
				end
				--Debug.Log("Fell for " .. tostring(sm.UserData.States.Falling.FallDuration) .. " seconds: playing " .. tostring(self.anim));
				sm.UserData.States.Falling.FallDuration = 0.0;
				
				sm.UserData.Fragments.Landing = sm:EnsureFragmentPlaying(sm.UserData.Fragments.Landing, 1, self.anim, "", false);
				CharacterAnimationRequestBus.Event.SetAnimationDrivenMotion(sm.EntityId, self.useRootMotion);
			end,
			
			OnExit = function(self, sm)
				sm.UserData:SetCanStrafe(true);
				sm.UserData:SetCanFire(true);
				
				sm.UserData.Fragments.Landing = sm:EnsureFragmentStopped(sm.UserData.Fragments.Landing);
				CharacterAnimationRequestBus.Event.SetAnimationDrivenMotion(sm.EntityId, false);
			end,
			
			OnUpdate = function(self, sm, deltaTime)
				
			end,
			
			Transitions =
			{
				Idle =
				{
					Evaluate = function(state, sm)
						return state.transitionToIdleReached and state.anim ~= "LandingSoft" and not sm.UserData:IsMoving();
					end
				},
				Idle2Move =
				{
					Priority = 2,
					Evaluate = function(state, sm)
						return state.transitionToMovingReached and sm.UserData:IsMoving();
					end
				},
				-- Soft landing naturally goes into a run, so we need to transition into a Move2Idle.
				Move2Idle =
				{
					Evaluate = function(state, sm)
						return (state.transitionToIdleReached or state.anim == "LandingSoft") and not sm.UserData:IsMoving();
					end
				},
				Moving =
				{
					Priority = 1,
					Evaluate = function(state, sm)
						return (state.transitionToMovingReached or state.anim == "LandingSoft") and sm.UserData:IsMoving();
					end
				},
			},
		},
		InteractStart =
		{
			OnAnimationEvent = function(self, evName, startTime)
				if(evName == "TransitionToIdle") then
					self.transitionToIdleReached = true;
				end
			end,
					
			OnEnter = function(self, sm)
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
				rayCastConfig.physicalEntityTypes = PhysicalEntityTypes.Static + PhysicalEntityTypes.Terrain;
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
				
				-- start interact anim
				self.transitionToIdleReached = false;
				sm.UserData.Fragments.InteractStart  = sm:EnsureFragmentPlaying(sm.UserData.Fragments.InteractStart, 1, "InteractStart", "", false);
				CharacterAnimationRequestBus.Event.SetAnimationDrivenMotion(sm.EntityId, true);
			end,
			OnExit = function(self, sm)
				sm.UserData.Fragments.InteractStart = sm:EnsureFragmentStopped(sm.UserData.Fragments.InteractStart);
			end,
			OnUpdate = function(self, sm, deltaTime)
			end,
			Transitions =
			{
				InteractIdle =
				{
					Evaluate = function(state, sm)
						return state.transitionToIdleReached or (not utilities.CheckFragmentPlaying(sm.EntityId, sm.UserData.Fragments.InteractStart)) -- interact anim completes
					end
				},
			},
		},
		InteractIdle =
		{
			OnEnter = function(self, sm)
				-- start idle anim
			   sm.UserData.Fragments.InteractIdle  = sm:EnsureFragmentPlaying(sm.UserData.Fragments.InteractIdle, 1, "InteractIdle", "", true);
				CharacterAnimationRequestBus.Event.SetAnimationDrivenMotion(sm.EntityId, false);
			end,
			OnExit = function(self, sm)
				sm.UserData.Fragments.InteractIdle = sm:EnsureFragmentStopped(sm.UserData.Fragments.InteractIdle);
			end,
			OnUpdate = function(self, sm, deltaTime)
			end,
			Transitions =
			{
				InteractEnd =
				{
					Evaluate = function(state, sm)
						return sm.UserData.InteractComplete; -- wait for UI to close
					end
				},
			},
		},
		InteractEnd =
		{
			OnAnimationEvent = function(self, evName, startTime)
				if(evName == "TransitionToIdle") then
					self.transitionToIdleReached = true;
				end
			end,
		
			OnEnter = function(self, sm)
				-- start interact end anim
				self.transitionToIdleReached = false;
				sm.UserData.Fragments.InteractEnd  = sm:EnsureFragmentPlaying(sm.UserData.Fragments.InteractEnd, 2, "InteractEnd", "", false);
				CharacterAnimationRequestBus.Event.SetAnimationDrivenMotion(sm.EntityId, true);
			end,
			OnExit = function(self, sm)
				sm.UserData.Fragments.InteractEnd = sm:EnsureFragmentStopped(sm.UserData.Fragments.InteractEnd);
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
				Idle =
				{
					Evaluate = function(state, sm)
						-- interact end anim completes
						return state.transitionToIdleReached or (not utilities.CheckFragmentPlaying(sm.EntityId, sm.UserData.Fragments.InteractEnd));
					end
				},
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
				sm.UserData:SetCanStrafe(false);
				if (sm.UserData.shouldImmediatelyRagdoll) then
					RagdollPhysicsRequestBus.Event.EnterRagdoll(sm.EntityId);
				else
				local deathTag = "DeathFront";
				if ((sm.UserData.hitParam ~= nil) and(sm.UserData.hitParam > 0.25) and (sm.UserData.hitParam < 0.75)) then
					deathTag = "DeathBack";
				end
				sm.UserData.Fragments.Dead = sm:EnsureFragmentPlaying(sm.UserData.Fragments.Dead, 2, "Dead", deathTag, false);
				end
				
				CharacterAnimationRequestBus.Event.SetAnimationDrivenMotion(sm.EntityId, true);
				sm.UserData:SetControlsEnabled(false);
				self.switchRagdoll = false;
				
				-- Announce to the entity that it's died.
				GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(sm.EntityId, "OnDeath", "float"), 1.0);
			end,
			
			OnUpdate = function(self, sm, deltaTime)
				if(self.switchRagdoll == true) then
					RagdollPhysicsRequestBus.Event.EnterRagdoll(sm.EntityId);
					self.switchRagdoll = false;
				end
			end,
			
			Transitions =
			{
			},
		},
	},
	NoTurnIdleAngle = 0.78, -- The player won't play a turn idle animation if the move in a direction less than this angle from the facing direction
}

--------------------------------------------------
-- Component behavior
--------------------------------------------------

function movementcontroller:OnActivate()	
	-- Play the specified Audio Trigger (wwise event) on this component
	AudioTriggerComponentRequestBus.Event.Play(self.entityId);

	self.Properties.RotationSpeed = Math.DegToRad(self.Properties.RotationSpeed);
	self.Fragments = {};
	
	-- Enable and disable events
	self.enableEventId = GameplayNotificationId(self.entityId, "Enable", "float");
	self.enableHandler = GameplayNotificationBus.Connect(self, self.enableEventId);
	self.disableEventId = GameplayNotificationId(self.entityId, "Disable", "float");
	self.disableHandler = GameplayNotificationBus.Connect(self, self.disableEventId);
	self.setVisibleEventId = GameplayNotificationId(self.entityId, "SetVisible", "float");
	self.setVisibleHandler = GameplayNotificationBus.Connect(self, self.setVisibleEventId);
	
	self.teleportToEventId = GameplayNotificationId(self.entityId, "TeleportTo", "float");
	self.teleportToHandler = GameplayNotificationBus.Connect(self, self.teleportToEventId);
	
	self.enableFiringEventId = GameplayNotificationId(self.entityId, "EnableFiring", "float");
	self.disableFiringEventId = GameplayNotificationId(self.entityId, "DisableFiring", "float");
	
	self.setIdleAnimEventId = GameplayNotificationId(self.entityId, "SetIdleAnim", "float");
	self.setIdleAnimHandler = GameplayNotificationBus.Connect(self, self.setIdleAnimEventId);
	
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
	
	-- Delay firing for a frame so gun comes up
	--self.fireTriggered = nil;
	--self.gunUpDelay = 0.0;
	
	-- Use this to get the camera information when firing. It saves making an entity property
	-- and linking the weapon to a specific camera entity.
	-- Note: this only returns the LAST entity with this tag, so make sure there's only one
	-- entity with the "PlayerCamera" tag otherwise weird stuff might happen.
	self.cameraTag = Crc32("PlayerCamera");
	self.wasHit = false;
	
	self.setMovementDirectionId = GameplayNotificationId(self.entityId, "SetMovementDirection", "float");
	self.setMovementDirectionHandler = GameplayNotificationBus.Connect(self, self.setMovementDirectionId);
	self.movementDirection = Vector3(0,0,0);
	
	self.jumpEventId = GameplayNotificationId(self.entityId, "Jump", "float");
	self.jumpHandler = GameplayNotificationBus.Connect(self, self.jumpEventId);
	self.wantsToJump = false;
	self.wantedToJumpLastFrame = false;
	self.landedBecauseSupported = false;
	self.landedBecauseSupportedPrevTm = nil;

	self.strafeEventId = GameplayNotificationId(self.entityId, "Strafe", "float");
	self.strafeHandler = GameplayNotificationBus.Connect(self, self.strafeEventId);
	self.wantsToStrafe = false;
	self.canStrafe = true;
	self.isStrafing = false;

	self.camAimSettingsEventArgs = CameraSettingsEventArgs();
	self.camAimSettingsEventArgs.name = "Aim";
	self.camAimSettingsEventArgs.entityId = self.entityId;
			
	self.startInteractEventId = GameplayNotificationId(self.entityId, "StartInteract", "float");
	self.startInteractHandler = GameplayNotificationBus.Connect(self, self.startInteractEventId);
	self.endInteractEventId = GameplayNotificationId(self.entityId, "EndInteract", "float");
	self.endInteractHandler = GameplayNotificationBus.Connect(self, self.endInteractEventId);
	
	self.Properties.Falling.SlopeAngle = self.Properties.Falling.SlopeAngle * math.pi / 180.0;
	self.fallingPrevTm = nil;
end

function movementcontroller:OnDeactivate()

	-- Terminate our state machine.
	self.StateMachine:Stop();
	self.tickBusHandler:Disconnect();
	self.tickBusHandler = nil;
	
	if (self.physicsNotificationHandler ~= nil) then
		self.physicsNotificationHandler:Disconnect();
		self.physicsNotificationHandler = nil;
	end

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
	if (self.setVisibleHandler ~= nil) then
		self.setVisibleHandler:Disconnect();
		self.setVisibleHandler = nil;	
	end
	if (self.teleportToHandler ~= nil) then
		self.teleportToHandler:Disconnect();
		self.teleportToHandler = nil;	
	end
	
	if (self.setIdleAnimHandler ~= nil) then
		self.setIdleAnimHandler:Disconnect();
		self.setIdleAnimHandler = nil;
	end
	
	if (self.jumpHandler ~= nil) then
		self.jumpHandler:Disconnect();
		self.jumpHandler = nil;
	end
	
end

function movementcontroller:IsMoving()
	return Vector3.GetLengthSq(self.movementDirection) > 0.01;
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
		self.landedBecauseSupportedPrevTm = nil;
	else
		if (self.landedBecauseSupported) then
			-- If we landed because we were in the falling state but not actually going downwards
			-- then we need to perform extra raycasts around us to check if we're still supported.
			-- I'd have liked to just use a simple 'have I moved down in the last frame' check but
			-- because this is called in the statemachine and the 'prevTm' would be stored in the
			-- MovementController it means I'd have to rely on a specific update order.
			local thisTm = TransformBus.Event.GetWorldTM(self.entityId);
			local pos = thisTm:GetTranslation();
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
				-- If we're not moving up or down then we're not falling.
				local zDiff = pos.z - self.landedBecauseSupportedPrevTm:GetTranslation().z;
				if (math.abs(zDiff) >= 0.1) then
					isFalling = true;
				end
			end
			
			self.landedBecauseSupportedPrevTm = TransformBus.Event.GetWorldTM(self.entityId);
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
	rayCastConfig.physicalEntityTypes = PhysicalEntityTypes.Static + PhysicalEntityTypes.Dynamic + PhysicalEntityTypes.Living + PhysicalEntityTypes.Terrain;
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

function movementcontroller:UpdateMovement(deltaTime)
	 -- Protect against no specified camera.
	--if (not self.Properties.Camera:IsValid()) then
	--	return
	--end
	
	local angleDelta, moveMag = self:GetAngleDelta();
	self:UpdateRotation(deltaTime, angleDelta);

	-- Request movement from character physics.
	local tm = TransformBus.Event.GetWorldTM(self.entityId);
	local maxSpeed = self.Properties.MoveSpeed;
	if (self.isStrafing) then
		maxSpeed = self.Properties.StrafeMoveSpeed;
	end
	
	local vel = (tm:GetColumn(1) * moveMag * maxSpeed);
	CryCharacterPhysicsRequestBus.Event.RequestVelocity(self.entityId, vel, 0);	
	return moveMag;
end

-- Updates the character's rotation.
function movementcontroller:UpdateRotation(deltaTime, angleDelta)
	local tm = TransformBus.Event.GetWorldTM(self.entityId);
	if (angleDelta ~= 0.0) then
		local rotationRate = self.Properties.RotationSpeed;
		local thisFrame = rotationRate * deltaTime;
		local absAngleDelta = self:Abs(angleDelta);
		if (absAngleDelta > FloatEpsilon) then
			thisFrame = self:Clamp(angleDelta, -thisFrame, thisFrame);
			local rotationTm = Transform.CreateRotationZ(thisFrame);
			tm = tm * rotationTm;
			tm:Orthogonalize();
			TransformBus.Event.SetWorldTM(self.entityId, tm);			
		end
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
		
		self.strafingEventId = nil;
		if (self.Properties.Drone:IsValid()) then
			self.strafingEventId = GameplayNotificationId(self.Properties.Drone, "IsStrafing", "float");
		end
		
		self.SetHideUIEventId = GameplayNotificationId(EntityId(), self.Properties.UIMessages.SetHideUIMessage, "float");

		-- Make sure we don't do this 'first update' again.
		self.performedFirstUpdate = true;	
	end
	
	self:UpdateHit();
	
	self:UpdateStrafeMode();
	
	if (self.wantedToJumpLastFrame) then
		self.wantsToJump = false;
		self.wantedToJumpLastFrame = false;
	elseif (self.wantsToJump) then
		self.wantedToJumpLastFrame = true;
	end
end

function movementcontroller:OnCollision(data)
	if (data and data ~= nil) then
		self.nudgingCollisionTimer = self.Properties.Falling.Nudging.CollisionTimer;
	end
end
   
function movementcontroller:Die()
	--Debug.Log("Player killed message");
	self.StateMachine:GotoState("Dead");
end

function movementcontroller:IsDead()
	local isDead = self.StateMachine.CurrentState == self.States.Dead;
	return isDead;
end

function movementcontroller:UpdateHit()
	if(self.wasHit == true) then
		CharacterAnimationRequestBus.Event.SetBlendParameter(self.entityId, utilities.BP_Hit, self.hitParam);	
		self.wasHit = utilities.CheckFragmentPlaying(self.entityId, self.Fragments.Hit);
	end
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
			MannequinRequestsBus.Event.ForceFinishRequest(self.entityId, self.Fragments.Hit);
		end
		self.wasHit = true;
		if(angle > 0) then
			self.hitParam = (angle / PI_2);
		else
			self.hitParam = (angle / PI_2) + 1.0;
		end
		self.Fragments.Hit = MannequinRequestsBus.Event.QueueFragment(self.entityId, 1, "Hit", "", false);
	end
end

function movementcontroller:UpdateStrafeMode()
	local doStrafe = self.canStrafe and self.wantsToStrafe;
	if ((not self.isStrafing) and doStrafe) then
		self.camAimSettingsEventArgs.name = "Aim";
		self.camAimSettingsEventArgs.entityId = self.entityId;
		self.camAimSettingsEventArgs.transitionTime = 0.25;
		GameplayNotificationBus.Event.OnEventBegin(self.camPushNotificationId, self.camAimSettingsEventArgs);
		self.isStrafing = true;
		if (self.strafingEventId ~= nil) then
			GameplayNotificationBus.Event.OnEventBegin(self.strafingEventId, self.isStrafing);
		end
	elseif (self.isStrafing and (not doStrafe)) then
		self.camAimSettingsEventArgs.name = "Aim";
		self.camAimSettingsEventArgs.entityId = self.entityId;
		self.camAimSettingsEventArgs.transitionTime = 0.25;
		GameplayNotificationBus.Event.OnEventBegin(self.camPopNotificationId, self.camAimSettingsEventArgs);
		self.isStrafing = false;
		if (self.strafingEventId ~= nil) then
			GameplayNotificationBus.Event.OnEventBegin(self.strafingEventId, self.isStrafing);
		end
	end
end

function movementcontroller:IsStrafing()
	return self.isStrafing;
end
	
function movementcontroller:SetCanStrafe(value)
	self.canStrafe = value;
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
	self.InteractComplete = false;
	self.StateMachine:GotoState("InteractStart");
end

function movementcontroller:EndInteract()
	self.InteractComplete = true;
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
		--	½ MaxSpeed == not pushing or pulling
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
			if (not StarterGameEntityUtility.GetEntitysVelocityLegacy(self.entityId, vVel)) then
				--Debug.Log(tostring(self.entityId) .. " failed to get velocity.");
			--else
			--	Debug.Log(tostring(self.entityId) .. " vel: " .. tostring(vVel));
			end
			--StarterGameEntityUtility.GetEntitysVelocity(self.entityId, vVel);
			
			-- Combine the vertical and horizontal velocities and set it.
			hVel.z = vVel.z;
			PhysicsComponentRequestBus.Event.SetVelocity(self.entityId, hVel);
		end
	end
	self.fallingPrevTm = thisTm;
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
		Light.Event.SetVisible(thischild, enabled);
	end	
end
	
function movementcontroller:OnEnable()
	self.StateMachine:Resume();
	self.tickBusHandler = TickBus.Connect(self);
	PhysicsComponentRequestBus.Event.EnablePhysics(self.entityId);
	MeshComponentRequestBus.Event.SetVisibility(self.entityId, true);
	MannequinRequestsBus.Event.ResumeAll(self.entityId, 7);
	
	-- Enable the lights on the character
	self:TurnOnLights(true);
end

function movementcontroller:OnDisable()
	MannequinRequestsBus.Event.PauseAll(self.entityId);
	MeshComponentRequestBus.Event.SetVisibility(self.entityId, false);
	PhysicsComponentRequestBus.Event.DisablePhysics(self.entityId);
	self.StateMachine:Stop();
	self.tickBusHandler:Disconnect();

	-- Enable the lights on the character
	self:TurnOnLights(false);
end
	
function movementcontroller:OnEventBegin(value)
	local busId = GameplayNotificationBus.GetCurrentBusId();
	--Debug.Log("movementcontroller:OnEventBegin( " .. tostring(value) .. " )");
	if (busId == self.controlsEnabledEventId) then
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
		if (busId == self.setMovementDirectionId) then
			--Debug.Log("setMovementDirectionId ( " .. tostring(value) .. " )");
			self.movementDirection = value;
		elseif (busId == self.jumpEventId) then
			self.wantsToJump = true;
		elseif (busId == self.strafeEventId) then
			self.wantsToStrafe = true;
		end
	end

	if (busId == self.getDiedEventId) then
		self:Die();
	elseif (busId == self.gotShotEventId) then
		-- React to being hit (if it wasn't done by themself).
		if (self.entityId ~= value.assailant) then
			self.shouldImmediatelyRagdoll = value.immediatelyRagdoll;
			self:Hit(value);
		end
	end
	
	if (busId == self.startInteractEventId) then
		self:StartInteract(value.positionEntity, value.cameraEntity);
	elseif (busId == self.endInteractEventId) then
		self:EndInteract();
	end
	
	if (busId == self.enableEventId) then
		self:OnEnable();
	elseif (busId == self.disableEventId) then
		self:OnDisable();
	elseif (busId == self.setVisibleEventId) then
		MeshComponentRequestBus.Event.SetVisibility(self.entityId, value);
	end
	
	if (busId == self.teleportToEventId) then
		local tm = TransformBus.Event.GetWorldTM(self.entityId);
		tm:SetTranslation(value);
		TransformBus.Event.SetWorldTM(self.entityId, tm);
	end
	
	if (busId == self.setIdleAnimEventId) then
		self.States.Idle.IdleAnim = value.animName;
		self.States.Idle.IdleAnimLoop = value.loop;
	end
end
	
function movementcontroller:OnEventEnd(value)
	local busId = GameplayNotificationBus.GetCurrentBusId();
	if (busId == self.strafeEventId) then
		self.wantsToStrafe = false;
	end
end	

return movementcontroller;
