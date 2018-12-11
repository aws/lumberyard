defectreportmenu =
{
    Properties =
    {
        RecordDefectEvent = "report_defect",
        OpenDefectUIEvent = "open_defect_report_ui",
        CustomFieldTypes = {"CustomCheckBoxField", "CustomRadioButtonField", "CustomTextField", "CustomObjectField"}
    },

    postTotalSteps = 0,
    postCurrentStep = 0,
    progressDesc = ""
}

function defectreportmenu:OnActivate()
    -- For handling tick events.
    self.tickBusHandler = TickBus.Connect(self,self.entityId)

    -- enable fps drop reporting
    CloudGemDefectReporterFPSDropReportingRequestBus.Broadcast.Enable(true)

    self.recordDefectEventInputID = InputEventNotificationId(self.Properties.RecordDefectEvent)
    self.recordDefectHandler = InputEventNotificationBus.Connect(self, self.recordDefectEventInputID)
    self.openDefectUIEventInputID = InputEventNotificationId(self.Properties.OpenDefectUIEvent)
    self.openDefectUIHandler = InputEventNotificationBus.Connect(self, self.openDefectUIEventInputID)

    self.defectReporterUINotificationHandler = CloudGemDefectReporterUINotificationBus.Connect(self, self.entityId)

    self.util = require("scripts.util")
    self.util.SetMouseCursorVisible(true)
    self:ShowNewReportPopupIfNeeded(0)

    self.messages = {}
    self.reachSoftCap = false;
end

function defectreportmenu:ShowNewReportPopupIfNeeded(pending)
    local reportIDs = CloudGemDefectReporterRequestBus.Broadcast.GetAvailableReportIDs()
    local numReports = #reportIDs
    if numReports > 0 then
        self:OnReportsUpdated(numReports, pending)
    end
end

function defectreportmenu:ShowDialog()
    -- hide popup when showing dialog
    if self.newReportPopupText ~= nil then
        UiCanvasBus.Event.SetEnabled(self.newReportPopupText, false)
    end

    self.maxAnswerChars = 180

    if self.canvasEntityId == nil then
        -- Let's load the canvas
        self.canvasEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("Levels/DefectReporterSample/UI/DefectReporterMenu.uicanvas")

        self.textInput = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "ErrorMessageInput")
        UiTextInputBus.Event.SetMaxStringLength(self.textInput, self.maxAnswerChars)

        self.screenShot = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "ScreenShot")

        self.submitButton = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "SubmitReportButton")
        self.submitButtonText = UiElementBus.Event.FindChildByName(self.submitButton, "Text")

        self.submitSingleButton = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "SubmitSingleReportButton")
        self.submitSingleButtonText = UiElementBus.Event.FindChildByName(self.submitSingleButton, "Text")

        self.cancelButton = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "CancelButton")

        attachEntityID = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "AttachmentList")
        maskEntityID = UiElementBus.Event.FindChildByName(attachEntityID, "Mask")
        self.attachmentList = UiElementBus.Event.FindChildByName(maskEntityID, "Content")

        metricsEntityID = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "MetricsList")
        metricsMaskEntityID = UiElementBus.Event.FindChildByName(metricsEntityID, "Mask")
        self.metricsList = UiElementBus.Event.FindChildByName(metricsMaskEntityID, "Content")

        self.reportIndexText = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "ReportIndexText")
        self.nextReportButton = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "NextReportButton")
        self.prevReportButton = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "PrevReportButton")

        -- Listen for action strings broadcast by the canvas
        self.buttonHandler = UiCanvasNotificationBus.Connect(self, self.canvasEntityId)

        self.CustomEmptyField = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "CustomEmptyField")
        self.customField = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "CustomField")


        self.Configuration = {}
        self.curCustomFieldIndex = 0
        self.maxCustomFieldIndex = 0

        self.FieldValues = {}

        CloudGemDefectReporterRequestBus.Broadcast.GetClientConfiguration()

    else
        UiCanvasBus.Event.SetEnabled(self.canvasEntityId, true)
    end

    self:RefreshAvailableIDs()
end

function defectreportmenu:SetScreenshot(filename)
    local success = ImageLoaderRequestBus.Broadcast.CreateRenderTargetFromImageFile("DefectReporterScreenshot", filename);
    if success then
        UiImageBus.Event.SetRenderTargetName(self.screenShot, "DefectReporterScreenshot")
    end
