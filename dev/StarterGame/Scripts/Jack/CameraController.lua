local utilities = require "scripts/common/utilities"

local cameracontroller =
{
	Properties =
	{
		InputEntity = {default = EntityId()},

		PlayerEntity = {default = EntityId()},

		Controller =
		{
			DeadzoneInner = {default = 0.15, description = "Deadzone for the minimum stick input."},
			DeadzoneOuter = {default = 0.95, description = "Deadzone for the maximum stick input."},
		},
		
		MinDistanceFromGeometry = {default = 0.1, description = "Distance (in meters) that the camera pushes itself away from geometry."},
		
		EaseOut =
		{
			Acceleration = {default = 15.0, description = "The acceleration of the camera as it starts to move back."},
			MaxSpeed = {default = 10.0, description = "The speed at which the camera pushes away from the player when obstacles are no longer restricting it."},
		},
		
		Events =
		{
			ControlsEnabled = { default = "EnableControlsEvent", description = "If passed '1.0' it will enable controls, otherwise it will disable them." },
			ResetOrientation = { default = "ResetOrientationEvent", description = "Resets the orientation of the camera." },
		},
	},
	
	-- Values that have come from the input device.
	InputValues = 
	{
		-- From programmatically forced events (death screen, rock slides, etc.).
		Forced =
		{
			LookLeftRight = 0.0,
			LookUpDown = 0.0,
		},
		-- Controller.
		Controller =
		{
			LookLeftRight = {0.0, false},
			LookUpDown = {0.0, false},
		},
		-- Mouse.
		Mouse =
		{
			LookLeftRight = {0.0, false},
			LookUpDown = {0.0, false},
		},
		
		-- These are the processed values.
		LookLeftRight = 0.0,
		LookUpDown = 0.0,
	},

	-- The camera's rotation values.
	CamProps =
	{
		OrbitHorizontal = 0.0,
		OrbitVertical = 0.0,
		
		Prev =
		{
			H = 0.0,
			V = 0.0,
		},
	},
}

