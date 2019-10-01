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

local aicontroller = 
{
    Properties = 
    {
		NavMarker = { default = EntityId(), description = "A blank marker for use with runtime navigation locations." },
		
		StateMachines =
		{
			AI =
			{
				InitialState = "Idle",
				DebugStateMachine = false,
				
				Idle =
				{
					MoveSpeed = { default = 2.0 },
					
					SentryTurnRate = { default = 6.0, description = "How long sentries wait before turning.", suffix = " seconds" },
					--SentryTurnAngle = { default = 30.0, description = "The angle that sentries turn each time.", suffix = " degrees" },
					SentryArrivalThreshold = { default = 4.0, description = "How far away a sentry has to be from its post to path back to it.", suffix = " m" },
				},
				
				Suspicious =
				{
					MoveSpeed = { default = 3.0 },
					
					SuspicionRange = { default = 10.0, description = "Detection range in front of the A.I." },
					SuspicionRangeRear = { default = 4.0, description = "Detection range outside the A.I.'s F.o.V." },
					AIFieldOfView = { default = 75.0, description = "The angle of vision in front of the A.I. (in degrees).", suffix = " degrees" },
					DurationOfSuspicion = { default = 8.0, description = "How long the A.I. will inspect a suspicious location (in seconds).", suffix = " s" },
				},
				
				Combat =
				{
					MoveSpeed = { default = 4.0 },
					MoveSpeedChasing = { default = 4.0, description = "Movement speed when attempting to get in range of the player." },
					AngleBeforeReCentering = { default = 50.0, description = "How far before the A.I. turns towards the target.", suffix = " degrees" },
					MinCombatTimer = { default = 5.0, description = "The minimum amount of time to spend in combat." },
					
					AssistRange = { default = 5.0, description = "The range at which A.I. should immediately assist allies in combat." },
					HearingScalar = { default = 1.0, description = "Scalar for the range at which A.I. can hear sounds. [0.0 .. 1.0]" },
					
					AggroRange = { default = 6.0, description = "The range at which the A.I. will start attacking." },
					SightRange = { default = 18.0, description = "The range at which the A.I. will stop attacking." },
					DelayBetweenShots = { default = 0.2, description = "The delay between firing shots." },
					DelayBetweenShotsVariance = { default = 0.2, description = "The margin of error on the DelayBetweenShots." },
					
					FirstShot =
					{
						Delay = { default = 1.0, description = "The delay before firing the first shot." },
						Variance = { default = 0.2, description = "The margin of error on the first shot delay." },
					},
					
					Goldilocks =
					{
						MinRange = { default = 3.0, description = "The range at which the A.I. will run away from the player." },
						MaxRange = { default = 9.0, description = "The range at which the A.I. will chase the player." },
					},
					
					Juking =
					{
						JukeDelay = { default = 1.0, description = "The delay between jukes." },
						JukeDelayVariance = { default = 0.8, description = "The margin of error on the JukeDelay." },
						JukeDistance = { default = 2.5, description = "The distance the A.I. will juke." },
						JukeDistanceVariance = { default = 1.0, description = "The margin of error on the JukeDistance." },
					},
				},
				
				Tracking =
				{
					MoveSpeed = { default = 4.0 },
					
					DurationToTrack = { default = 6.0, description = "How long the A.I. will inspect a suspicious location (in seconds).", suffix = " s" },
				},
			},
		},
		SoundResponses =
		{
			SimilarRejectionAngle = { default = 30.0, description = "Similar sounds will be rejected if their angle from the current sound is less than this.", suffix = " deg" },
			SimilarRejectionDistance = { default = 6.0, description = "Similar sounds will be rejected if their distance from the current sound is less than this.", suffix = " m" },
			
			WhizzPast =
			{
				Initial = { default = 0.5, description = "Distance A.I. will walk towards the shot's origin if heard while Idle.", suffix = " m" },
				AggroRangeScalar = { default = 0.75, description = "Range to get within shot's origin if heard while Suspicious. 0.5 means half of the AggroRange value." },
			},
			
			Explosion =
			{
				Distance = { default = 5.0, description = "The closest A.I. will walk to an explosion's epicenter.", suffix = " m" },
			},
		},
		Events =
		{
			GotShot = { default = "GotShotEvent", description = "Indicates we were shot by something and will likely get hurt." },
			
			RequestSuspiciousLoc = { default = "RequestSuspiciousLocation", description = "Requests the A.I.'s current suspicious location." },
			SendingSuspiciousLoc = { default = "SendingSuspiciousLocation", description = "Sends the / Receives another A.I.'s suspicious location." },
		},
    },
    
	NoTurnIdleAngle = 0.78, -- The player won't play a turn idle animation if the move in a direction less than this angle from the facing direction
	
    InputValues = 
    {
        NavForwardBack = 0.0,
        NavLeftRight = 0.0,
    },
	
	--------------------------------------------------
	-- A.I. State Machine Definition
	-- States:
	--		Idle		- patrolling / sentry
	--		Suspicious	- detected an enemy but not seen yet
	--		Combat		- found a target and attacking them
	--		Tracking	- lost sight of target and trying to find them again
	--		Dead		- he's dead Jim
	--		
	-- Transitions:
	--		Idle		<-> Suspicious
	--					 -> Combat
	--					 -> Tracking
	--					 -> Dead
	--		Suspicious	<-> Idle
	--					 -> Combat
	--					 -> Tracking
	--					 -> Dead
	--		Combat		<-  Suspicious
	--					 -> Tracking
	--					 -> Dead
	--		Tracking	<-  Idle
	--					<-  Suspicious
	--					<-  Combat
	--					 -> Dead
	--		Dead		<-  Idle
	--					<-  Suspicious
	--					<-  Combat
	--					<-  Tracking
    --------------------------------------------------
	AIStates =
	{
		Idle =
		{
			OnEnter = function(self, sm)
				VisualizeAIStatesSystemRequestBus.Broadcast.SetAIState(sm.UserData.entityId, AIStates.Idle);
				
				sm.UserData:SetMoveSpeed(sm.UserData.Properties.StateMachines.AI.Idle.MoveSpeed);
				sm.UserData:TravelToCurrentWaypoint();
				
				-- Reset the suspicious location as we're not suspicious of anything.
				sm.UserData.suspiciousLocation = nil;
				
				sm.UserData.performingWaypointEvents = false;
				sm.UserData.currentWaypointEvent = WaypointEventSettings();
			end,
			
			OnExit = function(self, sm)
				sm.UserData:CancelNavigation();
				sm.UserData.soundHeard = nil;
			end,
			
			OnUpdate = function(self, sm, deltaTime)
				sm.UserData:UpdatePatrolling(deltaTime);
				sm.UserData.justEnabled = false;
				
				-- Reset this variable.
				if (sm.UserData.soundHeardJustNow ~= nil and sm.UserData.reactToRecentSound == false) then
					sm.UserData.soundHeardJustNow = nil;
				end
			end,

			Transitions =
			{
				-- Become suspicious if an enemy (i.e. the player) is nearby.
				Suspicious =
				{
					Evaluate = function(state, sm)
						return sm.UserData:IsEnemyClose();
					end
				},
				
				-- Enter combat if shot and didn't die (if it did enough damage to kill then
				-- we'd transition straight to the 'Dead' state).
				Combat =
				{
					Evaluate = function(state, sm)
						return sm.UserData:WantsToFight(sm.UserData.Properties.StateMachines.AI.Combat.AggroRange);
					end
				},
				
				Tracking =
				{
					Evaluate = function(state, sm)
						return sm.UserData:InformedOfDanger();
					end
				},
			},
		},
		
		Suspicious =
		{
			OnEnter = function(self, sm)
				VisualizeAIStatesSystemRequestBus.Broadcast.SetAIState(sm.UserData.entityId, AIStates.Suspicious);
				
				local speed = sm.UserData.Properties.StateMachines.AI.Suspicious.MoveSpeed;
				if (self.suspiciousFromBeingShot == true) then
					speed = sm.UserData.Properties.StateMachines.AI.Combat.MoveSpeed;
				end
				sm.UserData:SetMoveSpeed(speed);
				
				sm.UserData.justBecomeSuspicious = true;
				sm.UserData.timeBeforeSuspicionReset = sm.UserData.Properties.StateMachines.AI.Suspicious.DurationOfSuspicion;
				
				-- If entering this state because of a sound then react to it before we perform our
				-- update as it will likely change our suspicious location.
				sm.UserData.isFirstSuspiciousSound = true;
				if (sm.UserData.reactToRecentSound == true) then
					sm.UserData:SuspiciousReactionToSound();
				end
			end,
			
			OnExit = function(self, sm)
				-- If we are currently navigating somewhere then stop (the next state has no
				-- responsibility to control the navigation initiated by this state).
				sm.UserData:CancelNavigation();
				
				-- In case we were in the middle of a suspicious animation, cancel it so the aim IK
				-- doesn't go wrong.
				sm.UserData:PlayAnim("Idle", true);
				
				-- Reset this variable.
				sm.UserData.suspiciousFromBeingShot = false;
				sm.UserData.soundHeard = nil;
			end,
			
			OnUpdate = function(self, sm, deltaTime)
				sm.UserData:UpdateSuspicions(deltaTime);
				sm.UserData.justEnabled = false;
				
				-- Reset this variable.
				if (sm.UserData.soundHeardJustNow ~= nil and sm.UserData.reactToRecentSound == false) then
					sm.UserData.soundHeardJustNow = nil;
				end
			end,

			Transitions =
			{

				-- Return to 'Idle' (patrolling/sentry) if the enemy couldn't be found.
				Idle =
				{
					Evaluate = function(state, sm)
						return sm.UserData:IsNoLongerSuspicious();
					end
				},
				
				-- Enter combat if shot and didn't die (if it did enough damage to kill then
				-- we'd transition straight to the 'Dead' state) or found the enemy.
				Combat =
				{
					Evaluate = function(state, sm)
						local range = sm.UserData.Properties.StateMachines.AI.Combat.AggroRange;
						if (sm.UserData.suspiciousFromBeingShot == true) then
							range = sm.UserData.Properties.StateMachines.AI.Combat.SightRange;
						end
						
						return sm.UserData:WantsToFight(range);
					end
				},
				
				Tracking =
				{
					Evaluate = function(state, sm)
						return sm.UserData:InformedOfDanger();
					end
				},
			},
		},
		
		Combat =
		{
			OnEnter = function(self, sm, prevState)
				VisualizeAIStatesSystemRequestBus.Broadcast.SetAIState(sm.UserData.entityId, AIStates.Combat);
				
				if (prevState ~= "Tracking") then
					sm.UserData:AdjustPersistentDataCount(sm.UserData.AIStates.AwareOfPlayerCount, 1);
				end
				
				sm.UserData.isChasing = false;
				
				self.minimumCombatTimer = sm.UserData.Properties.StateMachines.AI.Combat.MinCombatTimer;
				sm.UserData:SetMoveSpeed(sm.UserData.Properties.StateMachines.AI.Combat.MoveSpeed);
				sm.UserData.timeUntilNextShot = sm.UserData:CalculateTimeUntilNextShot(true);
				sm.UserData.timeBeforeNextJuke = sm.UserData.Properties.StateMachines.AI.Combat.Juking.JukeDelay;
				sm.UserData.aimAtPlayer = true;
				
				-- If we entered combat due to being shot then immediately juke (i.e. panic).
				if (sm.UserData.suspiciousFromBeingShot) then
					sm.UserData.timeBeforeNextJuke = 0.0;
				end
				
				-- If entering this state because of a sound then react to it before we perform our
				-- update as it will likely change our suspicious location.
				if (sm.UserData.reactToRecentSound == true) then
					sm.UserData:CombatReactionToSound();
				end
				
				-- Broadcast a cry for help to nearby allies who should immediately respond (the
				-- AlertID broadcast comes later).
				GameplayNotificationBus.Event.OnEventBegin(sm.UserData.assistNearbyAllyEventId, sm.UserData.entityId);
			end,
			
			OnExit = function(self, sm, nextState)
				if (nextState ~= "Tracking") then
					sm.UserData:AdjustPersistentDataCount(sm.UserData.AIStates.AwareOfPlayerCount, -1);
				end
				
				sm.UserData.aimAtPlayer = false;
				-- If we are currently navigating somewhere then stop (the next state has no
				-- responsibility to control the navigation initiated by this state).
				sm.UserData:CancelNavigation();
				sm.UserData.isJuking = false;
				
				sm.UserData.suspiciousFromBeingShot = false;
				sm.UserData.soundHeard = nil;
			end,
			
			OnUpdate = function(self, sm, deltaTime)
				if (self.minimumCombatTimer > 0.0) then
					self.minimumCombatTimer = self.minimumCombatTimer - deltaTime;
				end
				
				sm.UserData:UpdateCombat(deltaTime);
				sm.UserData.justEnabled = false;
				
				-- Reset this variable.
				if (sm.UserData.soundHeardJustNow ~= nil and sm.UserData.reactToRecentSound == false) then
					sm.UserData.soundHeardJustNow = nil;
				end
			end,

			Transitions =
			{

				Idle =
				{
					Evaluate = function(state, sm)
						return sm.UserData:TargetIsDead();
					end
				},
				
				-- Start to track down the enemy (i.e. the player) if lost sight.
				Tracking =
				{
					Evaluate = function(state, sm)
						return state.minimumCombatTimer <= 0.0 and sm.UserData:HasLostSightOfTarget();
					end
				},
			},
		},
		
		Tracking =
		{
			OnEnter = function(self, sm, prevState)
				if (prevState ~= "Combat") then
					sm.UserData:AdjustPersistentDataCount(sm.UserData.AIStates.AwareOfPlayerCount, 1);
				end
				
				VisualizeAIStatesSystemRequestBus.Broadcast.SetAIState(sm.UserData.entityId, AIStates.Tracking);
				
				sm.UserData:SetMoveSpeed(sm.UserData.Properties.StateMachines.AI.Tracking.MoveSpeed);
				sm.UserData.justBecomeSuspicious = true;
				sm.UserData.timeBeforeSuspicionReset = sm.UserData.Properties.StateMachines.AI.Tracking.DurationToTrack;
				
				-- If entering this state because of a sound then react to it before we perform our
				-- update as it will likely change our suspicious location.
				sm.UserData.isFirstSuspiciousSound = true;
				if (self.reactToRecentSound == true) then
					self:SuspiciousReactionToSound();
				end
			end,
			
			OnExit = function(self, sm, nextState)
				if (nextState ~= "Combat") then
					sm.UserData:AdjustPersistentDataCount(sm.UserData.AIStates.AwareOfPlayerCount, -1);
				end
				
				-- If we are currently navigating somewhere then stop (the next state has no
				-- responsibility to control the navigation initiated by this state).
				sm.UserData:CancelNavigation();
				
				sm.UserData.soundHeard = nil;
			end,
			
			OnUpdate = function(self, sm, deltaTime)
				sm.UserData:UpdateSuspicions(deltaTime);
				sm.UserData.justEnabled = false;
				
				-- Reset this variable.
				if (sm.UserData.soundHeardJustNow ~= nil and sm.UserData.reactToRecentSound == false) then
					sm.UserData.soundHeardJustNow = nil;
				end
			end,

			Transitions =
			{

				-- Return to 'Idle' if the enemy couldn't be found.
				Idle =
				{
					Evaluate = function(state, sm)
						return sm.UserData:IsNoLongerSuspicious();
					end
				},
				
				-- Enter combat if shot and didn't die (if it did enough damage to kill then
				-- we'd transition straight to the 'Dead' state) or found the enemy.
				Combat =
				{
					Evaluate = function(state, sm)
						-- Need the '-1' to make sure the A.I. stops within the sight range distance.
						return sm.UserData:WantsToFight(sm.UserData.Properties.StateMachines.AI.Combat.SightRange - 1.0);
					end
				},
			},
		},
		
		Dead =
		{
			OnEnter = function(self, sm)
				VisualizeAIStatesSystemRequestBus.Broadcast.ClearAIState(sm.UserData.entityId);
				sm.UserData:CancelNavigation();
			end,
			
			OnExit = function(self, sm)
				
			end,
			
			OnUpdate = function(self, sm, deltaTime)
				sm.UserData.justEnabled = false;
			end,

			Transitions =
			{
				-- "Heroes never die"... shame you're not a hero.
			},
		},
			
		AwareOfPlayerCount = "AI_AwareOfPlayerCount",
	},
}

