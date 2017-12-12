local isActive = true

local AWSBehaviorLambdaTest = {
}

function AWSBehaviorLambdaTest:OnActivate()
    local runTestEventId = GameplayNotificationId(self.entityId, "Run Tests", typeid(""))
    self.GamePlayHandler = GameplayNotificationBus.Connect(self, runTestEventId)

    self.lambdaHandler = AWSLambdaHandler.Connect(self, self.entityId)

end

function AWSBehaviorLambdaTest:OnEventBegin(message)
    if isActive == false then
        Debug.Log("AWSBehaviorLambdaTest not active")
        self:NotifyMainEntity("success")
        return
    end

    local lambda = AWSLambda()
    lambda.functionName = "CloudGemAWSScriptBehaviors.AWSBehaviorLambdaExample"
    lambda:InvokeAWSLambda()
end

function AWSBehaviorLambdaTest:OnDeactivate()
    self.lambdaHandler:Disconnect()
    self.GamePlayHandler:Disconnect()
end

function AWSBehaviorLambdaTest:OnSuccess(resultBody)
    Debug.Log("Lua AWSBehaviorLambdaTest: AWS Lambda success: " .. resultBody)
    self:NotifyMainEntity("success")
end

function AWSBehaviorLambdaTest:OnError(errorBody)
    Debug.Log("Lua AWSBehaviorLambdaTest: AWS Lambda error: " .. errorBody)
    self:NotifyMainEntity("fail")
end

function AWSBehaviorLambdaTest:NotifyMainEntity(message)
    local entities = {TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("Main"))}
    GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(entities[1], "Run Tests", typeid("")), message)
end

return AWSBehaviorLambdaTest
