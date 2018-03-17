require "Scripts/Utils/Math"

local CloudGemSpeechRecognitionSample = {
}

-- Bot name: LYTestBot
-- Bot alias: VOne

local PingEntities = {
}

local currentPings = {
    
}

local elicitInfoDlg = nil
local elicitInfoTxt = nil

function CloudGemSpeechRecognitionSample:OnActivate()
    self.tickBusHandler = TickBus.Connect(self)
    self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("Levels/CloudGemSpeechRecogintionSample/UI/CloudGemSpeechRecognitionSample.uicanvas")
    self.uiEventHandler = UiCanvasNotificationBus.Connect(self, self.canvasEntityId)
    self.lexNotificationHandler = CloudGemSpeechRecognitionNotificationBus.Connect(self, self.entityId)

    local map = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "imgMap")
    PingEntities["helpMe"] = UiElementBus.Event.FindChildByName(map, "imgMeHelp")
    PingEntities["pingMe"] = UiElementBus.Event.FindChildByName(map, "imgMePing")
    PingEntities["pingTop"] = UiElementBus.Event.FindChildByName(map, "imgTopPing")
    PingEntities["pingMiddle"] = UiElementBus.Event.FindChildByName(map, "imgMiddlePing")
    PingEntities["pingBottom"] = UiElementBus.Event.FindChildByName(map, "imgBottomPing")

    for key, entity in pairs(PingEntities) do
        UiElementBus.Event.SetIsEnabled(entity, false)
    end

    elicitInfoDlg = UiElementBus.Event.FindChildByName(map, "bgElicit")
    elicitInfoTxt = UiElementBus.Event.FindChildByName(elicitInfoDlg, "Text")
    UiElementBus.Event.SetIsEnabled(elicitInfoDlg, false)

    LyShineLua.ShowMouseCursor(true)

end

function CloudGemSpeechRecognitionSample:OnDeactivate()
    self.lexNotificationHandler:Disconnect()
    self.uiEventHandler:Disconnect()
    self.tickBusHandler:Disconnect()
end

function CloudGemSpeechRecognitionSample:PrintServiceErrorMessage(error)
    Debug.Log("Error type: " .. error.type)
    Debug.Log("Error message: " .. error.message)
end

function CloudGemSpeechRecognitionSample:CallTextService()
    local edtBotText = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "edtBotText")
    local textElement = UiElementBus.Event.FindChildByName(edtBotText, "Text")
    textToSend = UiTextBus.Event.GetText(textElement)
    UiTextBus.Event.SetText(textElement, "")

    local request = CloudGemSpeechRecognition_PostTextRequest();
    request.name = "LYTestBot"
    request.bot_alias = "$LATEST"
    request.user_id = "helloworld"
    request.text = textToSend
    
    local sessionAttrMap = StringMap()
    sessionAttrMap:SetValue("foo", "bar")
    request.session_attributes = sessionAttrMap:ToJSON()

    CloudGemSpeechRecognitionRequestBus.Event.PostServicePosttext(self.entityId, request, nil)
end

function CloudGemSpeechRecognitionSample:TalkStart()
    CloudGemSpeechRecoginition.Broadcast.BeginSpeechCapture()
end

function CloudGemSpeechRecognitionSample:TalkEnd()
    -- Just an example of some user data that might be sent along with request
    local sessionAttrMap = StringMap()
    sessionAttrMap:SetValue("user_position", "10,40")

    CloudGemSpeechRecoginition.Broadcast.EndSpeechCaptureAndCallBot("LYTestBot", "$LATEST", "lumberyard_user", sessionAttrMap:ToJSON())
end

function CloudGemSpeechRecognitionSample:HandleResponse(response)
    self:ProcessPingCommand(response)

    local outString =  "Message: " .. response.message .. "\n"
                    .. "Intent: " .. response.intent .. "\n"
                    .. "Slots: " .. response.slots .. "\n"
                    .. "Dialog State: " .. response.dialog_state .. "\n"
                    .. "Slot to elicit: " .. response.slot_to_elicit .. "\n"
                    .. "Input Transcript: " .. response.input_transcript .. "\n"
                    .. "Session attributes: " .. response.session_attributes .. "\n"
    local txtResults = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "txtResults")
    UiTextBus.Event.SetText(txtResults, outString)
