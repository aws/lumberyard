----------------------------
	--Animation Script--
----------------------------
--	The following script will allow user input to drive the animation graph

local LRV_Anim = 
{
	Properties =
	{
		moveSpeed = 5.0,
		turnSpeed = 5.0,
	},
}

function LRV_Anim:OnActivate()
	-- General variables
	self.move = 0;
	self.turn = 0;

	-- Listen for general actor events
	self.actorEventHandler = ActorNotificationBus.Connect(self, self.entityId);
	
	-- Listen for engine tick events
	self.tickBusHandler = TickBus.Connect(self);

	-- Listen for input events and connect to busses
	self.forwardBackInputBusId = InputEventNotificationId("Move");
	self.forwardBackInputBus = InputEventNotificationBus.Connect(self, self.forwardBackInputBusId);
	self.turnLeftRigthInputBusId = InputEventNotificationId("Turn");
	self.turnLeftRigthInputBus = InputEventNotificationBus.Connect(self, self.turnLeftRigthInputBusId);
	
end

function LRV_Anim:OnDeactivate()
	if(self.tickBusHandler) then
		self.tickBusHandler:Disconnect();
	end
	if(self.actorEventHandler) then
		self.actorEventHandler:Disconnect();
	end
	if(self.forwardBackInputBus) then
		self.forwardBackInputBus:Disconnect();
	end
	if(self.turnLeftRigthInputBus) then
		self.turnLeftRigthInputBus:Disconnect();
	end
end


function LRV_Anim:HandleInputValue(floatValue)
	local currentBusId = InputEventNotificationBus.GetCurrentBusId()
	if(currentBusId == self.forwardBackInputBusId) then
		-- self.move = floatValue;
		AnimGraphComponentRequestBus.Event.SetNamedParameterFloat(self.entityId, "Speed", floatValue);
	end
	if(currentBusId == self.turnLeftRigthInputBusId) then
		
		self.turn = floatValue;	
	end
end

-- called by the ActorNotificationBus when a key is first pressed
function LRV_Anim:OnPressed(floatValue)
	self:HandleInputValue(floatValue);
end

-- called by the ActorNotificationBus when a key is held
function LRV_Anim:OnHeld(floatValue)
	self:HandleInputValue(floatValue);
end

-- called by the ActorNotificationBus when a key is released
function LRV_Anim:OnReleased(floatValue)
	self:HandleInputValue(floatValue)
end

function LRV_Anim:PlayAnimation()
	
	Debug.Log("move in Play Animation: ".. self.move)
	-- AnimGraphComponentRequestBus.Event.SetNamedParameterFloat(self.entityId, "Speed", self.move);
	--AnimGraphCompnentRequestBus.Event.SetNamedParameterFloat(self.entityId, "Turn", self.turn)

end

function LRV_Anim:OnTick(deltaTime, timePoint)
	if(deltaTime > 0) then
		--self:PlayAnimation()
	end
end

return LRV_Anim