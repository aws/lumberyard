local isActive = true

local AWSBehaviorAPITest = {
}

function AWSBehaviorAPITest:OnActivate()
    local runTestEventId = GameplayNotificationId(self.entityId, "Run Tests", typeid(""))
    self.gamePlayHandler = GameplayNotificationBus.Connect(self, runTestEventId)

    self.apiHandler = AWSBehaviorAPINotificationsBus.Connect(self, self.entityId)
end

function AWSBehaviorAPITest:URLEncodeTimestamp(str)
   if (str) then
      str = string.gsub (str, ":", "%%3A")
      str = string.gsub (str, " ", "%%20")
   end
   return str    
end

function AWSBehaviorAPITest:OnEventBegin(message)
     if isActive == false then
        Debug.Log("AWSBehaviorAPITest not active")
        self:NotifyMainEntity("success")
        return
    end

    local apiCall = AWSBehaviorAPI()
    local timeVal = self:URLEncodeTimestamp(os.date("%b %d %Y %H:%M"))
    local lang = "Eng"
    apiCall.ResourceName = "CloudGemMessageOfTheDay.ServiceApi"
    apiCall.Query = "/player/messages?time=" .. timeVal .. "&lang=" .. lang
    apiCall.HTTPMethod = "GET"
    apiCall:Execute()
end

function AWSBehaviorAPITest:OnSuccess(resultBody)
    Debug.Log("Lua AWSBehaviorAPITest: Passed")
    Debug.Log("Result body: " .. resultBody)
    self:NotifyMainEntity("success")
end

function AWSBehaviorAPITest:OnError(errorBody)
    Debug.Log("Lua AWSBehaviorAPITest: Failed")
    Debug.Log("Error Body: " .. errorBody)
    self:NotifyMainEntity("fail")
end

function AWSBehaviorAPITest:GetResponse(responseCode, jsonResponseData)
    Debug.Log("Lua AWSBehaviorAPITest response code: " .. tostring(responseCode))
    Debug.Log("Lua AWSBehaviorAPITest jsonResponseData: " .. jsonResponseData)
    local jsonObject = JSON()
    jsonObject:FromString(jsonResponseData)
end

function AWSBehaviorAPITest:OnDeactivate()
    self.apiHandler:Disconnect()
    self.gamePlayHandler:Disconnect()
end

function AWSBehaviorAPITest:NotifyMainEntity(message)
    local entities = {TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("Main"))}
    GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(entities[1], "Run Tests", typeid("")), message)
end

return AWSBehaviorAPITest