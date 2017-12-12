local rotateobject =
{
	Properties =
	{		
		GraphicTurnTime = { default = 3, description = "Time to rotate a full rotation" },
	},
}

function rotateobject:OnActivate()
	self.tickBusHandler = TickBus.Connect(self);
		
	self.rotateTimer = 0;	
end

function rotateobject:OnDeactivate()		
	if(not self.tickBusHandler == nil) then
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end
end

function rotateobject:OnTick(deltaTime, timePoint)

	local twoPi = 6.283185307179586476925286766559;

	if(not (self.Properties.GraphicTurnTime == 0)) then
		self.rotateTimer = self.rotateTimer + (deltaTime / self.Properties.GraphicTurnTime);
		if(self.rotateTimer >= twoPi) then
			self.rotateTimer = self.rotateTimer - twoPi;
		end
	end
	
	
	--Debug.Log("Pickup graphic values:  Rotation: " .. self.rotateTimer .. " Height: " .. self.bobTimer);
	local tm = Transform.CreateRotationZ(self.rotateTimer);
	TransformBus.Event.SetLocalTM(self.entityId, tm);

end

return rotateobject;