end

function CloudGemSpeechRecognitionSample:ProcessPingCommand(response)
    local enableElicitInfoText = false

    if response.intent == "RequestHelp" then
        self:StartAnimating("helpMe")
        self:SaySomething("I need some help here!")
    end

    if response.intent == "Ping" then
        if string.find(response.slots, "middle") then
            self:StartAnimating("pingMiddle")
            self:SaySomething("Watch the middle lane")
        end
        if string.find(response.slots, "top") then
            self:StartAnimating("pingTop")
            self:SaySomething("Careful up top")
        end
        if string.find(response.slots, "bottom") then
            self:StartAnimating("pingBottom")
            self:SaySomething("Something's coming on the bottom")
        end
        if string.find(response.slots, "me") or string.find(response.slots, "myself") then
            self:StartAnimating("pingMe")
            self:SaySomething("Look at me! Look at me! I'm awsome!")
        end
        if response.dialog_state == "ElicitSlot" then
            enableElicitInfoText = true
            self:SaySomething(response.message)
        end
    end

    if response.dialog_state == "ElicitIntent" or response.dialog_state == "ConfirmIntent" then
        enableElicitInfoText = true
        self:SaySomething(response.message)
    end

    UiElementBus.Event.SetIsEnabled(elicitInfoDlg, enableElicitInfoText)

end

function CloudGemSpeechRecognitionSample:SaySomething(message)
    UiTextBus.Event.SetText(elicitInfoTxt, message)
    TextToSpeechRequestBus.Event.ConvertTextToSpeechWithoutMarks(self.entityId, "Kendra", message)
end


function CloudGemSpeechRecognitionSample:StartAnimating(pingName)
    -- if the current ping is already animating, don't start it again
    if currentPings[pingName] ~= nil then
        return
    end

    local entity = PingEntities[pingName]
    if entity ~= nil then
        UiElementBus.Event.SetIsEnabled(entity, true)
        local animInfo = {
            currentScale = 1.0
        }
        currentPings[pingName] = animInfo
    end
end

function CloudGemSpeechRecognitionSample:ServiceAnimations(deltaTime, timePoint)
    local remove = {}
    for pingName, animInfo in pairs(currentPings) do
        animInfo.currentScale = Lerp(animInfo.currentScale, 3.0, deltaTime)
        UiTransformBus.Event.SetScale(PingEntities[pingName], Vector2(animInfo.currentScale, animInfo.currentScale))
        if animInfo.currentScale > 2.5 then
            UiElementBus.Event.SetIsEnabled(PingEntities[pingName], false)
            UiTransformBus.Event.SetScale(PingEntities[pingName], Vector2(1.0, 1.0))
            table.insert(remove, pingName)
        end
    end
    for i, pingName in ipairs(remove) do
        currentPings[pingName] = nil
    end
end

function CloudGemSpeechRecognitionSample:LogError(error)
    Debug.Log("Error Type: " .. error.type)
    Debug.Log("Error Message: " .. error.message)
end

function CloudGemSpeechRecognitionSample:OnPostServicePosttextRequestSuccess(response)
    Debug.Log("Got Text Request")
    self:HandleResponse(response)

end

function CloudGemSpeechRecognitionSample:OnPostServicePosttextRequestError(error)
    self:LogError(error)
end

function CloudGemSpeechRecognitionSample:OnPostServicePostaudioRequestSuccess(response)
    Debug.Log("Got Audio Request")
    self:HandleResponse(response)
end

function CloudGemSpeechRecognitionSample:OnPostServicePostaudioRequestError(error)
    self:LogError(error)
end

function CloudGemSpeechRecognitionSample:OnAction(entityId, actionName)
    Debug.Log("Action Name: " .. actionName)
    if actionName == "SendTextMessage" or actionName == "TextEntered" then
        self:CallTextService()
    elseif actionName == "TalkStart" then
        self:TalkStart()
    elseif actionName == "TalkEnd" then
        self:TalkEnd()
    end
end

function CloudGemSpeechRecognitionSample:OnTick(deltaTime, timePoint)
    self:ServiceAnimations(deltaTime, timePoint)
end

return CloudGemSpeechRecognitionSample