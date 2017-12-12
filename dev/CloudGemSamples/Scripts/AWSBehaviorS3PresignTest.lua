local isActive = true

local AWSBehaviorS3PresignTest = {}

function AWSBehaviorS3PresignTest:OnActivate()
    -- Listen for the master entity events.
    local runTestsEventId = GameplayNotificationId(self.entityId, "Run Tests", typeid(""))
    self.GamePlayHandler = GameplayNotificationBus.Connect(self, runTestsEventId)

    -- Listen for the AWS behavior notification.
    self.S3PresignHandler = AWSBehaviorS3PresignNotificationsBus.Connect(self, self.entityId)
end

function AWSBehaviorS3PresignTest:OnEventBegin(message)
    if isActive == false then
        Debug.Log("AWSBehaviorS3PresignTest not active")
        self:NotifyMainEntity("success")
        return
    end

    -- Presign the file
    AWSBehaviorS3Presign = AWSBehaviorS3Presign()
    AWSBehaviorS3Presign.bucketName = "CloudGemAWSBehavior.s3nodeexamples" --Specify a bucket name
    AWSBehaviorS3Presign.keyName = "s3example.txt" --Specify a key name
    AWSBehaviorS3Presign.requestMethod = "GET" --Specify a request method
    AWSBehaviorS3Presign:Presign()
end

function AWSBehaviorS3PresignTest:OnDeactivate()
    self.GamePlayHandler:Disconnect()
    self.S3PresignHandler:Disconnect()
end

function AWSBehaviorS3PresignTest:OnSuccess(resultBody)
    Debug.Log(resultBody)
    self:NotifyMainEntity("success")
    
end

function AWSBehaviorS3PresignTest:OnError(errorBody)
    Debug.Log(errorBody)
    self:NotifyMainEntity("fail")
end

function AWSBehaviorS3PresignTest:NotifyMainEntity(message)
    local entities = {TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("Main"))}
    GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(entities[1], "Run Tests", typeid("")), message)
end

return AWSBehaviorS3PresignTest