function cameracontroller:OnActivate()

	self.lookLeftRightEventId = GameplayNotificationId(self.entityId, "LookLeftRight", "float");
	self.lookLeftRightHandler = GameplayNotificationBus.Connect(self, self.lookLeftRightEventId);
	self.lookUpDownEventId = GameplayNotificationId(self.entityId, "LookUpDown", "float");
	self.lookUpDownHandler = GameplayNotificationBus.Connect(self, self.lookUpDownEventId);
	
	self.lookLeftRightKMEventId = GameplayNotificationId(self.entityId, "LookLeftRightKM", "float");
	self.lookLeftRightKMHandler = GameplayNotificationBus.Connect(self, self.lookLeftRightKMEventId);
	self.lookUpDownKMEventId = GameplayNotificationId(self.entityId, "LookUpDownKM", "float");
	self.lookUpDownKMHandler = GameplayNotificationBus.Connect(self, self.lookUpDownKMEventId);

	self.lookLeftRightForcedEventId = GameplayNotificationId(self.entityId, "LookLeftRightForced", "float");
	self.lookLeftRightForcedHandler = GameplayNotificationBus.Connect(self, self.lookLeftRightForcedEventId);
	self.lookUpDownForcedEventId = GameplayNotificationId(self.entityId, "LookUpDownForced", "float");
	self.lookUpDownForcedHandler = GameplayNotificationBus.Connect(self, self.lookUpDownForcedEventId);
	
	self.controlsDisabledCount = 0;
	self.controlsEnabled = true;
	self.controlsEnabledEventId = GameplayNotificationId(self.entityId, self.Properties.Events.ControlsEnabled, "float");
	self.controlsEnabledHandler = GameplayNotificationBus.Connect(self, self.controlsEnabledEventId);
	self.resetOrientationEventId = GameplayNotificationId(self.entityId, self.Properties.Events.ResetOrientation, "float");
	self.resetOrientationHandler = GameplayNotificationBus.Connect(self, self.resetOrientationEventId);
	
	self.cameraSettingsEventId = GameplayNotificationId(self.entityId, "CameraSettings", "float");
	self.cameraSettingsHandler = GameplayNotificationBus.Connect(self, self.cameraSettingsEventId);

	self.debugNumPad1EventId = GameplayNotificationId(self.entityId, "NumPad1", "float");
	self.debugNumPad1Handler = GameplayNotificationBus.Connect(self, self.debugNumPad1EventId);
	self.debugNumPad2EventId = GameplayNotificationId(self.entityId, "NumPad2", "float");
	self.debugNumPad2Handler = GameplayNotificationBus.Connect(self, self.debugNumPad2EventId);
	self.debugNumPad3EventId = GameplayNotificationId(self.entityId, "NumPad3", "float");
	self.debugNumPad3Handler = GameplayNotificationBus.Connect(self, self.debugNumPad3EventId);
	self.debugNumPad4EventId = GameplayNotificationId(self.entityId, "NumPad4", "float");
	self.debugNumPad4Handler = GameplayNotificationBus.Connect(self, self.debugNumPad4EventId);
	self.debugNumPad5EventId = GameplayNotificationId(self.entityId, "NumPad5", "float");
	self.debugNumPad5Handler = GameplayNotificationBus.Connect(self, self.debugNumPad5EventId);
	self.debugNumPad6EventId = GameplayNotificationId(self.entityId, "NumPad6", "float");
	self.debugNumPad6Handler = GameplayNotificationBus.Connect(self, self.debugNumPad6EventId);
	self.debugNumPad7EventId = GameplayNotificationId(self.entityId, "NumPad7", "float");
	self.debugNumPad7Handler = GameplayNotificationBus.Connect(self, self.debugNumPad7EventId);
	self.debugNumPad8EventId = GameplayNotificationId(self.entityId, "NumPad8", "float");
	self.debugNumPad8Handler = GameplayNotificationBus.Connect(self, self.debugNumPad8EventId);
	self.debugNumPad9EventId = GameplayNotificationId(self.entityId, "NumPad9", "float");
	self.debugNumPad9Handler = GameplayNotificationBus.Connect(self, self.debugNumPad9EventId);

	self.CurrentSettings = CameraSettings();
	self.TransitionFromSettings = CameraSettings();
	self.TransitionToSettings = CameraSettings();
	self.TransitionTimer = 0.0;
	self.TransitionDuration = 0.0;
	self.TransitionDurationInv = 0.0;

	-- Take note of the camera's original rotation.
	local tm = TransformBus.Event.GetWorldTM(self.entityId);
	local flatForward = tm:GetColumn(0):Cross(Vector3(0,0,-1));
	self.CamProps.OrbitHorizontal = -Math.RadToDeg(StarterGameUtility.ArcTan2(flatForward.x, flatForward.y));
	
	self:StorePrevOrbitValues();
	self.easeOutSpeed = 0.0;

	self.currentHorizontalMax = 0.0;

	self.tickBusHandler = TickBus.Connect(self);
	self.performedFirstUpdate = false;
	self.performedFirstUpdateAfterReset = false;
	
end

function cameracontroller:OnDeactivate()

	self.lookLeftRightHandler:Disconnect();
	self.lookUpDownHandler:Disconnect();
	
	self.lookLeftRightKMHandler:Disconnect();
	self.lookUpDownKMHandler:Disconnect();
	
	self.lookLeftRightForcedHandler:Disconnect();
	self.lookUpDownForcedHandler:Disconnect();
	
	self.controlsEnabledHandler:Disconnect();
	self.resetOrientationHandler:Disconnect();
	
	self.debugNumPad1Handler:Disconnect();
	self.debugNumPad2Handler:Disconnect();
	self.debugNumPad3Handler:Disconnect();
	self.debugNumPad4Handler:Disconnect();
	self.debugNumPad5Handler:Disconnect();
	self.debugNumPad6Handler:Disconnect();
	self.debugNumPad7Handler:Disconnect();
	self.debugNumPad8Handler:Disconnect();
	self.debugNumPad9Handler:Disconnect();

	self.tickBusHandler:Disconnect();

end

function cameracontroller:OnTick(deltaTime, timePoint)
	
	self:UpdateSettings(deltaTime);

	self:UpdateInput(deltaTime);

	self:UpdateAngles(deltaTime);
	
	self:UpdateTransform(deltaTime);

