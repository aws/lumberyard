local StateMachine = require "scripts/common/StateMachine"

local commsarrayeffects =
{
	Properties =
	{
		InitiallyEnabled = { default = true, description = "Whether comms array should be active by default" },
		MessageIn = {
			EnableMessage = { default = "EnableCommsArray", description = "Message to enable/disable the comms array (needs boolean value)" },
		},
		MainMeshEntity = { default = EntityId(), description = ""},
		BeamMeshEntity = { default = EntityId(), description = ""},
		BeamEffectEntity = { default = EntityId(), description = ""},
		
		MainMeshEmissive1MatIndex = { default = 0, description = "" },
		MainMeshEmissive2MatIndex = { default = 1, description = "" },
		BeamMeshColor1MatIndex = { default = 0, description = "" },
		BeamMeshColor2MatIndex = { default = 1, description = "" },
		
		TransitionDuration = { default = 2, description = ""},
		ActivateSound = { default = "", description = "" },
		DeactivateSound = { default = "", description = "" },
	},
	
	States = 
	{
		Inactive = 
		{
            OnEnter = function(self, sm)
				sm.UserData:HideAll();
			end,
            OnExit = function(self, sm)
            end,            
            OnUpdate = function(self, sm, deltaTime)
            end,
            Transitions =
            {
				Activating = 
				{
                    Evaluate = function(state, sm)
						return sm.UserData.isEnabled;
                    end	
				},
            },
		},
		Active = 
		{
            OnEnter = function(self, sm)
            end,
            OnExit = function(self, sm)
            end,            
            OnUpdate = function(self, sm, deltaTime)
            end,
            Transitions =
            {
				Deactivating = 
				{
                    Evaluate = function(state, sm)
						return not sm.UserData.isEnabled;
                    end	
				},
            },
		},
		Activating = 
		{
            OnEnter = function(self, sm)
				sm.UserData:ShowAll();
				self.transitionDuration = sm.UserData.Properties.TransitionDuration;
				self.transitionTimer = sm.UserData.Properties.TransitionDuration;
				sm.UserData:UpdateTransition(0.0);
				if (sm.UserData.Properties.ActivateSound ~= "") then
					local varSound = sm.UserData.Properties.ActivateSound;
					AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.entityId, varSound);
				end
            end,
            OnExit = function(self, sm)
            end,            
            OnUpdate = function(self, sm, deltaTime)
				self.transitionTimer = math.max(self.transitionTimer - deltaTime, 0.0);
				sm.UserData:UpdateTransition(1.0 - (self.transitionTimer / self.transitionDuration));
            end,
            Transitions =
            {
				Active = 
				{
                    Evaluate = function(state, sm)
						return (state.transitionTimer <= 0.0);
                    end	
				},
            },
		},
		Deactivating = 
		{
            OnEnter = function(self, sm)
				self.transitionDuration = sm.UserData.Properties.TransitionDuration;
				self.transitionTimer = sm.UserData.Properties.TransitionDuration;
				if (sm.UserData.Properties.DeactivateSound ~= "") then
					local varSound = sm.UserData.Properties.DeactivateSound;
					AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.entityId, varSound);
				end
            end,
            OnExit = function(self, sm)
				ParticleComponentRequestBus.Event.Enable(sm.UserData.Properties.BeamEffectEntity, false);
            end,            
            OnUpdate = function(self, sm, deltaTime)
				self.transitionTimer = math.max(self.transitionTimer - deltaTime, 0.0);
				sm.UserData:UpdateTransition(self.transitionTimer / self.transitionDuration);
            end,
            Transitions =
            {
				Inactive = 
				{
                    Evaluate = function(state, sm)
						return (state.transitionTimer <= 0.0);
                    end	
				},
            },
		},
	},
}


function commsarrayeffects:OnActivate()
	self.enabledEventId = GameplayNotificationId(self.entityId, self.Properties.MessageIn.EnableMessage, "float");
	self.enabledHandler = GameplayNotificationBus.Connect(self, self.enabledEventId);

    self.StateMachine = {}
    setmetatable(self.StateMachine, StateMachine);

	self:SetEnabled(self.Properties.InitiallyEnabled);
	if (self.Properties.InitiallyEnabled) then
		self:ShowAll();
	else
		self:HideAll();
	end
	
    self.tickBusHandler = TickBus.Connect(self);
	
	self.mainMeshMaterialCloned = false;
	self.beamMeshMaterialCloned = false;
