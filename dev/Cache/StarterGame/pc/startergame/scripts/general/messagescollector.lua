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

local messagescollector =
{
	Properties =
	{
		MessageInA = { default = "", description = "First message aimed at this that I am going to trigger on." },
		MessageInB = { default = "", description = "Second message aimed at this that I am going to trigger on." },
		MessageOut = { default = "", description = "Message to send when both are collected." },
	},
}

function messagescollector:OnActivate()
	if (self.AHasTriggered == true and self.BHasTriggered == true) then
		return;
	end
	
	self.messageAId = GameplayNotificationId(self.entityId, self.Properties.MessageInA, "float");
	self.messageAHandler = GameplayNotificationBus.Connect(self, self.messageAId);
	self.messageBId = GameplayNotificationId(self.entityId, self.Properties.MessageInB, "float");
	self.messageBHandler = GameplayNotificationBus.Connect(self, self.messageBId);

	self.MessageOutId = GameplayNotificationId(self.entityId, self.Properties.MessageOut, "float");
end

function messagescollector:OnDeactivate()
	self:Disconnect();
end

function messagescollector:OnEventBegin(value)
	if (GameplayNotificationBus.GetCurrentBusId() == self.messageAId) then
		self.AHasTriggered = true;
-- 		Debug.Log("Message "..self.Properties.MessageInA.." triggered.");
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.messageBId) then
		self.BHasTriggered = true;
-- 		Debug.Log("Message "..self.Properties.MessageInB.." triggered.");
	end
	
	if (self.AHasTriggered == true and self.BHasTriggered == true) then
		self:Disconnect();
		GameplayNotificationBus.Event.OnEventBegin(self.MessageOutId, nil);
-- 		Debug.Log("Message "..self.Properties.MessageOut.." triggered.");
	end
end

function messagescollector:Disconnect()
	if (self.messageAHandler ~= nil) then
		self.messageAHandler:Disconnect();
		self.messageAHandler = nil;
	end
	if	(self.messageBHandler ~= nil) then
		self.messageBHandler:Disconnect();
		self.messageBHandler = nil;
	end
end

return messagescollector;