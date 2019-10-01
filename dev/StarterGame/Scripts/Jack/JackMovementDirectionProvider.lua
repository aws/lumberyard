--[[
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--]]

local jackmovementdirectionprovider = 
{
    Properties = 
    {
        Camera = {default = EntityId()},
        WalkSpeedMutiplier = { default = 0.25, description = "Multiplier for speed while walking.", },      
        Events =
        {
            ControlsEnabled = { default = "EnableControlsEvent", description = "If passed '1.0' it will enable controls, oherwise it will disable them." },
        },
    },
    InputValues = 
    {
        NavForwardBack = 0.0,
        NavLeftRight = 0.0,
    }
}

function jackmovementdirectionprovider:OnActivate() 

    -- Input listeners (movement).
    self.navForwardBackEventId = GameplayNotificationId(self.entityId, "NavForwardBack", "float");
    self.navForwardBackHandler = GameplayNotificationBus.Connect(self, self.navForwardBackEventId);
    self.navLeftRightEventId = GameplayNotificationId(self.entityId, "NavLeftRight", "float");
    self.navLeftRightHandler = GameplayNotificationBus.Connect(self, self.navLeftRightEventId);

    self.navForwardBackKMEventId = GameplayNotificationId(self.entityId, "NavForwardBackKM", "float");
    self.navForwardBackKMHandler = GameplayNotificationBus.Connect(self, self.navForwardBackKMEventId);
    self.navLeftRightKMEventId = GameplayNotificationId(self.entityId, "NavLeftRightKM", "float");
    self.navLeftRightKMHandler = GameplayNotificationBus.Connect(self, self.navLeftRightKMEventId);
    
    self.navWalkPressedEventId = GameplayNotificationId(self.entityId, "NavWalkPressed", "float");
    self.navWalkPressedHandler = GameplayNotificationBus.Connect(self, self.navWalkPressedEventId);
    self.navWalkReleasedEventId = GameplayNotificationId(self.entityId, "NavWalkReleased", "float");
    self.navWalkReleasedHandler = GameplayNotificationBus.Connect(self, self.navWalkReleasedEventId);

    -- Tick needed to detect aim timeout
    self.tickBusHandler = TickBus.Connect(self);

    self.controlsDisabledCount = 0;
    self.controlsEnabled = true;
    self.controlsEnabledEventId = GameplayNotificationId(self.entityId, self.Properties.Events.ControlsEnabled, "float");
    self.controlsEnabledHandler = GameplayNotificationBus.Connect(self, self.controlsEnabledEventId);
    
    self.WalkEnabled = false;
    self.WalkToggleHeld = false;

end   

function jackmovementdirectionprovider:OnDeactivate()

    self.navForwardBackHandler:Disconnect();
    self.navForwardBackHandler = nil;
    self.navLeftRightHandler:Disconnect();
    self.navLeftRightHandler = nil;
    
    self.navForwardBackKMHandler:Disconnect();
    self.navForwardBackKMHandler = nil;
    self.navLeftRightKMHandler:Disconnect();
    self.navLeftRightKMHandler = nil;
    
    self.navWalkPressedHandler:Disconnect();
    self.navWalkPressedHandler = nil;
    self.navWalkReleasedHandler:Disconnect();
    self.navWalkReleasedHandler = nil;
    self.tickBusHandler:Disconnect();
    self.tickBusHandler = nil;
    self.controlsEnabledHandler:Disconnect();
    self.controlsEnabledHandler = nil; 
end
    
function jackmovementdirectionprovider:SetControlsEnabled(newControlsEnabled)
    self.controlsEnabled = newControlsEnabled;
    if (not newControlsEnabled) then
        self.InputValues.NavForwardBack = 0.0;
        self.InputValues.NavLeftRight = 0.0;
        self.WalkEnabled = false;
    end 
end

