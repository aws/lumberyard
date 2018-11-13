local utilities = require "scripts/common/utilities"

local flycam = 
{
	Properties =
	{
		VerticalRotateSpeed = { default = 180.0, },
		HorizontalRotateSpeed = { default = 180.0, },
		MoveSpeed = { default = 10.0, },
		FastMoveSpeed = { default = 100.0, },
		SlowMoveSpeed = { default = 1.0, },
		MaxLookUpAngle = { default = 90.0, },
		MaxLookDownAngle = { default = -90.0, },
		MinFOV = { default = 10.0, },
		MaxFOV = {default = 110.0, },
		FOVStep = {default = 2.5, },
		
		MouseSensitivityScale = { default = 0.25, },
		MouseMaxSpeedScale = { default = 2.0, },
	},

	
	-- input values (move fore/back, move left/right, move up/down, pitch/yaw)
	-- separate pad and key/mouse values

	-- fast mode
	-- slow mode
	
	InputValues =
	{
		Controller =
		{
			LookLeftRight = {0.0, false},
			LookUpDown = {0.0, false},
			MoveForeBack = {0.0, false},
			MoveLeftRight = {0.0, false},
			MoveUpDown = {0.0, false},
			FastMode = {0.0, false},
			SlowMode = {0.0, false},
		},
		KeyMouse =
		{
			LookLeftRight = {0.0, false},
			LookUpDown = {0.0, false},
			MoveForeBack = {0.0, false},
			MoveLeftRight = {0.0, false},
			MoveUpDown = {0.0, false},
			FastMode = {0.0, false},
			SlowMode = {0.0, false},
		},
		Shared =
		{
			FOVChange = 0.0,
			FOVReset = 0.0,
		},

		-- selected and processed values
		LookLeftRight = 0.0,
		LookUpDown = 0.0,
		MoveForeBack = 0.0,
		MoveLeftRight = 0.0,
		MoveUpDown = 0.0,
		FastMode = 0.0,
		SlowMode = 0.0,
	},
	
	CamProps = 
	{
		OrbitVertical = 0.0,
		OrbitHorizontal = 0.0,
		FOV = 75.0,
	},
	
}


function flycam:OnActivate()
	self.enabledEventId = GameplayNotificationId(self.entityId, "EnableFlyCam", "float");
	self.enabledHandler = GameplayNotificationBus.Connect(self, self.enabledEventId);

	-- add handlers for input events
	self.lookLeftRightEventId = GameplayNotificationId(self.entityId, "LookLeftRight", "float");
	self.lookLeftRightHandler = GameplayNotificationBus.Connect(self, self.lookLeftRightEventId);
	self.lookUpDownEventId = GameplayNotificationId(self.entityId, "LookUpDown", "float");
	self.lookUpDownHandler = GameplayNotificationBus.Connect(self, self.lookUpDownEventId);
	self.moveForeBackEventId = GameplayNotificationId(self.entityId, "MoveForeBack", "float");
	self.moveForeBackHandler = GameplayNotificationBus.Connect(self, self.moveForeBackEventId);
	self.moveLeftRightEventId = GameplayNotificationId(self.entityId, "MoveLeftRight", "float");
	self.moveLeftRightHandler = GameplayNotificationBus.Connect(self, self.moveLeftRightEventId);
	self.moveUpDownEventId = GameplayNotificationId(self.entityId, "MoveUpDown", "float");
	self.moveUpDownHandler = GameplayNotificationBus.Connect(self, self.moveUpDownEventId);
	self.fastModeEventId = GameplayNotificationId(self.entityId, "FastMode", "float");
	self.fastModeHandler = GameplayNotificationBus.Connect(self, self.fastModeEventId);
	self.slowModeEventId = GameplayNotificationId(self.entityId, "SlowMode", "float");
	self.slowModeHandler = GameplayNotificationBus.Connect(self, self.slowModeEventId);
	
	self.lookLeftRightKMEventId = GameplayNotificationId(self.entityId, "LookLeftRightKM", "float");
	self.lookLeftRightKMHandler = GameplayNotificationBus.Connect(self, self.lookLeftRightKMEventId);
	self.lookUpDownKMEventId = GameplayNotificationId(self.entityId, "LookUpDownKM", "float");
	self.lookUpDownKMHandler = GameplayNotificationBus.Connect(self, self.lookUpDownKMEventId);
	self.moveForeBackKMEventId = GameplayNotificationId(self.entityId, "MoveForeBackKM", "float");
	self.moveForeBackKMHandler = GameplayNotificationBus.Connect(self, self.moveForeBackKMEventId);
	self.moveLeftRightKMEventId = GameplayNotificationId(self.entityId, "MoveLeftRightKM", "float");
	self.moveLeftRightKMHandler = GameplayNotificationBus.Connect(self, self.moveLeftRightKMEventId);
	self.moveUpDownKMEventId = GameplayNotificationId(self.entityId, "MoveUpDownKM", "float");
	self.moveUpDownKMHandler = GameplayNotificationBus.Connect(self, self.moveUpDownKMEventId);
	self.fastModeKMEventId = GameplayNotificationId(self.entityId, "FastModeKM", "float");
	self.fastModeKMHandler = GameplayNotificationBus.Connect(self, self.fastModeKMEventId);
	self.slowModeKMEventId = GameplayNotificationId(self.entityId, "SlowModeKM", "float");
	self.slowModeKMHandler = GameplayNotificationBus.Connect(self, self.slowModeKMEventId);

	self.fovChangeEventId = GameplayNotificationId(self.entityId, "FOVChange", "float");
	self.fovChangeHandler = GameplayNotificationBus.Connect(self, self.fovChangeEventId);
	self.fovResetEventId = GameplayNotificationId(self.entityId, "FOVReset", "float");
	self.fovResetHandler = GameplayNotificationBus.Connect(self, self.fovResetEventId);
	
	self.CamProps.FOV = CameraRequestBus.Event.GetFov(self.entityId);
	self.CamProps.DefaultFOV = self.CamProps.FOV;
	
	self.isEnabled = false;
