local Orbiting = 
{
	Properties = 
	{
		speed =
		{
			default = 10,
			description = "speed of orbit",
			suffix = " deg/sec",
		}
	}
}

function Orbiting:OnActivate()

	Debug.Log("Orbiting Activated")
	--self.tickHandler = TickBus.Connect(self)  "This is a shorthand to connect to TickBus instead of calling CreateHandler"
	self.tickHandler = TickBus.CreateHandler(self)
	self.tickHandler:Connect()
end

function Orbiting:OnDeactivate()
	self.tickHandler:Disconnect()
end

function Orbiting:OnTick(deltaTime,timePoint)
	TransformBus.Event.RotateAroundLocalZ(self.entityId, self.Properties.speed*deltaTime)
end

return Orbiting