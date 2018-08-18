local isActive = true

local AWSBehaviorS3UploadTest = {}

function AWSBehaviorS3UploadTest:OnActivate()
    -- Listen for the master entity events.
    local runTestsEventId = GameplayNotificationId(self.entityId, "Run Tests", typeid(""))  
    self.GamePlayHandler = GameplayNotificationBus.Connect(self, runTestsEventId)
    
    -- Listen for the AWS behavior notification.
    self.S3UploadHandler = AWSBehaviorS3UploadNotificationsBus.Connect(self, self.entityId)   
end

function AWSBehaviorS3UploadTest:OnEventBegin(message)
    if isActive == false then
        Debug.Log("AWSBehaviorS3UploadTest not active")
        self:NotifyMainEntity("success")
        return
    end

    -- Upload the file
    AWSBehaviorS3Upload = AWSBehaviorS3Upload()
    AWSBehaviorS3Upload.bucketName = "CloudGemAWSScriptBehaviors.s3nodeexamples" --Specify a bucket name
    AWSBehaviorS3Upload.keyName = "s3example.txt" --Specify a key name
    AWSBehaviorS3Upload.localFileName = "Levels/AWSBehaviorExamples/testdata/s3example.txt" --Specify a local file name
    AWSBehaviorS3Upload.contentType = "text/html" --Specify a content type
    AWSBehaviorS3Upload:Upload()
end

function AWSBehaviorS3UploadTest:OnDeactivate()
    self.GamePlayHandler:Disconnect()
    self.S3UploadHandler:Disconnect()
end

function AWSBehaviorS3UploadTest:OnSuccess(resultBody)
    Debug.Log(resultBody)
    self:NotifyMainEntity("success")
end

function AWSBehaviorS3UploadTest:OnError(errorBody)
    Debug.Log(errorBody)
    self:NotifyMainEntity("fail")
        
end

function AWSBehaviorS3UploadTest:NotifyMainEntity(message)
    local entities = {TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("Main"))}
    GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(entities[1], "Run Tests", typeid("")), message)
end

return AWSBehaviorS3UploadTest