end

function defectreportmenu:OnTick(deltaTime,timePoint)
end

function defectreportmenu:OnDeactivate()
    CloudGemDefectReporterFPSDropReportingRequestBus.Broadcast.Enable(false)

    --self.notificationHandler:Disconnect();
    self.tickBusHandler:Disconnect()
    if self.buttonHandler then
        self.buttonHandler:Disconnect()
    end
    self.defectReporterUINotificationHandler:Disconnect()
    self.openDefectUIHandler:Disconnect()
    self.openDefectUIHandler:Disconnect()

    if self.newReportPopupButtonHandler then
        self.newReportPopupButtonHandler:Disconnect()
    end

    if self.newReportPopupButtonHandler then
        self.newReportPopupButtonHandler:Disconnect()
    end
end

function defectreportmenu:OnAction(entityId, actionName)
    if actionName == "Cancel" then
        self:OnCancelPressed()
    elseif actionName == "Submit" then
        self:Submit()
    elseif actionName == "SubmitSingle" then
        self:SubmitSingle()
    elseif actionName == "Ok" then
        self.messages = {}
        self.reachSoftCap = false
        UiCanvasBus.Event.SetEnabled(self.defectReporterConfirmationEntityId, false)
        self.defectReporterConfirmationEntityId = nil
        CloudGemDefectReporterFPSDropReportingRequestBus.Broadcast.Enable(true)
    elseif actionName == "NextReport" then
        self:OnNextReportPressed()
    elseif actionName == "PrevReport" then
        self:OnPrevReportPressed()
    elseif actionName == "NextMessage" then
        self:OnNextMessagePressed()
    elseif actionName == "PrevMessage" then
        self:OnPrevMessagePressed()
    elseif actionName == "NextCustomField" then
        self:OnNextCustomFieldPressed()
    elseif actionName == "PrevCustomField" then
        self:OnPrevCustomFieldPressed()
    elseif actionName == "Delete" then
        self:OnDeletePressed()
    elseif actionName == "AnnotationChange" then
        self:UpdateCharCounter()
    elseif actionName == "NewReports" then
        CloudGemDefectReporterRequestBus.Broadcast.TriggerUserReportEditing()
    elseif actionName == "EndEditAnnotation" then
        self:RecordAnnotation()
    elseif actionName == "OnCustomFieldChange" then
        self:OnCustomFieldChange()
    end
end

function defectreportmenu:OnPressed(floatValue)
    -- don't try to do things when the report dialog active as it messes up the edit box
    if not self.canvasEntityId or not UiCanvasBus.Event.GetEnabled(self.canvasEntityId) then
        if (InputEventNotificationBus.GetCurrentBusId() == self.recordDefectEventInputID) then
            CloudGemDefectReporterRequestBus.Broadcast.TriggerDefectReport(false)
            self:ShowNewReportPopupIfNeeded(1)
        elseif (InputEventNotificationBus.GetCurrentBusId() == self.openDefectUIEventInputID) then
            Debug.Log("Triggering defect report dialog")
            CloudGemDefectReporterRequestBus.Broadcast.TriggerUserReportEditing()
        end
    end
end

function defectreportmenu:OnReachSoftCap()
    if not self.reachSoftCap then
        local message = "You have generated the max number of reports, please submit or delete them before trying to generate any more"
        self:UpdateMessages(message)
        self.reachSoftCap = true
    end
end

function defectreportmenu:OnPrevMessagePressed()
    if self.curMessageIndex > 1 then
        self.curMessageIndex = self.curMessageIndex - 1
        self:UpdateMessageIndex()
        self:DisplayDefectReporterConfirmationMessage()
    end
end

function defectreportmenu:OnNextMessagePressed()
    if self.curMessageIndex < #self.messages then
        self.curMessageIndex = self.curMessageIndex + 1
        self:UpdateMessageIndex()
        self:DisplayDefectReporterConfirmationMessage()
    end
end

