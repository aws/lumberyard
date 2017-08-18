----------------------------------------------------------------------------------------------------
--
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--
--
----------------------------------------------------------------------------------------------------
local vectorized_combination = 
{
    Properties = 
    {
		XCoordEventName = "",
		YCoordEventName = "",
		ZCoordEventName = "",
		OutgoingGameplayEventName = "",
		DeadZoneLength = 0.2,
    },
	Handlers = 
	{
		X={},
		Y={},
		Z={},
	},
}

function vectorized_combination:OnActivate()
	self.vectorizedValue = Vector3(0,0,0)
	self.active = false
	self.outgoingId = GameplayNotificationId(self.entityId, self.Properties.OutgoingGameplayEventName)
	local inputBusIdX = GameplayNotificationId(self.entityId, self.Properties.XCoordEventName)
	local inputBusIdY = GameplayNotificationId(self.entityId, self.Properties.YCoordEventName)
	local inputBusIdZ = GameplayNotificationId(self.entityId, self.Properties.ZCoordEventName)
	self.inputBusX = GameplayNotificationBus.Connect(self.Handlers.X, inputBusIdX)
	self.inputBusY = GameplayNotificationBus.Connect(self.Handlers.Y, inputBusIdY)
	self.inputBusZ = GameplayNotificationBus.Connect(self.Handlers.Z, inputBusIdZ)
	self.Handlers.X.root = self
	self.Handlers.Y.root = self
	self.Handlers.Z.root = self
end

function vectorized_combination:OnDeactivate()
	self.Handlers = {}
end

function vectorized_combination:SendGameplayEvent()
	local shouldBeActive = self.vectorizedValue:GetLength() > self.Properties.DeadZoneLength
	if shouldBeActive and not self.active then
		GameplayNotificationBus.Event.OnEventBegin(self.outgoingId, self.vectorizedValue)
	elseif shouldBeActive and self.active then
		GameplayNotificationBus.Event.OnEventUpdating(self.outgoingId, self.vectorizedValue)
	elseif not shouldBeActive and self.active then
		GameplayNotificationBus.Event.OnEventEnd(self.outgoingId, self.vectorizedValue)
	end
	self.active = shouldBeActive
	-- otherwise we were not active and shouldn't be active so do nothing
end

-- X coordinate handlers
function vectorized_combination.Handlers.X:OnEventBegin(floatValue)
	self.root.vectorizedValue.x = floatValue
	self.root:SendGameplayEvent()
end

function vectorized_combination.Handlers.X:OnEventUpdating(floatValue)
	self.root.vectorizedValue.x = floatValue
	self.root:SendGameplayEvent()
end

function vectorized_combination.Handlers.X:OnEventEnd(floatValue)
	self.root.vectorizedValue.x = 0
	self.root:SendGameplayEvent()
end

-- Y coordinate handlers
function vectorized_combination.Handlers.Y:OnEventBegin(floatValue)
	self.root.vectorizedValue.y = floatValue
	self.root:SendGameplayEvent()
end

function vectorized_combination.Handlers.Y:OnEventUpdating(floatValue)
	self.root.vectorizedValue.y = floatValue
	self.root:SendGameplayEvent()
end

function vectorized_combination.Handlers.Y:OnEventEnd(floatValue)
	self.root.vectorizedValue.y = 0
	self.root:SendGameplayEvent()
end

-- Z coordinate handlers
function vectorized_combination.Handlers.Z:OnEventBegin(floatValue)
	self.root.vectorizedValue.z = floatValue
	self.root:SendGameplayEvent()
end

function vectorized_combination.Handlers.Z:OnEventUpdating(floatValue)
	self.root.vectorizedValue.z = floatValue
	self.root:SendGameplayEvent()
end

function vectorized_combination.Handlers.Z:OnEventEnd(floatValue)
	self.root.vectorizedValue.z = 0
	self.root:SendGameplayEvent()
end

return vectorized_combination