end

function cameracontroller:UpdateInput(deltaTime)
	-- Check if the controller is being used. The current priority for input is:
	--  1. Forced
	--  2. Controller
	--  3. Mouse
	local inputVec = Vector2(0.0,0.0);
	local inputForced, magForced = self:GetForcedInputVec(self.InputValues.Forced);
	local inputCont, magCont = self:GetInputVec(self.InputValues.Controller);
	local inputMouse, magMouse = self:GetInputVec(self.InputValues.Mouse);
	if (magForced > 0.0) then
		inputVec = inputForced;
	elseif (magCont > 0.0) then
		-- rescale stick input so magnitude is 0 at inner deadzone and 1 at outer deadzone
		local dzInner = self.Properties.Controller.DeadzoneInner;
		local dzRange = self.Properties.Controller.DeadzoneOuter - dzInner;
		magCont = utilities.Clamp((magCont - dzInner) / dzRange, 0.0, 1.0);
		inputCont = inputCont:GetNormalized() * magCont;

		-- remap circle to square
		-- http://squircular.blogspot.co.uk/2015/09/mapping-circle-to-square.html
		-- x = ½ √( 2 + u² - v² + 2u√2 ) - ½ √( 2 + u² - v² - 2u√2 )
		-- y = ½ √( 2 - u² + v² + 2v√2 ) - ½ √( 2 - u² + v² - 2v√2 )		

		local u2 = (inputCont.x * inputCont.x);
		local v2 = (inputCont.y * inputCont.y);
		local twosqrt2 = 2.0 * math.sqrt(2.0);
		local subtermx = 2.0 + u2 - v2;
		local subtermy = 2.0 - u2 + v2;
		local termx1 = math.max(subtermx + inputCont.x * twosqrt2, 0.0);
		local termx2 = math.max(subtermx - inputCont.x * twosqrt2, 0.0);
		local termy1 = math.max(subtermy + inputCont.y * twosqrt2, 0.0);
		local termy2 = math.max(subtermy - inputCont.y * twosqrt2, 0.0);
		inputVec.x = utilities.Clamp(self:PowWithSign(0.5 * math.sqrt(termx1) - 0.5 * math.sqrt(termx2), self.CurrentSettings.ControllerSensitivityPower), -1.0, 1.0);
		inputVec.y = utilities.Clamp(self:PowWithSign(0.5 * math.sqrt(termy1) - 0.5 * math.sqrt(termy2), self.CurrentSettings.ControllerSensitivityPower), -1.0, 1.0);				
		
		-- if stick pushed to horizontal extreme, accelerate the rotation
		if (math.abs(inputVec.x) > 0.99) then
			self.currentHorizontalMax = math.min(self.currentHorizontalMax + (self.CurrentSettings.HorizontalBoostAccel * deltaTime), self.CurrentSettings.HorizontalBoostSpeed);
		else -- not pushing stick all the way, decelerate horizontal speed back to normal if necessary
			self.currentHorizontalMax = math.max(self.currentHorizontalMax - (self.CurrentSettings.HorizontalBoostDecel * deltaTime), self.CurrentSettings.HorizontalSpeed);
		end

	elseif (magMouse > 0.0) then
		-- If nothing else, default to use the mouse.
		self.currentHorizontalMax = self.CurrentSettings.HorizontalSpeed;
		-- TODO: work out how to smooth infrequent mouse updates and correct for frame time
		local mouseMaxSpeedH = self.CurrentSettings.HorizontalBoostSpeed / self.CurrentSettings.HorizontalSpeed;
		inputVec.x = utilities.Clamp(inputMouse.x * self.CurrentSettings.MouseSensitivityScale, -mouseMaxSpeedH, mouseMaxSpeedH);
		inputVec.y = utilities.Clamp(inputMouse.y * self.CurrentSettings.MouseSensitivityScale, -1.0, 1.0);
	else -- no input at all, reset horizontal speed back to normal
		self.currentHorizontalMax = self.CurrentSettings.HorizontalSpeed;
	end
	self.InputValues.LookLeftRight = inputVec.x;
	self.InputValues.LookUpDown = inputVec.y;