function defectreportmenu:UpdateMessageIndex()
    if self.messageIndexText ~= nil then
        local countStr = tostring(self.curMessageIndex) .. "/" .. tostring(#self.messages)
        UiTextBus.Event.SetText(self.messageIndexText, countStr)
    end
end

function defectreportmenu:UpdateCharCounter()
    local inputText = UiTextInputBus.Event.GetText(self.textInput)
    local counterString = tostring(#inputText).."/"..tostring(self.maxAnswerChars)

    local charCounterText = UiElementBus.Event.FindChildByName(self.textInput, "CharCounterText")
    UiTextBus.Event.SetText(charCounterText, counterString)
end

function defectreportmenu:Submit()
    CloudGemDefectReporterFPSDropReportingRequestBus.Broadcast.Enable(false)
    UiCanvasBus.Event.SetEnabled(self.canvasEntityId, false)
    CloudGemDefectReporterRequestBus.Broadcast.PostReports(self.reportIDs)
    CloudGemDefectReporterRequestBus.Broadcast.IsSubmittingReport(true)
    self.reportIDs = nil
end

function defectreportmenu:SubmitSingle()
    CloudGemDefectReporterFPSDropReportingRequestBus.Broadcast.Enable(false)

    local tempIDs = {}
    for i = 1, #self.reportIDs do
        tempIDs[i] = self.reportIDs[i]
    end

    self.reportIDs:clear()
    self.reportIDs:push_back(tempIDs[self.curReportIndex])

    CloudGemDefectReporterRequestBus.Broadcast.PostReports(self.reportIDs)
    CloudGemDefectReporterRequestBus.Broadcast.IsSubmittingReport(true)

    self.reportIDs:clear()
    for i = 1, #tempIDs do
        if i ~= self.curReportIndex then
            self.reportIDs:push_back(tempIDs[i])
        end
    end

    self.maxReportIndex = #self.reportIDs
    if self.curReportIndex > self.maxReportIndex then
        self.curReportIndex = self.curReportIndex - 1
    end

    self:DisplayCurrentReportDesc()
    self:UpdateReportIndex()

    if self.maxReportIndex == 0 then
        UiCanvasBus.Event.SetEnabled(self.canvasEntityId, false)
    end
end

function defectreportmenu:UpdateReportIndex()
    if self.reportIndexText ~= nil then
        local countStr = tostring(self.curReportIndex) .. "/" .. tostring(self.maxReportIndex)
        UiTextBus.Event.SetText(self.reportIndexText, countStr)
    end
end

function defectreportmenu:DisplayCurrentReportDesc()
    if self.curReportIndex >= 1 then
        self:UpdateAnnotationEditBox("")
        self.curDefectReport = CloudGemDefectReporterRequestBus.Broadcast.GetReport(self.reportIDs[self.curReportIndex])
        -- store the key/value pair to initialize the custom field
        self.FieldValues[self.curReportIndex] = {}
        if self.curDefectReport.reportID >= 0 then	-- reportID < 0 means the report is valid
            local numMetrics = #self.curDefectReport.metrics
            UiDynamicLayoutBus.Event.SetNumChildElements(self.metricsList, numMetrics)
            if numMetrics >= 1 then
                for count = 1, numMetrics do
                    local textElement = UiElementBus.Event.GetChild(self.metricsList, count - 1)
                    local key = self.curDefectReport.metrics[count].key
                    local originalValue = self.curDefectReport.metrics[count].data
                    -- truncate to 100 characters to not blow out text UI
                    local value = string.sub(originalValue, 0, 100)
                    -- get rid of linefeeds as they screw up the display
                    value = string.gsub(value, "%c", "")
                    -- replace some special characters because of the XML parser warning
                    value = string.gsub(value, "<", "&lt;")
                    value = string.gsub(value, ">", "&gt;")
                    value = string.gsub(value, "\"", "&#34;")

                    UiTextBus.Event.SetText(textElement, "Key: " .. key .. " Value: " .. value)
                    if key == "annotation" then
                        self:UpdateAnnotationEditBox(value)
                    end

                    self.FieldValues[self.curReportIndex][key] = originalValue
                end
            end

            local numAttachments = #self.curDefectReport.attachments
            UiDynamicLayoutBus.Event.SetNumChildElements(self.attachmentList, numAttachments)
            if numAttachments >= 1 then
                for count = 1, numAttachments do
                    local textElement = UiElementBus.Event.GetChild(self.attachmentList, count - 1)
                    UiTextBus.Event.SetText(textElement, self.curDefectReport.attachments[count].path)

                    if self.curDefectReport.attachments[count].type == "image/jpeg" then
                        self:SetScreenshot(self.curDefectReport.attachments[count].path)
                    end
                end
            end
        end

        self.curCustomFieldIndex = 0
        self:OnNextCustomFieldPressed()
    else
        -- hide and show "no available reports" text
    end
end

function defectreportmenu:getCurrentCustomField(customFieldSchema, parent)
    local numCustomFieldTypes = #self.Properties.CustomFieldTypes
    local customFieldMapping = {}
    for optionCount = 1, numCustomFieldTypes do
        local customFieldName = self.Properties.CustomFieldTypes[optionCount]
        local customField = UiElementBus.Event.FindChildByName(parent, customFieldName)
        customFieldMapping[customFieldName] = customField
        UiElementBus.Event.SetIsEnabled(customField, false)
    end

    local type = customFieldSchema["type"]
    local respose = nil
    if type == "predefined" and customFieldSchema["multipleSelect"] then
        respose = customFieldMapping["CustomCheckBoxField"]
    elseif type == "predefined" and not customFieldSchema["multipleSelect"] then
        respose = customFieldMapping["CustomRadioButtonField"]
    elseif type == "text" then
        respose = customFieldMapping["CustomTextField"]
    elseif type == "object" then
        respose = customFieldMapping["CustomObjectField"]
    end

    if respose~= nil then
        UiElementBus.Event.SetIsEnabled(respose, true)
    end
    return respose
end

function defectreportmenu:DisplayCurrentCustomFieldDesc(parent, customFieldSchema, fieldValue, isObjectProperty)
    local headerText = UiElementBus.Event.FindChildByName(parent, "Header")
    UiTextBus.Event.SetText(headerText, customFieldSchema['title'])

    local customField = self:getCurrentCustomField(customFieldSchema, parent)
    if not isObjectProperty and customFieldSchema["type"] ~= "text" then
        customField = UiElementBus.Event.FindChildByName(customField, 'Mask')
    end

    local fieldValueExists = fieldValue ~= nil

    if customFieldSchema["type"] == "predefined" then
        local options = #customFieldSchema["predefines"]
        -- Here we use a dynamic layout so retrieve it
        local contentEntityId = UiElementBus.Event.FindChildByName(customField, 'Content')
        -- Set the number of checkboxes/radio buttons that we want
        UiDynamicLayoutBus.Event.SetNumChildElements(contentEntityId, options)
        for optionCount = 1, options do
            -- Set the text of each option
            local childElement = UiElementBus.Event.GetChild(contentEntityId, optionCount - 1)
            local childElementText = UiElementBus.Event.FindChildByName(childElement, "Text")
            UiTextBus.Event.SetText(childElementText, customFieldSchema["predefines"][optionCount])
            if customFieldSchema["type"] == "predefined" and customFieldSchema["multipleSelect"] then
                isChecked = fieldValueExists and self.util.HasValue(self.util.ConvertStringToArray(fieldValue), customFieldSchema["predefines"][optionCount])
                UiCheckboxBus.Event.SetState(childElement, isChecked)
            elseif customFieldSchema["type"] == "predefined" and customFieldSchema["multipleSelect"] == false then
                isSelected = fieldValueExists and customFieldSchema["predefines"][optionCount] == fieldValue
                local radioButtonGroup = UiRadioButtonBus.Event.GetGroup(childElement)
                UiRadioButtonGroupBus.Event.SetState(radioButtonGroup, childElement, isSelected)
            end
        end
    elseif customFieldSchema["type"] == "text" then
        if not fieldValueExists then
            fieldValue = ""
        end

        local textInputElement = UiElementBus.Event.FindChildByName(customField, "Text Input")
        UiTextInputBus.Event.SetText(textInputElement, fieldValue)
        UiTextInputBus.Event.SetMaxStringLength(textInputElement, customFieldSchema["maxChars"])
    elseif customFieldSchema["type"] == "object" then
        local propertyValue = {}
        if fieldValueExists then
            propertyValue = self.util.DeserializeString(fieldValue, customFieldSchema)
        end
        local numOptions = #customFieldSchema["properties"]
        -- Here we use a dynamic layout so retrieve it
        local contentEntityId = UiElementBus.Event.FindChildByName(customField, 'Content')
        -- Set the number of checkboxes/radio buttons that we want
        UiDynamicLayoutBus.Event.SetNumChildElements(contentEntityId, numOptions)
        for optionCount = 1, numOptions do
            -- Set the text of each option
            local propertyField = UiElementBus.Event.GetChild(contentEntityId, optionCount - 1)
            local propertyFieldSchema = customFieldSchema["properties"][optionCount]
            self:DisplayCurrentCustomFieldDesc(propertyField, propertyFieldSchema, propertyValue[propertyFieldSchema['title']], true)
        end
    end
end

function defectreportmenu:OnOpenDefectReportEditorUI()
    CloudGemDefectReporterRequestBus.Broadcast.IsSubmittingReport(false);
    self:ShowDialog()
end

function defectreportmenu:OnReportsUpdated(numReportsAvailable, reportsProcessing)
    if self.newReportPopup == nil then
        self.newReportPopup = UiCanvasManagerBus.Broadcast.LoadCanvas("Levels/DefectReporterSample/UI/DefectReporterNewReportPopup.uicanvas")
        self.newReportPopupButtonHandler = UiCanvasNotificationBus.Connect(self, self.newReportPopup)
        self.newReportPopupText = UiCanvasBus.Event.FindElementByName(self.newReportPopup, "Text")
        self.newReportPendingText = UiCanvasBus.Event.FindElementByName(self.newReportPopup, "PendingText")
        UiElementBus.Event.SetIsEnabled(self.newReportPendingText, false)
    end

    local showPopup = true

    if numReportsAvailable == 0 and reportsProcessing == 0 then
        showPopup = false
    end

    UiCanvasBus.Event.SetEnabled(self.newReportPopup, showPopup)
    if self.newReportPopupButtonHandler ~= nil then
        if self.showPopup == false then
            self.newReportPopupButtonHandler:Disconnect()
        end
    else
        self.newReportPopupButtonHandler = UiCanvasNotificationBus.Connect(self, self.newReportPopup)
    end

    if self.newReportPopupText ~= nil then
        local popupText = tostring(numReportsAvailable) .. " Defect\nReports Available" .. "\n" .. self.progressDesc
        UiTextBus.Event.SetText(self.newReportPopupText, popupText)
    end

    if self.newReportPendingText ~= nil then
        if reportsProcessing > 0 then
        UiElementBus.Event.SetIsEnabled(self.newReportPendingText, true)
            self.numReportsProcessing = reportsProcessing
        else
        UiElementBus.Event.SetIsEnabled(self.newReportPendingText, false)
            self.numReportsProcessing = 0
        end
    end
end

function defectreportmenu:UpdateProgressDesc()
    if self.postTotalSteps > 0 and self.postCurrentStep >= 0 then
        local percent = math.min(100, math.floor(self.postCurrentStep / self.postTotalSteps * 100))
        self.progressDesc = "Uploading " .. tostring(percent) .. "%"
    else
        self.progressDesc = ""
    end
end

function defectreportmenu:OnDefectReportPostStart(totalSteps)
    self.postTotalSteps = totalSteps
    self.postCurrentStep = 0
    self:UpdateProgressDesc()
end

function defectreportmenu:ClearProgress()
    self.postTotalSteps = 0
    self.postCurrentStep = 0
    self.progressDesc = ""
end

function defectreportmenu:OnDefectReportPostStep()
    self.postCurrentStep = self.postCurrentStep + 1
    self:UpdateProgressDesc()

    if self.postCurrentStep >= self.postTotalSteps and self.postTotalSteps > 0 then
        self:ClearProgress()
        local message = "Your report was sent successfully"
        self:UpdateMessages(message)
    end
end


function defectreportmenu:OnDefectReportPostError(errorMsg)
    self:ClearProgress()
    local message = "Error submitting report: "..tostring(errorMsg)
    self:UpdateMessages(message)
end

function defectreportmenu:UpdateMessages(message)
    Debug.Log(tostring(message))
    self.messages[#self.messages + 1] = message
    self:LoadDefectReporterConfirmation()
end

function defectreportmenu:LoadDefectReporterConfirmation()
    if self.defectReporterConfirmationEntityId == nil then
        self.defectReporterConfirmationEntityId = UiCanvasManagerBus.Broadcast.LoadCanvas("Levels/DefectReporterSample/UI/DefectReporterConfirmation.uicanvas")
        self.confirmationButtonHandler = UiCanvasNotificationBus.Connect(self, self.defectReporterConfirmationEntityId)
        self.messageIndexText = UiCanvasBus.Event.FindElementByName(self.defectReporterConfirmationEntityId, "MessageIndexText")
        self.messageEntityId = UiCanvasBus.Event.FindElementByName(self.defectReporterConfirmationEntityId, "Message")

        UiCanvasBus.Event.SetEnabled(self.defectReporterConfirmationEntityId, false)
    end

    if not UiCanvasBus.Event.GetEnabled(self.defectReporterConfirmationEntityId) then
        UiCanvasBus.Event.SetEnabled(self.defectReporterConfirmationEntityId, true)
        self.curMessageIndex = 1
        self:DisplayDefectReporterConfirmationMessage()
    end

    self:UpdateMessageIndex()
end

function defectreportmenu:DisplayDefectReporterConfirmationMessage()
    UiTextBus.Event.SetText(self.messageEntityId, self.messages[self.curMessageIndex])
end

function defectreportmenu:OnClientConfigurationAvailable(clientConfiguration)
    -- add a new custom field
    self.Configuration = clientConfiguration
    self.maxCustomFieldIndex = #self.Configuration["clientConfiguration"]

    if self.currentFieldEntityID == nil and self.maxCustomFieldIndex > 0 then
        UiElementBus.Event.SetIsEnabled(self.CustomEmptyField, false)
        self:OnNextCustomFieldPressed()
    end

end

function defectreportmenu:OnNextReportPressed()
    if self.curReportIndex < self.maxReportIndex then
        self.curReportIndex = self.curReportIndex + 1
        self:UpdateReportIndex()
        self:DisplayCurrentReportDesc()
    end
end

function defectreportmenu:OnNextCustomFieldPressed()
    if self.curCustomFieldIndex < self.maxCustomFieldIndex then
        self.curCustomFieldIndex = self.curCustomFieldIndex + 1
        local currentFieldSchema = self.Configuration["clientConfiguration"][self.curCustomFieldIndex]
        local currentFieldValue = self:GetCurrentFieldValue(currentFieldSchema)
        self:DisplayCurrentCustomFieldDesc(self.customField, currentFieldSchema, currentFieldValue, false)
    end
end

function defectreportmenu:OnPrevReportPressed()
    if self.curReportIndex > 1 then
        self.curReportIndex = self.curReportIndex - 1
        self:UpdateReportIndex()
        self:DisplayCurrentReportDesc()
    end
end

function defectreportmenu:OnPrevCustomFieldPressed()
    if self.curCustomFieldIndex > 1 then
        self.curCustomFieldIndex = self.curCustomFieldIndex - 1
        local currentFieldSchema = self.Configuration["clientConfiguration"][self.curCustomFieldIndex]
        local currentFieldValue = self:GetCurrentFieldValue(currentFieldSchema)
        self:DisplayCurrentCustomFieldDesc(self.customField, currentFieldSchema, currentFieldValue, false)
    end
end

function defectreportmenu:GetCurrentFieldValue(customFieldSchema)
    if self.FieldValues[self.curReportIndex][customFieldSchema["title"]] == nil and customFieldSchema['defaultValue'] ~= nil then
        self.FieldValues[self.curReportIndex][customFieldSchema["title"]] = customFieldSchema['defaultValue']
        CloudGemDefectReporterRequestBus.Broadcast.AddCustomField(self.reportIDs[self.curReportIndex], customFieldSchema["title"], customFieldSchema['defaultValue'])
    end
    return self.FieldValues[self.curReportIndex][customFieldSchema["title"]]
end

function defectreportmenu:OnDeletePressed()
    CloudGemDefectReporterRequestBus.Broadcast.RemoveReport(self.reportIDs[self.curReportIndex])
    self:RefreshAvailableIDs()
end

function defectreportmenu:OnCancelPressed()
    UiCanvasBus.Event.SetEnabled(self.canvasEntityId, false)
    CloudGemDefectReporterRequestBus.Broadcast.IsSubmittingReport(true);
    self:ShowNewReportPopupIfNeeded(0)
end

function defectreportmenu:RefreshAvailableIDs()
    -- get the list of report ID's
    self.reportIDs = CloudGemDefectReporterRequestBus.Broadcast.GetAvailableReportIDs()
    self.maxReportIndex = #self.reportIDs
    if self.maxReportIndex == 0 then
        self.curReportIndex = 0
        self:OnCancelPressed()
    else
        self.curReportIndex = 1
        self:UpdateReportIndex()
        self:DisplayCurrentReportDesc()
    end
end

function defectreportmenu:RecordAnnotation()
    if self.textInput then
        local inputText = UiTextInputBus.Event.GetText(self.textInput)
        CloudGemDefectReporterRequestBus.Broadcast.AddAnnotation(self.reportIDs[self.curReportIndex], inputText)
        self:DisplayCurrentReportDesc()
    end
end

function defectreportmenu:UpdateAnnotationEditBox(message)
    UiTextInputBus.Event.SetText(self.textInput, message)
end

function defectreportmenu:OnCustomFieldChange()
    local currentCustomFieldSchema = self.Configuration["clientConfiguration"][self.curCustomFieldIndex]
    local currentCustomField = self:getCurrentCustomField(currentCustomFieldSchema, self.customField)

    if currentCustomFieldSchema['type'] ~= "text" then
        currentCustomField = UiElementBus.Event.FindChildByName(currentCustomField, 'Mask')
    end

    local value = self:GetCustomFieldValue(currentCustomField, currentCustomFieldSchema)
    if currentCustomFieldSchema['type'] == "predefined" and currentCustomFieldSchema["multipleSelect"] then
        value = self.util.convertArrayToString(value)
    end

    self.FieldValues[self.curReportIndex][currentCustomFieldSchema["title"]] = value
    CloudGemDefectReporterRequestBus.Broadcast.AddCustomField(self.reportIDs[self.curReportIndex], currentCustomFieldSchema["title"], value)
end

function defectreportmenu:GetCustomFieldValue(currentCustomField, currentCustomFieldSchema)
    local value = ""
    if currentCustomFieldSchema['type'] == "predefined" and not currentCustomFieldSchema["multipleSelect"] then
        value = self:GetRadioButtonFieldSelection(currentCustomField, currentCustomFieldSchema)
    elseif currentCustomFieldSchema['type'] == "predefined" and currentCustomFieldSchema["multipleSelect"] then
        value = self:GetCheckBoxFieldSelections(currentCustomField, currentCustomFieldSchema)
    elseif currentCustomFieldSchema['type'] == "text" then
        value = self:GetTextFieldContent(currentCustomField)
    elseif currentCustomFieldSchema['type'] == "object" then
        value = self:GetObjectFieldContent(currentCustomField, currentCustomFieldSchema)
    end
    return value
end

function defectreportmenu:GetRadioButtonFieldSelection(radioButtonField, radioButtonFieldSchema)
    local content = UiElementBus.Event.FindChildByName(radioButtonField, 'Content')
    local numChilds = UiElementBus.Event.GetNumChildElements(content)
    for childIndex = 1, numChilds do
        local radioButton = UiElementBus.Event.GetChild(content, childIndex - 1)
        -- If that radio button is checked
        if UiRadioButtonBus.Event.GetState(radioButton) == true then
            return radioButtonFieldSchema["predefines"][childIndex]
        end
    end

    return ""
end

function defectreportmenu:GetCheckBoxFieldSelections(checkBoxField, checkBoxFieldSchema)
    local content = UiElementBus.Event.FindChildByName(checkBoxField, 'Content')
    local numChilds = UiElementBus.Event.GetNumChildElements(content)

    local selections = {}
    for childIndex = 1, numChilds do
        local checkBox = UiElementBus.Event.GetChild(content, childIndex-1)
        -- If that checkbox is checked
        if UiCheckboxBus.Event.GetState(checkBox) == true then
            selections[#selections + 1] = "\""..checkBoxFieldSchema["predefines"][childIndex].."\""
        end
    end

    return selections
end

function defectreportmenu:GetTextFieldContent(textField)
    local textInput = UiElementBus.Event.FindChildByName(textField, "Text Input")
    local text = UiTextInputBus.Event.GetText(textInput)
    return text
end


function defectreportmenu:GetObjectFieldContent(objectField, objectFieldSchema)
    local contentEntityId = UiElementBus.Event.FindChildByName(objectField, 'Content')
    local numChilds = UiElementBus.Event.GetNumChildElements(contentEntityId)

    local value = {}
    for childIndex = 1, numChilds do
        local childField = UiElementBus.Event.GetChild(contentEntityId, childIndex - 1)
        local propertyFieldSchema = objectFieldSchema['properties'][childIndex]
        local propertyField = self:getCurrentCustomField(propertyFieldSchema, childField)
        local propertyValue = self:GetCustomFieldValue(propertyField, propertyFieldSchema)
        if #propertyValue ~= 0 then
            value[propertyFieldSchema['title']] = propertyValue
        end
    end
    return self.util.converTableToString(value)
end

return defectreportmenu
