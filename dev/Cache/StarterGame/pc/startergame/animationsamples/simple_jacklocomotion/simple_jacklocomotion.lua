----------------------------------------------------------------------------------------------------
--
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--
--
----------------------------------------------------------------------------------------------------
local EmotionFXTests =
{
    Properties =
    {
        --how far the camera is from the player
        CameraDistance = 3.0,
        --speed the character moves
        SpeedMagnitude = 10.0,
        --how quickly the player snaps to the new target direction as determined by the left stick/WASD
        LerpSpeed = 0.1,
        --how quickly the camera moves with the right stick
        CameraSensitivity = 2.0,
        --ID in the editor of the camera object
        CameraEntity = { default=EntityId() },
        --how quickly the player 
        LeanAmount = 0.8,
        --enable debug visualization in viewport
        Debug = true,
    },
}

function EmotionFXTests:OnActivate()
    --navigation
    self.moveForwardBack = 0;
    self.moveLeftRight = 0;
    self.cameraUpDownPower = 0;
    self.cameraLeftRightPower = 0;
    self.cameraUpDown = 0;
    self.cameraLeftRight = 0;
    --toggling aim animation
    self.aimAmount = 0;
    --for toggling looking at the camera by the player character
    self.lookAtValue = 1.0;
    self.lookAtEnabled = false;
    --flags for inverting camera, false - no inverting, true - invert
    self.invertVertical = false;
    self.invertHorizontal = false;
    
    -- Listen for input events we care about. How the events are generated is configured in the Input component's assigned inputbindings.
    self.forwardBackInputBusId = InputEventNotificationId("NavForwardBack");
    self.forwardBackInputBus = InputEventNotificationBus.Connect(self, self.forwardBackInputBusId);
    self.leftRightInputBusId = InputEventNotificationId("NavLeftRight");
    self.leftRightInputBus = InputEventNotificationBus.Connect(self, self.leftRightInputBusId);
    self.camUpDownInputBusId = InputEventNotificationId("CameraUpDown");
    self.camUpDownInputBus = InputEventNotificationBus.Connect(self, self.camUpDownInputBusId);
    self.camLeftRightInputBusId = InputEventNotificationId("CameraLeftRight");
    self.camLeftRightInputBus = InputEventNotificationBus.Connect(self, self.camLeftRightInputBusId);
    self.aimInputBusId = InputEventNotificationId("Aim");
    self.aimInputBus = InputEventNotificationBus.Connect(self, self.aimInputBusId);
    self.toggleLookAtBusId = InputEventNotificationId("ToggleLookAt");
    self.toggleLookAtBus = InputEventNotificationBus.Connect(self, self.toggleLookAtBusId);

    -- Listen for general actor events
    self.actorEventHandler = ActorNotificationBus.Connect(self, self.entityId);

    -- Listen for engine tick events.
    self.tickBusHandler = TickBus.Connect(self);

    -- Tell the Actor component to draw the root if the flag is set on the script.
    if (self.Properties.Debug) then
        ActorComponentRequestBus.Event.DebugDrawRoot(self.entityId, false);
    end
    
end

function EmotionFXTests:ApplyNavForwardBackValue(floatValue)
    self.moveForwardBack = floatValue;
end

function EmotionFXTests:ApplyNavLeftRightValue(floatValue)
    self.moveLeftRight = floatValue;
end
 
function EmotionFXTests:ApplyCameraUpDown(floatValue)
    --make sure the value is reasonable
    floatValue = math.min(math.max(-5,floatValue),5);
    self.cameraUpDownPower =  floatValue * self.Properties.CameraSensitivity;
end

function EmotionFXTests:ApplyCameraLeftRight(floatValue)
    floatValue = math.min(math.max(-5,floatValue),5);
    self.cameraLeftRightPower = floatValue * self.Properties.CameraSensitivity;
end

function EmotionFXTests:ApplyAim(floatValue)
    self.aimAmount = floatValue * 2.0;
    if (self.aimAmount > 1.0) then
        self.aimAmount = 1.0;
    end
end

