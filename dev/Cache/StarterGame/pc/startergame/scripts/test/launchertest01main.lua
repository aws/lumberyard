local LauncherTest01Main = 
{
    Properties = 
    {
        UIStartScreenEntity = {default = EntityId()},
        PlayerChangerEntity = {default = EntityId()},
        mainCameraEntity = {default = EntityId()},        
    },
}

function LauncherTest01Main:OnActivate()
    -- list of all possible states (make sure values are unique)
    self.initial = "initial"
    self.startScreen = "start screen"
    self.intoSequence = "into sequence"
    self.cameraSpinning = "camera spinning"
    self.teleport = "teleport"
    self.interact = "interact"
    self.interactWait = "interact wait"
    self.signalComplete = "signal complete"
    self.finished = "finished"

    -- constants
    self.camera_spin_time = 10

    -- markers are various points to teleport to in the level
    self.markers = {}
    self.markers[0] = { x=669, y=1096, z=94, station=false, playerRotZ=0.0}
    self.markers[1] = { x=716, y=1106, z=95, station=false, playerRotZ=0.0}
    self.markers[2] = { x=752, y=1105, z=95, station=false, playerRotZ=0.0}
    self.markers[3] = { x=799.10, y=1091.21, z=89, station=true, playerRotZ=3.0}

    -- The current state info
    self.currentState = self.initial
    self.currentTimeRemaining = 5
    self.stateBegin = 0
    self.nextPositionIndex = 0
    
    self.tickHandler = TickBus.Connect(self, 0)
end

function LauncherTest01Main:OnDeactivate()
    self.tickHandler:Disconnect()
end

function LauncherTest01Main:AdvanceState(state, deplay)
    Debug.Log("Advancing to state: " .. state)
    self.currentState = state
    self.currentTimeRemaining = deplay
    self.stateBegin = self.counter
end

function LauncherTest01Main:OnTick(deltaTime, timePoint)
    if (self.counter == nil) then
        self.counter = 0
    end

    -- initial state
    if self.currentState == self.initial and self.currentTimeRemaining <= 0 then
        self:AdvanceState(self.startScreen, 3)
    -- sitting on the start screen
    elseif self.currentState == self.startScreen and self.currentTimeRemaining <= 0 then
        GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(self.Properties.UIStartScreenEntity, "AnyKey", "float"), 1)
        GameplayNotificationBus.Event.OnEventEnd(GameplayNotificationId(self.Properties.UIStartScreenEntity, "AnyKey", "float"), 1)
        self:AdvanceState(self.intoSequence, 20)
    -- intro sequence
    elseif self.currentState == self.intoSequence and self.currentTimeRemaining <= 0 then
        GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(self.Properties.UIStartScreenEntity, "StopCutscene", "float"), 1)
        GameplayNotificationBus.Event.OnEventEnd(GameplayNotificationId(self.Properties.UIStartScreenEntity, "StopCutscene", "float"), 1)        
        self:AdvanceState(self.cameraSpinning, self.camera_spin_time)
    -- 360 camera spin state
    elseif self.currentState == self.cameraSpinning then
        if self.currentTimeRemaining <= 0 then
            if self.nextPositionIndex <= #self.markers then
                self:AdvanceState(self.teleport, 0)
            else
                self:AdvanceState(self.signalComplete, 0)
            end
        else
            TransformBus.Event.RotateAroundLocalZ(self.Properties.mainCameraEntity, ((self.counter - self.stateBegin) / self.camera_spin_time) * 6.28319)
        end
    -- teleport to position
    elseif self.currentState == self.teleport then
        TransformBus.Event.SetWorldX(self.Properties.PlayerChangerEntity, self.markers[self.nextPositionIndex].x)
        TransformBus.Event.SetWorldY(self.Properties.PlayerChangerEntity, self.markers[self.nextPositionIndex].y)
        TransformBus.Event.SetWorldZ(self.Properties.PlayerChangerEntity, self.markers[self.nextPositionIndex].z)
        TransformBus.Event.RotateAroundLocalZ(self.Properties.PlayerChangerEntity, self.markers[self.nextPositionIndex].playerRotZ)
        if self.markers[self.nextPositionIndex].station then
            self:AdvanceState(self.interact, 3)
        else
            self:AdvanceState(self.cameraSpinning, self.camera_spin_time)
        end
        self.nextPositionIndex = self.nextPositionIndex + 1 
    -- interact with something
    elseif self.currentState == self.interact and self.currentTimeRemaining <= 0 then
        GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(self.Properties.PlayerChangerEntity, "InteractPressed", "float"), 1)
        GameplayNotificationBus.Event.OnEventEnd(GameplayNotificationId(self.Properties.PlayerChangerEntity, "InteractPressed", "float"), 1)
        GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(self.Properties.PlayerChangerEntity, "InteractReleased", "float"), 1)
        GameplayNotificationBus.Event.OnEventEnd(GameplayNotificationId(self.Properties.PlayerChangerEntity, "InteractReleased", "float"), 1)
        self:AdvanceState(self.interactWait, 15)
     -- wait for an interaction
     elseif self.currentState == self.interactWait and self.currentTimeRemaining <= 0 then
         self:AdvanceState(self.cameraSpinning, self.camera_spin_time)
     -- complete the test
     elseif self.currentState == self.signalComplete then
        AutomatedLauncherTestingRequestBus.Broadcast.CompleteTest(true, "Test completed successfully.")
        self:AdvanceState(self.finished, 0)
    -- finished, nothing to do
    elseif self.currentState == self.finished then
        -- nothing to do
    end        

    self.currentTimeRemaining = self.currentTimeRemaining - deltaTime
    self.counter = self.counter + deltaTime
end

return LauncherTest01Main
