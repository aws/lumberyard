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


local uiaainfo =
{
	Properties = {
		FadeTime = { default = 1, description = "The time in seconds to fade the UI element" },
		HoldTime = { default = 2, description = "The time in seconds to fade the UI element" },
		
		MessagesIn = {
			-- necause i cannot have the console param listener component in a screen, it is on the parent entity, and i will listen to that one
			SetParent = { default = "UIAssetRefLinkParent", description = "The event to send to the screen with the parent / loader as the param." },
		},
		
		Text = {
			NoAA = { default = "no AA", description = "The text to display for this type of AA." },
			FXAA = { default = "FXAA - Fast Approximate AA", description = "The text to display for this type of AA." },
			SMAA = { default = "SMAA - Subpixel Morphological AA", description = "The text to display for this type of AA." },
			TAA  = { default = "TAA - Temporal AA", description = "The text to display for this type of AA." },
		},
	},
}

function uiaainfo:OnActivate()	
	self.fadeTime = 0;
	self.isShowing = false;
	UiFaderBus.Event.Fade(self.entityId, 0, 0);
	
	self.tickHandler = TickBus.Connect(self);
end

function uiaainfo:OnDeactivate()
	self.tickHandler:Disconnect();
	self.tickHandler = nil;
	if(self.cvarHandler ~= nil) then
		self.cvarHandler:Disconnect();
		self.cvarHandler = nil;
	end
end

function uiaainfo:OnTick(deltaTime, timePoint)

	if(self.firsttickHandeled ~= true) then
		self.canvas = UiElementBus.Event.GetCanvas(self.entityId);
		--Debug.Log("uiaainfo:OnTick() first frame : " .. tostring(self.canvas));

		self.SetParentEventId = GameplayNotificationId(self.canvas, self.Properties.MessagesIn.SetParent, "float");
		self.SetParentHandler = GameplayNotificationBus.Connect(self, self.SetParentEventId);
		
		self.firsttickHandeled = true;
	end

	if(self.isShowing) then
		self.fadeTime = self.fadeTime - deltaTime;
		
		if(self.fadeTime < 0) then
			self.fadeTime = 0;
			self.isShowing = false;
		elseif (self.fadeTime < self.Properties.FadeTime) then
			UiFaderBus.Event.Fade(self.entityId, 0, 1 / self.Properties.FadeTime);
		end
	end
end

function uiaainfo:OnEventBegin(value)
	--Debug.Log("uiaainfo:OnEventBegin() " .. tostring(self.entityId) .. " : " .. tostring(value));
	if (GameplayNotificationBus.GetCurrentBusId() == self.SetParentEventId) then
		if(self.cvarHandler ~= nil) then
			self.cvarHandler:Disconnect();
			self.cvarHandler = nil;
		end
		
		self.parentId = value;
		
		self.cvarHandler = ConsoleVarListenerComponentNotificationBus.Connect(self, self.parentId);
	end
end

function uiaainfo:OnBeforeConsoleVarChanged(name, valueOld, valueNew)
	--Debug.Log("CVar [" .. tostring(name) .. "] before: " .. tostring(valueOld) .. " to " .. tostring(valueNew)); 
	return true;
end

function uiaainfo:SetText(text)
	UiFaderBus.Event.Fade(self.entityId, 1, 1 / self.Properties.FadeTime);
	UiTextBus.Event.SetText(self.entityId, text);
	self.isShowing = true;
	self.fadeTime = (self.Properties.FadeTime * 2.0) + self.Properties.HoldTime;
end

--cvarcallback
function uiaainfo:OnAfterConsoleVarChanged(name, value)
	--Debug.Log("CVar [" .. tostring(name) .. "] after: " .. tostring(value));
	
	-- THIS ONE!
	if(name == "r_AntialiasingMode") then
		if(value == 0) then
			-- no AA
			self:SetText(self.Properties.Text.NoAA);			
		elseif (value == 1) then
			-- FXAA - Fast Approximate AA
			self:SetText(self.Properties.Text.FXAA);			
		elseif (value == 2) then
			-- SMAA - Subpixel Morphological AA
			self:SetText(self.Properties.Text.SMAA);			
		elseif (value == 3) then
			-- TAA - Temporal AA (Default)
			self:SetText(self.Properties.Text.TAA);			
		end
	end

end

return uiaainfo;
