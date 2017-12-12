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