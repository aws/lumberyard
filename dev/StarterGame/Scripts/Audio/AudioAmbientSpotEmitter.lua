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