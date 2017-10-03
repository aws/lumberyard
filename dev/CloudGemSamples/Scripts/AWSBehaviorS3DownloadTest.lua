local isActive = true

local AWSBehaviorS3DownloadTest = {}

function AWSBehaviorS3DownloadTest:OnActivate()
    -- Listen for the master entity events.
    local runTestsEventId = GameplayNotificationId(self.entityId, "Run Tests", typeid(""))  
    self.GamePlayHandler = GameplayNotificationBus.Connect(self, runTestsEventId)
    
    -- Listen for the AWS behavior notification.
    self.S3DownloadHandler =AWSBehaviorS3DownloadNotificationsBus.Connect(self, self.entityId) 
end

function AWSBehaviorS3DownloadTest:OnEventBegin(message)
    if isActive == false then
        Debug.Log("AWSBehaviorS3DownloadTest not active")
        self:NotifyMainEntity("success")
        return
    end

    -- Download the file
    AWSBehaviorS3Download = AWSBehaviorS3Download()
    AWSBehaviorS3Download.bucketName = "CloudGemAWSScriptBehaviors.s3nodeexamples" --Specify a bucket name
    AWSBehaviorS3Download.keyName = "s3example.txt" --Specify a key name
    AWSBehaviorS3Download.localFileName = "CloudGemSamples/Levels/AWSBehaviorExamples/testdata/s3downloadexample.txt" --Specify a local file name
    AWSBehaviorS3Download:Download()
end

function AWSBehaviorS3DownloadTest:OnDeactivate()
    self.GamePlayHandler:Disconnect()
    self.S3DownloadHandler:Disconnect()
end

function AWSBehaviorS3DownloadTest:OnSuccess(resultBody)
    Debug.Log(resultBody)
    self:NotifyMainEntity("success")
end

function AWSBehaviorS3DownloadTest:OnError(errorBody)
    Debug.Log(errorBody)
    self:NotifyMainEntity("fail")
end

function AWSBehaviorS3DownloadTest:NotifyMainEntity(message)
    local entities = {TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32("Main"))}
    GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(entities[1], "Run Tests", typeid("")), message)
end

return AWSBehaviorS3DownloadTest