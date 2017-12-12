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
Script.ReloadScript( "Scripts/Entities/Boids/Chickens.lua" );

Turtles =
{
	Properties = 
	{
		Boid = 
		{
			nCount = 4, --[0,1000,1,"Specifies how many individual objects will be spawned."]
			object_Model = "Objects/characters/animals/reptiles/turtle/red_eared_slider.cdf",
		},
		Movement =
		{
			SpeedMin = 0.2,
			SpeedMax = 0.5,
			MaxAnimSpeed = 2,
		},
	},

	Audio =
	{
		"Play_idle_turtle",		-- idle
		"Play_scared_turtle",	-- scared
		"Play_death_turtle",	-- die
		"Play_scared_turtle",	-- pickup
		"Play_scared_turtle",	-- throw
	},

	Animations =
	{
		"walk_loop",	-- walking
		"idle3",		-- idle1
		"scared",		-- scared anim
		"idle3",		-- idle3
		"pickup",		-- pickup
		"throw",		-- throw
	},
	Editor =
	{
		Icon = "bug.bmp"
	},
}

-------------------------------------------------------
MakeDerivedEntityOverride( Turtles,Chickens )

-------------------------------------------------------
function Turtles:OnSpawn()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
end

-------------------------------------------------------
function Turtles:GetFlockType()
	return Boids.FLOCK_TURTLES;
end