--determine where the input is coming from and respond 
function EmotionFXTests:HandleInputValue(floatValue)
    if (InputEventNotificationBus.GetCurrentBusId() == self.forwardBackInputBusId) then
        self:ApplyNavForwardBackValue(floatValue);
    elseif (InputEventNotificationBus.GetCurrentBusId() == self.leftRightInputBusId) then
        self:ApplyNavLeftRightValue(floatValue);
    elseif (InputEventNotificationBus.GetCurrentBusId() == self.camUpDownInputBusId) then
        self:ApplyCameraUpDown(floatValue);
    elseif (InputEventNotificationBus.GetCurrentBusId() == self.camLeftRightInputBusId) then
        self:ApplyCameraLeftRight(floatValue);
    elseif (InputEventNotificationBus.GetCurrentBusId() == self.aimInputBusId) then
        self:ApplyAim(floatValue);
    end
end

--called by the ActorNotificationBus when a key is first pressed
function EmotionFXTests:OnPressed(floatValue)
    if (InputEventNotificationBus.GetCurrentBusId() == self.toggleLookAtBusId) then 
        self.lookAtEnabled = not self.lookAtEnabled;
        self.lookAtValue = 0.0;
    else
        self:HandleInputValue(floatValue);
    end
end

--called by the ActorNotificationBus when a key is held
function EmotionFXTests:OnHeld(floatValue)
    self:HandleInputValue(floatValue);
end

--called by the ActorNotificationBus when a key is released
function EmotionFXTests:OnReleased(floatValue)
    self:HandleInputValue(0.0);
end

--called by the TickBus
function EmotionFXTests:OnTick(deltaTime, timePoint)
    if (deltaTime > 0) then
        self:UpdateCamera(deltaTime);
        self:UpdateLocomotion(deltaTime);
    end
end

function EmotionFXTests:UpdateCamera(deltaTime)
    if (not self.Properties.CameraEntity) then return end;
    --update params based on input events
    self.cameraUpDown = self.cameraUpDown + self.cameraUpDownPower * deltaTime;
    self.cameraLeftRight = self.cameraLeftRight + self.cameraLeftRightPower * deltaTime;
    --get the current player's transform in world space
    local playerTm = TransformBus.Event.GetWorldTM(self.entityId);
    --calculate the transformation matrix of the camera: translate the camera along the look vector by some constant, and raise it a little bit
    local relCameraTm = Transform.CreateIdentity();
    relCameraTm:SetTranslation(relCameraTm:GetTranslation() - relCameraTm:GetColumn(1) * self.Properties.CameraDistance + Vector3(0, 0, 1.5));
    --extract the translation component of the player's transformation matrix
    local playerTranslationTm = Transform.CreateTranslation(playerTm:GetTranslation());
    --taking into account camera movement inverting flags
    local cameraVerticalRotation = Transform.CreateRotationX(-self.cameraUpDown);
    if self.invertVertical then cameraVerticalRotation = Transform.CreateRotationX(self.cameraUpDown) end;
    local cameraHorizontalRotation = Transform.CreateRotationZ(-self.cameraLeftRight);
    if self.invertHorizontal then cameraHorizontalRotation = Transform.CreateRotationZ(self.cameraLeftRight) end;
    --calculate the camera matrix as some offset from the player's current location
    local cameraTm = playerTranslationTm * cameraHorizontalRotation * cameraVerticalRotation * relCameraTm;
    --update the camera position
    TransformBus.Event.SetWorldTM(self.Properties.CameraEntity, cameraTm);
    --update the anim graph
    AnimGraphComponentRequestBus.Event.SetNamedParameterVector3(self.entityId, "LookAtTarget", cameraTm:GetTranslation());
    
end

