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

Crabs = 
{
	Properties = 
	{
	},
--	Sounds = 
--	{
--		"Sounds/environment:random_oneshots_natural:crab_idle",		-- idle
--		"Sounds/environment:random_oneshots_natural:crab_scared",	-- scared
--		"Sounds/environment:random_oneshots_natural:crab_idle",		-- die
--		"Sounds/environment:random_oneshots_natural:crab_idle",		-- pickup
--		"Sounds/environment:random_oneshots_natural:crab_idle",		-- throw
--	},
	Animations = 
	{
		"walk_loop",   -- walking
		"idle01",      -- idle1
		"idle01",      -- scared anim
		"idle01",      -- idle3
		"idle01",      -- pickup
		"idle01",      -- throw
	},
	Editor = 
	{
		Icon = "Bug.bmp"
	},
}

MakeDerivedEntity( Crabs,Chickens )

function Crabs:OnSpawn()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
end