--------------------------------------------------
-- Component behavior
--------------------------------------------------

function aicontroller:OnActivate()

	-- Play the specified Audio Trigger (wwise event) on this component
	AudioTriggerComponentRequestBus.Event.Play(self.entityId);

	self.Properties.StateMachines.AI.Combat.AngleBeforeReCentering = Math.DegToRad(self.Properties.StateMachines.AI.Combat.AngleBeforeReCentering);

	-- Listen for which spawner created us.
	self.spawnedEventId = GameplayNotificationId(self.entityId, "AISpawned", "float");
	self.spawnedHandler = GameplayNotificationBus.Connect(self, self.spawnedEventId);
	self.startFightingEventId = GameplayNotificationId(self.entityId, "FightPlayer", "float");
	self.startFightingHandler = GameplayNotificationBus.Connect(self, self.startFightingEventId);
	self.setAlertIdEventId = GameplayNotificationId(self.entityId, "SetAlertId", "float");
	self.setAlertIdHandler = GameplayNotificationBus.Connect(self, self.setAlertIdEventId);
	self.alertIdEventId = nil;
	self.alertIdHandler = nil;
	self.broadcastAlertEventId = GameplayNotificationId(self.entityId, "BroadcastAlert", "float");
	self.broadcastAlertHandler = GameplayNotificationBus.Connect(self, self.broadcastAlertEventId);

	-- Enable and disable events
	self.enableEventId = GameplayNotificationId(self.entityId, "Enable", "float");
	self.enableHandler = GameplayNotificationBus.Connect(self, self.enableEventId);
	self.disableEventId = GameplayNotificationId(self.entityId, "Disable", "float");
	self.disableHandler = GameplayNotificationBus.Connect(self, self.disableEventId);
	
	self.weaponFirePressedEventId = GameplayNotificationId(self.entityId, "WeaponFirePressed", "float");
	self.weaponFireReleasedEventId = GameplayNotificationId(self.entityId, "WeaponFireReleased", "float");
	self.weaponFireSuccessEventId = GameplayNotificationId(self.entityId, "WeaponFireSuccess", "float");
	self.weaponFireSuccessHandler = GameplayNotificationBus.Connect(self, self.weaponFireSuccessEventId);
	self.weaponFireFailEventId = GameplayNotificationId(self.entityId, "WeaponFireFail", "float");
	self.weaponFireFailHandler = GameplayNotificationBus.Connect(self, self.weaponFireFailEventId);
	
	self.setAimDirectionId = GameplayNotificationId(self.entityId, "SetAimDirection", "float");
	self.setAimOriginId = GameplayNotificationId(self.entityId, "SetAimOrigin", "float");
	
	self.requestAimUpdateEventId = GameplayNotificationId(self.entityId, "RequestAimUpdate", "float");
	self.requestAimUpdateHandler = GameplayNotificationBus.Connect(self, self.requestAimUpdateEventId);
	
	self.turnStartedEventId = GameplayNotificationId(self.entityId, "EventTurnStarted", "float");
	self.turnStartedHandler = GameplayNotificationBus.Connect(self, self.turnStartedEventId);
	self.turnEndedEventId = GameplayNotificationId(self.entityId, "EventTurnEnded", "float");
	self.turnEndedHandler = GameplayNotificationBus.Connect(self, self.turnEndedEventId);
	self.setMovementDirectionId = GameplayNotificationId(self.entityId, "SetMovementDirection", "float");
	
	self.onDeathEventId = GameplayNotificationId(self.entityId, "HealthEmpty", "float");
	self.onDeathHandler = GameplayNotificationBus.Connect(self, self.onDeathEventId);
    
	-- Input listeners (events).
	self.gotShotEventId = GameplayNotificationId(self.entityId, self.Properties.Events.GotShot, "float");
	self.gotShotHandler = GameplayNotificationBus.Connect(self, self.gotShotEventId);
	self.heardSoundEventId = GameplayNotificationId(self.entityId, "HeardSound", "float");
	self.heardSoundHandler = GameplayNotificationBus.Connect(self, self.heardSoundEventId);
	self.assistNearbyAllyEventId = GameplayNotificationId(EntityId(), "AssistNearbyAlly", "float");
	self.assistNearbyAllyHandler = GameplayNotificationBus.Connect(self, self.assistNearbyAllyEventId);
	
	self.requestSuspiciousLocEventId = GameplayNotificationId(self.entityId, self.Properties.Events.RequestSuspiciousLoc, "float");
	self.requestSuspiciousLocHandler = GameplayNotificationBus.Connect(self, self.requestSuspiciousLocEventId);
	self.sendingSuspiciousLocEventId = GameplayNotificationId(self.entityId, self.Properties.Events.SendingSuspiciousLoc, "float");
	self.sendingSuspiciousLocHandler = GameplayNotificationBus.Connect(self, self.sendingSuspiciousLocEventId);
	
	AISoundManagerSystemRequestBus.Broadcast.RegisterEntity(self.entityId);
	self.Properties.SoundResponses.SimilarRejectionAngle = Math.DegToRad(self.Properties.SoundResponses.SimilarRejectionAngle);
	self.Properties.SoundResponses.SimilarRejectionDistanceSq = self.Properties.SoundResponses.SimilarRejectionDistance * self.Properties.SoundResponses.SimilarRejectionDistance;
	self.justGotShotBy = EntityId();
	self.suspiciousFromBeingShot = false;
	self.soundHeard = nil;
	self.soundHeardJustNow = nil;
	self.reactToRecentSound = false;
	self.isFirstSuspiciousSound = true;
	self.shouldAssistAlly = EntityId();
	self.alertedOfTarget = false;
	self.alertedLocation = nil;

	-- Tick needed to detect aim timeout
    self.tickBusHandler = TickBus.Connect(self);
	self.performedFirstUpdate = false;
	self.justEnabled = false;
	
	-- Need to store the previous Transform so we can keep track of the A.I.'s movement
	-- because we don't get any information directly from the navigation system.
	self.prevTm = TransformBus.Event.GetWorldTM(self.entityId);
	
	self:SetTurning(false, "activated");
	self.waitingForTurningConfirmation = false;
	self.framesSpentWaitingForTurningConfirmation = 0;
	self.justFinishedTurning = false;

	-- Create the A.I.'s state machine (but don't start it yet).
	self.AIStateMachine = {}
	setmetatable(self.AIStateMachine, StateMachine);
	
	-- Initialise the sentry values.
	self.sentry = false;
	self.lazySentry = false;
	self.timeUntilSentryTurns = self.Properties.StateMachines.AI.Idle.SentryTurnRate;
	--self.Properties.StateMachines.AI.Idle.SentryTurnAngle = Math.DegToRad(self.Properties.StateMachines.AI.Idle.SentryTurnAngle);
	
	self.performingWaypointEvents = false;
	self.currentWaypointEvent = WaypointEventSettings();
	self.setIdleAnimEventId = GameplayNotificationId(self.entityId, "SetIdleAnim", "float");
	
	-- Ensure the movement values are initialised.
	self.moveSpeed = 0.0;
	self:SetMovement(0.0, 0.0);
	
	self.aimDirection = Vector3(1.0,0.0,0.0);
	self.aimOrigin = Vector3(0.0,0.0,0.0);
	
	self.shouldLog = false;
	self.shouldLogEventId = GameplayNotificationId(self.entityId, "ShouldLog", "float");
	self.shouldLogHandler = GameplayNotificationBus.Connect(self, self.shouldLogEventId);
	self.aiSkeletonNS = "";
	if(StarterGameUtility.IsLegacyCharacter(self.entityId) == false) then
		self.aiSkeletonNS = utilities.EMFXNamespace;
	end
	self.playerId = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"));
	self.playerSkeletonNS = "";
	if(StarterGameUtility.IsLegacyCharacter(self.playerId) == false) then
		self.playerSkeletonNS = utilities.EMFXNamespace;
	end
