
local PlayTestSequence = 
{
    Properties = 
    {
        SequenceEntity = {default = EntityId()},
    },
}

function PlayTestSequence:OnActivate()

	-- If no sequence entity assigned, use our entity
    if (not self.Properties.SequenceEntity:IsValid()) then
        self.Properties.SequenceEntity = self.entityId;
    end

    self.tickHandler = TickBus.Connect(self, 0)
 
    self.sequenceHandler = SequenceComponentNotificationBus.Connect(self, self.Properties.SequenceEntity);  
 
end

function PlayTestSequence:OnDeactivate()
    self.sequenceHandler:Disconnect()
    self.tickHandler:Disconnect()
end

function PlayTestSequence:OnTick(deltaTime, timePoint)
    if (self.counter == nil) then
        self.counter = 0
    end

    if (self.counter == 0) then
        SequenceComponentRequestBus.Event.Play(self.Properties.SequenceEntity)
    end

    self.counter = self.counter + deltaTime    
end

function PlayTestSequence:OnStop(entityId, stopTime)
    AutomatedLauncherTestingRequestBus.Broadcast.CompleteTest(true, "Fly over test completed successfully.");
end

return PlayTestSequence
