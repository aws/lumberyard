local isActive = true

local AWSBehaviorURLDecodeTest = {}

function AWSBehaviorURLDecodeTest:OnActivate()
     -- Connect our EBus to listen for the master entity events.
    local runTestsEventId = GameplayNotificationId(self.entityId, "Run Tests", typeid(""))  
    self.GamePlayHandler = GameplayNotificationBus.Connect(self, runTestsEventId)
    
    -- Listen for the AWS behavior notification.
    self.URLHandler = AWSBehaviorURLNotificationsBus.Connect(self, self.entityId)  
end

function AWSBehaviorURLDecodeTest:OnEventBegin(message)
    if isActive == false then
        Debug.Log("AWSBehaviorURLDecodeTest not active")
        self:NotifyMainEntity("success")
        return
    end

    -- Decode the URL
    AWSBehaviorURL = AWSBehaviorURL()
    AWSBehaviorURL.URL = "http%3A%2F%2Fdocs.aws.amazon.com%2Flumberyard%2Flatest%2Fdeveloperguide%2Fcloud-canvas-intro.html"
    AWSBehaviorURL:Decode()
end

function AWSBehaviorURLDecodeTest:OnDeactivate()
    self.GamePlayHandler:Disconnect()
    self.URLHandler:Disconnect()
end

function AWSBehaviorURLDecodeTest:OnSuccess(resultBody)
    Debug.Log(resultBody)
    self:NotifyMainEntity("success")
end

function AWSBehaviorURLDecodeTest:OnError(errorBody)
    Debug.Log(errorBody)
    self:NotifyMainEntity("fail") 
end

function AWSBehaviorURLDecodeTest:NotifyMainEntity(message)
    local entities = {TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("Main"))}
    GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(entities[1], "Run Tests", typeid("")), message)
end

return AWSBehaviorURLDecodeTest