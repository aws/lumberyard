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
Tornado = {
	type = "Tornado",
	Properties = {
		Radius = 30.0,
		fWanderSpeed = 10.0,
		FunnelEffect = "tornado.tornado.large",
		fCloudHeight = 376.0,
		
		fSpinImpulse = 9.0,
		fAttractionImpulse = 13.0,
		fUpImpulse = 18.0,
	},
	
	Editor={
		--Model="Editor/Objects/T.cgf",
		Icon="Tornado.bmp",
		Category="Environment"
	},
}


function Tornado:OnInit()
	self:OnReset();
end

function Tornado:OnPropertyChange()
	self:OnReset();
end

function Tornado:OnReset()
end

function Tornado:OnShutDown()
end

function Tornado:Event_TargetReached( sender )
end

function Tornado:Event_Enable( sender )
end

function Tornado:Event_Disable( sender )
end

Tornado.FlowEvents =
{
	Inputs =
	{
		Disable = { Tornado.Event_Disable, "bool" },
		Enable = { Tornado.Event_Enable, "bool" },
	},
	Outputs =
	{
		Disable = "bool",
		Enable = "bool",
		TargetReached = "bool",
	},
}