end

function aicontroller:OnDeactivate()

    -- Terminate our state machine.
	self.AIStateMachine:Stop();
	
	AISoundManagerSystemRequestBus.Broadcast.UnregisterEntity(self.entityId);
	VisualizeAIStatesSystemRequestBus.Broadcast.ClearAIState(self.entityId);
	
	if (self.tickBusHandler ~= nil) then
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end
	
	if (self.navHandler ~= nil) then
		self.navHandler:Disconnect();
		self.navHandler = nil;
	end
	if (self.rsNavHandler ~= nil) then
		self.rsNavHandler:Disconnect();
		self.rsNavHandler = nil;
	end
	
	if (self.weaponFireSuccessHandler ~= nil) then
		self.weaponFireSuccessHandler:Disconnect();
		self.weaponFireSuccessHandler = nil;
	end
	if (self.weaponFireFailHandler ~= nil) then
		self.weaponFireFailHandler:Disconnect();
		self.weaponFireFailHandler = nil;
	end
	
	if (self.idleTurnStartedHandler ~= nil) then
		self.idleTurnStartedHandler:Disconnect();
		self.idleTurnStartedHandler = nil;
	end
	if (self.idleTurnEndedHandler ~= nil) then
		self.idleTurnEndedHandler:Disconnect();
		self.idleTurnEndedHandler = nil;
	end
	
	if (self.onDeathHandler ~= nil) then
		self.onDeathHandler:Disconnect();
		self.onDeathHandler = nil;
	end
	
	if (self.gotShotHandler ~= nil) then
		self.gotShotHandler:Disconnect();
		self.gotShotHandler = nil;
	end
	if (self.heardShotHandler ~= nil) then
		self.heardShotHandler:Disconnect();
		self.heardShotHandler = nil;
	end
	if (self.assistNearbyAllyHandler ~= nil) then
		self.assistNearbyAllyHandler:Disconnect();
		self.assistNearbyAllyHandler = nil;
	end
	
	if (self.requestSuspiciousLocHandler ~= nil) then
		self.requestSuspiciousLocHandler:Disconnect();
		self.requestSuspiciousLocHandler = nil;
	end
	if (self.sendingSuspiciousLocHandler ~= nil) then
		self.sendingSuspiciousLocHandler:Disconnect();
		self.sendingSuspiciousLocHandler = nil;
	end
	
	if (self.disableHandler ~= nil) then
		self.disableHandler:Disconnect();
		self.disableHandler = nil;	
	end
	if (self.enableHandler ~= nil) then
		self.enableHandler:Disconnect();
		self.enableHandler = nil;
	end
	
	if (self.broadcastAlertHandler ~= nil) then
		self.broadcastAlertHandler:Disconnect();
		self.broadcastAlertHandler = nil;
	end
	if (self.alertIdHandler ~= nil) then
		self.alertIdHandler:Disconnect();
		self.alertIdHandler = nil;
	end
	if (self.setAlertIdHandler ~= nil) then
		self.setAlertIdHandler:Disconnect();
		self.setAlertIdHandler = nil;
	end
	if (self.startFightingHandler ~= nil) then
		self.startFightingHandler:Disconnect();
		self.startFightingHandler = nil;
	end
	if (self.spawnedHandler ~= nil) then
		self.spawnedHandler:Disconnect();
		self.spawnedHandler = nil;
	end
	
	if (self.shouldLogHandler ~= nil) then
		self.shouldLogHandler:Disconnect();
		self.shouldLogHandler = nil;
	end

end

function aicontroller:IsMoving()
    return (self.InputValues.NavForwardBack ~= 0 or self.InputValues.NavLeftRight ~= 0);
end

-- Returns a Vector3 containing the stick input values in x and y
function aicontroller:GetInputVector()
	return Vector3(self.InputValues.NavForwardBack, self.InputValues.NavLeftRight, 0);
end

-- Returns the angle to turn in the requested direction and the length of the input vector
function aicontroller:GetAngleDelta(moveLocal)
    if (moveLocal:GetLengthSq() > 0.01) then
        local tm = TransformBus.Event.GetWorldTM(self.entityId);
        
        local moveMag = moveLocal:GetLength();
        if (moveMag > 1.0) then 
            moveMag = 1.0 
        end

		local desiredFacing = moveLocal:GetNormalized();
        local facing = tm:GetColumn(1):GetNormalized();
        local dot = facing:Dot(desiredFacing);
		local angleDelta = Math.ArcCos(dot);
		
		local side = desiredFacing:Dot(tm:GetColumn(0):GetNormalized());
        if (side > 0.0) then
            angleDelta = -angleDelta;
        end
		
		-- Guard against QNANs.
		if (utilities.IsNaN(angleDelta)) then
			angleDelta = 0.0;
		end
		
        return angleDelta, moveMag, desiredFacing;
	else
		return 0.0;
	end

end

function aicontroller:IsFacing(target)
	local angleDelta = self:GetAngleDelta(target);
	if(angleDelta < 0) then
		angleDelta = -angleDelta;
	end
	utilities.LogIf(self, "AngleDelta: " .. tostring(angleDelta) .. ", NoTurnIdleAngle: " .. tostring(self.NoTurnIdleAngle));
	return angleDelta < self.NoTurnIdleAngle;
end

function aicontroller:OnTick(deltaTime, timePoint)
	-- Initialise anything that requires other entities here because they might not exist
	-- yet in the 'OnActivate()' function.
	if (not self.performedFirstUpdate) then
		-- Initialise the navigation variables and start the A.I. state machine.
		self.navHandler = NavigationComponentNotificationBus.Connect(self, self.entityId);
		self.rsNavHandler = StarterGameNavigationComponentNotificationBus.Connect(self, self.entityId);
		self.arrivalDistanceThreshold = StarterGameAIUtility.GetArrivalDistanceThreshold(self.entityId);
		self.requestId = 0;
		self.navPath = nil;
		self.searchingForPath = false;
		self.reachedWaypoint = false;
		self.navCancelled = false;
		self.pathfindingFailed = false;
		self.AIStateMachine:Start("AI", self.entityId, self, self.AIStates, true, self.Properties.StateMachines.AI.InitialState, self.Properties.StateMachines.AI.DebugStateMachine);
		
		-- This is needed because otherwise the engine disables the A.I.'s physics when it's
		-- spawned because it thinks it's intersecting with something.
		-- This is an engine issue with components not initialising correctly.
		PhysicsComponentRequestBus.Event.EnablePhysics(self.entityId);
		self.prevTm = TransformBus.Event.GetWorldTM(self.entityId);
		
		-- Setup the variables for debugging the range visualisations.
		VisualizeRangeComponentRequestsBus.Event.SetSightRange(self.entityId, self.Properties.StateMachines.AI.Combat.AggroRange, self.Properties.StateMachines.AI.Combat.SightRange);
		VisualizeRangeComponentRequestsBus.Event.SetSuspicionRange(self.entityId, self.Properties.StateMachines.AI.Suspicious.SuspicionRange, self.Properties.StateMachines.AI.Suspicious.AIFieldOfView, self.Properties.StateMachines.AI.Suspicious.SuspicionRangeRear);
		
		-- Make sure we don't do this 'first update' again.
		self.performedFirstUpdate = true;
	end
	
	-- We need to hear back from the MovementController within a couple of frames indicating whether
	-- or not we've actually started turning. Otherwise, we haven't and we need to go back to doing
	-- whatever we were previously doing.
	if (self.waitingForTurningConfirmation) then
		self.framesSpentWaitingForTurningConfirmation = self.framesSpentWaitingForTurningConfirmation + 1;
		if (self.framesSpentWaitingForTurningConfirmation >= 2) then
			self:SetTurning(false, "no confirmation came through");
			self.waitingForTurningConfirmation = false;
			
			-- We technically didn't turn, but this kicks all the states out of their "wait for the
			-- turn" cases.
			self.justFinishedTurning = true;
		end
	end
	
	-- Determine how much the A.I. has moved in the last frame and in which direction.
	self:CalculateCurrentMovement(deltaTime);
	self:UpdateMovement();
	self:UpdateWeaponAim();
	self:SetStrafeFacing();
