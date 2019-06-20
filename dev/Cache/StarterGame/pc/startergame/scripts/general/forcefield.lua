local StateMachine = require "scripts/common/StateMachine"

local forcefield =
{
	Properties =
	{
		InitiallyEnabled = { default = true, description = "Whether forcefield should be active by default" },
		MessageIn = {
			EnableMessage = { default = "EnableForcefield", description = "Message to enable/disable the forcefield (needs boolean value)" },
		},
		TransitionDuration = { default = 1.0, description = ""},
		ActivateSound = { default = "", description = "" },
		DeactivateSound = { default = "", description = "" },
	},
	
	States = 
	{
		Inactive = 
		{
            OnEnter = function(self, sm)
				PhysicsComponentRequestBus.Event.DisablePhysics(sm.UserData.entityId);
				MeshComponentRequestBus.Event.SetVisibility(sm.UserData.entityId, false);
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
				PhysicsComponentRequestBus.Event.EnablePhysics(sm.UserData.entityId);
				MeshComponentRequestBus.Event.SetVisibility(sm.UserData.entityId, true);
				StarterGameMaterialUtility.SetSubMtlShaderFloat(sm.UserData.entityId, "opacity", 0, 0.0);
				self.transitionDuration = sm.UserData.Properties.TransitionDuration;
				self.transitionTimer = sm.UserData.Properties.TransitionDuration;
				if (sm.UserData.Properties.ActivateSound ~= "") then
					local varSound = sm.UserData.Properties.ActivateSound;
					AudioTriggerComponentRequestBus.Event.ExecuteTrigger(sm.UserData.entityId, varSound);
				end
            end,
            OnExit = function(self, sm)
            end,            
            OnUpdate = function(self, sm, deltaTime)
				self.transitionTimer = math.max(self.transitionTimer - deltaTime, 0.0);
				local opacity = sm.UserData.OpacityFinal * (1.0 - (self.transitionTimer / self.transitionDuration));
				StarterGameMaterialUtility.SetSubMtlShaderFloat(sm.UserData.entityId, "opacity", 0, opacity);
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
				PhysicsComponentRequestBus.Event.DisablePhysics(sm.UserData.entityId);
				MeshComponentRequestBus.Event.SetVisibility(sm.UserData.entityId, false);
            end,            
            OnUpdate = function(self, sm, deltaTime)
				self.transitionTimer = math.max(self.transitionTimer - deltaTime, 0.0);
				local opacity = sm.UserData.OpacityFinal * (self.transitionTimer / self.transitionDuration);
				StarterGameMaterialUtility.SetSubMtlShaderFloat(sm.UserData.entityId, "opacity", 0, opacity);
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


function forcefield:OnActivate()
	self.enabledEventId = GameplayNotificationId(self.entityId, self.Properties.MessageIn.EnableMessage, "float");
	self.enabledHandler = GameplayNotificationBus.Connect(self, self.enabledEventId);

    self.StateMachine = {}
    setmetatable(self.StateMachine, StateMachine);

	self:SetEnabled(self.Properties.InitiallyEnabled);
	if (self.Properties.InitiallyEnabled) then
		PhysicsComponentRequestBus.Event.EnablePhysics(self.entityId);
		MeshComponentRequestBus.Event.SetVisibility(self.entityId, true);
		-- activate sound also starts a looping component, so play it now as we won't go through activating state
		if (self.Properties.ActivateSound ~= "") then
			local varSound = self.Properties.ActivateSound;
			AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, varSound);
		end
	else
		PhysicsComponentRequestBus.Event.DisablePhysics(self.entityId);
		MeshComponentRequestBus.Event.SetVisibility(self.entityId, false);
	end
	
    self.tickBusHandler = TickBus.Connect(self);
	
	self.materialCloned = false;
end

function forcefield:OnDeactivate()
	if (self.materialCloned) then
		StarterGameMaterialUtility.RestoreOriginalMaterial(self.entityId);
	end
end

function forcefield:OnTick(deltaTime)
	-- material may not be available for cloning immediately, so we keep trying until the clone is successful
	self.materialCloned = StarterGameMaterialUtility.ReplaceMaterialWithClone(self.entityId);
	if (self.materialCloned) then
		local initialState = "Inactive";
		if (self.Properties.InitiallyEnabled) then
			initialState = "Active";
		end
		
		-- get target opacity value from material
		self.OpacityFinal = StarterGameMaterialUtility.GetSubMtlShaderFloat(self.entityId, "opacity", 0);		
		
		self.StateMachine:Start("Forcefield", self.entityId, self, self.States, false, initialState, false);
		-- now that we're initialised, there is no need for any more ticks
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end
end

function forcefield:SetEnabled(enable)
	self.isEnabled = enable;
end

function forcefield:OnEventBegin(value)
	if (GameplayNotificationBus.GetCurrentBusId() == self.enabledEventId) then
		self:SetEnabled(value);
	end
end

return forcefield;