function EmotionFXTests:UpdateLocomotion(deltaTime)

    self.turnSpeed = 0;
    self.moveSpeed = 0;
    --update the graph 
    local cameraTm = Transform.CreateIdentity();
    if (self.Properties.CameraEntity) then
        cameraTm = TransformBus.Event.GetWorldTM(self.Properties.CameraEntity);
        AnimGraphComponentRequestBus.Event.SetNamedParameterFloat(self.entityId, "LookAtPower", 1.0);
        AnimGraphComponentRequestBus.Event.SetNamedParameterVector3(self.entityId, "LookAtTarget", cameraTm:GetTranslation());
    else
        AnimGraphComponentRequestBus.Event.SetNamedParameterFloat(self.entityId, "LookAtPower", 1.0);
        AnimGraphComponentRequestBus.Event.SetNamedParameterVector3(self.entityId, "LookAtTarget", Vector3.CreateZero());
    end
    
    -- Align with camera.
    local playerTm = TransformBus.Event.GetWorldTM(self.entityId);
    if (self.moveLeftRight ~= 0 or self.moveForwardBack ~= 0) then
        --calculate the angle of the player, camera, and target in the x-y plane
        local playerFacing = playerTm:GetColumn(1);
        local currentAngle = math.atan2(-playerFacing.x, playerFacing.y);
        local cameraFacing = cameraTm:GetColumn(1);
        local cameraAngle = math.atan2(-cameraFacing.x, cameraFacing.y);        
        local targetAngle = math.atan2(-self.moveLeftRight, self.moveForwardBack);
        local twoPi = math.pi * 2.0;
        targetAngle = targetAngle + cameraAngle;
        --convert angles to range -2pi <= x <= 2pi
        targetAngle = targetAngle - 2 * math.pi * math.floor((targetAngle+math.pi)/(2*math.pi))
        local targetRotation = Quaternion.CreateRotationZ(targetAngle);
        local currentRotation = Quaternion.CreateFromTransform(playerTm);
        currentRotation = currentRotation:Slerp(targetRotation, self.Properties.LerpSpeed);
        local newPlayerTm = Transform.CreateFromQuaternionAndTranslation(currentRotation, playerTm:GetTranslation());
        newPlayerTm:Orthogonalize();
        --update the world position
        TransformBus.Event.SetWorldTM(self.entityId, newPlayerTm);
        --check the controls
        --take the delta and make sure its in the right period
        local delta = targetAngle - currentAngle;
        delta = delta - 2 * math.pi * math.floor((delta+math.pi)/(2*math.pi))
        self.turnSpeed = (-(delta) / deltaTime) * self.Properties.LeanAmount;
        local stickMagnitude = Vector2(self.moveLeftRight, self.moveForwardBack):GetLength();
        self.moveSpeed = stickMagnitude * self.Properties.SpeedMagnitude;
    end

    --look at the camera if we should by updating the anim graph
    if(self.lookAtEnabled) then
        AnimGraphComponentRequestBus.Event.SetNamedParameterFloat(self.entityId, "LookAtPower", math.min(1,self.lookAtValue));
    else 
        AnimGraphComponentRequestBus.Event.SetNamedParameterFloat(self.entityId, "LookAtPower", math.max(0,1.0 - self.lookAtValue));
    end
    self.lookAtValue = self.lookAtValue + deltaTime * 3.0;
    --update anim graph with new parameters
    AnimGraphComponentRequestBus.Event.SetNamedParameterFloat(self.entityId, "Aim", self.aimAmount);
    AnimGraphComponentRequestBus.Event.SetNamedParameterFloat(self.entityId, "TurnSpeed", self.turnSpeed);
    AnimGraphComponentRequestBus.Event.SetNamedParameterFloat(self.entityId, "Speed", self.moveSpeed);
end

function EmotionFXTests:OnDeactivate()
    if (self.tickBusHandler) then
        self.tickBusHandler:Disconnect();
    end
    if (self.actorEventHandler) then
        self.actorEventHandler:Disconnect();
    end
end

-- Example of receiving motion events. In this case we're just printing them.
function EmotionFXTests:OnMotionEvent(eventInfo)
    Debug.Log("Received motion event: " .. eventInfo.eventTypeName);
end

-- Examples of monitoring for motion playback events.
function EmotionFXTests:OnMotionLoop(motionName)
    Debug.Log("Looped motion: " .. motionName);
end

-- Examples of monitoring for state changes.
function EmotionFXTests:OnStateEntering(stateName)
    Debug.Log("Entering animation graph state: " .. stateName);
end
function EmotionFXTests:OnStateEntered(stateName)
    Debug.Log("Entered animation graph state: " .. stateName);
end
function EmotionFXTests:OnStateExiting(stateName)
    Debug.Log("Exiting animation graph state: " .. stateName);
end
function EmotionFXTests:OnStateExited(stateName)
    Debug.Log("Exited animation graph state: " .. stateName);
end
function EmotionFXTests:OnStateTransitionStart(from, to)
    Debug.Log("Transitioning: " .. from .. " -> " .. to);
end

return EmotionFXTests;
