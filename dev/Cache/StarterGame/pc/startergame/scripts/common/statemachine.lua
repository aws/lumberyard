--------------------------------------------------
-- Generic State Machine Implementation
-- Everything from here on will be moved to a reusable
-- library that scripts can leverage.
--------------------------------------------------
local utilities = require "scripts/common/utilities"

local StateMachine =
{
}
StateMachine.__index = StateMachine;

function StateMachine:Start(name, entityId, userData, statesTable, stateChangeEvents, initialStateName, isDebuggingEnabled)
	self.Name = name;
    self.EntityId = entityId;
    self.UserData = userData;
    self.States = statesTable;
	self.StateChangeEvents = stateChangeEvents;
	self:CreateSortedTransitionKeys(statesTable);
    self.IsDebuggingEnabled = isDebuggingEnabled;
	
    -- State machine needs to tick to evaluate transitions.
    self.tickBusHandler = TickBus.Connect(self);
	
    -- State machine needs to play fragments.
    self.animationEventBusHandler = CharacterAnimationNotificationBus.Connect(self, self.EntityId);
	
    -- Jump to initial state if specified.
    if (initialStateName ~= nil) then
        self:GotoState(initialStateName);
    end
end
-- Store a sorted list of transition keys in each state so we can control the order transition functions are called. This allows us to remove redundant code in evaluate functions
function StateMachine:CreateSortedTransitionKeys(states)
	for stateKey, stateTable in pairs(states) do
		local priorityKeys = {};
		local miscKeys = {};
        for transKey, transTable in pairs(stateTable.Transitions) do
			if (transTable.Priority ~= nil) then
				if (priorityKeys[transTable.Priority] == nil) then
					table.insert(priorityKeys, transTable.Priority, transKey);
				else
					Debug.Warning("[StateMachine " .. tostring(self.Name) .. "] Has two states with the same priority: " .. tostring(priorityKeys[transTable.Priority]) .. " and " .. tostring(transKey));
				end
			else
				table.insert(miscKeys, transKey);
			end
		end
		table.sort(miscKeys);
		
		local sortedKeys = {};
		for transKey, transTable in pairs(priorityKeys) do
			if (transTable ~= nil) then
				table.insert(sortedKeys, transTable);
			end
		end
		for transKey, transTable in pairs(miscKeys) do
			table.insert(sortedKeys, transTable);
		end
		stateTable.Transitions.SortedKeys = sortedKeys;
	end
end
function StateMachine:Stop()
	if(self.tickBusHandler ~= nil) then
    	self.tickBusHandler:Disconnect();
    	self.tickBusHandler = nil;
	end
end

function StateMachine:Update(deltaTime)
   
    if (self.CurrentState == nil) then
        return
    end
    -- Check conditions for the current state's outgoing transitions.
    if (self.CurrentState.Transitions ~= nil) then
        for i, transKey in ipairs(self.CurrentState.Transitions.SortedKeys) do
			local transTable = self.CurrentState.Transitions[transKey];
            if (transTable) then
                local result = transTable.Evaluate(self.CurrentState, self);
                if (result == true) then
                    self:GotoState(transKey);
                    return;
                end
            end
        end
    end
    
    -- Call current state's update if bound.
    if (self.CurrentState.OnUpdate ~= nil) then
        self.CurrentState:OnUpdate(self, deltaTime);
    end
   
end