end

function flycam:OnDeactivate()
	
end



function flycam:HandleInput(controllerIn, keyMouseIn, isMouse)
	local out = 0.0;
	
	if (controllerIn[1] ~= 0.0) then
		out = self:SquareWithSign(controllerIn[1]);
	elseif (isMouse) then
		local raw = keyMouseIn[1];
		local scaled = (raw * self.Properties.MouseSensitivityScale);
		out = utilities.Clamp(scaled, -self.Properties.MouseMaxSpeedScale, self.Properties.MouseMaxSpeedScale);
	else
		out = keyMouseIn[1];
	end

	-- if we got an end event for this input, reset the values
	if (controllerIn[2]) then 
		controllerIn[1] = 0.0;
		controllerIn[2] = false;
	end	
	if (keyMouseIn[2]) then
		keyMouseIn[1] = 0.0;
		keyMouseIn[2] = false;
	end
	
	return out;
end

function flycam:OnTick(deltaTime)

	self.InputValues.LookLeftRight 	= self:HandleInput(self.InputValues.Controller.LookLeftRight, 	self.InputValues.KeyMouse.LookLeftRight, 	true);
	self.InputValues.LookUpDown 	= self:HandleInput(self.InputValues.Controller.LookUpDown, 		self.InputValues.KeyMouse.LookUpDown, 		true);
	self.InputValues.MoveForeBack 	= self:HandleInput(self.InputValues.Controller.MoveForeBack, 	self.InputValues.KeyMouse.MoveForeBack, 	false);
	self.InputValues.MoveLeftRight 	= self:HandleInput(self.InputValues.Controller.MoveLeftRight, 	self.InputValues.KeyMouse.MoveLeftRight, 	false);
	self.InputValues.MoveUpDown 	= self:HandleInput(self.InputValues.Controller.MoveUpDown, 		self.InputValues.KeyMouse.MoveUpDown, 		false);
	self.InputValues.FastMode 		= self:HandleInput(self.InputValues.Controller.FastMode, 		self.InputValues.KeyMouse.FastMode, 		false);
	self.InputValues.SlowMode 		= self:HandleInput(self.InputValues.Controller.SlowMode, 		self.InputValues.KeyMouse.SlowMode, 		false);

	-- update fov
	if (self.InputValues.Shared.FOVChange ~= 0.0) then
		self.CamProps.FOV = utilities.Clamp(self.CamProps.FOV + (self.Properties.FOVStep * self.InputValues.Shared.FOVChange), self.Properties.MinFOV, self.Properties.MaxFOV);
		CameraRequestBus.Event.SetFovDegrees(self.entityId, self.CamProps.FOV);
		Debug.Log("FlyCam FOV: " .. self.CamProps.FOV .. " (default is " .. self.CamProps.DefaultFOV .. ")");
		self.InputValues.Shared.FOVChange = 0.0;
	end
	if (self.InputValues.Shared.FOVReset ~= 0.0) then
		self.CamProps.FOV = self.CamProps.DefaultFOV;
		CameraRequestBus.Event.SetFovDegrees(self.entityId, self.CamProps.FOV);
		Debug.Log("FlyCam FOV Reset to " .. self.CamProps.FOV);
		self.InputValues.Shared.FOVReset = 0.0;
	end

	-- update direction
	local leftRight = self.Properties.HorizontalRotateSpeed * deltaTime * self.InputValues.LookLeftRight;
	local upDown = self.Properties.VerticalRotateSpeed * deltaTime * self.InputValues.LookUpDown;
	
	self.CamProps.OrbitHorizontal = self.CamProps.OrbitHorizontal - leftRight;
	self.CamProps.OrbitVertical = self.CamProps.OrbitVertical + upDown;

	if (self.CamProps.OrbitHorizontal < -180.0) then
		self.CamProps.OrbitHorizontal = self.CamProps.OrbitHorizontal + 360.0;
	elseif (self.CamProps.OrbitHorizontal >= 180.0) then
		self.CamProps.OrbitHorizontal = self.CamProps.OrbitHorizontal - 360.0;
	end
	
	-- Clamp the vertical look value (so the camera doesn't go up-side-down).
	if (self.CamProps.OrbitVertical > self.Properties.MaxLookUpAngle) then
		self.CamProps.OrbitVertical = self.Properties.MaxLookUpAngle;
	elseif (self.CamProps.OrbitVertical < self.Properties.MaxLookDownAngle) then
		self.CamProps.OrbitVertical = self.Properties.MaxLookDownAngle;
	end

	local rotZTm = Transform.CreateRotationZ(Math.DegToRad(self.CamProps.OrbitHorizontal));
	local rotXTm = Transform.CreateRotationX(Math.DegToRad(self.CamProps.OrbitVertical));
	local camOrientation = rotZTm * rotXTm;
	
	-- update position
	local speed = self.Properties.MoveSpeed;
	if (self.InputValues.FastMode == 1.0) then
		speed = self.Properties.FastMoveSpeed;
	elseif (self.InputValues.SlowMode == 1.0) then
		speed = self.Properties.SlowMoveSpeed;
	end
	local moveDist = deltaTime * speed;
	local right = camOrientation:GetColumn(0) * moveDist * self.InputValues.MoveLeftRight;
	local forward = camOrientation:GetColumn(1) * moveDist * self.InputValues.MoveForeBack;
	local up = camOrientation:GetColumn(2) * moveDist * self.InputValues.MoveUpDown;
	
	local cameraTm = TransformBus.Event.GetWorldTM(self.entityId);
	local oldPos = cameraTm:GetTranslation();
	local newPos = oldPos + right + forward + up;

	cameraTm = camOrientation;
	cameraTm:SetTranslation(newPos);
	cameraTm:Orthogonalize();

	-- Set the final transform.
	TransformBus.Event.SetWorldTM(self.entityId, cameraTm);

end

function flycam:SetEnabled(enable)
	self.isEnabled = enable;

	-- reset all input values
	self.InputValues.Controller.LookLeftRight[1] = 0.0;
	self.InputValues.Controller.LookUpDown[1] = 0.0;
	self.InputValues.Controller.MoveForeBack[1] = 0.0;
	self.InputValues.Controller.MoveLeftRight[1] = 0.0;
	self.InputValues.Controller.MoveUpDown[1] = 0.0;
	self.InputValues.Controller.FastMode[1] = 0.0;
	self.InputValues.Controller.SlowMode[1] = 0.0;
	self.InputValues.KeyMouse.LookLeftRight[1] = 0.0;
	self.InputValues.KeyMouse.LookUpDown[1] = 0.0;
	self.InputValues.KeyMouse.MoveForeBack[1] = 0.0;
	self.InputValues.KeyMouse.MoveLeftRight[1] = 0.0;
	self.InputValues.KeyMouse.MoveUpDown[1] = 0.0;
	self.InputValues.KeyMouse.FastMode[1] = 0.0;
	self.InputValues.KeyMouse.SlowMode[1] = 0.0;
	
	if (enable) then
		local cameraTm = TransformBus.Event.GetWorldTM(self.entityId);
		
		local forward = cameraTm:GetColumn(1):GetNormalized();
		local flatForward = cameraTm:GetColumn(0):Cross(Vector3(0,0,-1));
		self.CamProps.OrbitHorizontal = -Math.RadToDeg(StarterGameUtility.ArcTan2(flatForward.x, flatForward.y));
		self.CamProps.OrbitVertical = Math.RadToDeg(Math.ArcSin(forward.z));

		self:SetControls("PlayerCharacter", "EnableControlsEvent", 0);
		self:SetControls("PlayerCamera", "EnableControlsEvent", 0);
				
		self.tickBusHandler = TickBus.Connect(self);
	else
		self:SetControls("PlayerCharacter", "EnableControlsEvent", 1);
		self:SetControls("PlayerCamera", "EnableControlsEvent", 1);
		
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end
end

function flycam:SetControls(tag, event, enabled)
	local entity = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32(tag));
	local controlsEventId = GameplayNotificationId(entity, event, "float");
	GameplayNotificationBus.Event.OnEventBegin(controlsEventId, enabled);
