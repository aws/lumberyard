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


local cameramanager =
{
	Properties =
	{
		InitialCamera = { default = EntityId(), },
		InitialCameraTag = { default = "PlayerCamera" },
		PlayerCameraTag = { default = "PlayerCamera" },
		FlyCamTag = { default = "FlyCam" },
		
		ActiveCamTag = { default = "ActiveCamera", description = "The tag that's used and applied at runtime to identify the active camera." },
	},
}

function cameramanager:OnActivate()

	self.activateEventId = GameplayNotificationId(self.entityId, "ActivateCameraEvent", "float");
	self.activateHandler = GameplayNotificationBus.Connect(self, self.activateEventId);
	
	self.pushCameraSettingsEventId = GameplayNotificationId(self.entityId, "PushCameraSettings", "float");
	self.pushCameraSettingsHandler = GameplayNotificationBus.Connect(self, self.pushCameraSettingsEventId);
	self.popCameraSettingsEventId = GameplayNotificationId(self.entityId, "PopCameraSettings", "float");
	self.popCameraSettingsHandler = GameplayNotificationBus.Connect(self, self.popCameraSettingsEventId);

	self.toggleFlyCamEventId = GameplayNotificationId(self.entityId, "ToggleFlyCam", "float");
	self.toggleFlyCamHandler = GameplayNotificationBus.Connect(self, self.toggleFlyCamEventId);
	
	self.transitionTime = 0.0;
	
	self.activeCamTagCrc = Crc32(self.Properties.ActiveCamTag);
	
	-- We can't set the initial camera here because there's no gaurantee that a camera won't
	-- be initialised AFTER the camera manager and override what we've set here. As a result
	-- we need to join the tick bus so we can set the initial camera on the first tick and
	-- then disconnect from the tick bus (since there shouldn't be anything that we need to
	-- do every frame).
	self.tickHandler = TickBus.Connect(self);
	
	local touchDevice = InputDeviceRequestBus.Event.GetInputDevice(InputDeviceId(InputDeviceTouch.name))
	if (touchDevice and touchDevice:IsSupported()) then
		-- Load the virtual gamepad ui canvas, but disable it if a physical gamepad is already connected,
		-- and listen for input device notifications to detect if a physical gamepad connects/disconnects
		self.virtualGamepadCanvasId = UiCanvasManagerBus.Broadcast.LoadCanvas("UI/Canvases/UIVirtualGamepad.uicanvas");
		if (self.virtualGamepadCanvasId) then
			self.inputDeviceNotificationBus = InputDeviceNotificationBus.Connect(self);
			local gamepadDevice = InputDeviceRequestBus.Event.GetInputDevice(InputDeviceId(InputDeviceGamepad.name))
			if (gamepadDevice and gamepadDevice:IsConnected()) then
				UiCanvasBus.Event.SetEnabled(self.virtualGamepadCanvasId, false);
			end
		end
	end

end

function cameramanager:OnDeactivate()
	
	if (self.cameraSettingsHandler ~= nil) then
		self.cameraSettingsHandler:Disconnect();
		self.cameraSettingsHandler = nil;
	end
	if (self.activateHandler ~= nil) then
		self.activateHandler:Disconnect();
		self.activateHandler = nil;
	end

    if (self.virtualGamepadCanvasId) then
        UiCanvasManagerBus.Broadcast.UnloadCanvas(self.virtualGamepadCanvasId);
    end
    if (self.inputDeviceNotificationBus) then
        self.inputDeviceNotificationBus:Disconnect();
    end

end
	
-- Function when an input device is connected
function cameramanager:OnInputDeviceConnectedEvent(inputDevice)
    if (inputDevice.deviceName == InputDeviceGamepad.name and inputDevice.deviceIndex == 0) then
        UiCanvasBus.Event.SetEnabled(self.virtualGamepadCanvasId, false);
    end
end

-- Function when an input device is disconnected
function cameramanager:OnInputDeviceDisonnectedEvent(inputDevice)
    if (inputDevice.deviceName == InputDeviceGamepad.name and inputDevice.deviceIndex == 0) then
        UiCanvasBus.Event.SetEnabled(self.virtualGamepadCanvasId, true);
    end
end

