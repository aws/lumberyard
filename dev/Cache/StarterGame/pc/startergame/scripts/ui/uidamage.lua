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

local UIDamage =
{
	Properties =
	{
		Directions =
		{
			TopLeft = { default = EntityId(), description = "Damage indicator." },
			TopRight = { default = EntityId(), description = "Damage indicator." },
			BottomLeft = { default = EntityId(), description = "Damage indicator." },
			BottomRight = { default = EntityId(), description = "Damage indicator." },
			MiddleTop = { default = EntityId(), description = "Damage indicator." },
			MiddleBottom = { default = EntityId(), description = "Damage indicator." },
			MiddleLeft = { default = EntityId(), description = "Damage indicator." },
			MiddleRight = { default = EntityId(), description = "Damage indicator." },
		},
		
		AnimSpeed = { default = 30.0, description = "Speed of the animation in F.P.S.", suffix = " fps" },
	},
}

function UIDamage:OnActivate()
	--Debug.Log("UIDamage:OnActivate()");
	
	--self.Properties.Angles.Forward = Math.DegToRad(self.Properties.Angles.Forward);
	--self.Properties.Angles.ForwardHalf = self.Properties.Angles.Forward * 0.5;
	--self.Properties.Angles.Behind = Math.DegToRad(self.Properties.Angles.Behind);
	--self.Properties.Angles.BehindHalf = self.Properties.Angles.Behind * 0.5;
	
	self.FrameDuration = 1.0 / self.Properties.AnimSpeed;
	self.performedFirstUpdate = false;
	self.tickBusHandler = TickBus.Connect(self);	

end
function UIDamage:OnTick(deltaTime, timePoint)
	if (not self.performedFirstUpdate) then
		-- Disconnect first as the 'SetInteractEnabled()' call should be what determines whether
		-- we're connected to the tick bus or not.
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
		
		-- This is done here instead of 'OnActivate()' because we can't enforce the player being
		-- created before this object is activated.
		self.playerId = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("PlayerCharacter"));
		if (not self.playerId:IsValid()) then
			Debug.Error("UIDamage couldn't get the player entity.");
		end
		self.playerHitEventId = GameplayNotificationId(self.playerId, "GotShotEvent", "float");
		self.playerHitHandler = GameplayNotificationBus.Connect(self, self.playerHitEventId);
			
		self.performedFirstUpdate = true;
		
		-- If we didn't reconnect then don't perform the rest of the update.
		if (self.tickBusHandler == nil) then
			return;
		end
	end
	
	self.onScreen = self:OnScreen(self.posOnScreen);

	--self:UpdateScreenPrompt(false);
end
function UIDamage:OnDeactivate()
	--Debug.Log("UIDamage:OnDeactivate()");
	
	if (self.playerHitHandler ~= nil) then
		self.playerHitHandler:Disconnect();
		self.playerHitHandler = nil;
	end
end

function UIDamage:DisplayDamageIndicator(shooter)
	local cam = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("ActiveCamera"));
	if (cam == nil or not cam:IsValid()) then
		Debug.Log("UIDamage couldn't get the active camera.");
		return;
	end
	
	local right = TransformBus.Event.GetWorldTM(cam):GetColumn(0):GetNormalized();
	local forward = right:Cross(Vector3(0,0,-1));
	local shotDir = TransformBus.Event.GetWorldTM(shooter):GetTranslation() - TransformBus.Event.GetWorldTM(cam):GetTranslation();
	shotDir:Normalize();
	
	local forwardDot = forward:Dot(shotDir);
	local forwardAngle = Math.RadToDeg(Math.ArcCos(forwardDot));
	local rightDot = right:Dot(shotDir);
	local rightAngle = Math.RadToDeg(Math.ArcCos(rightDot));
	local indicator = nil;
	--Debug.Log("Forward: " .. tostring(forward) .. ", ShotDir: " .. tostring(shotDir));
	--Debug.Log("Dot: " .. tostring(forwardDot) .. ", Angle: " .. tostring(forwardAngle));
	if (forwardAngle <= 22.5) then
		-- MiddleTop
		indicator = self.Properties.Directions.MiddleTop;
	elseif (forwardAngle <= 45.0) then
		if (rightDot >= 0.0) then
			-- TopRight
			indicator = self.Properties.Directions.TopRight;
		else
			-- TopLeft
			indicator = self.Properties.Directions.TopLeft;
		end
	elseif (forwardAngle >= 157.5) then
		-- MiddleBottom
		indicator = self.Properties.Directions.MiddleBottom;
	elseif (forwardAngle >= 135.0) then
		if (rightDot >= 0.0) then
			-- BottomRight
			indicator = self.Properties.Directions.BottomRight;
		else
			-- BottomLeft
			indicator = self.Properties.Directions.BottomLeft;
		end
	else
		if (rightDot >= 0.0) then
			-- MiddleRight
			indicator = self.Properties.Directions.MiddleRight;
		else
			-- MiddleLeft
			indicator = self.Properties.Directions.MiddleLeft;
		end
	end
	
	if (indicator ~= nil) then
		local eventId = GameplayNotificationId(indicator, "PlayFrame", "float");
		GameplayNotificationBus.Event.OnEventBegin(eventId, self.FrameDuration);
		--Debug.Log("Damage from: " .. tostring(StarterGameEntityUtility.GetEntityName(indicator)));
	else
		Debug.Log("UIDamage couldn't find the correct indicator for the hit direction.");
	end
end

function UIDamage:OnEventBegin(value)
	--Debug.Log("Recieved message");
	local busId = GameplayNotificationBus.GetCurrentBusId();
	if (busId == self.playerHitEventId) then
		--Debug.Log("Recieved player hit message");
		self:DisplayDamageIndicator(value.assailant);
	end
end

return UIDamage;
