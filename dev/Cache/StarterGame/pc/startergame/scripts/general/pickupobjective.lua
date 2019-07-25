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

local pickupobjective =
{
	Properties =
	{
		EventName = { default = "ObjectivePickupEvent", description = "The name of the event to broadcast." },
		ObjectiveID = { default = -1, description = "The ID of the objective (sent as past of the event when picked up)." },
		ShowMarkerMessage = { default = "ShowMarkerEvent", description = "The message to send to \"self\" to control its map marker." },
		SetStatusMessage = { default = "SetStatusEvent", description = "The message to send to \"self\" to control its on screen tracking." },
		
		GraphicEntity = {default = EntityId(), description = "The entity used fo the graphical description of the pickup."},
		
		GraphicTurnTime = { default = 3, description = "Time to rotate a full rotation" },
		GraphicBobTime = { default = 1, description = "Time to rotate a full bob." },
		GraphicBobDist = { default = 0.5, description = "Distacne that the bob moves." },
		
		CollectionSound = { default = "Play_HUD_press_start", description = "Sound to play on collection" },
	},
}

function pickupobjective:OnActivate()

	-- Make sure the objective has a valid objective ID.
	Debug.Assert(self.Properties.ObjectiveID >= 0);

	self.triggerAreaHandler = TriggerAreaNotificationBus.Connect(self, self.entityId);
	
	self.tickBusHandler = TickBus.Connect(self);
		
	self.rotateTimer = 0;
	self.bobTimer = 0;
	
	self.audioCollectionSound = self.Properties.CollectionSound;
end

function pickupobjective:OnDeactivate()

	if(not self.tickBusHandler == nil) then
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end
	if(not self.triggerAreaHandler == nil) then
		self.triggerAreaHandler:Disconnect();
		self.triggerAreaHandler = nil;
	end
end


function pickupobjective:OnTick(deltaTime, timePoint)

	local twoPi = 6.283185307179586476925286766559;

	if(not (self.Properties.GraphicTurnTime == 0)) then
		self.rotateTimer = self.rotateTimer + (deltaTime / self.Properties.GraphicTurnTime);
		if(self.rotateTimer >= twoPi) then
			self.rotateTimer = self.rotateTimer - twoPi;
		end
	end
	
	if((not (self.Properties.GraphicTurnTime == 0)) and (not (self.Properties.GraphicBobDist == 0))) then
		self.bobTimer = self.bobTimer + (deltaTime / self.Properties.GraphicBobTime);
		if(self.bobTimer >= twoPi) then
			self.bobTimer = self.bobTimer - twoPi;
		end
	end

	--Debug.Log("Pickup graphic values:  Rotation: " .. self.rotateTimer .. " Height: " .. self.bobTimer);
	local tm = Transform.CreateRotationZ(self.rotateTimer);
	tm:SetTranslation(Vector3(0, 0, Math.Sin(self.bobTimer) * self.Properties.GraphicBobDist));
	TransformBus.Event.SetLocalTM(self.Properties.GraphicEntity, tm);

end

function pickupobjective:OnTriggerAreaEntered(enteringEntityId)

	--Debug.Log("Picked up objective " .. tostring(self.Properties.ObjectiveID) .. ": sending " .. self.Properties.EventName);

	AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.audioCollectionSound);
	
	-- Send a message to whatever picked us up that they picked up an objective.
	local notificationId = GameplayNotificationId(enteringEntityId, self.Properties.EventName, "float");
	GameplayNotificationBus.Event.OnEventBegin(notificationId, self.Properties.ObjectiveID);
	local markernotificationId = GameplayNotificationId(self.entityId, self.Properties.ShowMarkerMessage, "float");
	GameplayNotificationBus.Event.OnEventBegin(markernotificationId, 0);
	local statusnotificationId = GameplayNotificationId(self.entityId, self.Properties.SetStatusMessage, "float");
	GameplayNotificationBus.Event.OnEventBegin(statusnotificationId, 1);

	if(not self.triggerAreaHandler == nil) then
		self.triggerAreaHandler:Disconnect();
		self.triggerAreaHandler = nil;
	end
	if(not self.tickBusHandler == nil) then
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end
	
	-- disable the emitter particles
	ParticleComponentRequestBus.Event.Enable(self.entityId, false);
	
	-- enable the collected particles
	ParticleComponentRequestBus.Event.Enable(self.Properties.GraphicEntity, true);
		
	-- hide the graphic
	MeshComponentRequestBus.Event.SetVisibility(self.Properties.GraphicEntity, false);
	
	-- Delete the pickup so it can't be acquired again.
	-- TODO: Deleting the trigger while something's in it causes the editor to crash so
	-- we need to find another way of deleting the pickup. Possibly put the trigger area
	-- onto a different entity so that the pickup can be deleted and the trigger area is
	-- orphaned?
	--GameEntityContextRequestBus.Broadcast.DestroyGameEntity(self.entityId);

end

return pickupobjective;