end

function flycam:SquareWithSign(value)
	return Math.Sign(value) * value * value;
end

function flycam:HandleInputEvent(value)
	if (self.isEnabled) then
		if (GameplayNotificationBus.GetCurrentBusId() == self.lookLeftRightEventId) then
			self.InputValues.Controller.LookLeftRight[1] = value;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.lookUpDownEventId) then
			self.InputValues.Controller.LookUpDown[1] = value;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.moveForeBackEventId) then
			self.InputValues.Controller.MoveForeBack[1] = value;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.moveLeftRightEventId) then
			self.InputValues.Controller.MoveLeftRight[1] = value;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.moveUpDownEventId) then
			self.InputValues.Controller.MoveUpDown[1] = value;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.fastModeEventId) then
			self.InputValues.Controller.FastMode[1] = value;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.slowModeEventId) then
			self.InputValues.Controller.SlowMode[1] = value;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.lookLeftRightKMEventId) then
			self.InputValues.KeyMouse.LookLeftRight[1] = value;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.lookUpDownKMEventId) then
			self.InputValues.KeyMouse.LookUpDown[1] = value;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.moveForeBackKMEventId) then
			self.InputValues.KeyMouse.MoveForeBack[1] = value;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.moveLeftRightKMEventId) then
			self.InputValues.KeyMouse.MoveLeftRight[1] = value;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.moveUpDownKMEventId) then
			self.InputValues.KeyMouse.MoveUpDown[1] = value;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.fastModeKMEventId) then
			self.InputValues.KeyMouse.FastMode[1] = value;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.slowModeKMEventId) then
			self.InputValues.KeyMouse.SlowMode[1] = value;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.fovChangeEventId) then
			self.InputValues.Shared.FOVChange = value;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.fovResetEventId) then
			self.InputValues.Shared.FOVReset = value;
		end
	end
