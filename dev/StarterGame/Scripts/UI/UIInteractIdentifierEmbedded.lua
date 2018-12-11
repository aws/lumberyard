local uiinteractidentifierembedded =
{
	Properties =
	{
		ButtonContentStandalone = { default = EntityId(), description = "The content to show inside the button on generic machines (IE. Windows, Mac, etc)" },
		ButtonContentProvo = { default = EntityId(), description = "The content to show inside the button on Provo" },
		ButtonContentXenia = { default = EntityId(), description = "The content to show inside the button on Xenia" },
	}
}

function uiinteractidentifierembedded:OnActivate()
	if (self.initialized) then
		return;
	end
	
	self.tickHandler = TickBus.Connect(self);
	
	if (Platform.Current == Platform.Provo) then
		self.contentEntityId = self.Properties.ButtonContentProvo;
		self.getColor = UiImageBus.Event.GetColor;
		self.setColor = UiImageBus.Event.SetColor;
	elseif (Platform.Current == Platform.Xenia) then
		self.contentEntityId = self.Properties.ButtonContentXenia;
		self.getColor = UiImageBus.Event.GetColor;
		self.setColor = UiImageBus.Event.SetColor;
	else
		self.contentEntityId = self.Properties.ButtonContentStandalone;
		self.getColor = UiTextBus.Event.GetColor;
		self.setColor = UiTextBus.Event.SetColor;
	end	
end

function uiinteractidentifierembedded:OnDeactivate()
	self:Disconnect();
end

function uiinteractidentifierembedded:Disconnect()
	if (self.tickHandler == nil) then
		return;
	end
	self.tickHandler:Disconnect();
	self.tickHandler = nil;
end

function uiinteractidentifierembedded:OnTick(deltaTime, timePoint)
	
	local color = self.getColor(self.contentEntityId);
	if (color == nil) then
		return;
	end	
	color.a = 1;
	self.setColor(self.contentEntityId, color);
	
	self.initialized = true;	
	self:Disconnect();
end

return uiinteractidentifierembedded;