end

function cameracontroller:UpdateAngles(deltaTime)
	-- Store the camera rotations.
	local leftRight = self.currentHorizontalMax * deltaTime * self.InputValues.LookLeftRight;
	local upDown = self.CurrentSettings.VerticalSpeed * deltaTime * self.InputValues.LookUpDown;
	self.CamProps.OrbitHorizontal = self.CamProps.OrbitHorizontal - leftRight;
	self.CamProps.OrbitVertical = self.CamProps.OrbitVertical + upDown;

	if (self.CamProps.OrbitHorizontal < -180.0) then
		self.CamProps.OrbitHorizontal = self.CamProps.OrbitHorizontal + 360.0;
	elseif (self.CamProps.OrbitHorizontal >= 180.0) then
		self.CamProps.OrbitHorizontal = self.CamProps.OrbitHorizontal - 360.0;
	end
	
	-- Clamp the vertical look value (so the camera doesn't go up-side-down).
	if (self.CamProps.OrbitVertical > self.CurrentSettings.MaxLookUpAngle) then
		self.CamProps.OrbitVertical = self.CurrentSettings.MaxLookUpAngle;
	elseif (self.CamProps.OrbitVertical < self.CurrentSettings.MaxLookDownAngle) then
		self.CamProps.OrbitVertical = self.CurrentSettings.MaxLookDownAngle;
	end
end

function cameracontroller:UpdateTransform(deltaTime)
	-- Find out the camera's follow distance.
	-- When above level we want to use the 'CameraFollowDistance'. When below level we want to
	-- interpolate between 'CameraFollowDistance' and 'CameraFollowDistanceBelow' so that when
	-- the player is looking directly up the distance is shorter (so the designer can pull the
	-- camera out of the ground cover and foliage).
	local camFollowDist = self.CurrentSettings.FollowDistance;
	if (self.CamProps.OrbitVertical > 0) then
		local s = self.CamProps.OrbitVertical / self.CurrentSettings.MaxLookUpAngle;
		local a = camFollowDist * (1.0 - s);
		local b = self.CurrentSettings.FollowDistanceLow * s;
		camFollowDist = a + b;
	end
	
	-- Use the camera's rotation to get the corrent vector we want to push away with.
	local rotZTm = Transform.CreateRotationZ(Math.DegToRad(self.CamProps.OrbitHorizontal));
	local rotXTm = Transform.CreateRotationX(Math.DegToRad(self.CamProps.OrbitVertical));
	local camOrientation = rotZTm * rotXTm;
	local camPushBack = camOrientation * Vector3(0.0, camFollowDist, 0.0);
	local newCamRight = camOrientation * Vector3(1.0, 0.0, 0.0);
	
	-- Find the pivot that we want to use based on the height of the camera (front or back).
	local pivotFrontOrBackOffset = Vector3(camPushBack.x, camPushBack.y, 0.0);
	pivotFrontOrBackOffset:Normalize();
	local pivotRange = self.CurrentSettings.PivotOffsetBack;
	if (self.CamProps.OrbitVertical < 0) then
		pivotRange = self.CurrentSettings.PivotOffsetFront;
	end
	pivotFrontOrBackOffset = pivotFrontOrBackOffset * pivotRange;
	local pivotOffsetLength = pivotFrontOrBackOffset:GetLength();
	if (self.CamProps.OrbitVertical < 0) then
		pivotFrontOrBackOffset = pivotFrontOrBackOffset * -1.0;
		pivotOffsetLength = pivotOffsetLength * -1.0;
	end

	-- Find the pivot point that the camera will be rotating about.
	local characterTm = TransformBus.Event.GetWorldTM(self.Properties.PlayerEntity);
	local pivotOffset = newCamRight * self.CurrentSettings.FollowOffset;
	local pivotPoint = characterTm:GetTranslation() + Vector3(0.0, 0.0, self.CurrentSettings.FollowHeight) + pivotOffset - pivotFrontOrBackOffset;
	
	-- Raycast from a point centered on the player to the offset pivot point to make sure the pivot point
	-- doesn't go to the other side of a wall when the player stands next to one.
	local pointInPlayer = characterTm:GetTranslation() + Vector3(0.0, 0.0, self.CurrentSettings.FollowHeight);
	local playerToPivot = pivotPoint - pointInPlayer;
	local playerToPivotDist = playerToPivot:GetLength();
	playerToPivot = playerToPivot / playerToPivotDist; -- normalize
	local mask = PhysicalEntityTypes.Static + PhysicalEntityTypes.Dynamic + PhysicalEntityTypes.Independent + PhysicalEntityTypes.Terrain;
	
	local rayCastConfig = RayCastConfiguration();
	rayCastConfig.origin = pointInPlayer;
	rayCastConfig.direction =  playerToPivot;
	rayCastConfig.maxDistance = playerToPivot:GetLength();
	rayCastConfig.maxHits = 10;
	rayCastConfig.physicalEntityTypes = mask;
	rayCastConfig.piercesSurfacesGreaterThan = 13;
	
	local offsetHits = PhysicsSystemRequestBus.Broadcast.RayCast(rayCastConfig);
	local hasOffsetHit = offsetHits:HasBlockingHit();
	local offsetBlockingHit = offsetHits:GetBlockingHit();
	
	if (hasOffsetHit) then
		-- If there's something between the pivot point and the player then move the pivot point in.
		pivotPoint = offsetBlockingHit.position - (playerToPivot:GetNormalized() * self.Properties.MinDistanceFromGeometry);
	end

	-- Recalculate the pushback for the new length (as the pivot point's moved).
	camPushBack = camOrientation * Vector3(0.0, camFollowDist - pivotOffsetLength, 0.0);

	-- Push the camera back from the player's position based on the camera's rotation.
	local camPos = pivotPoint - camPushBack;
	
	-- Pull the camera back in so it doesn't go through geometry.
	local charToCam = camPos - pivotPoint;
	local charToCamDist = charToCam:GetLength();
	charToCam = charToCam / charToCamDist; -- normalize
	local length = charToCamDist + self.Properties.MinDistanceFromGeometry;
	rayCastConfig.origin = pivotPoint;
	rayCastConfig.direction =  charToCam;
	rayCastConfig.maxDistance = length;
	
	local hits = PhysicsSystemRequestBus.Broadcast.RayCast(rayCastConfig);
	local hasHit = hits:HasBlockingHit();
	local blockingHit = hits:GetBlockingHit();
	
	-- If the distance to the collision is 0.0 then we hit nothing.
	-- If the entityID is valid then we hit a component entity; otherwise we hit a legacy entity (or terrain).
	if (hasHit) then
		camPos = blockingHit.position - (charToCam:GetNormalized() * self.Properties.MinDistanceFromGeometry);
		--camPos = camPos + (rch.normal * 0.05);	-- Push it off the surface.
	end
	
	local newCamDist = 0.0;
	
	-- We need to set the camera to the correct location on the first tick so that
	-- all of the 'previous' values can be initialised correctly.
	if (not (self.performedFirstUpdate and self.performedFirstUpdateAfterReset)) then
		self.performedFirstUpdate = true;
		self.performedFirstUpdateAfterReset = true;
	else
		-- If the camera is being pushed out (i.e. an obstacle that was blocking it is no longer
		-- in the way) then gradually ease it out instead of snapping.
		newCamDist = (camPos - pivotPoint):GetLength();
		-- If we've gone past the pivot point then adjust the distance we stored last frame so we
		-- have a good comparison to the distance this frame.
		if ((self.CamProps.OrbitVertical < 0 and self.CamProps.Prev.V >= 0) or (self.CamProps.OrbitVertical >= 0 and self.CamProps.Prev.V < 0)) then
			self.prevCamDist = self.prevCamDist + (self.CurrentSettings.PivotOffsetFront + self.CurrentSettings.PivotOffsetBack);
		end
		local distDiff = newCamDist - self.prevCamDist;
		
		if (distDiff <= 0.0001) then
			-- The camera either didn't move or has been pushed in. Either way, we want to reset
			-- the speed and not clamp the distance.
			self.easeOutSpeed = 0.0;
		else
			-- Ease out.
			--		v^2 = u^2 + 2as
			--		0 = u^2 + 2as
			--		-2as = u^2
			--		a = u^2 / -2s
			-- Calculate 'a'
			-- Compare 'a' with standard acceleration.
			-- If abs 'a' is less than standard acc then keep accelerating.
			-- If abs 'a' is more than standard acc then decelerate.
			local standardAcc = self.Properties.EaseOut.Acceleration;
			local u = self.easeOutSpeed;
			local s = distDiff;
			
			local a = math.abs((u*u) / (2.0 * s));
			
			if (a < standardAcc) then
				-- Accelerate.
				self.easeOutSpeed = utilities.Clamp(self.easeOutSpeed + (standardAcc * deltaTime), 0.0, self.Properties.EaseOut.MaxSpeed);
			else
				-- Decelerate.
				self.easeOutSpeed = utilities.Clamp(self.easeOutSpeed - (a * deltaTime), 0.0, self.Properties.EaseOut.MaxSpeed);
			end
			newCamDist = math.min(self.prevCamDist + (self.easeOutSpeed * deltaTime), newCamDist);
		end
		camPos = pivotPoint + (charToCam * newCamDist);
	end

	-- Now apply the rotation.
	local cameraTm = TransformBus.Event.GetWorldTM(self.entityId);
	cameraTm = camOrientation;
	cameraTm:SetTranslation(camPos);
	cameraTm:Orthogonalize();

	-- Set the final transform.
	TransformBus.Event.SetWorldTM(self.entityId, cameraTm);
	
	-- Prepare for the next tick.
	self:StorePrevOrbitValues();
	self.prevCamDist = (camPos - pivotPoint):GetLength();
end

function cameracontroller:StorePrevOrbitValues()
	self.CamProps.Prev.H = self.CamProps.OrbitHorizontal;
	self.CamProps.Prev.V = self.CamProps.OrbitVertical;
end

function cameracontroller:GetForcedInputVec(input)
	local vec = Vector2(input.LookLeftRight, input.LookUpDown);
	return vec, vec:GetLength();
end

function cameracontroller:GetInputVec(input)
	local vec = Vector2(input.LookLeftRight[1], input.LookUpDown[1]);
	-- if we got end events for these inputs, reset them
	if (input.LookLeftRight[2]) then
		input.LookLeftRight[1] = 0.0;
		input.LookLeftRight[2] = false;
	end
	if (input.LookUpDown[2]) then
		input.LookUpDown[1] = 0.0;
		input.LookUpDown[2] = false;
	end
	return vec, vec:GetLength();
end

-- This function square a float while maintaining the original sign.
function cameracontroller:PowWithSign(i, p)

	local res = math.pow(math.abs(i), p);
	if (i < 0.0) then
		res = res * -1.0;
	end
	
	return res;

end

function cameracontroller:SetNewSettings(newSettings)
	--Debug.Log("Camera changes to " .. newSettings.Name .. " time = " .. newSettings.TransitionTime);
	local transTime = newSettings.TransitionTime;
	
	-- if setting from nothing, snap
	-- elseif going back to previous settings while still transitioning, reverse the timer (and rescale)
	-- else use supplied transition time
	if (self.CurrentSettings.Name == "" or transTime == 0.0) then
		--Debug.Log("... snap");
		self.CurrentSettings:CopySettings(newSettings);
		self:ApplyRenderCameraValues();
		self.TransitionTimer = 0.0;
		self.TransitionDuration = 0.0;
		self.TransitionDurationInv = 0.0;
	elseif (self.TransitionTimer > 0.0 and newSettings.Name == self.TransitionFromSettings.Name) then
		--Debug.Log("... bounce");
		local rescale = transTime * self.TransitionDurationInv; -- if transition time is different, need to rescale how long we take accordingly
		self.TransitionTimer = (self.TransitionDuration - self.TransitionTimer) * rescale;
		self.TransitionDuration = transTime;
		self.TransitionDurationInv = 1.0 / transTime;
		self.TransitionFromSettings:CopySettings(self.TransitionToSettings);
		self.TransitionToSettings:CopySettings(newSettings);
	else
		--Debug.Log("... slide");
		self.TransitionTimer = transTime; -- TODO: get duration for this mode change, snap if first setting
		self.TransitionDuration = transTime;
		self.TransitionDurationInv = 1.0 / transTime;
		self.TransitionFromSettings:CopySettings(self.CurrentSettings);
		self.TransitionToSettings:CopySettings(newSettings);
	end

	self.CurrentSettings.Name = newSettings.Name;
end

function cameracontroller:UpdateSettings(deltaTime)
	if (self.TransitionTimer > 0.0) then
		self.TransitionTimer = math.max(self.TransitionTimer - deltaTime, 0.0);
		self.CurrentSettings:BlendSettings(self.TransitionFromSettings, self.TransitionToSettings, 1.0 - (self.TransitionTimer * self.TransitionDurationInv));
		self:ApplyRenderCameraValues();
	end
end

function cameracontroller:ApplyRenderCameraValues()
	CameraRequestBus.Event.SetFovDegrees(self.entityId, self.CurrentSettings.FOV);
	CameraRequestBus.Event.SetNearClipDistance(self.entityId, self.CurrentSettings.NearClip);
	CameraRequestBus.Event.SetFarClipDistance(self.entityId, self.CurrentSettings.FarClip);
end

function cameracontroller:SetControlsEnabled(newControlsEnabled)
	self.controlsEnabled = newControlsEnabled;
	if (newControlsEnabled) then 
		self:ResetOrientation();
	else
		self.InputValues.Controller.LookLeftRight[1] = 0.0;
		self.InputValues.Controller.LookUpDown[1] = 0.0;
		self.InputValues.Mouse.LookLeftRight[1] = 0.0;
		self.InputValues.Mouse.LookUpDown[1] = 0.0;
	end	
end

function cameracontroller:ResetOrientation()
	local characterTm = TransformBus.Event.GetWorldTM(self.Properties.PlayerEntity);
	local flatForward = characterTm:GetColumn(0):Cross(Vector3(0,0,-1));
	self.CamProps.OrbitHorizontal = -Math.RadToDeg(StarterGameUtility.ArcTan2(flatForward.x, flatForward.y));
	self.CamProps.OrbitVertical = 0.0;
	self:StorePrevOrbitValues();
	self.performedFirstUpdateAfterReset = false;
	self:UpdateTransform(0.0);
end

function cameracontroller:OnEventBegin(value)
	
	local busId = GameplayNotificationBus.GetCurrentBusId();
	if (busId == self.controlsEnabledEventId) then
		if (value == 1.0) then
			self.controlsDisabledCount = self.controlsDisabledCount - 1;
		else
			self.controlsDisabledCount = self.controlsDisabledCount + 1;
		end
		if (self.controlsDisabledCount < 0) then
			Debug.Log("[Warning] CameraController: controls disabled ref count is less than 0.  Probable enable/disable mismatch.");
			self.controlsDisabledCount = 0;
		end		

		local newEnabled = self.controlsDisabledCount == 0;
		if (self.controlsEnabled ~= newEnabled) then
			self:SetControlsEnabled(newEnabled);
		end
	elseif (busId == self.resetOrientationEventId) then
		self:ResetOrientation();
	end
	
	if (busId == self.lookLeftRightForcedEventId) then
		--Debug.Log("lookLeftRightForcedEvent: " .. tostring(value));
		self.InputValues.Forced.LookLeftRight = value;
	elseif (busId == self.lookUpDownForcedEventId) then
		--Debug.Log("LookUpDownForcedEvent: " .. tostring(value));
		self.InputValues.Forced.LookUpDown = value;
	end
	
	if (self.controlsEnabled == true) then
		if (busId == self.lookLeftRightEventId) then
			self.InputValues.Controller.LookLeftRight[1] = value;
		elseif (busId == self.lookUpDownEventId) then
			self.InputValues.Controller.LookUpDown[1] = value;
		elseif (busId == self.lookLeftRightKMEventId) then
			self.InputValues.Mouse.LookLeftRight[1] = value;
		elseif (busId == self.lookUpDownKMEventId) then
			self.InputValues.Mouse.LookUpDown[1] = value;
		end
	end
	
	if (busId == self.cameraSettingsEventId) then
		--Debug.Log("Heard event to change camera settings to '" .. tostring(value.Name) .. "'");
		-- TODO: get transition duration
		self:SetNewSettings(value);
	end
			
	-- Debug input for moving the camera.
	if (self.TransitionTimer <= 0.0) then
		local debugCamMovement = false;
		local adjustStep = 0.05;
		if (busId == self.debugNumPad1EventId) then
			self.CurrentSettings.FollowDistanceLow = self.CurrentSettings.FollowDistanceLow - adjustStep;
			debugCamMovement = true;
		elseif (busId == self.debugNumPad3EventId) then
			self.CurrentSettings.FollowDistanceLow = self.CurrentSettings.FollowDistanceLow + adjustStep;
			debugCamMovement = true;
		elseif (busId == self.debugNumPad8EventId) then
			self.CurrentSettings.FollowDistance = self.CurrentSettings.FollowDistance - adjustStep;
			debugCamMovement = true;
		elseif (busId == self.debugNumPad2EventId) then
			self.CurrentSettings.FollowDistance = self.CurrentSettings.FollowDistance + adjustStep;
			debugCamMovement = true;
		elseif (busId == self.debugNumPad4EventId) then
			self.CurrentSettings.FollowOffset = self.CurrentSettings.FollowOffset - adjustStep;
			debugCamMovement = true;
		elseif (busId == self.debugNumPad6EventId) then
			self.CurrentSettings.FollowOffset = self.CurrentSettings.FollowOffset + adjustStep;
			debugCamMovement = true;
		elseif (busId == self.debugNumPad9EventId) then
			self.CurrentSettings.FollowHeight = self.CurrentSettings.FollowHeight - adjustStep;
			debugCamMovement = true;
		elseif (busId == self.debugNumPad7EventId) then
			self.CurrentSettings.FollowHeight = self.CurrentSettings.FollowHeight + adjustStep;
			debugCamMovement = true;
		elseif (busId == self.debugNumPad5EventId) then
			self.CurrentSettings:CopySettings(self.TransitionToSettings);
			debugCamMovement = true;
		end
	
		if (debugCamMovement == true) then
			Debug.Log("FollowDistanceLow: " .. self.CurrentSettings.FollowDistanceLow .. ", FollowDistance: " .. self.CurrentSettings.FollowDistance .. ", FollowHeight: " .. self.CurrentSettings.FollowHeight .. ", FollowOffset: " .. self.CurrentSettings.FollowOffset);
		end
	end
	
end

function cameracontroller:OnEventUpdating(value)
	
	local busId = GameplayNotificationBus.GetCurrentBusId();
	if (self.controlsEnabled == true) then
		if (busId == self.lookLeftRightEventId) then
			self.InputValues.Controller.LookLeftRight[1] = value;
		elseif (busId == self.lookUpDownEventId) then
			self.InputValues.Controller.LookUpDown[1] = value;
		elseif (busId == self.lookLeftRightKMEventId) then
			self.InputValues.Mouse.LookLeftRight[1] = value;
		elseif (busId == self.lookUpDownKMEventId) then
			self.InputValues.Mouse.LookUpDown[1] = value;
		end
	end

end

function cameracontroller:OnEventEnd()
	
	local busId = GameplayNotificationBus.GetCurrentBusId();
	if (busId == self.lookLeftRightEventId) then
		self.InputValues.Controller.LookLeftRight[2] = true;
	elseif (busId == self.lookUpDownEventId) then
		self.InputValues.Controller.LookUpDown[2] = true;
	elseif (busId == self.lookLeftRightKMEventId) then
		self.InputValues.Mouse.LookLeftRight[2] = true;
	elseif (busId == self.lookUpDownKMEventId) then
		self.InputValues.Mouse.LookUpDown[2] = true;
	elseif (busId == self.lookLeftRightForcedEventId) then
		self.InputValues.Forced.LookLeftRight = 0;
	elseif (busId == self.lookUpDownForcedEventId) then
		self.InputValues.Forced.LookUpDown = 0;
	end

end

return cameracontroller;