end

function aicontroller:CalculateCurrentMovement(deltaTime)

	local tm = TransformBus.Event.GetWorldTM(self.entityId);
	if (self:IsNavigating() and not self.searchingForPath) then
		local validMovement = false;
		while (not self.reachedWaypoint and not validMovement) do
			-- This inner 'while' is to emulate a 'continue' because Lua doesn't have one.
			while (true) do
				local pathPoint = self.navPath:GetCurrentPoint();
				local dir = pathPoint - tm:GetTranslation();
				dir.z = 0.0;
				local distToPoint = dir:GetLengthSq();
				if (distToPoint < self.arrivalDistanceThreshold) then
					-- If we're already there then move on to the next point and break out of the
					-- inner loop so we restart the process of determining which direction we need
					-- to travel.
					-- If it happens to have been the last point then we'll exit out of both loops.
					if (self.navPath:IsLastPoint()) then
						self:OnTraversalComplete(self.requestId);
					else
						self.navPath:ProgressToNextPoint();
					end
					break;
				end
				
				-- Set the direction we want to move in.
				dir = dir:GetNormalized();
				self:SetMovement(dir.x, dir.y);
				validMovement = true;
				
				-- Check if we'll reach our destination within a frame.
				local moveLocal = self:GetInputVector();
				local moveDistNextFrame = moveLocal:GetLengthSq() * deltaTime;
				if (moveDistNextFrame >= distToPoint) then
					-- If that was the last point then complete pathing.
					if (self.navPath:IsLastPoint()) then
						self:OnTraversalComplete(self.requestId);
					else
						-- If it wasn't then we need to start moving to the next point.
						self.navPath:ProgressToNextPoint();
						
						pathPoint, dir = self:GetPathPointAndDir(tm);
						dir = dir:GetNormalized();
						self:SetMovement(dir.x, dir.y);
					end
				end
				break;
			end
		end
	end
	self.prevTm = tm;
	
end

function aicontroller:GetPathPointAndDir(tm)
	local pathPoint = self.navPath:GetCurrentPoint();
	local dir = pathPoint - tm:GetTranslation();
	dir.z = 0.0;
	
	return pathPoint, dir;
end

function aicontroller:UpdateMovement()
	local moveLocal = self:GetInputVector();
	local movementDirection = Vector3(0,0,0);
	if (moveLocal:GetLengthSq() > 0.01) then
		movementDirection = moveLocal:GetNormalized();
	end
	local moveMag = moveLocal:GetLength();
	if (moveMag > 1.0) then
		moveMag = 1.0;
	end
	movementDirection = movementDirection * moveMag;
	GameplayNotificationBus.Event.OnEventBegin(self.setMovementDirectionId, movementDirection);
end

function aicontroller:UpdateWeaponAim()
	-- Get the position we're aiming at (i.e. the player).
	local playerId = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"));
	if (not playerId:IsValid()) then
		return;
	end
	local pos = StarterGameUtility.GetJointWorldTM(playerId, self.playerSkeletonNS..utilities.Joint_Torso_Upper):GetTranslation();
	-- Get the position we're aiming from (i.e. the A.I.).
	local aiPos = StarterGameUtility.GetJointWorldTM(self.entityId, self.aiSkeletonNS..utilities.Joint_Torso_Upper):GetTranslation();
	self.aimDirection = (pos - aiPos):GetNormalized();
	self.aimOrigin = aiPos + (self.aimDirection * 0.5);
end

function aicontroller:IsDead()
	return self.StateMachine.CurrentState == self.States.Dead;
end

function aicontroller:WeaponFired(value)
	GameplayNotificationBus.Event.OnEventBegin(self.weaponFireReleasedEventId, 0.0);
end

function aicontroller:OnEventBegin(value)
	
	if (GameplayNotificationBus.GetCurrentBusId() == self.spawnedEventId) then
		-- Copy the waypoints, that the A.I. should navigate around, from the spawner.
		WaypointsComponentRequestsBus.Event.CloneWaypoints(self.entityId, value);
		self.sentry = WaypointsComponentRequestsBus.Event.IsSentry(self.entityId);
		self.lazySentry = WaypointsComponentRequestsBus.Event.IsLazySentry(self.entityId);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.startFightingEventId) then
		-- Only if the debug manager says so...
		if (utilities.GetDebugManagerBool("EnableAICombat", true)) then
			-- We know that the A.I. only target the player.
			local playerId = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"));
			if (playerId:IsValid()) then
				self.suspiciousLocation = TransformBus.Event.GetWorldTM(playerId):GetTranslation();
				self.Properties.StateMachines.AI.InitialState = "Combat";
			end
		end
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.setAlertIdEventId) then
		if (value ~= nil and value ~= "") then
			self.alertIdEventId = GameplayNotificationId(EntityId(), value, "float");
			self.alertIdHandler = GameplayNotificationBus.Connect(self, self.alertIdEventId);
		end
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.broadcastAlertEventId) then
		-- Send an alert to all allies on the alert system.
		if (self.alertIdEventId ~= nil) then
			--Debug.Log(tostring(self.entityId) .. "BROADCASTING ALERT!");
			-- If our suspicious location is nil then it means we went from out-of-combat to below
			-- the broadcast alert threshold in one hit. As a result, we should use the
			-- 'justGotShotBy' entity's position because it's likely that that was the reason why
			-- we got hurt. If that's also nil then just take the position of the player.
			local location = self.suspiciousLocation;
			if (location == nil) then
				if (self.justGotShotBy:IsValid()) then
					location = TransformBus.Event.GetWorldTM(self.justGotShotBy):GetTranslation();
				else
					local playerId = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"));
					if (playerId:IsValid()) then
						location = TransformBus.Event.GetWorldTM(playerId):GetTranslation();
					else
						-- Absolute fall-back (should only occur in test levels where the player
						-- doesn't exit.
						location = TransformBus.Event.GetWorldTM(self.entityId):GetTranslation();
					end
				end
			end
			GameplayNotificationBus.Event.OnEventBegin(self.alertIdEventId, location);
		end
	end
	
	if (GameplayNotificationBus.GetCurrentBusId() == self.getHealthEventId) then
		self:Heal(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.loseHealthEventId) then
		self:Injure(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.gotShotEventId) then
		-- React to being hit (if it wasn't done by themself).
		if (self.entityId ~= value.assailant) then
			self.justGotShotBy = value.assailant;
		end
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.heardSoundEventId) then
		if (self:SoundIsCloseEnoughToHear(value)) then
			local acceptNewSound = true;
			if (self.soundHeardJustNow ~= nil) then
				-- This means we've heard two new sounds without being able to analyse the previous.
				-- To resolve this, we'll just do a trivial reject to determine the most important
				-- of the two sounds.
				if (self:NewSoundIsLessImportant(self.soundHeardJustNow, value)) then
					acceptNewSound = false;
				end
			end
			if (acceptNewSound) then
				self.soundHeardJustNow = SoundProperties.Clone(value);
			end
		end
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.assistNearbyAllyEventId) then
		-- Only assist if it's not ourself.
		if (self.entityId ~= value) then
			local pos = TransformBus.Event.GetWorldTM(self.entityId):GetTranslation();
			local ally = TransformBus.Event.GetWorldTM(value):GetTranslation();
			local range = self.Properties.StateMachines.AI.Combat.AssistRange * self.Properties.StateMachines.AI.Combat.AssistRange;
			if ((pos - ally):GetLengthSq() <= range) then
				self.shouldAssistAlly = value;
			end
		end
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.alertIdEventId) then
		-- If this message is from itself then it should be ignored because we're already in combat.
		self.alertedOfTarget = true;
		self.alertedLocation = value;
	end
	
	if (GameplayNotificationBus.GetCurrentBusId() == self.enableEventId) then
		self:OnEnable();
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.disableEventId) then
		self:OnDisable();
	end
	
	if (GameplayNotificationBus.GetCurrentBusId() == self.requestSuspiciousLocEventId) then
		GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(value, self.Properties.Events.SendingSuspiciousLoc, "float"), self.suspiciousLocation);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.sendingSuspiciousLocEventId) then
		self.suspiciousLocation = value;
	end
	
	if (GameplayNotificationBus.GetCurrentBusId() == self.weaponFireSuccessEventId or GameplayNotificationBus.GetCurrentBusId() == self.weaponFireFailEventId) then
		self:WeaponFired(value);
    end
	
	if (GameplayNotificationBus.GetCurrentBusId() == self.turnStartedEventId) then
		self:SetTurning(true, "animation event");
		self.waitingForTurningConfirmation = false;
		self.justFinishedTurning = false;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.turnEndedEventId) then
		self:SetTurning(false, "animation event");
		self.justFinishedTurning = true;
	end
	
	if (GameplayNotificationBus.GetCurrentBusId() == self.onDeathEventId) then
		self.AIStateMachine:GotoState("Dead");
	end
	
	if (GameplayNotificationBus.GetCurrentBusId() == self.requestAimUpdateEventId) then
		GameplayNotificationBus.Event.OnEventBegin(self.setAimDirectionId, self.aimDirection);
		GameplayNotificationBus.Event.OnEventBegin(self.setAimOriginId, self.aimOrigin);
	end
	
	if (GameplayNotificationBus.GetCurrentBusId() == self.shouldLogEventId) then
		self.shouldLog = not self.shouldLog;
		self.AIStateMachine.IsDebuggingEnabled = self.shouldLog;
		if (self.shouldLog) then
			Debug.Log(tostring(self.entityId) .. " started logging.");
		else
			Debug.Log(tostring(self.entityId) .. " stopped logging.");
		end
	end
	
