local jackaimdirectionprovider = 
{
    Properties = 
    {
		Camera = {default = EntityId()},
    },
}

--------------------------------------------------
-- Component behavior
--------------------------------------------------

function jackaimdirectionprovider:OnActivate()    
	self.requestAimUpdateEventId = GameplayNotificationId(self.entityId, "RequestAimUpdate", "float");
	self.requestAimUpdateHandler = GameplayNotificationBus.Connect(self, self.requestAimUpdateEventId);

	self.SetAimDirectionId = GameplayNotificationId(self.entityId, "SetAimDirection", "float");
	self.SetAimOriginId = GameplayNotificationId(self.entityId, "SetAimOrigin", "float");
end

function jackaimdirectionprovider:OnEventBegin(value)
	if (GameplayNotificationBus.GetCurrentBusId() == self.requestAimUpdateEventId) then
		local camTm = TransformBus.Event.GetWorldTM(self.Properties.Camera);
		local pos = camTm:GetTranslation();
		local dir = camTm:GetColumn(1);
		GameplayNotificationBus.Event.OnEventBegin(self.SetAimDirectionId, dir);
		GameplayNotificationBus.Event.OnEventBegin(self.SetAimOriginId, pos);	
	end
end
	

return jackaimdirectionprovider;