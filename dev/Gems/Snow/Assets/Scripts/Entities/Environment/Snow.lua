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
Snow = {
	type = "Snow",
	Properties = {
		bEnabled = 1,
		fRadius = 50.0,
		Surface =
		{
			fSnowAmount = 10.0,
			fFrostAmount = 1.0,
			fSurfaceFreezing = 1.0,
		},
		SnowFall =
		{
			nSnowFlakeCount = 100,
			fSnowFlakeSize = 1,
			fBrightness = 1.0,
			fGravityScale = 0.1,
			fWindScale = 0.1,
			fTurbulenceStrength = 0.5,
			fTurbulenceFreq = 0.1,
		},
	},
	Editor={
		Icon="shake.bmp",
	},
}


function Snow:OnInit()
	self:OnReset();
end

function Snow:OnPropertyChange()
	self:OnReset();
end

function Snow:OnReset()
end

function Snow:OnSave(tbl)
end

function Snow:OnLoad(tbl)
end

function Snow:OnShutDown()
end

function Snow:Event_Enable( sender )
	self.Properties.bEnabled = 1;
	self:ActivateOutput("Enable", true);
end

function Snow:Event_Disable( sender )
	self.Properties.bEnabled = 0;
	self:ActivateOutput("Disable", true);
end

function Snow:Event_SetRadius( i, val )
	self.Properties.fRadius = val;
end

function Snow:Event_SetSnowAmount( i, val )
	self.Properties.Surface.fSnowAmount = val;
end

function Snow:Event_SetFrostAmount( i, val )
	self.Properties.Surface.fFrostAmount = val;
end

function Snow:Event_SetSurfaceFreezing( i, val )
	self.Properties.Surface.fSurfaceFreezing = val;
end

function Snow:Event_SetSnowFlakeSize( i, val )
	self.Properties.SnowFall.fSnowFlakeSize = val;
end

function Snow:Event_SetSnowFallBrightness( i, val )
	self.Properties.SnowFall.fBrightness = val;
end

function Snow:Event_SetSnowFallGravityScale( i, val )
	self.Properties.SnowFall.fGravityScale = val;
end

function Snow:Event_SetSnowFallWindScale( i, val )
	self.Properties.SnowFall.fWindScale = val;
end

function Snow:Event_SetSnowFallTurbulence( i, val )
	self.Properties.SnowFall.fTurbulenceStrength = val;
end

function Snow:Event_SetSnowFallTurbulenceFreq( i, val )
	self.Properties.SnowFall.fTurbulenceFreq = val;
end

Snow.FlowEvents =
{
	Inputs =
	{
		Disable = { Snow.Event_Disable, "bool" },
		Enable = { Snow.Event_Enable, "bool" },
		Radius = { Snow.Event_SetRadius, "float" },
		SnowAmount = { Snow.Event_SetSnowAmount, "float" },
		FrostAmount = { Snow.Event_SetFrostAmount, "float" },
		SurfaceFreezing = { Snow.Event_SetSurfaceFreezing, "float" },
		SnowFlakeSize = { Snow.Event_SetSnowFlakeSize, "float" },
		SnowFallBrightness = { Snow.Event_SetSnowFallBrightness, "float" },
		SnowFallGravityScale = { Snow.Event_SetSnowFallGravityScale, "float" },
		SnowFallWindScale = { Snow.Event_SetSnowFallWindScale, "float" },
		SnowFallTurbulence = { Snow.Event_SetSnowFallTurbulence, "float" },
		SnowFallTurbulenceFreq = { Snow.Event_SetSnowFallTurbulenceFreq, "float" },	
	},
	Outputs =
	{
		Disable = "bool",
		Enable = "bool",
	},
}
