
local uimissionsuccess =
{
	Properties =
	{
		ShowScreenEvent = { default = "TimerStop", description = "The event used to show this screen." },
	},
}



function uimissionsuccess:OnActivate()
	--Debug.Log("uimissionsuccess:OnActivate");
	self.ShowScreenEventId = GameplayNotificationId(self.entityId, self.Properties.ShowScreenEvent, "float");
	self.ShowScreenHandler = GameplayNotificationBus.Connect(self, self.ShowScreenEventId);	
end

function uimissionsuccess:OnDeactivate()
	--Debug.Log("uimissionsuccess:OnDeactivate");
	if (self.canvasEntityId ~= nil) then
		UiCanvasManagerBus.Broadcast.UnloadCanvas(self.canvasEntityId);
		self.canvasEntityId = nil;
	end

end

function uimissionsuccess:ShowScreen()
	Debug.Log("maze_complete:ShowScreen");

	self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("UI/Canvases/Maze_Complete.uicanvas");

end

function uimissionsuccess:OnEventBegin(value)
	--Debug.Log("Recieved message - uimissionsuccess");
	if (GameplayNotificationBus.GetCurrentBusId() == self.ShowScreenEventId) then
		--Debug.Log("Show screen");
		self:ShowScreen();
	end

end



return uimissionsuccess;
