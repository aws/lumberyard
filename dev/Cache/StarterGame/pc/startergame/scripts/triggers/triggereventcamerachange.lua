local triggereventcamerachange =
{
	Properties =
	{
		StaticCamera = { default = EntityId() },
		ExitCamera = {  default = EntityId(), description = "Returns to the original camera if not set." },
		
		ShowCrosshair = { default = false, description = "If true, the crosshair will remain visible on the static camera view." },
	},
}

function triggereventcamerachange:OnActivate()

	-- Listen for anything entering our area.
	self.triggerAreaHandler = TriggerAreaNotificationBus.Connect(self, self.entityId);

end

function triggereventcamerachange:OnDeactivate()

	-- Release the handler.
	self.triggerAreaHandler:Disconnect();
	self.triggerAreaHandler = nil;

end

function triggereventcamerachange:ChangeCameraTo(newCam, enableCrosshair)

	local camManager = TagGlobalRequestBus.Event.RequestTaggetEntities(Crc32("CameraManager"));
	local eventId = GameplayNotificationId(camManager, "ActivateCameraEvent", "float");
	GameplayNotificationBus.Event.OnEventBegin(eventId, newCam);
	
	-- We only want to change the crosshair's state if the property is false.
	if (self.Properties.ShowCrosshair == false) then
		local ch = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("Crosshair"));
		local chEventId = GameplayNotificationId(ch, "EnableCrosshairEvent", "float");
		GameplayNotificationBus.Event.OnEventBegin(chEventId, enableCrosshair);
	end

end

function triggereventcamerachange:OnTriggerAreaEntered(enteringEntityId)
	--Debug.Log("OnTriggerAreaEntered " .. tostring(self.entityId));
	
	-- Store the camera we want to return to.
	self.exitCam = self.Properties.ExitCamera;
	if (not self.exitCam:IsValid()) then
		-- If it's not a fixed camera then find the currently active one.
		self.exitCam = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("ActiveCamera"));
		
		-- If there isn't an active one then something bad has happened.
		if (not self.exitCam:IsValid()) then
			-- TODO: Change to an assert.
			Debug.Log("Couldn't find an active camera!");
		end
	end
	
	-- Change the camera to the static one.
	self:ChangeCameraTo(self.Properties.StaticCamera, 0.0);
	
end

function triggereventcamerachange:OnTriggerAreaExited(enteringEntityId)
	--Debug.Log("OnTriggerAreaExited " .. tostring(self.entityId));

	-- Change to the exit camera (or the camera that was being used before this trigger was entered).
	self:ChangeCameraTo(self.exitCam, 1.0);

end

return triggereventcamerachange;