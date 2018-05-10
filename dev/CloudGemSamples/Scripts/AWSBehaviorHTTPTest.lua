local isActive = true

local AWSBehaviorHTTPTest = {
    
}

function AWSBehaviorHTTPTest:OnActivate()
    local runTestEventId = GameplayNotificationId(self.entityId, "Run Tests", typeid(""))
    self.gamePlayHandler = GameplayNotificationBus.Connect(self, runTestEventId)

    self.httpHandler = AWSBehaviorHTTPNotificationsBus.Connect(self, self.entityId)
end

function AWSBehaviorHTTPTest:OnEventBegin(message)
     if isActive == false then
        Debug.Log("AWSBehaviorHTTPTest not active")
        self:NotifyMainEntity("success")
        return
    end

    local http = AWSBehaviorHTTP()
    http.URL = "http://example.com/"
    http:Get()
end

function AWSBehaviorHTTPTest:OnDeactivate()
    self.httpHandler:Disconnect()
    self.gamePlayHandler:Disconnect()
end

function AWSBehaviorHTTPTest:OnSuccess(resultBody)
    Debug.Log("Lua AWSBehaviorHTTPTest: AWS HTTP success")
    Debug.Log("HTTP Result Body: " .. resultBody)
    self:NotifyMainEntity("success")
end

function AWSBehaviorHTTPTest:OnError(errorBody)
    Debug.Log("Lua AWSBehaviorHTTPTest: AWS HTTP error")
    Debug.Log("HTTP Error Result Body: " .. errorBody)
    self:NotifyMainEntity("fail")
end

function AWSBehaviorHTTPTest:GetResponse(responseCode, headerMap, contentType, responseBody)
    Debug.Log("Lua AWSBehaviorHTTPTest: Response Info")
    Debug.Log("\tResponse Code: " .. responseCode)

    Debug.Log("\tContent Type: " .. contentType)
    Debug.Log("\tResponse Body: " .. responseBody)

    headerMap:LogToDebugger()

end

function AWSBehaviorHTTPTest:NotifyMainEntity(message)
    local entities = {TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("Main"))}
    GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(entities[1], "Run Tests", typeid("")), message)
end


return AWSBehaviorHTTPTest