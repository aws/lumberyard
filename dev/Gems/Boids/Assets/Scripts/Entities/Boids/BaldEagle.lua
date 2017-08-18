----------------------------------------------------------------------------------------------------
--
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--
--
----------------------------------------------------------------------------------------------------
BaldEagle =
{
	type = "Birds",
	MapVisMask = 0,
	ENTITY_DETAIL_ID = 1,

	Properties =
	{
		Flocking =
		{
			bEnableFlocking = 0,
			FieldOfViewAngle = 250,
			FactorAlign = 1,
			FactorCohesion = 1,
			FactorSeparation = 10,
			AttractDistMin = 5,
			AttractDistMax = 20,
		},
		Movement =
		{
			HeightMin = 50,
			HeightMax = 300,
			SpeedMin = 2.5,
			SpeedMax = 15,
			FactorOrigin = 0.1,
			FactorHeight = 0.4,
			FactorAvoidLand = 10,
			MaxAnimSpeed = 1.7,
		},
		Boid =
		{
			nCount = 10, --[0,1000,1,"Specifies how many individual objects will be spawned."]
			object_Model = "objects/characters/animals/birds/bald_eagle/eagle.chr",
			gravity_at_death = -9.81,
			Mass = 10,
			bInvulnerable = false,
		},
		Options =
		{
			bPickableWhenAlive = 0,
			bPickableWhenDead = 1,
			PickableMessage = "",
			bFollowPlayer = 0,
			bNoLanding = 0,
			bObstacleAvoidance = 0,
			VisibilityDist = 200,
			bActivate = 1,
			Radius = 20,
			bSpawnFromPoint = 0,
		},
	},
	Animations =
	{
		"fly_loop", -- flying
		"landing",
		"walk_loop", -- walk
		"idle_loop", -- idle
	},
	Editor =
	{
		Icon = "Bird.bmp"
	},
	params={x=0,y=0,z=0},
}

-------------------------------------------------------
function BaldEagle:OnSpawn()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
end

-------------------------------------------------------
function BaldEagle:OnInit()
	self.flock = 0;
	self.currpos = {x=0,y=0,z=0};
	if (self.Properties.Options.bActivate == 1) then
		self:CreateFlock();
	end
end

-------------------------------------------------------
function BaldEagle:OnShutDown()
end

-------------------------------------------------------
function BaldEagle:CreateFlock()

	local Flocking = self.Properties.Flocking;
	local Movement = self.Properties.Movement;
	local Boid = self.Properties.Boid;
	local Options = self.Properties.Options;

	local params = self.params;

	params.count = __max(0, Boid.nCount);
	params.model = Boid.object_Model;

	params.boid_size = 1;
	params.boid_size_random = 0;
	params.min_height = Movement.HeightMin;
	params.max_height = Movement.HeightMax;
	params.min_attract_distance = Flocking.AttractDistMin;
	params.max_attract_distance = Flocking.AttractDistMax;
	params.min_speed = Movement.SpeedMin;
	params.max_speed = Movement.SpeedMax;

	if (Flocking.bEnableFlocking == 1) then
		params.factor_align = Flocking.FactorAlign;
	else
		params.factor_align = 0;
	end
	params.factor_cohesion = Flocking.FactorCohesion;
	params.factor_separation = Flocking.FactorSeparation;
	params.factor_origin = Movement.FactorOrigin;
	params.factor_keep_height = Movement.FactorHeight;
	params.factor_avoid_land = Movement.FactorAvoidLand;

	params.spawn_radius = Options.Radius;
	params.gravity_at_death = Boid.gravity_at_death;
	params.boid_mass = Boid.Mass;
	params.invulnerable = Boid.bInvulnerable;

	params.fov_angle = Flocking.FieldOfViewAngle;

	params.pickable_alive = Options.bPickableWhenAlive;
	params.pickable_dead = Options.bPickableWhenDead;
	params.pickable_message = Options.PickableMessage;
	
	params.max_anim_speed = Movement.MaxAnimSpeed;
	params.follow_player = Options.bFollowPlayer;
	params.no_landing = Options.bNoLanding;
	params.avoid_obstacles = Options.bObstacleAvoidance;
	params.max_view_distance = Options.VisibilityDist;
	params.spawn_from_point = Options.bSpawnFromPoint;
	params.Animations = self.Animations;

	if (self.flock == 0) then
		self.flock = 1;
		Boids.CreateFlock( self, Boids.FLOCK_BIRDS, params );
	end
	if (self.flock ~= 0) then
		Boids.SetFlockParams( self, params );
	end
end

-------------------------------------------------------
function BaldEagle:OnPropertyChange()
	self:OnShutDown();
	if (self.Properties.Options.bActivate == 1) then
		self:CreateFlock();
		self:Event_Activate();
	else
		self:Event_Deactivate();
	end
end

-------------------------------------------------------
function BaldEagle:Event_Activate()

	if (self.Properties.Options.bActivate == 0) then
		if (self.flock == 0) then
			self:CreateFlock();
		end
	end

	if (self.flock ~= 0) then
		Boids.EnableFlock( self,1 );
	end

	self:ActivateOutput("Activate", true);
end

-------------------------------------------------------
function BaldEagle:Event_Deactivate()
	if (self.flock ~= 0) then
		Boids.EnableFlock( self,0 );
	end
	self:ActivateOutput("Deactivate", true);
end

-------------------------------------------------------
function BaldEagle:Event_AttractTo(sender, point)
	Boids.SetAttractionPoint(self, point);
end

-------------------------------------------------------
function BaldEagle:OnAttractPointReached()
	self:ActivateOutput("AttractEnd", true);
end

-------------------------------------------------------
function BaldEagle:OnProceedFadeArea( player,areaId,fadeCoeff )
	if (self.flock ~= 0) then
		Boids.SetFlockPercentEnabled( self,fadeCoeff*100 );
	end
end

-------------------------------------------------------
-------------------------------------------------------
BaldEagle.FlowEvents =
{
	Inputs =
	{
		Activate = { BaldEagle.Event_Activate, "bool" },
		Deactivate = { BaldEagle.Event_Deactivate, "bool" },
		AttractTo = { BaldEagle.Event_AttractTo, "Vec3" },
	},
	Outputs =
	{
		Activate = "bool",
		Deactivate = "bool",
		AttractEnd = "bool",
	},
}
