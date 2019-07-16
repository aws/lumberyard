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