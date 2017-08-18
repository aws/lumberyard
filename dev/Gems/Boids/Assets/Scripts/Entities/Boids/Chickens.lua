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
Chickens =
{
	type = "Boids",
	MapVisMask = 0,
	ENTITY_DETAIL_ID = 1,

	Properties = 
	{
		Flocking =
		{
			bEnableFlocking = 0,
			FieldOfViewAngle = 250,
			FactorAlign = 0.01,
			FactorCohesion = 0.01,
			FactorSeparation = 1,
			AttractDistMin = 5,
			AttractDistMax = 20,
		},
		Movement =
		{
			HeightMin = 0,
			HeightMax = 1,
			SpeedMin = 1,
			SpeedMax = 5,
			FactorOrigin = 0.1,
			FactorHeight = 1,
			FactorAvoidLand = 10,
			MaxAnimSpeed = 4,
		},
		Boid =
		{
			nCount = 10, --[0,1000,1,"Specifies how many individual objects will be spawned."]
			object_Model = "objects/characters/animals/birds/rooster/rooster.cdf",
			gravity_at_death = -9.81,
			Mass = 10,
			bInvulnerable = false,
		},
		Options =
		{
			bPickableWhenAlive = 1,
			bPickableWhenDead = 1,
			PickableMessage = "",
			bFollowPlayer = 0,
			bAvoidWater = 1,
			bNoLanding = 0,
			bObstacleAvoidance = 1,
			VisibilityDist = 30,
			bActivate = 1,
			Radius = 10,
		},
	},

	Audio =
	{
		"Play_idle_chicken",		-- idle
		"Play_scared_chicken",	-- scared
		"Play_death_chicken",		-- die
		"Play_scared_chicken",	-- pickup
		"Play_scared_chicken",	-- throw
	},

	Animations =
	{
		"walk_loop",	-- walking
		"idle01_loop",	-- idle1
		"idle01_loop",	-- idle2
		"idle01_loop",	-- idle3
		"pickup",		-- pickup
		"throw",		-- throw
	},
	Editor = 
	{
		Icon = "Bird.bmp"
	},
	params={x=0,y=0,z=0},
}

-------------------------------------------------------
function Chickens:OnSpawn()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
  self:SetFlagsExtended(ENTITY_FLAG_EXTENDED_NEEDS_MOVEINSIDE, 0);
end

-------------------------------------------------------
function Chickens:OnInit()

	--self:NetPresent(0);
	--self:EnableSave(1);

	self.flock = 0;
	self.currpos = {x=0,y=0,z=0};
	if (self.Properties.Options.bActivate == 1) then
		self:CreateFlock();
	end
end

-------------------------------------------------------
function Chickens:OnShutDown()
end

-------------------------------------------------------
--function Chickens:OnLoad(table)
--end

-------------------------------------------------------
--function Chickens:OnSave(table)
--end

-------------------------------------------------------
function Chickens:GetFlockType()
	return Boids.FLOCK_CHICKENS;
end

-------------------------------------------------------
function Chickens:CreateFlock()

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

	params.max_anim_speed = Movement.MaxAnimSpeed;
	params.follow_player = Options.bFollowPlayer;
	params.pickable_alive = Options.bPickableWhenAlive;
	params.pickable_dead = Options.bPickableWhenDead;
	params.pickable_message = Options.PickableMessage;
	params.avoid_water = Options.bAvoidWater;
	params.no_landing = Options.bNoLanding;
	params.avoid_obstacles = Options.bObstacleAvoidance;
	params.max_view_distance = Options.VisibilityDist;

	params.Audio = self.Audio;
	params.Animations = self.Animations;

	if (self.flock == 0) then
		self.flock = 1;
		Boids.CreateFlock( self, self:GetFlockType(), params );
	end
	if (self.flock ~= 0) then
		Boids.SetFlockParams( self, params );
	end
end

-------------------------------------------------------
function Chickens:OnPropertyChange()
	self:OnShutDown();
	if (self.Properties.Options.bActivate == 1) then
		self:CreateFlock();
		self:Event_Activate();
	else
		self:Event_Deactivate();
	end
end

-------------------------------------------------------
function Chickens:Event_Activate()

	if (self.Properties.Options.bActivate == 0) then
		if (self.flock==0) then
			self:CreateFlock();
		end
	end

	if (self.flock ~= 0) then
		Boids.EnableFlock( self,1 );
		self:RegisterForAreaEvents(1);
	end
end

-------------------------------------------------------
function Chickens:Event_Deactivate()
	if (self.flock ~= 0) then
		Boids.EnableFlock( self,0 );
		self:RegisterForAreaEvents(0);
	end
end

-------------------------------------------------------
function Chickens:OnProceedFadeArea( player,areaId,fadeCoeff )
	if (self.flock ~= 0) then
		Boids.SetFlockPercentEnabled( self,fadeCoeff*100 );
	end
end

-------------------------------------------------------
-------------------------------------------------------
Chickens.FlowEvents =
{
	Inputs =
	{
		Activate = { Chickens.Event_Activate, "bool" },
		Deactivate = { Chickens.Event_Deactivate, "bool" },
	},
	Outputs =
	{
		Activate = "bool",
		Deactivate = "bool",
	},
}
