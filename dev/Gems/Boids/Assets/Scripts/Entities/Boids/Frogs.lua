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
Frogs =
{
	type = "Boids",

	Properties =
	{
		Movement =
		{
			SpeedMin = 2,
			SpeedMax = 4,
			MaxAnimSpeed = 1,
		},
		Boid =
		{
			nCount = 10, --[0,1000,1,"Specifies how many individual objects will be spawned."]
			object_Model = "objects/characters/animals/amphibians/toad/toad.cdf",
			Mass = 10,
			bInvulnerable = false,
		},
		Options =
		{
			bPickableWhenAlive = 1,
			bPickableWhenDead = 1,
			PickableMessage = "",
			bFollowPlayer = 0,
			bObstacleAvoidance = 1,
			VisibilityDist = 50,
			bActivate = 1,
			Radius = 10,
		},
		ParticleEffects =
		{
			waterJumpSplash = "collisions.small.water",
			fEffectScale = 0.5,
		},
	},

	Audio =
	{
		"Play_idle_frog",		-- idle
		"Play_scared_frog",	-- scared
		"Play_death_frog",	-- die
		"Play_scared_frog",	-- pickup
		"Play_scared_frog",	-- throw
	},

	Animations =
	{
		"walk_loop",	-- walking
		"idle01_loop",	-- idle1
		"idle01_loop",	-- idle1
		"idle01_loop",	-- idle1
		"pickup",		-- pickup
		"swim",
	},

	Editor =
	{
		Icon = "Bug.bmp"
	},
	params={x=0,y=0,z=0},
}

-------------------------------------------------------
function Frogs:OnSpawn()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
	self:SetFlagsExtended(ENTITY_FLAG_EXTENDED_NEEDS_MOVEINSIDE, 0);
end

-------------------------------------------------------
function Frogs:OnInit()

	--self:NetPresent(0);
	--self:EnableSave(1);

	self.flock = 0;
	self.currpos = {x=0,y=0,z=0};
	if (self.Properties.Options.bActivate == 1) then
		self:CreateFlock();
	end

	self:CacheResources()
end

-------------------------------------------------------
function Frogs:CacheResources()
	self:PreLoadParticleEffect( self.Properties.ParticleEffects.waterJumpSplash );
end

-------------------------------------------------------
function Frogs:OnShutDown()
end

-------------------------------------------------------
--function Frogs:OnLoad(table)
--end

-------------------------------------------------------
--function Frogs:OnSave(table)
--end

-------------------------------------------------------
function Frogs:CreateFlock()

	local Movement = self.Properties.Movement;
	local Boid = self.Properties.Boid;
	local Options = self.Properties.Options;

	local params = self.params;

	params.count = __max(0, Boid.nCount);
	params.model = Boid.object_Model;

	params.boid_size = 1;
	params.boid_size_random = 0;
	params.min_speed = Movement.SpeedMin;
	params.max_speed = Movement.SpeedMax;

	params.factor_align = 0;

	params.spawn_radius = Options.Radius;
	params.gravity_at_death = -9.81;
	params.boid_mass = Boid.Mass;
	params.invulnerable = Boid.bInvulnerable;

	params.max_anim_speed = Movement.MaxAnimSpeed;
	params.follow_player = Options.bFollowPlayer;
	params.avoid_obstacles = Options.bObstacleAvoidance;
	params.max_view_distance = Options.VisibilityDist;
	
	params.pickable_alive = Options.bPickableWhenAlive;
	params.pickable_dead = Options.bPickableWhenDead;
	params.pickable_message = Options.PickableMessage;
	
	params.Audio = self.Audio;
	params.Animations = self.Animations;

	if (self.flock == 0) then
		self.flock = 1;
		Boids.CreateFlock( self, Boids.FLOCK_FROGS, params );
	end
	if (self.flock ~= 0) then
		Boids.SetFlockParams( self, params );
	end
end

-------------------------------------------------------
function Frogs:OnPropertyChange()
	self:OnShutDown();
	if (self.Properties.Options.bActivate == 1) then
		self:CreateFlock();
		self:Event_Activate();
	else
		self:Event_Deactivate();
	end
end

-------------------------------------------------------
function Frogs:OnWaterSplash(pos)
	Particle.SpawnEffect(self.Properties.ParticleEffects.waterJumpSplash, pos, g_Vectors.v001, self.Properties.ParticleEffects.fEffectScale);
end

-------------------------------------------------------
function Frogs:Event_Activate()

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
function Frogs:Event_Deactivate()
	if (self.flock ~= 0) then
		Boids.EnableFlock( self,0 );
		self:RegisterForAreaEvents(0);
	end
end

-------------------------------------------------------
function Frogs:OnProceedFadeArea( player,areaId,fadeCoeff )
	if (self.flock ~= 0) then
		Boids.SetFlockPercentEnabled( self,fadeCoeff*100 );
	end
end

-------------------------------------------------------
-------------------------------------------------------
Frogs.FlowEvents =
{
	Inputs =
	{
		Activate = { Frogs.Event_Activate, "bool" },
		Deactivate = { Frogs.Event_Deactivate, "bool" },
	},
	Outputs =
	{
		Activate = "bool",
		Deactivate = "bool",
	},
}