end

function commsarrayeffects:ShowAll()
	MeshComponentRequestBus.Event.SetVisibility(self.Properties.BeamMeshEntity, true);
	ParticleComponentRequestBus.Event.SetVisibility(self.Properties.BeamEffectEntity, true);
	ParticleComponentRequestBus.Event.Enable(self.Properties.BeamEffectEntity, true);
end

function commsarrayeffects:HideAll()
	MeshComponentRequestBus.Event.SetVisibility(self.Properties.BeamMeshEntity, false);
	ParticleComponentRequestBus.Event.Enable(self.Properties.BeamEffectEntity, false);
	ParticleComponentRequestBus.Event.SetVisibility(self.Properties.BeamEffectEntity, false);
end

function commsarrayeffects:UpdateTransition(amount)
	local emissive = amount * self.MainMeshEmissive1Max;
	StarterGameMaterialUtility.SetSubMtlShaderFloat(self.Properties.MainMeshEntity, "emissive_intensity", self.Properties.MainMeshEmissive1MatIndex, emissive);
	emissive = amount * self.MainMeshEmissive2Max;
	StarterGameMaterialUtility.SetSubMtlShaderFloat(self.Properties.MainMeshEntity, "emissive_intensity", self.Properties.MainMeshEmissive2MatIndex, emissive);

	local color = self.BeamMeshColor1Max * amount;
	StarterGameMaterialUtility.SetSubMtlShaderVec3(self.Properties.BeamMeshEntity, "StartColor", self.Properties.BeamMeshColor1MatIndex, color);
	color = self.BeamMeshColor2Max * amount;
	StarterGameMaterialUtility.SetSubMtlShaderVec3(self.Properties.BeamMeshEntity, "StartColor", self.Properties.BeamMeshColor2MatIndex, color);
end

function commsarrayeffects:OnDeactivate()
	if (self.mainMeshMaterialCloned) then
		StarterGameMaterialUtility.RestoreOriginalMaterial(self.Properties.MainMeshEntity);
	end
	if (self.beamMeshMaterialCloned) then
		StarterGameMaterialUtility.RestoreOriginalMaterial(self.Properties.BeamMeshEntity);
	end
end

function commsarrayeffects:OnTick(deltaTime)
	if (not self.mainMeshMaterialCloned) then
		self.mainMeshMaterialCloned = StarterGameMaterialUtility.ReplaceMaterialWithClone(self.Properties.MainMeshEntity);
	end
	if (not self.beamMeshMaterialCloned) then
		self.beamMeshMaterialCloned = StarterGameMaterialUtility.ReplaceMaterialWithClone(self.Properties.BeamMeshEntity);
	end
	if (self.mainMeshMaterialCloned and self.beamMeshMaterialCloned) then
		local initialState = "Inactive";
		if (self.Properties.InitiallyEnabled) then
			initialState = "Active";
		end
		
		-- get target emissive values from main mesh materials
		self.MainMeshEmissive1Max = StarterGameMaterialUtility.GetSubMtlShaderFloat(self.Properties.MainMeshEntity, "emissive_intensity", self.Properties.MainMeshEmissive1MatIndex);
		self.MainMeshEmissive2Max = StarterGameMaterialUtility.GetSubMtlShaderFloat(self.Properties.MainMeshEntity, "emissive_intensity", self.Properties.MainMeshEmissive2MatIndex);
		
		-- get target color values from beam mesh materials
		self.BeamMeshColor1Max = StarterGameMaterialUtility.GetSubMtlShaderVec3(self.Properties.BeamMeshEntity, "StartColor", self.Properties.BeamMeshColor1MatIndex);
		self.BeamMeshColor2Max = StarterGameMaterialUtility.GetSubMtlShaderVec3(self.Properties.BeamMeshEntity, "StartColor", self.Properties.BeamMeshColor2MatIndex);
		
		self.StateMachine:Start("CommsArrayEffects", self.entityId, self, self.States, false, initialState, false);
		-- now that we're initialised, there is no need for any more ticks
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end
end

function commsarrayeffects:SetEnabled(enable)
	self.isEnabled = enable;
end

function commsarrayeffects:OnEventBegin(value)
	if (GameplayNotificationBus.GetCurrentBusId() == self.enabledEventId) then
		self:SetEnabled(value);
	end
end

return commsarrayeffects;