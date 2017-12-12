local timedexistence =
{
	Properties =
	{
		Lifespan = { default = 2, description = "Time that the entity will exist for.", suffix = " seconds" }
	},
}

function timedexistence:OnActivate()

	--Debug.Log("Activated");
	self.lifeRemaining = self.Properties.Lifespan;
	self.tickBusHandler = TickBus.Connect(self);

end

function timedexistence:OnDeactivate()

	if (self.tickBusHandler ~= nil) then
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end

end

function timedexistence:OnTick(deltaTime, timepoint)

	self.lifeRemaining = self.lifeRemaining - deltaTime;
	if (self.lifeRemaining <= 0.0) then
		--Debug.Log("Deleting " .. tostring(self.entityId));
		self:OnDeactivate();

		GameEntityContextRequestBus.Broadcast.DestroyGameEntity(self.entityId);
	end

end

return timedexistence;