function cameramanager:OnTick(deltaTime, timePoint)
	
	local startingCam = self.Properties.InitialCamera;
	if (not startingCam:IsValid()) then
		-- TODO: Change this to a 'Warning()'.
		Debug.Log("Camera Manager: No initial camera has been set - finding and using the camera tagged with " .. self.Properties.InitialCameraTag .. " instead.");
		startingCam = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32(self.Properties.InitialCameraTag));
		
		-- If we couldn't find a camera with the fallback tag then we can't reliably assume
		-- which camera became the active one.
		if (not startingCam:IsValid()) then
			-- TODO: Change this to an 'Assert()'.
			Debug.Log("Camera Manager: No initial camera could be found.");
		end
	end

	self.playerCameraId = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32(self.Properties.PlayerCameraTag));
	self.playerCameraSettingsEventId = GameplayNotificationId(self.playerCameraId, "CameraSettings", "float");

	self.flyCamId = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32(self.Properties.FlyCamTag));
	self.flyCamEnableEventId = GameplayNotificationId(self.flyCamId, "EnableFlyCam", "float");
	self.flyCamActive = false;
	
	self.currentSettingsName = "";
	self:UpdatePlayerCameraSettings();

	self:ActivateCam(startingCam);
	
	-- This function is a work-around because we can't disconnect the tick handler directly
	-- inside the 'OnTick()' function, but we can inside another function; apparently, even
	-- if that function is called FROM the 'OnTick()' function.
	self:StopTicking();

end

function cameramanager:StopTicking()

	self.tickHandler:Disconnect();
	self.tickHandler = nil;
	
end

function cameramanager:ToggleFlyCam()
	if (self.flyCamActive) then
		-- disable fly cam
		GameplayNotificationBus.Event.OnEventBegin(self.flyCamEnableEventId, false);
		self.flyCamActive = false;

		-- activate previous cam
		self:ActivateCam(self.previousActiveCamera);
	else
		-- remember current active cam
		self.previousActiveCamera = self.activeCamera;
		
		-- move fly cam to where active cam is
		local cameraTm = TransformBus.Event.GetWorldTM(self.previousActiveCamera);
		TransformBus.Event.SetWorldTM(self.flyCamId, cameraTm);

		-- enable fly cam
		GameplayNotificationBus.Event.OnEventBegin(self.flyCamEnableEventId, true);
		
		-- activate fly cam
		self:ActivateCam(self.flyCamId);
		
		self.flyCamActive = true;
	end

end

function cameramanager:ActivateCam(camToActivate)
	if (self.flyCamActive) then
		-- activating a camera while in fly cam means we need to change the camera to which we return once we exit fly cam
		self.previousActiveCamera = camToActivate;
	else
		local camToDeactivate = TagGlobalRequestBus.Event.RequestTaggedEntities(self.activeCamTagCrc);
		if (camToDeactivate ~= nil) then
			--Debug.Log("Found camera");
			TagComponentRequestBus.Event.RemoveTag(camToDeactivate, self.activeCamTagCrc);
		else
			Debug.Log("'CameraManager.lua' couldn't find camera [" .. tostring(camToDeactivate) .. "] to deactivate.");
		end
		
		-- We need to ensure the entity has a TagComponent otherwise the 'AddTag' call will
		-- silently fail.
		StarterGameEntityUtility.AddTagComponentToEntity(camToActivate);
		TagComponentRequestBus.Event.AddTag(camToActivate, self.activeCamTagCrc);
		
		self.activeCamera = camToActivate;
		--Debug.Log("Activating camera: " .. tostring(camToActivate));
		CameraRequestBus.Event.MakeActiveView(camToActivate);
	end	
end

function cameramanager:UpdatePlayerCameraSettings()
	local activeSettings = CameraSettingsComponentRequestsBus.Event.GetActiveSettings(self.entityId);
	if (self.currentSettingsName == "" or self.currentSettingsName ~= activeSettings.Name) then
		-- camera mode has changed, tell the player camera to go to new mode
		self.currentSettingsName = activeSettings.Name;
		activeSettings.TransitionTime = self.transitionTime;
		GameplayNotificationBus.Event.OnEventBegin(self.playerCameraSettingsEventId, activeSettings);
	end
end

function cameramanager:OnEventBegin(value)
	local settingsChanged = false;

	if (GameplayNotificationBus.GetCurrentBusId() == self.activateEventId) then
		self:ActivateCam(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.pushCameraSettingsEventId) then
		CameraSettingsComponentRequestsBus.Event.PushSettings(self.entityId, value.name, value.entityId);
		self.transitionTime = value.transitionTime;
		settingsChanged = true;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.popCameraSettingsEventId) then
		CameraSettingsComponentRequestsBus.Event.PopSettings(self.entityId, value.name, value.entityId);
		self.transitionTime = value.transitionTime;
		settingsChanged = true;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.toggleFlyCamEventId) then
		self:ToggleFlyCam();
	end

	if (settingsChanged) then
		self:UpdatePlayerCameraSettings();
	end
		
end

return cameramanager;