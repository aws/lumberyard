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
LightningArc =
{
	Properties =
	{
		bActive = 1,
		
		Timing = {
			fDelay = 2,
			fDelayVariation = 0.5,
			
		},
		
		Render = {
			ArcPreset = "Default",
		},
	},
	
	Editor =
	{
		Model="Editor/Objects/Particles.cgf",
		Icon="Lightning.bmp",
	},
}


function LightningArc:OnPropertyChange()
	self.lightningArc:ReadLuaParameters();
end



function LightningArc:Event_Strike()
	self.lightningArc:TriggerSpark();
end



function LightningArc:Event_Enable()
	self.lightningArc:Enable(true);
end



function LightningArc:Event_Disable()
	self.lightningArc:Enable(false);
end



function LightningArc:OnStrike(strikeTime, targetEntity)
	self:ActivateOutput("StrikeTime", strikeTime);
	self:ActivateOutput("EntityId", targetEntity.id);
end



LightningArc.FlowEvents =
{
	Inputs =
	{
		Strike = { LightningArc.Event_Strike, "bool" },
		Enable = { LightningArc.Event_Enable, "bool" },
		Disable = { LightningArc.Event_Disable, "bool" },
	},
	Outputs =
	{
		StrikeTime = "float",
		EntityId = "entity",
	},
}
