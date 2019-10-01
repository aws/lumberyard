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

local pickuphealth =
{
	Properties =
	{
		EventInquiryName = { default = "HealthGetUnUsedCapacity", description = "The name of the event to ask if you need health." },
		EventInquiryReturnName = { default = "HealthUnUsedCapacity", description = "The name of the event that gets returned from a request." },
		EventName = { default = "HealthDeltaValueEvent", description = "The name of the event to award health." },
		Health = { default = 50, description = "Amount of health to give the player." },
		
		GraphicEntity = {default = EntityId(), description = "The entity used fo the graphical description of the pickup."},
		
		GraphicTurnTime = { default = 3, description = "Time to rotate a full rotation" },
		GraphicBobTime = { default = 1, description = "Time to rotate a full bob." },
		GraphicBobDist = { default = 0.5, description = "Distacne that the bob moves." },
		
		CollectionSound = { default = "Play_HUD_press_start", description = "Sound to play on collection" },
	},
}

function pickuphealth:OnActivate()
	self.triggerAreaHandler = TriggerAreaNotificationBus.Connect(self, self.entityId);
	
	self.tickBusHandler = TickBus.Connect(self);
		
	self.rotateTimer = 0;
	self.bobTimer = 0;

	self.enteringEntityID = nil;
	
	self.EntityInquiryReturnEventId = GameplayNotificationId(self.entityId, self.Properties.EventInquiryReturnName, "float");
	self.EntityInquiryReturnHandler = GameplayNotificationBus.Connect(self, self.EntityInquiryReturnEventId);
	
	self.audioCollectionSound = self.Properties.CollectionSound;
end

function pickuphealth:OnDeactivate()
	if(not (self.EntityInquiryReturnHandler == nil)) then
		self.EntityInquiryReturnHandler:Disconnect();
		self.EntityInquiryReturnHandler = nil;
	end
	if(not self.triggerAreaHandler == nil) then
		self.triggerAreaHandler:Disconnect();
		self.triggerAreaHandler = nil;
	end
	if(not self.tickBusHandler == nil) then
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end
end

function pickuphealth:OnTick(deltaTime, timePoint)

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

function pickuphealth:OnEventBegin(value)
	Debug.Log("pickuphealth:OnEventBegin : " .. tostring(value));
	if ((GameplayNotificationBus.GetCurrentBusId() == self.EntityInquiryReturnEventId) and (value > 0)) then
		-- i want the health
		Debug.Log(tostring(self.enteringEntityId) .. " got health");
		
		-- award the health
		local notificationId = GameplayNotificationId(self.enteringEntityId, self.Properties.EventName, "float");
		GameplayNotificationBus.Event.OnEventBegin(notificationId, self.Properties.Health);

		AudioTriggerComponentRequestBus.Event.ExecuteTrigger(self.entityId, self.audioCollectionSound);
		
		if(not (self.EntityInquiryReturnHandler == nil)) then
			self.EntityInquiryReturnHandler:Disconnect();
			self.EntityInquiryReturnHandler = nil;
		end
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
	end

end

function pickuphealth:OnTriggerAreaEntered(enteringEntityId)

	Debug.Log(tostring(enteringEntityId) .. "Picked up Health " .. self.Properties.Health);

	-- Send a message to whatever picked us up that they are standing on Health.
	-- see if the thing that walked over us wants us
	self.enteringEntityId = enteringEntityId;
	local notificationId = GameplayNotificationId(self.enteringEntityId, self.Properties.EventInquiryName, "float");
	GameplayNotificationBus.Event.OnEventBegin(notificationId, self.entityId);
end


return pickuphealth;