local MetricsMainMenu = {
	Properties = {
		Player = { default = EntityId() }
	}
}

function MetricsMainMenu:OnActivate()
	self.tickBusHandler = TickBus.Connect(self,self.entityId)
	self.tickTime = 0
	LyShineLua.ShowMouseCursor(true)
	self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("UI/Canvases/Metrics/main.uicanvas")
	self.uiEventHandler = UiCanvasNotificationBus.Connect(self, self.canvasEntityId)
	self.encrypt = 0
	self.compress = 0
	displayText = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "ConsoleOutput")
	UiTextBus.Event.SetText(displayText, "Level Started \n Try generating some metrics data")
end


function MetricsMainMenu:OnDeactivate()
	self.uiEventHandler:Disconnect()
	self.tickBusHandler:Disconnect()
end

function MetricsMainMenu:OnTick(deltaTime,timePoint)
    self.tickTime = self.tickTime + deltaTime
    if self.tickTime > 1.0 then
        local transform = TransformBus.Event.GetWorldTM(self.Properties.Player)
        playerTranslation = transform:GetTranslation()
        playerScale = transform:ExtractScale()
		playerRotation = MathUtils.ConvertTransformToEulerDegrees(transform)

        Debug.Log("Player translation: " .. tostring(playerTranslation))
        -- Debug.Log("World scale: " .. tostring(worldScale))
		Debug.Log("Player rotation: " .. tostring(playerRotation))

        self.tickTime = 0
    end
end

function MetricsMainMenu:SendPosition(disableBuffer)

	translationXAttr = CloudGemMetric_MetricsAttribute()
    translationXAttr:SetName("translationx")
	translationXAttr:SetDoubleValue(playerTranslation.x)

	translationYAttr = CloudGemMetric_MetricsAttribute()
    translationYAttr:SetName("translationy")
	translationYAttr:SetDoubleValue(playerTranslation.y)

	translationZAttr = CloudGemMetric_MetricsAttribute()
    translationZAttr:SetName("translationz")
	translationZAttr:SetDoubleValue(playerTranslation.z)

	attribute_list = CloudGemMetric_AttributesSubmissionList()
	attribute_list.attributes:push_back(translationXAttr)
	attribute_list.attributes:push_back(translationYAttr)
	attribute_list.attributes:push_back(translationZAttr)

	parameter_sensitive = CloudGemMetric_MetricsEventParameter()
	parameter_sensitive:SetName(parameter_sensitive:GetSensitivityTypeName())
	parameter_sensitive:SetVal(self.encrypt)
	
	parameter_compression = CloudGemMetric_MetricsEventParameter()
	parameter_compression:SetName(parameter_compression:GetCompressionModeName())
	parameter_compression:SetVal(self.compress)
	
	params = CloudGemMetric_EventParameterList() 
	params.parameters:push_back(parameter_sensitive)
	params.parameters:push_back(parameter_compression)

	if disableBuffer then
		-- Send metrics with no file buffer
		Debug.Log("Sending Metrics with no buffer")
		UiTextBus.Event.SetText(displayText, "Sending metrics with no file buffer")
		-- self.UpdateMessage("Sending Metrics with no buffer")
		CloudGemMetricRequestBus.Broadcast.SendMetrics("translation", attribute_list.attributes, params.parameters)
	else
		-- Send metrics with a file buffer
		Debug.Log("Sending Metrics to buffer")
		UiTextBus.Event.SetText(displayText, "Sending metrics to buffer")
		CloudGemMetricRequestBus.Broadcast.SubmitMetrics("translation", attribute_list.attributes, params.parameters)
	end
end

function MetricsMainMenu:FlushMetrics()
	Debug.Log("Flushing Metrics")
	UiTextBus.Event.SetText(displayText, "Flushing Metrics")
	CloudGemMetricRequestBus.Broadcast.SendBufferedMetrics()
end

function MetricsMainMenu:EnableEncryption()
	Debug.Log("Enabling Encryption")
	UiTextBus.Event.SetText(displayText, "Enabling Encryption")	
	self.encrypt = 1    
end

function MetricsMainMenu:DisableEncryption()
	Debug.Log("Disabling Encryption")
	UiTextBus.Event.SetText(displayText, "Disabling Encryption")	
	self.encrypt = 0
end

function MetricsMainMenu:EnableCompression()
	Debug.Log("Enabling Compression")
	UiTextBus.Event.SetText(displayText, "Enabling Compression")	
	self.compress = 1
end

function MetricsMainMenu:DisableCompression()
	Debug.Log("Disabling Compression")
	UiTextBus.Event.SetText(displayText, "Disabling Compression")	
	self.compress = 0
end

function MetricsMainMenu:OnAction(entityId, actionName)
	if actionName == "SendPosition" then
		self:SendPosition(true)
	elseif actionName == "SendPositionBuffered" then
		self:SendPosition(false)
	elseif actionName == "FlushMetrics" then
		self:FlushMetrics()
	elseif actionName == "EnableEncryption" then
		self:EnableEncryption()
	elseif actionName == "DisableEncryption" then
		self:DisableEncryption()
	elseif actionName == "EnableCompression" then
		self:EnableCompression()
	elseif actionName == "DisableCompression" then
		self:DisableCompression()
	end
end

return MetricsMainMenu