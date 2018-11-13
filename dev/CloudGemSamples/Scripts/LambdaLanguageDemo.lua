local lambdalanguagedemo = {
}

function lambdalanguagedemo:OnActivate()
    self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("Levels/LambdaLanguage/UI/LambdaLanguageChecklist.uicanvas")
    self.textOutput = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "Output")
    self.lambdaHandler = AWSLambdaHandler.Connect(self, self.entityId)
    self.index = 0
    self.lambdas = {"DotnetLambda", "GoLambda", "JavaJarLambda", "NodeLambda"}
    self:CallNextlambda()
end

function lambdalanguagedemo:OnDeactivate()
    self.lambdaHandler:Disconnect()
end

function lambdalanguagedemo:CallNextlambda()
    self:NextLambda()
    self:CallLambdaByName(self.testingLambda)
end

function lambdalanguagedemo:CallLambdaByName(name)
    if self.testingLambda == "" then
        return
    end
    local lambda = AWSLambda()
    lambda.functionName = "AWSLambdaLanguageDemo."..name
    lambda.requestBody = ""
    Debug.Log("calling function " .. name)
    lambda:InvokeAWSLambda()
end

function lambdalanguagedemo:OnSuccess(resultBody)
    self:RecordSuccess(resultBody)
    self:CallNextlambda()
end

function lambdalanguagedemo:OnError(errorBody)
    self:RecordError(errorBody)
    self:CallNextlambda()
end

function lambdalanguagedemo:RecordSuccess(body)
    Debug.Log("AWS Lambda success")
    self:UIAppendText(self.testingLambda.. " success")
    if body ~= nil then
        Debug.Log("Response body = "..body)
    end
end

function lambdalanguagedemo:RecordError(body)
    Debug.Log("AWS Lambda error")
    self:UIAppendText(self.testingLambda.. " error")
    if body ~= nil then
        Debug.Log("Response body = "..body)
    end
end

function lambdalanguagedemo:UIAppendText(text)
    local textToSend = UiTextBus.Event.GetText(self.textOutput)
    textToSend = textToSend.."\n"..text
    UiTextBus.Event.SetText(self.textOutput, textToSend)
end

function lambdalanguagedemo:NextLambda()
    self.index = self.index + 1
    if self.index <= #self.lambdas then
        self.testingLambda = self.lambdas[self.index]
    else
        self.testingLambda = ""
    end
end


return lambdalanguagedemo