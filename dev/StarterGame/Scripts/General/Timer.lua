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

local timer = 
{
	Properties =
	{
		StartEventName = { default = "TimerStart", description = "Name of the event to trigger starting the timer." },
		StopEventName = { default = "TimerStop", description = "Name of the event to trigger stopping the timer." },
        TimerElementName = {default = "TimerText", description = "Name of UI element"},
    },
}

function timer:OnActivate()
    self.startTriggerEventId = GameplayNotificationId(self.entityId, self.Properties.StartEventName, "float");
	self.startTriggerHandler = GameplayNotificationBus.Connect(self, self.startTriggerEventId);

    self.stopTriggerEventId = GameplayNotificationId(self.entityId, self.Properties.StopEventName, "float");
	self.stopTriggerHandler = GameplayNotificationBus.Connect(self, self.stopTriggerEventId);
end

function timer:OnDeactivate()
    if (self.startTriggerHandler ~= nil) then
        self.startTriggerHandler:Disconnect();
        self.startTriggerHandler = nil;
    end

    if (self.stopTriggerHandler ~= nil) then
        self.stopTriggerHandler:Disconnect();
        self.stopTriggerHandler = nil;
    end

	if (self.tickHandler ~= nil) then
		self.tickHandler:Disconnect();
		self.tickHandler = nil;
	end
end

function timer:OnTick(deltaTime, timePoint)
    self.elapsedTime = self.elapsedTime + deltaTime;

    local min = self.elapsedTime / 60;
    local sec = self.elapsedTime - (math.floor(min)*60);
    local displayString = tostring(math.floor(min)) .. ":" .. string.format("%05.2f",sec);

    UiTextBus.Event.SetText(self.UiTextElement, displayString)
end

function timer:OnEventBegin(value)
	if (GameplayNotificationBus.GetCurrentBusId() == self.startTriggerEventId) then
    	if (self.tickHandler == nil) then
            self.elapsedTime = 0;
            self.tickHandler = TickBus.Connect(self);
        	self.canvasEntityId = UiCanvasAssetRefBus.Event.LoadCanvas(self.entityId);
		    self.UiTextElement = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, self.Properties.TimerElementName);
        end
	end

	if (GameplayNotificationBus.GetCurrentBusId() == self.stopTriggerEventId) then
    	if (self.tickHandler ~= nil) then
            self.tickHandler:Disconnect();
            self.tickHandler = nil;
        end
	end
end


return timer;