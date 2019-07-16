local utilities = require "scripts/common/utilities"

local aispawntrigger = {
	Properties = {
		AISpawnGroup = { default = "", description = "The spawn group to be triggered when the player enters the trigger." },
		Switch =
		{
			On = { default = "", description = "The event name when switched on." },
			Off = { default = "", description = "The event name when switched off." },
		},
	},
}

function aispawntrigger:OnActivate()
	self.triggerHandler = TriggerAreaNotificationBus.Connect(self, self.entityId);
	self.enteredAreaId = GameplayNotificationId(EntityId(), "EnteredAITrigger", "float");
	self.exitedAreaId = GameplayNotificationId(EntityId(),"ExitedAITrigger", "float");
	self.somethingDeepInside = false;
	
	-- Initialise the values with defaults first and then set them up for listening, if necessary.
	self.switchedOn = true;
	self.switchedOnEventId = nil;
	self.switchedOnHandler = nil;
	self.switchedOffEventId = nil;
	self.switchedOffHandler = nil;
	local validOn = self:IsValidString(self.Properties.Switch.On);
	local validOff = self:IsValidString(self.Properties.Switch.Off);
	if (validOn and validOff) then
		self.switchedOn = false;
		self.switchedOnEventId = GameplayNotificationId(self.entityId, self.Properties.Switch.On, "float");
		self.switchedOnHandler = GameplayNotificationBus.Connect(self, self.switchedOnEventId);
		self.switchedOffEventId = GameplayNotificationId(self.entityId, self.Properties.Switch.Off, "float");
		self.switchedOffHandler = GameplayNotificationBus.Connect(self, self.switchedOffEventId);
	elseif (validOn or validOff) then
		-- Output a warning that something was set but it'll be ignored.
		Debug.Log("[LuaWarning] AISpawnTrigger '" .. tostring(StarterGameEntityUtility.GetEntityName(self.entityId)) .. "': one of the switches has an event name but the other doesn't. The switch will not be active.");
	end
end

function aispawntrigger:OnDeactivate()
	if (self.switchedOffHandler ~= nil) then
		self.switchedOffHandler:Disconnect();
		self.switchedOffHandler = nil;
	end
	if (self.switchedOnHandler ~= nil) then
		self.switchedOnHandler:Disconnect();
		self.switchedOnHandler = nil;
	end
	if (self.triggerHandler ~= nil) then
		self.triggerHandler:Disconnect();
		self.triggerHandler = nil;
	end
end

function aispawntrigger:IsValidString(str)
	return str ~= "" and str ~= nil;
end

function aispawntrigger:OnTriggerAreaEntered(entityId)
	--Debug.Log("A.I. Trigger Volume entered");
	if (entityId ~= nil) then
		self.somethingDeepInside = true;
	end
	if (not utilities.GetDebugManagerBool("PreventAIDisabling", false) and self.switchedOn) then
		GameplayNotificationBus.Event.OnEventBegin(self.enteredAreaId, self.Properties.AISpawnGroup);
	end
end

function aispawntrigger:OnTriggerAreaExited(entityId)
	--Debug.Log("A.I. Trigger Volume exited");
	if (entityId ~= nil) then
		self.somethingDeepInside = false;
	end
	if (not utilities.GetDebugManagerBool("PreventAIDisabling", false)) then
		GameplayNotificationBus.Event.OnEventBegin(self.exitedAreaId, self.Properties.AISpawnGroup);
	end
end

function aispawntrigger:OnEventBegin(value)
	if (self.switchedOnEventId ~= nil and self.switchedOffEventId ~= nil) then
		if (GameplayNotificationBus.GetCurrentBusId() == self.switchedOnEventId) then
			if (not self.switchedOn) then
				self.switchedOn = true;
				if (self.somethingDeepInside) then
					self:OnTriggerAreaEntered(nil);
				end
			end
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.switchedOffEventId) then
			if (self.switchedOn) then
				self.switchedOn = false;
				if (self.somethingDeepInside) then
					self:OnTriggerAreaExited(nil);
				end
			end
		end
	end
end

return aispawntrigger;