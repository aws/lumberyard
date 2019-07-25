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
