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

local UIAssetRefLinkParent =
{
-- this basically sends a message at a newly loaded screen through the UI Asset Ref system so that you can traverse to the thing that loaded it.
	Properties =
	{
		EventToSend = { default = "UIAssetRefLinkParent", description = "The event to send to the screen with the parent / loader as the param." },
		Load = { default = "UIAssetRefLoad", description = "The event to send to the screen with the parent / loader as the param." },
		UnLoad = { default = "UIAssetRefUnLoad", description = "The event to send to the screen with the parent / loader as the param." },
		WaitFrames = { default = 3, description = "Should the message wait for a frame before being passed." },
		IsAutoLoaded = { default = false, description = "If the screen is loaded automatically we need to use slightly different methodology to hook it up."}
	},
}

function UIAssetRefLinkParent:OnActivate()
	self.AssetRefHandler = UiCanvasAssetRefNotificationBus.Connect(self, self.entityId);

	self.loadEventId = GameplayNotificationId(self.entityId, self.Properties.Load, "float");
	self.loadHandler = GameplayNotificationBus.Connect(self, self.loadEventId);
	self.unLoadEventId = GameplayNotificationId(self.entityId, self.Properties.UnLoad, "float");
	self.unLoadHandler = GameplayNotificationBus.Connect(self, self.unLoadEventId);
	
	self.isWaiting = -1;
	self.autoLoadMessageSent = false;
	
	if(self.Properties.WaitFrames and (self.Properties.WaitFrames > 0)) then
		self.tickHandler = TickBus.Connect(self);
	end
end

function UIAssetRefLinkParent:OnDeactivate()
	if (self.AssetRefHandler ~= nil) then
		self.AssetRefHandler:Disconnect();
		self.AssetRefHandler = nil;
	end
	if (self.loadHandler ~= nil) then
		self.loadHandler:Disconnect();
		self.loadHandler = nil;
	end	
	if (self.unLoadHandler ~= nil) then
		self.unLoadHandler:Disconnect();
		self.unLoadHandler = nil;
	end
	if (self.tickHandler ~= nil) then
		self.tickHandler:Disconnect();
		self.tickHandler = nil;
	end
end

function UIAssetRefLinkParent:OnTick(deltaTime, timePoint)
	if (self.Properties.IsAutoLoaded and (not self.autoLoadMessageSent)) then
		local myAutoloadedCanvas = UiCanvasRefBus.Event.GetCanvas(self.entityId);
		if(myAutoloadedCanvas ~= nil) then
			--Debug.Log("UIAssetRefLinkParent:OnTick() autoloaded canvas found. " .. tostring(self.entityId) .. " : " .. tostring(myAutoloadedCanvas));
			self.isWaiting = self.Properties.WaitFrames;
			self.canvas = myAutoloadedCanvas;
			self.autoLoadMessageSent = true;
		end
	end
	
	if (self.isWaiting >= 0) then
		self.isWaiting = self.isWaiting - 1;
		if(self.isWaiting == 0) then
			self:SendLoadedMessage(self.canvas);
			self.isWaiting = -1;
		end
	end
end

function UIAssetRefLinkParent:SendLoadedMessage(canvas)
	--Debug.Log("UIAssetRefLinkParent:SendLoadedMessage() " .. tostring(self.entityId) .. " : " .. tostring(canvas));
	local messageEventId = GameplayNotificationId(canvas, self.Properties.EventToSend, "float");
	GameplayNotificationBus.Event.OnEventBegin(messageEventId, self.entityId);
end

function UIAssetRefLinkParent:OnCanvasLoadedIntoEntity(canvas)
	--Debug.Log("UIAssetRefLinkParent:OnCanvasLoadedIntoEntity() " .. tostring(self.entityId) .. " : " .. tostring(canvas));
	if(self.Properties.WaitFrames) then
		self.isWaiting = self.Properties.WaitFrames;
		self.canvas = canvas;
	else
		self:SendLoadedMessage(canvas);
	end
end

function UIAssetRefLinkParent:OnEventBegin(value)
	--Debug.Log("uidiegeticmenu:OnEventBegin() " .. tostring(self.entityId) .. " : " .. tostring(value));
	if (GameplayNotificationBus.GetCurrentBusId() == self.loadEventId) then
		UiCanvasAssetRefBus.Event.LoadCanvas(self.entityId);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.unLoadEventId) then
		UiCanvasAssetRefBus.Event.UnloadCanvas(self.entityId);
	end
end

return UIAssetRefLinkParent;