end

function aicontroller:OnEnable()
	self.tickBusHandler = TickBus.Connect(self);
	self.AIStateMachine:Resume();
	if (self.AIStateMachine.CurrentStateName == "Combat" or self.AIStateMachine.CurrentStateName == "Tracking") then
		self:AdjustPersistentDataCount(self.AIStates.AwareOfPlayerCount, 1);
	end
	AISoundManagerSystemRequestBus.Broadcast.RegisterEntity(self.entityId);
	self.justEnabled = true;
end

function aicontroller:OnDisable()
	AISoundManagerSystemRequestBus.Broadcast.UnregisterEntity(self.entityId);
	if (self.AIStateMachine.CurrentStateName == "Combat" or self.AIStateMachine.CurrentStateName == "Tracking") then
		self:AdjustPersistentDataCount(self.AIStates.AwareOfPlayerCount, -1);
	end
	self.AIStateMachine:Stop();
	self.tickBusHandler:Disconnect();
end

function aicontroller:AdjustPersistentDataCount(key, adjustment)
	local value = 0;
	if (PersistentDataSystemRequestBus.Broadcast.HasData(key)) then
		value = PersistentDataSystemRequestBus.Broadcast.GetData(key);
		-- Currently the manipulator will create a 'nil' data element when it's activated
		-- so we have to check for that.
		if (value == nil) then
			value = 0;
		end
	end
	value = value + adjustment;
	local res = PersistentDataSystemRequestBus.Broadcast.SetData(key, value, true);
	--Debug.Log("Set " .. tostring(key) .. " to " .. tostring(value) .. "? " .. tostring(res));
end

--------------------------------------------------------
--------------------------------------------------------

function aicontroller:UpdatePatrolling(deltaTime)
	-- Start performing the waypoint events, if there are any.
	if (self.reachedWaypoint == true and not self.performingWaypointEvents) then
		if (StarterGameEntityUtility.EntityHasComponent(self.currentWaypoint, "WaypointSettingsComponent")) then
			if (WaypointSettingsComponentRequestsBus.Event.HasEvents(self.currentWaypoint, self.entityId)) then
				self.currentWaypointEvent = WaypointSettingsComponentRequestsBus.Event.GetNextEvent(self.currentWaypoint, self.entityId);
				if (self.currentWaypointEvent:IsValid()) then
					self.performingWaypointEvents = true;
				end
			end
		end
	end
	
	if (self.performingWaypointEvents) then
		local completedEvent = false;
		--Debug.Log("WaypointEvent: Type: " .. tostring(self.currentWaypointEvent.type));
		if (self.currentWaypointEvent.type == WaypointEventType.Wait) then
			self.currentWaypointEvent.duration = self.currentWaypointEvent.duration - deltaTime;
			if (self.currentWaypointEvent.duration <= 0.0) then
				completedEvent = true;
			end
		elseif (self.currentWaypointEvent.type == WaypointEventType.TurnToFace) then
			if (self.justFinishedTurning) then
				self:SetMovement(0.0, 0.0);
				self.justFinishedTurning = false;
				completedEvent = true;
				--Debug.Log("WaypointEvent: TurnToFace completed");
			elseif (not self.isTurningToFace) then
				local pos = TransformBus.Event.GetWorldTM(self.entityId):GetTranslation() + self.currentWaypointEvent.directionToFace;
				if (self.currentWaypointEvent.turnTowardsEntity) then
					pos = TransformBus.Event.GetWorldTM(self.currentWaypointEvent.entityToFace):GetTranslation();
				end
				self:TurnTowardsLocation(pos);
				
				-- If we still haven't started turning then it means we're already facing the
				-- correct direction.
				if (not self.isTurningToFace) then
					completedEvent = true;
				end
				--Debug.Log("WaypointEvent: TurnToFace started: " .. tostring(completedEvent));
			end
		elseif (self.currentWaypointEvent.type == WaypointEventType.PlayAnim) then
			self:PlayAnim(self.currentWaypointEvent.animName, true);
			completedEvent = true;
		else
			-- If this case is hit then it means we don't know how to react to the given event type.
			completedEvent = true;
		end
		
		-- If the event is completed then acquire the next one.
		if (completedEvent) then
			-- If that was the last event then exit the waypoints events state.
			self.currentWaypointEvent = WaypointSettingsComponentRequestsBus.Event.GetNextEvent(self.currentWaypoint, self.entityId);
			if (not self.currentWaypointEvent:IsValid()) then
				self.performingWaypointEvents = false;
			end
		end
	end
	
	-- This must be a separate check because the value of 'self.performingWaypointEvents' can be
	-- changed above.
	if (not self.performingWaypointEvents) then
		if (self.sentry == true or self.lazySentry == true) then
			-- If we're turning then reset the movement values so we don't start moving
			-- in that direction once we've finished turning.
			if (self.isTurningToFace == true) then
				self:SetMovement(0.0, 0.0);
			end
			
			-- If the A.I. is a sentry then stand in one place and occasionally turn.
			-- If we're not already navigating then check we're a reasonable distance
			-- from our sentry position.
			if (not self:IsNavigating()) then
				local pos = TransformBus.Event.GetWorldTM(self.entityId):GetTranslation();
				local wpPos = TransformBus.Event.GetWorldTM(self.currentWaypoint):GetTranslation();
				local distSq = (wpPos - pos):GetLengthSq();
				local threshold = self.Properties.StateMachines.AI.Idle.SentryArrivalThreshold * self.Properties.StateMachines.AI.Idle.SentryArrivalThreshold;
				if (distSq > threshold) then
					self:TravelToCurrentWaypoint();
				end
			end
			
			if (self.sentry == true) then
				-- Turn every so often.
				-- We need to check again if we're navigating as the previous 'if' may have
				-- started navigation.
				if (not self:IsNavigating() and not self.isTurningToFace) then
					if (self.reachedWaypoint == true) then
						self.timeUntilSentryTurns = self.Properties.StateMachines.AI.Idle.SentryTurnRate;
						self.reachedWaypoint = false;
					end
					
					self.timeUntilSentryTurns = self.timeUntilSentryTurns - deltaTime;
					if (self.timeUntilSentryTurns <= 0.0) then
						local tm = TransformBus.Event.GetWorldTM(self.entityId);
						local forward = tm:GetColumn(1):GetNormalized() * Transform.CreateRotationZ(self.NoTurnIdleAngle + 0.01);
						--self:TravelToNavMarker(tm:GetTranslation() + forward);
						forward.z = 0.0;
						forward = forward:GetNormalized();
						self:SetMovement(forward.x, forward.y);
						
						self.timeUntilSentryTurns = self.Properties.StateMachines.AI.Idle.SentryTurnRate;
					end
				end
			end
		else
			-- If not a sentry then patrol through the given waypoints.
			if (self.justFinishedTurning == true or self.justEnabled == true) then
				self:TravelToCurrentWaypoint();
				self.justFinishedTurning = false;
			elseif (self.reachedWaypoint == true) then
				self:TravelToNextWaypoint();
			elseif (self.pathfindingFailed == true and not self.isTurningToFace) then
				-- This case covers the possibility that the navigation system failed to find a
				-- path to the desired waypoint.
				-- It seems the navigation system can occasionally respond with "no path found" yet
				-- succeed when told to try again. As a result, it's difficult to implement a "remove
				-- bad waypoints" system when the waypoints may actually be good but something goes
				-- goes awry in the pathfinding system.
				-- As a result, we'll just try to travel to the same waypoint again.
				self:TravelToCurrentWaypoint();
			end
		end
	end
end

function aicontroller:PlayAnim(name, looping)
	local params = PlayAnimParams();
	params.animName = name;
	params.loop = looping;
	GameplayNotificationBus.Event.OnEventBegin(self.setIdleAnimEventId, params);
end

function aicontroller:CancelNavigation()
	if (self.requestId ~= 0) then
		-- The 'Stop()' function is processed immediately but I want 'self.requestId'
		-- to be 0 so I know that I'm deliberately stopping the navigation: hence
		-- the temporary variable.
		local id = self.requestId;
		self.requestId = 0;
		self:OnTraversalCancelled(id);
		NavigationComponentRequestBus.Event.Stop(self.entityId, id);
	end
end

function aicontroller:TargetIsDead()
	-- For now, assume the player is invincible.
	return false;
end

function aicontroller:IsEnemyClose()
	-- If the debug manager says no...
	if (not utilities.GetDebugManagerBool("EnableAICombat", true)) then
		return false;
	end
	
	-- Check if the player is nearby.
	-- We know that the A.I. only target the enemy so I can do this in Lua, but
	-- if we wanted them to attack other things as well (wildlife, practice targets,
	-- other A.I.) then we'd need to do the check in C++ to get the whole list of
	-- nearby enemies.
	local isSuspicious = false;
	local playerId = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"));
	if (not playerId:IsValid()) then
		return false;
	end
	local playerPos = StarterGameUtility.GetJointWorldTM(playerId, self.playerSkeletonNS..utilities.Joint_Torso_Upper):GetTranslation();
	local aiTM = StarterGameUtility.GetJointWorldTM(self.entityId, self.aiSkeletonNS..utilities.Joint_Head);
	local aiPos = aiTM:GetTranslation();
	local aiToPlayer = playerPos - aiPos;
	local distSq = aiToPlayer:GetLengthSq();
	
	-- If the player is close enough to be detected behind then immediately become suspicious.
	if (distSq <= utilities.Sqr(self.Properties.StateMachines.AI.Suspicious.SuspicionRangeRear)) then
		isSuspicious = true;
	-- Otherwise, check if the player is within the broad suspicion range.
	elseif (distSq <= utilities.Sqr(self.Properties.StateMachines.AI.Suspicious.SuspicionRange)) then
		-- The player is nearby. Now check if they're in front of the A.I.
		local aiForward = aiTM:GetColumn(1);
		aiForward.z = 0.0;
		aiForward:Normalize();
		local aiToPlayerFlat = aiToPlayer;
		aiToPlayerFlat.z = 0.0;
		aiToPlayerFlat:Normalize();
		local dot = aiForward:Dot(aiToPlayerFlat);
		local angle = Math.RadToDeg(Math.ArcCos(dot));
		
		-- If the player is in front of the A.I. then be suspicious of them.
		if (angle < (self.Properties.StateMachines.AI.Suspicious.AIFieldOfView * 0.5)) then
			if (self:HasLoSOfPlayer(self.Properties.StateMachines.AI.Suspicious.SuspicionRange)) then
				isSuspicious = true;
			end
		end
	end
	
	-- If the A.I. is suspicious then record the location for the 'Suspicious' state.
	if (isSuspicious) then
		self.suspiciousLocation = playerPos;
	end
	
	-- If the A.I. isn't directly suspicious of the player then check if we heard anything
	-- suspicious (this is done afterwards because it has a lower priority).
	if (not isSuspicious and self.soundHeardJustNow ~= nil) then
		if (self:NewSoundIsSuspicious()) then
			self.reactToRecentSound = true;
			isSuspicious = true;
		end
	end
	
	return isSuspicious;
