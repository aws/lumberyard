
local CommunicatorSample = 
{
    Properties = 
    {
    },
}

function CommunicatorSample:OnActivate()
    self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("Levels/CommunicatorSample/UI/CommunicatorSample.uicanvas")

    -- Listen for action strings broadcast by the canvas
    self.buttonHandler = UiCanvasNotificationBus.Connect(self, self.canvasEntityId)

    -- Display the mouse cursor
    LyShineLua.ShowMouseCursor(true)
   
      -- Listen for updates
    self.communicatorUpdateBus = CloudGemWebCommunicatorUpdateBus.Connect(self, WebCommunicator.ModuleEntity)
    self:UpdateStatus()
end

local defaultChannel = "Broadcasts"

function CommunicatorSample:UpdateStatus()

    local displayEntity = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "RegistrationStatus")
    local returnValue = CloudGemWebCommunicatorRequestBus.Broadcast.GetRegistrationStatus()
    Debug.Log("Got RegistrationStatus value: " .. returnValue)   
    UiTextBus.Event.SetText(displayEntity, returnValue)

    displayEntity = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "ConnectionStatus")
    returnValue = CloudGemWebCommunicatorRequestBus.Broadcast.GetConnectionStatus()
    Debug.Log("Got ConnectionStatus value: " .. returnValue)   
    UiTextBus.Event.SetText(displayEntity, returnValue)

    returnList = CloudGemWebCommunicatorRequestBus.Broadcast.GetSubscriptionList()

    Debug.Log("Got " .. tostring(#returnList)   .. " channels:")

    for channelIndex = 1, 5 do
        channelBoxName = "Channel" .. tostring(channelIndex)
        displayEntity = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, channelBoxName)
        if #returnList >= channelIndex then
            UiTextBus.Event.SetText(displayEntity, returnList[channelIndex])
        else
            UiTextBus.Event.SetText(displayEntity, " ")
        end
    end

    displayEntity = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "Endpoint")
    returnValue = CloudGemWebCommunicatorRequestBus.Broadcast.GetEndpointPortString()
    Debug.Log("Got Endpoint String: " .. returnValue)   
    UiTextBus.Event.SetText(displayEntity, returnValue)

end

function CommunicatorSample:RegistrationStatusChanged(registrationStatus)
    Debug.Log("RegistraitonStatusChanged update: " .. registrationStatus)
    self:UpdateStatus()
end

function CommunicatorSample:ConnectionStatusChanged(connectionStatus)
    Debug.Log("ConnectionStatusChanged update: " .. connectionStatus)
    self:UpdateStatus()
end

function CommunicatorSample:SubscriptionStatusChanged(channelName, subscriptionStatus)
    Debug.Log("SubscriptionStatusChanged: " .. channelName .. " Status: " .. subscriptionStatus)
    self:UpdateStatus()
end

function CommunicatorSample:MessageReceived(channelName, messageData)
    Debug.Log("Received Message on channel: " .. channelName .. " Message: " .. messageData)
    local messageText = channelName..": "..messageData
    self:ShowMessage(messageText)
end

function CommunicatorSample:ShowMessage(messageText)
    displayEntity = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "BroadcastText")
    UiTextBus.Event.SetText(displayEntity, messageText)
end

function CommunicatorSample:OnAction(entityId, actionName)

    Debug.Log("Received Action: " .. actionName)

    if actionName == "RegisterOpenSSL" then
        CloudGemWebCommunicatorRequestBus.Broadcast.RequestRegistration("OPENSSL")
    elseif actionName == "RegisterWebsocket" then
        Debug.Log("Requesting websocket registration")
        CloudGemWebCommunicatorRequestBus.Broadcast.RequestRegistration("WEBSOCKET")
    elseif actionName == "RefreshDeviceInfo" then
        CloudGemWebCommunicatorRequestBus.Broadcast.RequestRefreshDeviceInfo()
    elseif actionName == "ConnectOpenSSL" then
        CloudGemWebCommunicatorRequestBus.Broadcast.RequestConnection("OPENSSL")
    elseif actionName == "ConnectWebsocket" then
        CloudGemWebCommunicatorRequestBus.Broadcast.RequestConnection("WEBSOCKET")
    elseif actionName == "RequestList" then
        CloudGemWebCommunicatorRequestBus.Broadcast.RequestChannelList()
    elseif actionName == "Subscribe" then
        displayEntity = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "ChannelInputText")
        local broadcastChannel = UiTextBus.Event.GetText(displayEntity)
        if broadcastChannel == nil or broadcastChannel == '' then
            self:ShowMessage("Enter a channel to subscribe to")
            return
        end
        CloudGemWebCommunicatorRequestBus.Broadcast.RequestSubscribeChannel(broadcastChannel)
    elseif actionName == "Unsubscribe" then
        displayEntity = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "ChannelInputText")
        local broadcastChannel = UiTextBus.Event.GetText(displayEntity)
        if broadcastChannel == nil or broadcastChannel == '' then
            self:ShowMessage("Enter a channel to unsubscribe from")
            return
        end
        CloudGemWebCommunicatorRequestBus.Broadcast.RequestUnsubscribeChannel(broadcastChannel)
    elseif actionName == "Disconnect" then
        CloudGemWebCommunicatorRequestBus.Broadcast.RequestDisconnect()
    else
        Debug.Log("Unknown action")
    end
end


function CommunicatorSample:OnDeactivate()
    self.buttonHandler:Disconnect()
    self.communicatorUpdateBus:Disconnect()
end

return CommunicatorSample