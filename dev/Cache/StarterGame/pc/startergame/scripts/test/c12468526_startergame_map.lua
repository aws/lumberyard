--
-- Test Rail: C12468526 StarterGame Map Functionality
--
local AutomatedTest =
{
    Properties =
    {},
}

function AutomatedTest:OnActivate()
    -- list of all possible states (make sure values are unique)
    self.initialState = "initial"
    self.startGameState = "start game"
    self.movePlayerState = "move player"
    self.playerScreenShotState = "player screen shot"
    self.signalComplete = "signal complete"
    self.finished = "finished"

    self.forwardEvent = "NavForwardBack"
    self.sideEvent = "NavLeftRight"
    self.turnEvent = "LookLeftRightKM"
    self.weaponPressedEvent = "WeaponFirePressed"
    self.weaponReleasedEvent = "WeaponFireReleased"
    self.flyCamEvent = "ToggleFlyCam"

    -- Move around map
    self.playerMove = {}
    self.playerMove[0] = { event = self.forwardEvent, value = 1, duration = 1.5 }
    self.playerMove[1] = { event = self.forwardEvent, value = 0, duration = 0 }
    self.playerMove[2] = { event = self.turnEvent, value = 2, duration = 10 }
    self.playerMove[3] = { event = self.turnEvent, value = 0, duration = 0 }
    self.playerMove[4] = { event = self.weaponPressedEvent, value = 1, duration = 1 }
    self.playerMove[5] = { event = self.weaponReleasedEvent, value = 1, duration = 0 }
    self.playerMove[6] = { event = self.forwardEvent, value = 1, duration = 2 }
    self.playerMove[7] = { event = self.forwardEvent, value = 0, duration = 0 }
    self.playerMove[8] = { event = self.turnEvent, value = 2, duration = 12.5 }
    self.playerMove[9] = { event = self.turnEvent, value = 0, duration = 0 }
    self.playerMove[10] = { event = self.flyCamEvent, value = 0, duration = 1 }
    self.playerMove[11] = { event = self.forwardEvent, value = 1, duration = 0.5 }
    self.playerMove[12] = { event = self.forwardEvent, value = 0, duration = 0 }
    self.playerMove[13] = { event = self.turnEvent, value = 2, duration = 15 }
    self.playerMove[14] = { event = self.turnEvent, value = 0, duration = 0 }

    self.playerMoveIndex = 0

    -- The current state info
    self.currentState = self.initialState
    self.currentTimeRemaining = 5
    self.stateBegin = 0

    -- FPS variables
    self.total_delta_time = 0
    self.number_frames = 0
    self.minimum_fps = 30.0
    self.frames_below_minimum = 0

    self.tickHandler = TickBus.Connect(self, 0)
end

function AutomatedTest:OnDeactivate()
    self.tickHandler:Disconnect()
end

function AutomatedTest:AdvanceState(state, deplay)
    Debug.Log("[Automated Test] Advancing to state: " .. state)
    self.currentState = state
    self.currentTimeRemaining = deplay
    self.stateBegin = self.counter
end

function AutomatedTest:OnTick(deltaTime, timePoint)
    if (self.counter == nil) then
        self.counter = 0
    end

    -- Calculate FPS
    current_fps = 1.0 / deltaTime
    if current_fps < self.minimum_fps then
        self.frames_below_minimum = self.frames_below_minimum + 1
    end
    self.total_delta_time = self.total_delta_time + deltaTime
    self.number_frames = self.number_frames + 1
    -- Initial State, take screen shot
    if self.currentState == self.initialState and self.currentTimeRemaining <= 0 then
        Debug.Log("[Automated Test] Screen Shot 'C12468526_Start_Screen'")
        self:AdvanceState(self.startGameState, 2)
    -- Press any key screen
    elseif self.currentState == self.startGameState and self.currentTimeRemaining <= 0 then
        InputEventNotificationBus.Event.OnPressed(InputEventNotificationId("AnyKey"), 1)
        self:AdvanceState(self.movePlayerState, 55) -- Wait for long intro sequence to complete
    -- Move the player around the screen
    elseif self.currentState == self.movePlayerState and self.currentTimeRemaining <= 0 then
        InputEventNotificationBus.Event.OnPressed(InputEventNotificationId(self.playerMove[self.playerMoveIndex].event), self.playerMove[self.playerMoveIndex].value)
        if self.playerMoveIndex < #self.playerMove then
            self:AdvanceState(self.movePlayerState, self.playerMove[self.playerMoveIndex].duration)
        else
            self:AdvanceState(self.playerScreenShotState, 5)
        end
        self.playerMoveIndex = self.playerMoveIndex + 1
    -- Take a screen shot
    elseif self.currentState == self.playerScreenShotState and self.currentTimeRemaining <= 0 then
        Debug.Log("[Automated Test] Screen Shot 'C12468526_Player_Move'")
        self:AdvanceState(self.signalComplete, 1)
    elseif self.currentState == self.signalComplete then
        -- Report FPS numbers
        if self.frames_below_minimum > 0 then
            percent = self.frames_below_minimum / self.number_frames * 100.0
            Debug.Log(string.format("[Automated Test] WARNING: FPS Below Minimum %f detected in %d/%d frames %f%%", self.minimum_fps, self.frames_below_minimum, self.number_frames, percent))
        end

        average_fps = self.number_frames / self.total_delta_time
        if average_fps < self.minimum_fps then
            Debug.Log(string.format("[Automated Test] ERROR: Average FPS: %f is below minimum %f", average_fps, self.minimum_fps))
        else
            Debug.Log(string.format("[Automated Test] Average FPS: %f", average_fps))
        end

        -- End the test
        AutomatedLauncherTestingRequestBus.Broadcast.CompleteTest(true, "Test completed successfully.")
        self:AdvanceState(self.finished, 0)
        -- finished, nothing to do
    elseif self.currentState == self.finished then
        -- nothing to do
    end

    self.currentTimeRemaining = self.currentTimeRemaining - deltaTime
    self.counter = self.counter + deltaTime
end

return AutomatedTest