end

function aicontroller:HasLoSOfPlayer(range)
	local hasLoS = false;
	local playerId = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"));
	local playerPos = StarterGameUtility.GetJointWorldTM(playerId, self.playerSkeletonNS..utilities.Joint_Torso_Upper):GetTranslation();
	local aiPos = StarterGameUtility.GetJointWorldTM(self.entityId, self.aiSkeletonNS..utilities.Joint_Head):GetTranslation();
	local aiToPlayer = playerPos - aiPos;
	local mask = PhysicalEntityTypes.Static + PhysicalEntityTypes.Dynamic + PhysicalEntityTypes.Living + PhysicalEntityTypes.Terrain;
	local dir = aiToPlayer:GetNormalized();
	
	-- TODO: Remove the "+ (dir * 2.0)" from the equation. This has been added because
	-- otherwise the raycast will hit the A.I. before it reaches the target.
	local rayCastConfig = RayCastConfiguration();
	rayCastConfig.origin = aiPos + (dir * 2.0);
	rayCastConfig.direction = dir;
	rayCastConfig.maxDistance = range;
	rayCastConfig.maxHits = 1;
	rayCastConfig.physicalEntityTypes = mask;
	rayCastConfig.piercesSurfacesGreaterThan = 13;
	local hits = PhysicsSystemRequestBus.Broadcast.RayCast(rayCastConfig);
	if (hits:HasBlockingHit()) then
		local hitId = hits:GetBlockingHit().entityId;
		-- Make sure that it was the player the raycast hit...
		if (hitId == playerId) then
			-- ... if it was then the A.I. has L.o.S. of the player.
			hasLoS = true;
		end
	end
	
	return hasLoS;
end

function aicontroller:UpdateSuspicions(deltaTime)
	-- The transitions are checked BEFORE the update is performed, so as long as
	-- 'self.reactToRecentSound' is false we can interpret any incoming sounds here without
	-- fear of stepping on the transitions' toes.
	if (self.reactToRecentSound == false and self.soundHeardJustNow ~= nil) then
		if (self:NewSoundIsSuspicious()) then
			self:CancelNavigation();
			self:SuspiciousReactionToSound();
			
			self.isFirstSuspiciousSound = false;
			
			-- This is ill-advised but it means I don't have to duplicate the 'turning and
			-- navigating' code. It should also get reset within the if statement below so there's
			-- a very small window for anything to go wrong.
			self.justBecomeSuspicious = true;
		end
	end
	
	if (self.justBecomeSuspicious == true) then
		self:TurnTowardsLocation(self.suspiciousLocation);
		
		-- Perhaps move this to be later in the state so the A.I. will look at
		-- the location for a second or two before moving towards it.
		if (self.isTurningToFace == false) then
			self:TravelToNavMarker(self.suspiciousLocation);
		end
		
		self.justBecomeSuspicious = false;
	end
	
	-- If we just finished turning then we need to now start moving towards the location.
	utilities.LogIf(self, "Suspicious: Updating - " .. tostring(self:GetInputVector()));
	if (self.justFinishedTurning == true or self.justEnabled == true) then
		utilities.LogIf(self, "Suspicious: finished turning");
		self:TravelToNavMarker(self.suspiciousLocation);
		self.justFinishedTurning = false;
	elseif (not self:IsNavigating()) then
		utilities.LogIf(self, "Suspicious: not navigating");
		if (self.isTurningToFace == true) then
			utilities.LogIf(self, "Suspicious: turning");
			-- We're not moving because we're currently turning to look at the suspicious
			-- location.
		elseif (self.navCancelled) then
			utilities.LogIf(self, "Suspicious: navigation cancelled");
			-- If we can't navigate to the suspicious location, then choose a point mid-way
			-- between the suspicious location and the A.I.'s position.
			local pos = TransformBus.Event.GetWorldTM(self.entityId):GetTranslation();
			local markerPos = TransformBus.Event.GetWorldTM(self.Properties.NavMarker):GetTranslation();
			local vec = markerPos - pos;
			local halfDist = vec:GetLength() * 0.5;
			
			-- Only move the marker if the distance is reasonble.
			if (halfDist >= 2.0) then
				local newPos = pos + (vec:GetNormalized() * halfDist);
				self:TravelToNavMarker(newPos);
			end
			
			-- Reset the cancellation signal.
			self.navCancelled = false;
		elseif (self.reachedWaypoint == true) then
			utilities.LogIf(self, "Suspicious: reached waypoint");
			-- Play a 'looking around' animation.
			if (self.timeBeforeSuspicionReset >= self.Properties.StateMachines.AI.Suspicious.DurationOfSuspicion) then
				self:PlayAnim("IdleAmbientLookSuspicious", false);
			end
			
			-- If the A.I. has reached the waypoint and is still in the suspicion state
			-- then it means it hasn't come close enough to the target to enter combat.
			-- Therefore we can now count down before we decide to return to patrolling.
			self.timeBeforeSuspicionReset = self.timeBeforeSuspicionReset - deltaTime;
		else
			utilities.LogIf(self, "Suspicious: no idea");
			-- I'm not sure what it means if the A.I. enters this case, but in this state
			-- the A.I. should always be attempting to travel to a suspicion location
			-- marked by the nav marker). If the A.I. isn't navigating (first 'if') and
			-- the navigation hasn't been cancelled or completed then they've stopped
			-- navigating for an undetected reason; so just tell them to start navigating
			-- again.
			self:TravelToNavMarker(self.suspiciousLocation);
		end
	else
		-- Don't count down until the A.I. reaches the suspicious location.
		self.timeBeforeSuspicionReset = self.Properties.StateMachines.AI.Suspicious.DurationOfSuspicion;
	end
	
end

function aicontroller:TurnTowardsLocation(location)
	local a = location;
	local b = TransformBus.Event.GetWorldTM(self.entityId):GetTranslation();
	if (a == nil) then
		Debug.Log("Suspicious Location is nil");
	end
	if (b == nil) then
		Debug.Log("Position is nil");
	end
	local dir = (a - b):GetNormalized();
	utilities.LogIf(self, "is moving? " .. tostring(self:GetInputVector()));
	if (not self:IsFacing(dir)) then
		self:SetTurning(true, "turning towards suspicious location");
		self:SetMovement(dir.x, dir.y);
	else
		self:SetTurning(false, "already facing suspicious location");
	end
end

function aicontroller:IsNoLongerSuspicious()
	return self.timeBeforeSuspicionReset <= 0.0;
end

function aicontroller:WantsToFight(range)
	-- If the debug manager says no...
	if (not utilities.GetDebugManagerBool("EnableAICombat", true)) then
		return false;
	end
	
	-- If the A.I. has just been shot then we immediately want to enter combat.
	if (self.justGotShotBy:IsValid()) then
		-- We also want to investigate.
		self.suspiciousLocation = TransformBus.Event.GetWorldTM(self.justGotShotBy):GetTranslation();
		
		self.justGotShotBy = EntityId();
		self.suspiciousFromBeingShot = true;
		return true;
	end
	
	-- If a nearby ally has just called for help then assist them.
	if (self.shouldAssistAlly:IsValid()) then
		-- Get the ally's suspicious location.
		GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(self.shouldAssistAlly, self.Properties.Events.RequestSuspiciousLoc, "float"), self.entityId);
		
		self.shouldAssistAlly = EntityId();
		return true;
	end
	
	-- Check if an enemy is nearby and in view.
	-- We know that the A.I. only target the player so I can specifically just get the
	-- player's entity ID and perform checks against that but if we wanted the A.I. to
	-- attack other things as well (wildlife, practice targets, other A.I.) then we'd
	-- need a more rebust system; and possible have it done in C++ rather than Lua.
	local wantsToFight = false;
	local playerId = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"));
	if (not playerId:IsValid()) then
		return false;
	end
	local playerPos = StarterGameUtility.GetJointWorldTM(playerId, self.playerSkeletonNS..utilities.Joint_Torso_Upper):GetTranslation();
	local aiTM = StarterGameUtility.GetJointWorldTM(self.entityId, self.aiSkeletonNS..utilities.Joint_Head);
	local aiPos = aiTM:GetTranslation();
	local aiToPlayer = playerPos - aiPos;
	local distSq = aiToPlayer:GetLengthSq();
	-- If the player is in range then check if they're also in front of the A.I.
	if (distSq <= utilities.Sqr(range)) then
		local aiForward = aiTM:GetColumn(1);
		aiForward.z = 0.0;
		aiForward:Normalize();
		aiToPlayer.z = 0.0;
		aiToPlayer:Normalize();
		local dot = aiForward:Dot(aiToPlayer);
		local angle = Math.RadToDeg(Math.ArcCos(dot));
		
		-- If the player is in front of the A.I. and in aggro range then start attacking.
		if (angle < (self.Properties.StateMachines.AI.Suspicious.AIFieldOfView * 0.5)) then
			if (self:HasLoSOfPlayer(range)) then
				self.suspiciousLocation = playerPos;
				wantsToFight = true;
			end
		end
	end
	
	-- If the A.I. doesn't get directly pulled into combat by the player then check if we heard
	-- anything (this is done afterwards because it has a lower priority).
	if (not wantsToFight and self.soundHeardJustNow ~= nil) then
		if (self:NewSoundShouldBeFought()) then
			self.reactToRecentSound = true;
			wantsToFight = true;
		end
	end
	
	return wantsToFight;
end

function aicontroller:InformedOfDanger()
	-- If an ally called for help on the alert system then help them out.
	if (self.alertedOfTarget) then
		self.alertedOfTarget = false;
		self.suspiciousLocation = self.alertedLocation;
		self:TravelToNavMarker(self.suspiciousLocation);
		return true;
	end
	
	return false;
end