function StateMachine:GotoState(targetStateName)
    
    if (self.States == nil) then
        return
    end
    
    for stateKey, stateTable in pairs(self.States) do
        if (tostring(stateKey) == targetStateName) then
            if (self.CurrentState ~= nil) then
                -- Unbind any input events we were monitoring from the previous state.
                if (self.CurrentState.InputListeners ~= nil) then
                    for inputKey, inputTable in pairs(self.CurrentState.InputListeners) do
                        if (eventTable.EventHandler ~= nil) then
                            eventTable.EventHandler:Disconnect();
                        end
                    end
                end
                self.CurrentState.InputListeners = nil;
            
                -- Invoke previous state's OnExit handler.
                if (self.CurrentState.OnExit ~= nil) then
                    self.CurrentState:OnExit(self, targetStateName);
                end
				-- Send an event signifying the old state being exited.
				self:SendStateChangeEvent("Exited" .. tostring(self.CurrentStateName));
            end
            
            -- Invoke new state's OnEnter handler.
            if (stateTable.OnEnter ~= nil) then
                stateTable:OnEnter(self, self.CurrentStateName);
            end
			-- Send an event signifying the new state being entered.
			self:SendStateChangeEvent("Entered" .. tostring(targetStateName));
			
            -- Identify any input conditions in the new state's transitions and register event handlers.
            if (stateTable.Transitions ~= nil) then
                for transKey, transTable in pairs(stateTable.Transitions) do
                    transTable.InputListeners = {};
                end
                for transKey, transTable in pairs(stateTable.Transitions) do
                    if (transTable.InputEvent ~= nil) then
                        local listenerCount = table.getn(transTable.InputListeners);
                        transTable.InputListeners[listenerCount+1] = {};
                        local sm = self;
                        transTable.InputListeners[listenerCount+1].OnEventBegin = function(value)
						sm:GotoState(tostring(transKey));
					end
					transTable.InputListeners[listenerCount+1].EventHandler =
						GameplayNotificationBus.Connect(transTable.InputListeners[listenerCount+1], GameplayNotificationId(self.EntityId, transTable.InputEvent, "float"));
                    end
                end
            end
			
            if (self.IsDebuggingEnabled) then
                Debug.Log("[StateMachine " .. tostring(self.Name) .. "] Successfully transitioned: " .. targetStateName);
            end
            
			-- Send an event signifying the state change.
			self:SendStateChangeEvent(tostring(self.CurrentStateName) .. "To" .. tostring(targetStateName));
			
            self.CurrentState = stateTable;
			self.CurrentStateName = stateKey;
            
            return
        end
    end
    
    if (self.IsDebuggingEnabled) then
        Debug.Log("[StateMachine " .. tostring(self.Name) .. ":" .. tostring(self.EntityId) .. "] Failed to find state: " .. targetStateName);
    end
end

function StateMachine:SendStateChangeEvent(eventName)
	if (self.StateChangeEvents) then
		local eventId = GameplayNotificationId(self.EntityId, eventName, "float");
		GameplayNotificationBus.Event.OnEventBegin(eventId, eventName);
		
		if (self.IsDebuggingEnabled) then
			Debug.Log("Sending " .. tostring(eventName) .. " to " .. tostring(StarterGameEntityUtility.GetEntityName(self.EntityId)) .. " " .. tostring(self.EntityId));
		end
	end
end

function StateMachine:OnAnimationEvent(event)
	--Debug.Log("got animation event: "..event.name);
	if(self.CurrentState.OnAnimationEvent ~= nil) then
    	self.CurrentState:OnAnimationEvent(event.name, event.time);
	end
end

function StateMachine:OnEmotionAnimationEvent(event)
	if(self.CurrentState.OnEmotionAnimationEvent ~= nil) then
    	self.CurrentState:OnEmotionAnimationEvent(event, self);
	end
end

function StateMachine:OnTick(deltaTime, timePoint)
    self:Update(deltaTime);
end

function StateMachine:Resume()
	self.tickBusHandler = TickBus.Connect(self);
end

function StateMachine:EnsureFragmentPlaying(requestId, priority, fragmentName, fragmentTags, isPersistent)
    if (utilities.CheckFragmentPlaying(self.EntityId, requestId) == true) then
		return requestId;
    end
    --Debug.Log("queing fragment "..fragmentName .. " EntityId:" .. tostring(self.EntityId))
	return MannequinRequestsBus.Event.QueueFragment(self.EntityId, priority, fragmentName, fragmentTags, isPersistent);
end

function StateMachine:EnsureFragmentStopped(requestId)
    if (requestId) then
        MannequinRequestsBus.Event.ForceFinishRequest(self.EntityId, requestId);
    end
    return nil;
end

return StateMachine;