local characterstatedeathhandler = 
{
    Properties = 
    {
		LensFlare = {default = EntityId()},
    }
}

function characterstatedeathhandler:OnActivate() 
    self.onDeathEventId = GameplayNotificationId(self.entityId, "OnDeath", "float");
    self.onDeathHandler = GameplayNotificationBus.Connect(self, self.onDeathEventId);
end   

function characterstatedeathhandler:OnDeactivate()
    self.onDeathHandler:Disconnect();
    self.onDeathHandler = nil;
end

function characterstatedeathhandler:OnEventBegin(value)
    if (GameplayNotificationBus.GetCurrentBusId() == self.onDeathEventId) then
		LensFlareComponentRequestBus.Event.TurnOffLensFlare(self.Properties.LensFlare);
    end
end

return characterstatedeathhandler;