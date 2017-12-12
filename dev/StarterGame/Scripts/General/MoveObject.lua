local moveobject =
{
	Properties =
	{
		EventName = { default = "EventName", description = "Name of the event to trigger the movement." },
		Target = { default = EntityId() },
		
		NewPosition =
		{
			PositionOffset = { default = Vector3(0.0, 0.0, 0.0), description = "When triggered, move the entity to this position relative to it's parent" },
			Duration = { default = 1.0, description = "When triggered, take this long to move the entity to it's new position" },
		},
		
		LerpType =
		{
			Linear = { default = true },
			Exponential = { default = false },
		},
	},
}

function moveobject:OnActivate()

	self.triggerEventId = GameplayNotificationId(self.entityId, self.Properties.EventName, "float");
	self.triggerHandler = GameplayNotificationBus.Connect(self, self.triggerEventId);
	
	self.tickHandler = TickBus.Connect(self);
	self.performedFirstUpdate = false;
	
	if (not self.Properties.Target:IsValid()) then
		self.Properties.Target = self.entityId;
	end
	
	self.isMoving = false;
	self.originalTM = nil;
	
	self.targetPos = self.Properties.NewPosition.PositionOffset; 
	--Debug.Log("Target pos: " .. tostring(self.targetPos));

end

function moveobject:OnDeactivate()

	if (self.tickHandler ~= nil) then
		self.tickHandler:Disconnect();
		self.tickHandler = nil;
	end

	if (self.triggerHandler ~= nil) then
		self.triggerHandler:Disconnect();
		self.triggerHandler = nil;
	end

end

function moveobject:OnTick(deltaTime, timePoint)

	if (self.performedFirstUpdate == false) then
		self.performedFirstUpdate = true;
	end
	
	if (self.isMoving) then
		local tm = TransformBus.Event.GetLocalTM(self.Properties.Target);
		local newPos = nil;
		self.timeSpent = self.timeSpent + deltaTime;
		if (self.timeSpent >= self.Properties.NewPosition.Duration) then
			newPos = self.targetPos;
			self.isMoving = false;
		else
			local l = self.timeSpent / self.Properties.NewPosition.Duration;
			
			newPos = self:LerpThis(self.originalTM:GetTranslation(), self.targetPos, l);
		end
		tm:SetTranslation(newPos);
		--Debug.Log("New position: " .. tostring(newPos));
		TransformBus.Event.SetLocalTM(self.Properties.Target, tm);
	end

end

-- s == start
-- e == end
-- l == lerp
-- This function expects 's' and 'e' to be 'Vector3's.
function moveobject:LerpThis(s, e, l)

	local ret;
	if (self.Properties.LerpType.Linear == true) then
		ret = s:Lerp(e, l);
	elseif (self.Properties.LerpType.Exponential == true) then
		ret = s:Lerp(e, l * l);
	else
		Debug.Assert("Unspecified lerp type.");
	end
	
	return ret;

end

function moveobject:OnEventBegin(value)

	if (GameplayNotificationBus.GetCurrentBusId() == self.triggerEventId) then
		self.isMoving = true;
		self.timeSpent = 0.0;
		self.originalTM = TransformBus.Event.GetLocalTM(self.Properties.Target);
		--Debug.Log("Moving");
	end

end

return moveobject;