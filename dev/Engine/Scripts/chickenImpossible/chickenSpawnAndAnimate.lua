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
local ChickenSpawnAndAnimate = 
{
    Properties = 
    {
    },
    EventCallbacks = 
    {
        UpArrow = {},
        DownArrow = {},
        LeftArrow = {},
        RightArrow = {},
    },
    Handlers =
    {
    },
}

function ChickenSpawnAndAnimate:OnActivate()
    -- Set up bus listeners to listen to notifications from Components
    -- Set up animation notifications listener
    self.animNotificationsBusHandler = SimpleAnimationComponentNotificationBus.Connect(self,self.entityId)
    -- Set up tick bus listener
    self.tickBusHandler = TickBus.Connect(self, 0)    
    
    -- Flag to make sure animation only plays once
    self.OnAnimPlayed = false
    
    -- Set up a random delay to desync spawns
    self.chickeningDelay = math.random()
    self.chickeningCounter = 0
    
    self.Handlers.Up = FloatActionNotificationBus.Connect(self.EventCallbacks.UpArrow, ActionNotificationId(0, "UpPressed"))
    self.Handlers.Down= FloatActionNotificationBus.Connect(self.EventCallbacks.DownArrow, ActionNotificationId(0, "DownPressed"))
    self.Handlers.Left = FloatActionNotificationBus.Connect(self.EventCallbacks.LeftArrow, ActionNotificationId(0, "LeftPressed"))
    self.Handlers.Right= FloatActionNotificationBus.Connect(self.EventCallbacks.RightArrow, ActionNotificationId(0, "RightPressed"))
end


function ChickenSpawnAndAnimate.EventCallbacks.UpArrow:OnEventAction(floatValue)
end
function ChickenSpawnAndAnimate.EventCallbacks.DownArrow:OnEventAction(floatValue)
end
function ChickenSpawnAndAnimate.EventCallbacks.LeftArrow:OnEventAction(floatValue)
end
function ChickenSpawnAndAnimate.EventCallbacks.RightArrow:OnEventAction(floatValue)
end

function ChickenSpawnAndAnimate.EventCallbacks.UpArrow:OnEventFailed()
end
function ChickenSpawnAndAnimate.EventCallbacks.DownArrow:OnEventFailed()
end
function ChickenSpawnAndAnimate.EventCallbacks.LeftArrow:OnEventFailed()
end
function ChickenSpawnAndAnimate.EventCallbacks.RightArrow:OnEventFailed()
end

function ChickenSpawnAndAnimate:OnTick(deltaTime, timePoint)
    self.chickeningCounter = self.chickeningCounter + deltaTime
    if(self.chickeningCounter >= self.chickeningDelay) then    
        -- Check the flag
        if (self.OnAnimPlayed == false) then
            self.OnAnimPlayed = true        
            -- Set up an animated layer to be played
            self.layer = AnimatedLayer("anim_chicken_flapping",0,false,1.0,0.15)
            -- Send a request to the Animation Component
            SimpleAnimationComponentRequestBus.Event.StartAnimation(self.entityId, self.layer)
        end
        
        self.chickeningDelay = math.random(7,12)
        self.chickeningCounter = 0
        
        -- Send a request to the Audio component
        AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, "Play_Chicken_Squack")
    end
end


function ChickenSpawnAndAnimate:OnAnimationStopped(layer)
    -- When the flap animation ends, start the idle animation
    self.layer = AnimatedLayer("anim_chicken_idle",0,true,1.0,0.15)
    SimpleAnimationComponentRequestBus.Event.StartAnimation(self.entityId, self.layer)
end

function ChickenSpawnAndAnimate:OnDeactivate()
    self.tickBusHandler:Disconnect()
    self.animNotificationsBusHandler:Disconnect()
    self.Handlers.Up:Disconnect()
    self.Handlers.Down:Disconnect()
    self.Handlers.Left:Disconnect()
    self.Handlers.Right:Disconnect()
end

return ChickenSpawnAndAnimate