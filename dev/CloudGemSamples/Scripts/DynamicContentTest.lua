
local DynamicContentTest =
{
    Properties =
    {
        UserRequestedPak = {default = "UserRequestedData.shared.pak"}
    },
}

function DynamicContentTest:UpdateGameProperties()
    if StaticDataRequestBus.Event == nil then
        Debug.Log("No StaticData Request Events found")
        return
    end
    StaticDataRequestBus.Event.LoadRelativeFile(StaticData.ModuleEntity,"StaticData/CSV/gameproperties.csv")
    StaticDataRequestBus.Event.LoadRelativeFile(StaticData.ModuleEntity,"StaticData/CSV/userrequest.csv")
end

function DynamicContentTest:OnActivate()

    self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("Levels/CloudGemTests/DynamicContentTest/UI/DynamicContentTest.uicanvas")

    self.requestList = {}

    -- Listen for action strings broadcast by the canvas
	self.buttonHandler = UiCanvasNotificationBus.Connect(self, self.canvasEntityId)

    local util = require("scripts.util")
    util.SetMouseCursorVisible(true)

      -- Listen for updates
    self.dynamicContentUpdateBus = DynamicContentUpdateBus.Connect(self)
    self.staticDataUpdateBus = StaticDataUpdateBus.Connect(self)
	if CloudGemWebCommunicatorUpdateBus ~= nil then
	    Debug.Log("Listening for communicator updates")
		self.communicatorUpdateBus = CloudGemWebCommunicatorUpdateBus.Connect(self, WebCommunicator.ModuleEntity)
	else
		Debug.Log("Web Communicator not found")
	end
	self.userStatus = nil
    self:UpdateGameProperties()
    self:UpdateUserDownloadable()
end

function DynamicContentTest:TypeReloaded(outputFile)
    Debug.Log("Static Data type reloaded:" .. outputFile)
    if outputFile == "gameproperties" or outputFile == "userrequest" then
        self:UpdateText()
    end
end

function DynamicContentTest:UpdateText()
    local displayEntity = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "PropertyText")
    local wasSuccess = false
    local returnValue = StaticDataRequestBus.Event.GetStrValue(StaticData.ModuleEntity,"gameproperties","DynamicMessage", "Value", wasSuccess)
    Debug.Log("Got DynamicMessage value: " .. returnValue)
    UiTextBus.Event.SetText(displayEntity, returnValue)

    local requestDisplayEntity = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "UserPropertyText")
    returnValue = StaticDataRequestBus.Event.GetStrValue(StaticData.ModuleEntity,"userrequest","DynamicMessage", "Value", wasSuccess)
    Debug.Log("Got User Request DynamicMessage value: " .. returnValue)
    UiTextBus.Event.SetText(requestDisplayEntity, returnValue)
end

function DynamicContentTest:RequestsCompleted()
    Debug.Log("Requests completed - refreshing")
    self:RefreshStatus()
end

function DynamicContentTest:RefreshStatus()
    self:UpdateGameProperties()
    self:UpdateUserDownloadable()
end

function DynamicContentTest:OnAction(entityId, actionName)

    if actionName == "RequestManifest" then
        Debug.Log("Requesting manifest..")
        if DynamicContentRequestBus.Event == nil then
            Debug.Log("No Content Request Events found")
            return
        end
        DynamicContentRequestBus.Event.RequestManifest(DynamicContent.ModuleEntity,"DynamicContentTest.json")
        self:UpdateText()
    elseif actionName == "RequestPak" then
        DynamicContentRequestBus.Event.RequestFileStatus(DynamicContent.ModuleEntity,self.Properties.UserRequestedPak, "")
    elseif actionName == "ClearContent" then
        DynamicContentRequestBus.Event.ClearAllContent(DynamicContent.ModuleEntity)
        self:RefreshStatus()
    elseif actionName == "DeletePak" then
        DynamicContentRequestBus.Event.DeletePak(DynamicContent.ModuleEntity,self.Properties.UserRequestedPak)
        self:RefreshStatus()
    end
end


function DynamicContentTest:OnDeactivate()
    DynamicContentRequestBus.Event.ClearAllContent(DynamicContent.ModuleEntity)
    self.buttonHandler:Disconnect()
    self.dynamicContentUpdateBus:Disconnect()
    self.staticDataUpdateBus:Disconnect()
end

function DynamicContentTest:UpdateUserDownloadable()
    self.requestList = DynamicContentRequestBus.Event.GetDownloadablePaks(DynamicContent.ModuleEntity)
    local numRequests = #self.requestList
    Debug.Log("Paks available for Download: " .. numRequests)
    local displayEntity = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "RequestPakText")
    local pakStatus = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "PakStatusText")
    fileStatus = DynamicContentRequestBus.Event.GetPakStatusString(DynamicContent.ModuleEntity,self.Properties.UserRequestedPak)
    Debug.Log("Status for ".. self.Properties.UserRequestedPak .. " is ".. fileStatus)

    if fileStatus == "WAITING_FOR_USER" then
      	Debug.Log("Download: " .. self.Properties.UserRequestedPak)
        UiTextBus.Event.SetText(displayEntity, self.Properties.UserRequestedPak)
    elseif fileStatus == "INITIALIZED" or fileStatus == "READY" or fileStatus == "MOUNTED" then
        UiTextBus.Event.SetText(displayEntity, "File Downloaded")
    else
        UiTextBus.Event.SetText(displayEntity, "No Paks Available")
    end

	if self.userStatus ~= nil then
		UiTextBus.Event.SetText(pakStatus, self.userStatus)
	end
end

function DynamicContentTest:IsDynamicContentUpdate(channelName)
	-- Test for the CloudGemDynamicContent broadcast channel
	if channelName == "CloudGemDynamicContent" or channelName == "CloudGemDynamicContentPrivate" then
		return true
	end
	return false
end

function DynamicContentTest:MessageReceived(channelName, messageData)
	if self:IsDynamicContentUpdate(channelName) then
    	Debug.Log("DynamicContent update received: " .. messageData)
		DynamicContentRequestBus.Event.HandleWebCommunicatorUpdate(DynamicContent.ModuleEntity, messageData)
    end
end

function DynamicContentTest:FileStatusChanged(fileName, fileStatus)
    Debug.Log("Received file status update for " .. fileName .. " to status " .. fileStatus)
	if fileName == self.Properties.UserRequestedPak then
		self.userStatus = fileStatus
		self:UpdateUserDownloadable()
	end
end

return DynamicContentTest
