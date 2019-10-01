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

local uiinteractidentifierembedded =
{
	Properties =
	{
		ButtonContentStandalone = { default = EntityId(), description = "The content to show inside the button on generic machines (IE. Windows, Mac, etc)" },
		ButtonContentProvo = { default = EntityId(), description = "The content to show inside the button on Provo" },
		ButtonContentXenia = { default = EntityId(), description = "The content to show inside the button on Xenia" },
		ButtonContentMobile = { default = EntityId(), description = "The content to show inside the button on Mobile platforms(IE. iOS, Android)" },
	}
}

function uiinteractidentifierembedded:OnActivate()
	if (self.initialized) then
		return;
	end
	
	self.tickHandler = TickBus.Connect(self);
	
	if (Platform.Current == Platform.Provo) then
		self.contentEntityId = self.Properties.ButtonContentProvo;
		self.getColor = UiImageBus.Event.GetColor;
		self.setColor = UiImageBus.Event.SetColor;
	elseif (Platform.Current == Platform.Xenia) then
		self.contentEntityId = self.Properties.ButtonContentXenia;
		self.getColor = UiImageBus.Event.GetColor;
		self.setColor = UiImageBus.Event.SetColor;
	elseif ((Platform.Current == Platform.iOS) or
			(Platform.Current == Platform.Android) or
			(Platform.Current == Platform.Android64)) then
		self.contentEntityId = self.Properties.ButtonContentMobile;
		self.getColor = UiTextBus.Event.GetColor;
		self.setColor = UiTextBus.Event.SetColor;
	else
		self.contentEntityId = self.Properties.ButtonContentStandalone;
		self.getColor = UiTextBus.Event.GetColor;
		self.setColor = UiTextBus.Event.SetColor;
	end	
end

function uiinteractidentifierembedded:OnDeactivate()
	self:Disconnect();
end

function uiinteractidentifierembedded:Disconnect()
	if (self.tickHandler == nil) then
		return;
	end
	self.tickHandler:Disconnect();
	self.tickHandler = nil;
end

function uiinteractidentifierembedded:OnTick(deltaTime, timePoint)
	
	local color = self.getColor(self.contentEntityId);
	if (color == nil) then
		return;
	end	
	color.a = 1;
	self.setColor(self.contentEntityId, color);
	
	self.initialized = true;	
	self:Disconnect();
end

return uiinteractidentifierembedded;