end

function flycam:OnEventBegin(value)
	if (GameplayNotificationBus.GetCurrentBusId() == self.enabledEventId) then
		self:SetEnabled(value);
	end
	
	self:HandleInputEvent(value);
end

function flycam:OnEventUpdating(value)
	self:HandleInputEvent(value);
end

function flycam:OnEventEnd(value)
	if (self.isEnabled) then
		if (GameplayNotificationBus.GetCurrentBusId() == self.lookLeftRightEventId) then
			self.InputValues.Controller.LookLeftRight[2] = true;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.lookUpDownEventId) then
			self.InputValues.Controller.LookUpDown[2] = true;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.moveForeBackEventId) then
			self.InputValues.Controller.MoveForeBack[2] = true;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.moveLeftRightEventId) then
			self.InputValues.Controller.MoveLeftRight[2] = true;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.moveUpDownEventId) then
			self.InputValues.Controller.MoveUpDown[2] = true;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.fastModeEventId) then
			self.InputValues.Controller.FastMode[2] = true;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.slowModeEventId) then
			self.InputValues.Controller.SlowMode[2] = true;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.lookLeftRightKMEventId) then
			self.InputValues.KeyMouse.LookLeftRight[2] = true;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.lookUpDownKMEventId) then
			self.InputValues.KeyMouse.LookUpDown[2] = true;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.moveForeBackKMEventId) then
			self.InputValues.KeyMouse.MoveForeBack[2] = true;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.moveLeftRightKMEventId) then
			self.InputValues.KeyMouse.MoveLeftRight[2] = true;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.moveUpDownKMEventId) then
			self.InputValues.KeyMouse.MoveUpDown[2] = true;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.fastModeKMEventId) then
			self.InputValues.KeyMouse.FastMode[2] = true;
		elseif (GameplayNotificationBus.GetCurrentBusId() == self.slowModeKMEventId) then
			self.InputValues.KeyMouse.SlowMode[2] = true;
		end
	end
end


return flycam;