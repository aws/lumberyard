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

local audioambientspotemitter =
{
	Properties =
	{
		RtpcValue = "",
	},

	IsActive = false,
}

function audioambientspotemitter:OnActivate()

	-- Play the specified Audio Trigger (wwise event) on this component
	AudioTriggerComponentRequestsBus.Event.Play(self.entityId);
	
	-- Set varRtpcValue to the RTPC value defined in the Properties
	varRtpcValue = self.Properties.RtpcValue;
	
	if(varRtpcValue == "") then
		Debug.Log("SND: NO RTPC TO SET");
	else
		-- Tell Wwise to set the value of the RTPC
		AudioRtpcComponentRequestsBus.Event.SetValue(self.entityId, varRtpcValue);
	end
		
end

return audioambientspotemitter;