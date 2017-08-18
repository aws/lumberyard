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
Bugs =
{
	type = "Bugs",
	MapVisMask = 0,
	ENTITY_DETAIL_ID = 1,

	Properties =
	{
		Movement =
		{
			HeightMin = 1,
			HeightMax = 5,
			SpeedMin = 1,
			SpeedMax = 4,
			FactorOrigin = 1,
			--FactorHeight = 0.4,
			--FactorAvoidLand = 10,
			MaxAnimSpeed = 1,
			RandomMovement = 1,
		},
		Boid =
		{
			nCount = 5, --[0,1000,1,"Specifies how many individual objects will be spawned."]
			object_Model1 = "objects/characters/animals/insects/butterfly/butterfly_blue.chr",
			object_Model2 = "objects/characters/animals/insects/butterfly/butterfly_brown.chr",
			object_Model3 = "objects/characters/animals/insects/butterfly/butterfly_fly.chr",
			object_Model4 = "objects/characters/animals/insects/butterfly/butterfly_green.chr",
			object_Model5 = "objects/characters/animals/insects/butterfly/butterfly_yellow.chr",
			--object_Character = "",
			Animation = "",
			nBehaviour = 0, -- 0 = bugs, 1 = dragonfly, 2 = frog
			bInvulnerable = false,
		},
		Options =
		{
			bFollowPlayer = 0,
			bNoLanding = 1,
			--bObstacleAvoidance = 0,
			VisibilityDist = 50,
			bActivate = 1,
			Radius = 6,
		},
	},

	Properties1 =
	{
		object_Model1 = "",
		object_Model2 = "",
		object_Model3 = "",
		object_Model4 = "",
		object_Model5 = "",
		object_Character = "",
		nNumBugs = 10,
		nBehaviour = 0,
		Scale = 1,
		HeightMin = 1,
		HeightMax = 5,
		SpeedMin = 5,
		SpeedMax = 15,
		FactorOrigin = 1,
		--FactorHeight = 0.4,
		--FactorAvoidLand = 10,
		RandomMovement = 1,
		bFollowPlayer = 0,
		Radius = 10,
		bActivateOnStart = 1,
		bNoLanding = 0,
		AnimationSpeed = 1,
		Animation = "",
		VisibilityDist = 100,
	},

	Animations =
	{
		"walk_loop",	-- walking
		"idle01",		-- idle1
		"idle01",		-- idle2
		"idle01",		-- idle3
		"idle01",		-- pickup
		"idle01",		-- throw
	},

	Editor =
	{
		Icon = "Bug.bmp"
	},

	params={},
}

-------------------------------------------------------
--function Birds:OnLoad(table)
--end

-------------------------------------------------------
--function Birds:OnSave(table)
--end

-------------------------------------------------------
function Bugs:OnInit()
	self:NetPresent(0);
	--self:EnableSave(1);
	self.flock = 0;
	self:CreateFlock();
	if (self.Properties.Options.bActivate ~= 1 and self.flock ~= 0) then
		Boids.EnableFlock( self,0 );
		self:RegisterForAreaEvents(0);
	end
end

-------------------------------------------------------
function Bugs:OnSpawn()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
	self:SetFlagsExtended(ENTITY_FLAG_EXTENDED_NEEDS_MOVEINSIDE, 0);
end

-------------------------------------------------------
function Bugs:OnShutDown()
end

-------------------------------------------------------
function Bugs:CreateFlock()
	--local Flocking = self.Properties.Flocking;
	local Movement = self.Properties.Movement;
	local Boid = self.Properties.Boid;
	local Options = self.Properties.Options;

	local params = self.params;
	params.count = __max(0, Boid.nCount);

	params.behavior = Boid.nBehaviour;

	params.model = Boid.object_Model;
	params.model = Boid.object_Model1;
	params.model1 = Boid.object_Model2;
	params.model2 = Boid.object_Model3;
	params.model3 = Boid.object_Model4;
	params.model4 = Boid.object_Model5;
	--params.character = Boid.object_Character;

	params.boid_size = 1;
	params.boid_size_random = 0;
	params.min_height = Movement.HeightMin;
	params.max_height = Movement.HeightMax;
	--params.min_attract_distance = Flocking.AttractDistMin;
	--params.max_attract_distance = Flocking.AttractDistMax;
	params.min_speed = Movement.SpeedMin;
	params.max_speed = Movement.SpeedMax;

	--if (Flocking.bEnableFlocking == 1) then
		--params.factor_align = Flocking.FactorAlign;
	--else
		--params.factor_align = 0;
	--end
	--params.fov_angle = Flocking.FieldOfViewAngle;
	--params.factor_cohesion = Flocking.FactorCohesion;
	--params.factor_separation = Flocking.FactorSeparation;
	params.factor_origin = Movement.FactorOrigin;
	params.factor_keep_height = Movement.FactorHeight;
	params.factor_avoid_land = Movement.FactorAvoidLand;

	params.spawn_radius = Options.Radius;
	params.gravity_at_death = Boid.gravity_at_death;
	params.invulnerable = Boid.bInvulnerable;

	params.max_anim_speed = Movement.MaxAnimSpeed;
	params.follow_player = Options.bFollowPlayer;
	params.no_landing = Options.bNoLanding;
	params.avoid_obstacles = Options.bObstacleAvoidance;
	params.max_view_distance = Options.VisibilityDist;

	if (self.flock == 0) then
		self.flock = 1;
		Boids.CreateFlock( self, Boids.FLOCK_BUGS, params );
	end
	if (self.flock ~= 0) then
		Boids.SetFlockParams( self, params );
	end
end

-------------------------------------------------------
function Bugs:OnPropertyChange()
	if (self.Properties.Options.bActivate == 1) then
		self:CreateFlock();
		self:Event_Activate();
	else
		self:Event_Deactivate();
	end
end

-------------------------------------------------------
function Bugs:Event_Activate( sender )
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
function Bugs:Event_Deactivate( sender )
	if (self.flock ~= 0) then
		Boids.EnableFlock( self,0 );
		self:RegisterForAreaEvents(0);
	end
end

-------------------------------------------------------
function Bugs:OnProceedFadeArea( player,areaId,fadeCoeff )
	if (self.flock ~= 0) then
		Boids.SetFlockPercentEnabled( self,fadeCoeff*100 );
	end
end

-------------------------------------------------------
-------------------------------------------------------
Bugs.FlowEvents =
{
	Inputs =
	{
		Activate = { Bugs.Event_Activate, "bool" },
		Deactivate = { Bugs.Event_Deactivate, "bool" },
	},
	Outputs =
	{
		Activate = "bool",
		Deactivate = "bool",
	},
}
