local jackstrafefacingprovider = 
{
    Properties = 
    {
		Camera = {default = EntityId()},
    },
}

--------------------------------------------------
-- Component behavior
--------------------------------------------------

function jackstrafefacingprovider:OnActivate()    

	-- Tick needed to detect aim timeout
	self.tickBusHandler = TickBus.Connect(self);

end

function jackstrafefacingprovider:OnDeactivate()
	self.tickBusHandler:Disconnect();
	self.tickBusHandler = nil;
end
function jackstrafefacingprovider:OnTick(deltaTime, timePoint)
	local cameraTm = TransformBus.Event.GetWorldTM(self.Properties.Camera);
	local flatForward = cameraTm:GetColumn(0):Cross(Vector3(0,0,-1));
	self.setStrafeFacingId = GameplayNotificationId(self.entityId, "SetStrafeFacing", "float");
	GameplayNotificationBus.Event.OnEventBegin(self.setStrafeFacingId, flatForward);
end
	

return jackstrafefacingprovider;