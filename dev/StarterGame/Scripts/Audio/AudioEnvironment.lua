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

local audioenvironment =
{
	Properties =
	{
		EnvAmount = "",
	},

	IsActive = false,
}

function audioenvironment:OnActivate()

	-- Set varEnvEmount to the Float value defined in the Properties
	varEnvAmount = self.Properties.EnvAmount;
	
	-- Set Environment to = varEnvEmount
	AudioEnvironmentComponentRequestBus.Event.SetAmount(self.entityId, varEnvAmount);

	-- Debug.Log("SND: Env Amount =  (" .. tostring(varEnvAmount) .. ")");
	
end

return audioenvironment;