function aicontroller:UpdateCombat(deltaTime)
	-- Make sure this is reset in case we exit combat and get immediately re-alerted because it's
	-- not been ignored.
	self.alertedOfTarget = false;
	self.shouldAssistAlly = EntityId();
	
	-- Update the shooting.
	self.timeUntilNextShot = self.timeUntilNextShot - deltaTime;
	if (self:HasLoSOfPlayer(self.Properties.StateMachines.AI.Combat.SightRange)) then
		if (self.timeUntilNextShot <= 0.0) then
			GameplayNotificationBus.Event.OnEventBegin(self.weaponFirePressedEventId, 100);	-- unlimited energy!
			
			self.timeUntilNextShot = self:CalculateTimeUntilNextShot(false);
		end
	end
	
	-- If we just finished turning then start moving towards the nav marker.
	-- The nav marker should ALWAYS be what we're moving towards in this state.
	if (self.justFinishedTurning == true) then
		self:StartNavigation(self.Properties.NavMarker);
		self.justFinishedTurning = false;
	end
	
	local closeEnoughToJuke = self:UpdateGoldilocksPositioning(deltaTime);
	-- Change movement speed if finished chasing.
	if (not self:IsNavigating() and not self.isTurningToFace and self.isChasing) then
		self.isChasing = false;
		self:SetMoveSpeed(self.Properties.StateMachines.AI.Combat.MoveSpeed);
	end
	
	-- If we're already inside the Goldilocks range then do some juking.
	if (closeEnoughToJuke) then
		-- If we're already moving then don't do anything.
		-- If we AREN'T moving then wait for a randomised period of time before
		-- moving to a randomised position (ensuring that position is also within
		-- the Goldilocks range).
		if (not self:IsNavigating() and not self.isTurningToFace) then
			self.timeBeforeNextJuke = self.timeBeforeNextJuke - deltaTime;
			
			-- Perform a juke.
			-- This juking code isn't done based on any kind of behaviour; it just picks a
			-- valid location near the player and moves to it.
			if (self.timeBeforeNextJuke <= 0.0) then
				-- Calculate a position to juke to.
				-- 1. Choose left or right.
				local left = math.random(0.0, 1.0) > 0.5;
				-- 2. Choose random distance.
				local variance = utilities.RandomPlusMinus(self.Properties.StateMachines.AI.Combat.Juking.JukeDistanceVariance, 0.5);
				local jukeDist = self.Properties.StateMachines.AI.Combat.Juking.JukeDistance + variance;
				-- 3. Calculate the juke position.
				local pos = TransformBus.Event.GetWorldTM(self.entityId):GetTranslation();
				local playerPos = TransformBus.Event.GetWorldTM(TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"))):GetTranslation();
				local forward = (playerPos - pos):GetNormalized();
				local up = TransformBus.Event.GetWorldTM(self.entityId):GetColumn(2);
				local jukeOffset;
				if (left) then
					jukeOffset = Vector3.Cross(up, forward);
				else
					jukeOffset = Vector3.Cross(forward, up);
				end
				local jukePos = pos + (jukeOffset * jukeDist);
				-- 4. Push/Pull that distance so it's within a comfortable range
				--		within the Goldilocks range.
				local jukePosToPlayer = (jukePos - playerPos):GetNormalized();
				local minRange, maxRange = self:GetGoldilocksMinMax();
				jukePos = playerPos + (jukePosToPlayer * ((math.random() * (maxRange - minRange)) + minRange));
				
				-- 5. Move to it.
				self:TravelToNavMarker(jukePos);
				self.isJuking = true;
				
				-- Reset the timer.
				variance = utilities.RandomPlusMinus(self.Properties.StateMachines.AI.Combat.Juking.JukeDelayVariance, 0.5);
				self.timeBeforeNextJuke = self.Properties.StateMachines.AI.Combat.Juking.JukeDelay + variance;
			end
		end
	end
end

function aicontroller:CalculateTimeUntilNextShot(firstShot)
	local delay = self.Properties.StateMachines.AI.Combat.DelayBetweenShots;
	local variance = self.Properties.StateMachines.AI.Combat.DelayBetweenShotsVariance;
	if (firstShot) then
		delay = self.Properties.StateMachines.AI.Combat.FirstShot.Delay;
		variance = self.Properties.StateMachines.AI.Combat.FirstShot.Variance;
	end
	
	local offset = utilities.RandomPlusMinus(variance, 0.5);
	local res = delay + offset;
	if (res < 0.0) then
		Debug.Warning("AIController: 'timeUntilNextShot' was calculated to be less than 0 (i.e. next frame).");
	end
	--Debug.Log(tostring(self.entityId) .. " shot timer: " .. tostring(res) .. " (offset: " .. tostring(offset) .. ")");
	return res;
end

-- When in combat we want to stay within a certain range of the enemy so that
-- we don't lose sight but also so that the enemy can't get close.
--
--          (1)              (2) (3)
--           v                v   v
-- (A.I.) ------- (enemy) ---------
--
-- (1) minimum range
-- (2) maximum range
-- (3) sight range
--
-- When the enemy reaches (3) they will no longer be visible and the A.I. will
-- change state to 'Tracking'.
-- If the enemy gets closer than (1) then they're too close.
-- Ideally, the A.I. wants to keep the enemy at a distance between (1) and (2).
-- These ranges could be 25% and 75% of the A.I.'s sight range; therefore if the
-- A.I.'s sight range is 10 then the following rules apply:
--	- retreat at range 2.5
--	- move closer at range 7.5
-- The target destinations that the A.I. will move to will be 3.0 and 7.0 to
-- ensure the A.I. is comfortably within the 'Goldilocks range'.
function aicontroller:UpdateGoldilocksPositioning(deltaTime)
	local pos = TransformBus.Event.GetWorldTM(self.entityId):GetTranslation();
	local playerPos = TransformBus.Event.GetWorldTM(TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"))):GetTranslation();
	local dir = playerPos - pos;
	local dist = dir:GetLength();	-- I can't be bothered squaring everything else; just get sqr
	local minRange, maxRange = self:GetGoldilocksMinMax();
	
	-- If the enemy is within the Goldilocks range then do nothing.
	if (dist >= minRange and dist <= maxRange) then
		return true;
	end
	
	-- If we're not already moving somewhere then move.
	-- 'isJuking' means we're doing a pointless movement, so override that.
	-- TODO: 'self.navCancelled' needs to be handled in a separate case as it means
	-- the nav system failed to find a path to our desired destination. As a result,
	-- we'll need to run to the side or behind the player to get away from them if
	-- they're too close. If they're too far away then do nothing as it means they're
	-- outside the nav area.
	if (self.isJuking == true or self.requestId == 0 or self.reachedWaypoint == true or self.navCancelled == true) then
		-- Push the max and min ranges in so that the A.I. will move to a point
		-- COMFORTABLY within the Goldilocks range.
		-- The reason we have to make sure is because the navigation system has a
		-- "close enough" value. So if we get the A.I. to move TO the min or max range
		-- then there's a good chance they'll stop before actually getting inside the
		-- Goldilocks range.
		local margin = (maxRange - minRange) * 0.25;
		minRange = minRange + margin;
		maxRange = maxRange - margin;
		
		local offsetDist = 0.0;
		-- Retreat!
		if (dist < minRange) then
			offsetDist = (dist - minRange);
		-- Chase!
		elseif (dist > maxRange) then
			offsetDist = (dist - maxRange);
			self.isChasing = true;
		end
		local offset = dir:GetNormalized() * offsetDist;
		local targetPos = pos + offset;
		
		if (self.isChasing) then
			self:SetMoveSpeed(self.Properties.StateMachines.AI.Combat.MoveSpeedChasing);
		end
		
		self:TravelToNavMarker(targetPos);
		self.isJuking = false;
	end
	
	return false;
end

function aicontroller:GetGoldilocksMinMax()
	local minRange = self.Properties.StateMachines.AI.Combat.Goldilocks.MinRange;
	local maxRange = self.Properties.StateMachines.AI.Combat.Goldilocks.MaxRange;
	return minRange, maxRange;
end

function aicontroller:HasLostSightOfTarget()
	local isBlind = true;
	local playerId = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"));
	local playerPos = StarterGameUtility.GetJointWorldTM(playerId, self.playerSkeletonNS..utilities.Joint_Torso_Upper):GetTranslation();
	local aiPos = StarterGameUtility.GetJointWorldTM(self.entityId, self.aiSkeletonNS..utilities.Joint_Head):GetTranslation();
	local aiToPlayer = playerPos - aiPos;
	local distSq = aiToPlayer:GetLengthSq();
	
	-- If the player has moved out of sight range then we've lost them.
	local sr = self.Properties.StateMachines.AI.Combat.SightRange;
	if (distSq <= utilities.Sqr(2.0)) then
		isBlind = false;
	elseif (distSq <= utilities.Sqr(sr)) then
		if (self:HasLoSOfPlayer(sr)) then
			-- Store this as the new 'suspiciousLocation' in case we lose track of
			-- the player next frame (as this will be the location we'll attempt to
			-- path to when trying to track them down).
			self.suspiciousLocation = playerPos;
			isBlind = false;
		end
	end
	
	return isBlind;
end

function aicontroller:SoundIsCloseEnoughToHear(sound)
	local pos = TransformBus.Event.GetWorldTM(self.entityId):GetTranslation();
	local soundPos = sound.closestPoint;
	local range = sound.range * self.Properties.StateMachines.AI.Combat.HearingScalar;
	range = range * range;
	
	return (pos - soundPos):GetLengthSq() <= range;
end

function aicontroller:NewSoundIsLessImportant(oldSound, newSound)
	-- This could become state-dependant but for now it's just a straight-forward compare.
	if (oldSound == nil) then
		return false;
	elseif (newSound.type < oldSound.type) then
		return true;
	end
	
	return false;
end

-- This function expects 'self.soundHeardJustNow' to be non-nil.
function aicontroller:NewSoundIsSuspicious()
	if (self:NewSoundIsLessImportant(self.soundHeard, self.soundHeardJustNow)) then
		return false;
	end
	
	local oldSound = self.soundHeard;
	local newSound = self.soundHeardJustNow;
	if (not self.isFirstSuspiciousSound and oldSound ~= nil) then
		if (newSound.type == oldSound.type) then
			-- Ignore the new sound if it's approximately the same (origin, direction, etc.).
			local pos = TransformBus.Event.GetWorldTM(self.entityId):GetTranslation();
			local dirToOld = (pos - oldSound.origin):GetNormalized();
			local dirToNew = (pos - newSound.origin):GetNormalized();
			local angleInRads = Math.ArcCos(dirToOld:Dot(dirToNew));
			--Debug.Log("Angle: " .. tostring(angleInRads) .. ", " .. tostring(self.Properties.SoundResponses.SimilarRejectionAngle));
			--Debug.Log("Distance: " .. tostring((oldSound.origin - newSound.origin):GetLengthSq()) .. ", " .. tostring(self.Properties.SoundResponses.SimilarRejectionDistanceSq));
			if (angleInRads < self.Properties.SoundResponses.SimilarRejectionAngle) then
				return false;
			elseif ((oldSound.origin - newSound.origin):GetLengthSq() < self.Properties.SoundResponses.SimilarRejectionDistanceSq) then
				return false;
			end
		end
	end
	
	-- If the sound hasn't been rejected by any checks in the function, be suspicious of it.
	return true;
end

-- This function expects 'self.soundHeardJustNow' to be non-nil.
function aicontroller:NewSoundShouldBeFought() -- I know that name doesn't make sense, but I couldn't think of another
	if (self:NewSoundIsLessImportant(self.soundHeard, self.soundHeardJustNow)) then
		return false;
	end
	
	--local isSameSound = self.soundHeard ~= nil and self.soundHeard.type == self.soundHeardJustNow.type;
	local soundType = self.soundHeardJustNow.type;
	if (self.AIStateMachine.CurrentStateName ~= "Idle" and soundType == SoundTypes.GunShot) then
		return true;
	end
	
	return false;
end

function aicontroller:SuspiciousReactionToSound()
	local sound = self.soundHeardJustNow;
	local pos = TransformBus.Event.GetWorldTM(self.entityId):GetTranslation();
	
	--Debug.Log("Reacting to suspicious sound: " .. tostring(sound.type) .. ", Origin: " .. tostring(sound.origin) .. ", ClosestPoint: " .. tostring(sound.closestPoint));
	if (sound.type == SoundTypes.LauncherExplosion) then
		-- This should send the A.I. towards the epicenter of the explosion but they won't actually
		-- go TO the epicenter.
		local dir = sound.closestPoint - pos;
		self.suspiciousLocation = pos + (dir:GetNormalized() * (dir:GetLength() - self.Properties.SoundResponses.Explosion.Distance));
	elseif (sound.type == SoundTypes.WhizzPast) then
		local dir = sound.origin - pos;
		if (self.justBecomeSuspicious) then
			-- Turn towards the direction the shot came from.
			self.suspiciousLocation = pos + (dir:GetNormalized() * self.Properties.SoundResponses.WhizzPast.Initial);
		else
			-- Move towards the origin of the shot so it's origin is within aggro range.
			local range = self.Properties.StateMachines.AI.Combat.AggroRange * self.Properties.SoundResponses.WhizzPast.AggroRangeScalar;
			local distance = dir:GetLength();
			if (distance <= range) then
				-- Move to the origin if the distance is short enough.
				self.suspiciousLocation = sound.origin;
			else
				-- Move so the origin is within aggro range if not already close to it.
				self.suspiciousLocation = pos + (dir:GetNormalized() * (distance - range));
			end
		end
	else
		self.suspiciousLocation = sound.closestPoint;
	end
	
	self:CompleteReactionToSound();
end

function aicontroller:CombatReactionToSound()
	--Debug.Log("Reacting to combat sound");
	local sound = self.soundHeardJustNow;
	self.suspiciousLocation = sound.closestPoint;
	
	self:CompleteReactionToSound();
end

function aicontroller:CompleteReactionToSound()
	self.soundHeard = self.soundHeardJustNow;
	self.soundHeardJustNow = nil;
	self.reactToRecentSound = false;
end

--------------------------------------------------------
--------------------------------------------------------

function aicontroller:SetMoveSpeed(speed)
	NavigationComponentRequestBus.Event.SetAgentSpeed(self.entityId, speed);
	-- 5.6 is the movement speed from the MovementController (which should be the maximum the
	-- animations run at).
	self.moveSpeed = speed / 5.6;
end

function aicontroller:OnPathFoundFirstPoint(navRequestId, path)
	--Debug.Log("Navigation Path Found: First pos = " .. tostring(firstPos));
	self.searchingForPath = false;
	
	self.navPath = path;
	VisualizePathSystemRequestBus.Broadcast.AddPath(self.entityId, path);
	self.cancelSentByOnPathFound = true;
	return false;
	
	---- If we're already moving then we can turn while continuing to move towards the destination.
	--if (self:IsMoving()) then
	--	return true;
	--end
	
	---- If we're not already moving then we need to stop it and perform an idle turn first.
	--local dir = firstPos - TransformBus.Event.GetWorldTM(self.entityId):GetTranslation();
	--local isFacing = self:IsFacing(dir);
	--if (not isFacing) then
	--	--Debug.Log("Navigation path found: not close enough: " .. tostring(firstPos) .. ". Turning to: " .. tostring(dir));
	--	self:SetTurning(true, "turning towards first pathing point");
	--	self:SetMovement(dir.x, dir.y);
	--else
	--	self:SetTurning(false, "already facing first pathing point");
	--end
	--return isFacing;
end

function aicontroller:OnTraversalStarted(navRequestId)
	--Debug.Log("Navigation Started");
end

function aicontroller:OnTraversalInProgress(navRequestId, distance)
	--Debug.Log("Navigation Progress " .. tostring(distance));
end

function aicontroller:OnTraversalComplete(navRequestId)
	--Debug.Log("Navigation Complete " .. tostring(navRequestId));
	self.requestId = 0;
	self.navPath = nil;
	VisualizePathSystemRequestBus.Broadcast.ClearPath(self.entityId);
	self.reachedWaypoint = true;
	self:SetMovement(0.0, 0.0);
end

function aicontroller:OnTraversalCancelled(navRequestId)
	--Debug.Log("Navigation Cancelled " .. tostring(self.requestId));
	-- Ignore the traversal cancellation if it occured because we rejected allowing the navigation
	-- system to push us along the path.
	if (self.cancelSentByOnPathFound) then
		self.cancelSentByOnPathFound = false;
		return;
	end
	
	self.searchingForPath = false;
	self.navCancelled = true;
	if (self.isTurningToFace == false) then
		self:SetMovement(0.0, 0.0);
	end
	
	-- If the request ID is 0 then we cancelled it on purpose. Doing this
	-- stops the navigation system from moving the entity to new points but
	-- it still continues along it's previous vector, so we need to kill its
	-- velocity.
	PhysicsComponentRequestBus.Event.SetVelocity(self.entityId, Vector3(0.0, 0.0, 0.0));
	if (self.requestId ~= 0) then
		self.pathfindingFailed = true;
	end
	
	-- Reset the request ID as we don't want to store an invalid ID.
	self.requestId = 0;
	self.navPath = nil;
	VisualizePathSystemRequestBus.Broadcast.ClearPath(self.entityId);
end

function aicontroller:TravelToFirstWaypoint()
	self.currentWaypoint = WaypointsComponentRequestsBus.Event.GetFirstWaypoint(self.entityId);
	self:StartNavigation(self.currentWaypoint);
end

function aicontroller:TravelToCurrentWaypoint()
	--Debug.Log("Travel to current");
	if (self.currentWaypoint == nil or not self.currentWaypoint:IsValid()) then
		self.currentWaypoint = WaypointsComponentRequestsBus.Event.GetFirstWaypoint(self.entityId);
	end
	self:StartNavigation(self.currentWaypoint);
end

function aicontroller:TravelToNextWaypoint()
	--Debug.Log("Travel to next");
	self.currentWaypoint = WaypointsComponentRequestsBus.Event.GetNextWaypoint(self.entityId);
	self:StartNavigation(self.currentWaypoint);
end

function aicontroller:TravelToNavMarker(pos)
	local closestPoint = Vector3(0.0);
	local success = StarterGameAIUtility.GetClosestPointInNavMesh(pos, closestPoint);
	if (success) then
		pos = closestPoint;
	end
	
	self:MoveNavMarkerTo(pos);
	self:StartNavigation(self.Properties.NavMarker);
end

function aicontroller:MoveNavMarkerTo(pos)
	local tm = TransformBus.Event.GetWorldTM(self.Properties.NavMarker);
	tm:SetTranslation(pos);
	TransformBus.Event.SetWorldTM(self.Properties.NavMarker, tm);
end

function aicontroller:StartNavigation(dest)
	if (dest ~= nil and dest:IsValid()) then
		self.requestId = NavigationComponentRequestBus.Event.FindPathToEntity(self.entityId, dest);
		self.navPath = nil;
		self.searchingForPath = true;
		self.reachedWaypoint = false;
		self.navCancelled = false;
		self.pathfindingFailed = false;
		self:SetTurning(false, "started navigation");
		self.justFinishedTurning = false;
		--Debug.Log("Navigating to: " .. tostring(TransformBus.Event.GetWorldTM(dest):GetTranslation()) .. " (with ID: " .. tostring(self.requestId) .. ")");
	else
		Debug.Log(tostring(self.entityId) .. " tried navigating to a waypoint that doesn't exist.");
	end
end

function aicontroller:IsNavigating()
	return self.requestId ~= 0 and self.navCancelled == false and self.pathfindingFailed == false;
end

function aicontroller:SetMovement(fb, lr)
	-- Scale the movement values by the movement speed.
	self.InputValues.NavForwardBack = fb * self.moveSpeed;
	self.InputValues.NavLeftRight = lr * self.moveSpeed;
end

function aicontroller:SetTurning(isTurning, reason)
	if (isTurning ~= self.isTurningToFace) then
		self.isTurningToFace = isTurning;
		self.waitingForTurningConfirmation = isTurning;
		self.framesSpentWaitingForTurningConfirmation = 0;
	end
	
	if (isTurning) then
		utilities.LogIf(self, "TURNING: " .. reason);
	else
		utilities.LogIf(self, "STOPPED TURNING: " .. reason);
	end
end

function aicontroller:SetStrafeFacing()
	self.setStrafeFacingId = GameplayNotificationId(self.entityId, "SetStrafeFacing", "float");
	local flatAiming = Vector3(self.aimDirection.x, self.aimDirection.y, 0.0);
	if(flatAiming:GetLengthSq() > 0) then
		flatAiming:Normalize();
		GameplayNotificationBus.Event.OnEventBegin(self.setStrafeFacingId, flatAiming);
	end
end
return aicontroller;