function jackmovementdirectionprovider:OnEventBegin(value)
    
    if (GameplayNotificationBus.GetCurrentBusId() == self.controlsEnabledEventId) then
        --Debug.Log("controlsEnabledEventId " .. tostring(value));
        if (value == 1.0) then
            self.controlsDisabledCount = self.controlsDisabledCount - 1;
        else
            self.controlsDisabledCount = self.controlsDisabledCount + 1;
        end
        if (self.controlsDisabledCount < 0) then
            Debug.Log("[Warning] JackMovementDirectionProvider: controls disabled ref count is less than 0.  Probable enable/disable mismatch.");
            self.controlsDisabledCount = 0;
        end     
        local newEnabled = self.controlsDisabledCount == 0;
        if (self.controlsEnabled ~= newEnabled) then
            self:SetControlsEnabled(newEnabled);
        end
    end
    
    if (self.controlsEnabled) then
        if (GameplayNotificationBus.GetCurrentBusId() == self.navForwardBackEventId) then  
            self.InputValues.NavForwardBack = value;
        elseif (GameplayNotificationBus.GetCurrentBusId() == self.navLeftRightEventId) then
            self.InputValues.NavLeftRight = value;
        elseif (GameplayNotificationBus.GetCurrentBusId() == self.navForwardBackKMEventId) then  
            if(self.WalkEnabled) then
                self.InputValues.NavForwardBack = value * self.Properties.WalkSpeedMutiplier;
            else
                self.InputValues.NavForwardBack = value;
            end
        elseif (GameplayNotificationBus.GetCurrentBusId() == self.navLeftRightKMEventId) then
            if(self.WalkEnabled) then
                self.InputValues.NavLeftRight = value * self.Properties.WalkSpeedMutiplier;
            else
                self.InputValues.NavLeftRight = value;
            end
        elseif (GameplayNotificationBus.GetCurrentBusId() == self.navWalkPressedEventId) then  
            if (self.WalkToggleHeld == false) then
                self.WalkToggleHeld = true
                if (self.WalkEnabled) then
                    self.WalkEnabled = false
                else
                    self.WalkEnabled = true
                end
            end
        elseif (GameplayNotificationBus.GetCurrentBusId() == self.navWalkReleasedEventId) then
            self.WalkToggleHeld = false
        end
    else
        self.InputValues.NavForwardBack = 0;        
        self.InputValues.NavLeftRight = 0;  
    end
    
end

function jackmovementdirectionprovider:OnEventUpdating(value)
    if (self.controlsEnabled == true) then
        if (GameplayNotificationBus.GetCurrentBusId() == self.navForwardBackEventId) then  
            self.InputValues.NavForwardBack = value;
        elseif (GameplayNotificationBus.GetCurrentBusId() == self.navLeftRightEventId) then
            self.InputValues.NavLeftRight = value;
        elseif (GameplayNotificationBus.GetCurrentBusId() == self.navForwardBackKMEventId) then  
            if(self.WalkEnabled) then
                self.InputValues.NavForwardBack = value * self.Properties.WalkSpeedMutiplier;
            else
                self.InputValues.NavForwardBack = value;
            end
        elseif (GameplayNotificationBus.GetCurrentBusId() == self.navLeftRightKMEventId) then
            if(self.WalkEnabled) then
                self.InputValues.NavLeftRight = value * self.Properties.WalkSpeedMutiplier;
            else
                self.InputValues.NavLeftRight = value;
            end
        end
    else
        self.InputValues.NavForwardBack = 0;        
        self.InputValues.NavLeftRight = 0;
    end
end

function jackmovementdirectionprovider:GetInputVector()
    --Debug.Log("InputVector: "..self.InputValues.NavLeftRight..", "..self.InputValues.NavForwardBack);
    local v = Vector3(self.InputValues.NavLeftRight, self.InputValues.NavForwardBack, 0);
    return v
end

function jackmovementdirectionprovider:OnEventEnd()
    if (GameplayNotificationBus.GetCurrentBusId() == self.navForwardBackEventId) then
        --Debug.Log("ended forwardBack")
        self.InputValues.NavForwardBack = 0;
    elseif (GameplayNotificationBus.GetCurrentBusId() == self.navLeftRightEventId) then
        self.InputValues.NavLeftRight = 0;
    elseif (GameplayNotificationBus.GetCurrentBusId() == self.navForwardBackKMEventId) then
        --Debug.Log("ended forwardBack")
        self.InputValues.NavForwardBack = 0;
    elseif (GameplayNotificationBus.GetCurrentBusId() == self.navLeftRightKMEventId) then
        self.InputValues.NavLeftRight = 0;  
    end 
end

function jackmovementdirectionprovider:OnTick(deltaTime, timePoint)
    local moveLocal = self:GetInputVector();    
    local movementDirection = Vector3(0,0,0);
    if (moveLocal:GetLengthSq() > 0.01) then    
        local tm = TransformBus.Event.GetWorldTM(self.entityId);
        
        local cameraOrientation = TransformBus.Event.GetWorldTM(self.Properties.Camera);
        cameraOrientation:SetTranslation(Vector3(0,0,0));

        local camRight = cameraOrientation:GetColumn(0);        -- right
        local camForward = camRight:Cross(Vector3(0,0,-1));
        local desiredFacing = (camForward * moveLocal.y) + (camRight * moveLocal.x);
        movementDirection = desiredFacing:GetNormalized();
    end
    local moveMag = moveLocal:GetLength();
    if (moveMag > 1.0) then 
        moveMag = 1.0 
    end
    movementDirection = movementDirection * moveMag;
    self.SetMovementDirectionId = GameplayNotificationId(self.entityId, "SetMovementDirection", "float");
    GameplayNotificationBus.Event.OnEventBegin(self.SetMovementDirectionId, movementDirection);
    --Debug.Log("MDC: "..movementDirection.x..", "..movementDirection.y .. " to ID : " .. tostring(self.entityId));
end

return jackmovementdirectionprovider;