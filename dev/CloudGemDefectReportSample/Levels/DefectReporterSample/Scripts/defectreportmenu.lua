defectreportmenu =
{
    Properties =
    {
        RecordDefectEvent = "report_defect",
        OpenDefectUIEvent = "open_defect_report_ui"
    },
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

        local welcomeMessage = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "WelcomeMessage")

        self.textInput = UiElementBus.Event.FindChildByName(welcomeMessage, "Text Input")
        UiTextInputBus.Event.SetMaxStringLength(self.textInput, self.maxAnswerChars)

        self.screenShot = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "ScreenShot")

        self.submitButton = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "SubmitReportButton")
        self.submitButtonText = UiElementBus.Event.FindChildByName(self.submitButton, "Text")

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

        self.CustomCheckBoxField = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "CustomCheckBoxField")
        self.CustomRadioButtonField = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "CustomRadioButtonField")
        self.CustomTextField = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "CustomTextField")
        self.CustomEmptyField = UiCanvasBus.Event.FindElementByName(self.canvasEntityId, "CustomEmptyField")

        UiElementBus.Event.SetIsEnabled(self.CustomCheckBoxField, false)
        UiElementBus.Event.SetIsEnabled(self.CustomRadioButtonField, false)
        UiElementBus.Event.SetIsEnabled(self.CustomTextField, false)

        
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
        UiCanvasBus.Event.SetEnabled(self.newReportPopup, false)
        CloudGemDefectReporterRequestBus.Broadcast.TriggerUserReportEditing()
    elseif actionName == "EndEditAnnotation" then
        self:RecordAnnotation()
    elseif actionName == "CustomCheckBoxFieldChange" then
        self:RecordCustomCheckBoxField()
    elseif actionName == "CustomRadioButtonFieldChange" then
        self:RecordCustomRadioButtonField()
    elseif actionName == "EndEditCustomTextField" then
        self:RecordCustomTextField()
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

function defectreportmenu:UpdateReportIndex()
    if self.reportIndexText ~= nil then
        if self.maxReportIndex > 1 then
            UiTextBus.Event.SetText(self.submitButtonText, "Submit All")
        else
            UiTextBus.Event.SetText(self.submitButtonText, "Submit")
        end
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

function defectreportmenu:UpdateCurrentCustomField()
    -- hide the previous custom field
    if self.currentFieldEntityID ~= nil then
        UiElementBus.Event.SetIsEnabled(self.currentFieldEntityID, false)
    end

    --set the current entity ID according to the type of the custom field
    local type = self.Configuration["clientConfiguration"][self.curCustomFieldIndex]["type"]
    if type == "predefined" and self.Configuration["clientConfiguration"][self.curCustomFieldIndex]["multipleSelect"] then
        self.currentFieldEntityID = self.CustomCheckBoxField
    elseif type == "predefined" and self.Configuration["clientConfiguration"][self.curCustomFieldIndex]["multipleSelect"] == false then
        self.currentFieldEntityID = self.CustomRadioButtonField
    elseif type == "text" then
        self.currentFieldEntityID =self.CustomTextField
    end

    UiElementBus.Event.SetIsEnabled(self.currentFieldEntityID, true)

    local headerText = UiElementBus.Event.FindChildByName(self.currentFieldEntityID, "Header")
    UiTextBus.Event.SetText(headerText, "Custom Field: "..tostring(self.Configuration["clientConfiguration"][self.curCustomFieldIndex]["title"])) 
end

function defectreportmenu:DisplayCurrentCustomFieldDesc()
    local currentField = self.Configuration["clientConfiguration"][self.curCustomFieldIndex]
    local currentCustomFieldValue = self.FieldValues[self.curReportIndex][currentField["title"]]

    if currentField["type"] ~= "text" then
        local numOptions = #currentField["predefines"]
        -- Here we use a dynamic layout so retrieve it
        local maskEntityId = UiElementBus.Event.FindChildByName(self.currentFieldEntityID, 'Mask')
        local contentEntityId = UiElementBus.Event.FindChildByName(maskEntityId, 'Content')
        -- Set the number of checkboxes/radio buttons that we want
        UiDynamicLayoutBus.Event.SetNumChildElements(contentEntityId, numOptions)

        for optionCount = 1, numOptions do
            -- Set the text of each option
            local childElement = UiElementBus.Event.GetChild(contentEntityId, optionCount-1)
            local childText = UiElementBus.Event.FindChildByName(childElement, "Text")
            UiTextBus.Event.SetText(childText, currentField["predefines"][optionCount])

            local isChecked = currentCustomFieldValue ~= nil
            if currentField["type"] == "predefined" and currentField["multipleSelect"] then
                isChecked = isChecked and self.util.HasValue(self.util.UnserializeStringToArray(currentCustomFieldValue), currentField["predefines"][optionCount])
                UiCheckboxBus.Event.SetState(childElement, isChecked)
            elseif currentField["type"] == "predefined" and currentField["multipleSelect"] == false then
                isSelected = isChecked and currentField["predefines"][optionCount] == currentCustomFieldValue
                local radioButtonGroup = UiRadioButtonBus.Event.GetGroup(childElement)
                UiRadioButtonGroupBus.Event.SetState(radioButtonGroup, childElement, isSelected)
            end
        end
    else
        local textInputElement = UiElementBus.Event.FindChildByName(self.currentFieldEntityID, "Text Input")
        local content = currentCustomFieldValue ~= nil and tostring(currentCustomFieldValue) or ""
        UiTextInputBus.Event.SetText(textInputElement, content)
        UiTextInputBus.Event.SetMaxStringLength(textInputElement, currentField["maxChars"])
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

    -- don't show popup if main dialog already open
    if self.canvasEntityId ~= nil then
        if UiCanvasBus.Event.GetEnabled(self.canvasEntityId) then
            showPopup = false
        end
    end

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
        local popupText = tostring(numReportsAvailable) .. " Defect\nReports Available"
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

function defectreportmenu:OnDefectReportPostStatus(currentReport, totalReports)
    if currentReport == totalReports then
        local message = "Your report was sent successfully thank you for your help."
        self:UpdateMessages(message)
    end
end

function defectreportmenu:OnDefectReportPostError(errorMsg)
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
        self:UpdateCurrentCustomField()
        self:DisplayCurrentCustomFieldDesc()
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
        self:UpdateCurrentCustomField()
        self:DisplayCurrentCustomFieldDesc()
    end
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

function defectreportmenu:RecordCustomCheckBoxField()
    local maskEntityId = UiElementBus.Event.FindChildByName(self.CustomCheckBoxField, 'Mask')
    local contentEntityId = UiElementBus.Event.FindChildByName(maskEntityId, 'Content')
    local numChilds = UiElementBus.Event.GetNumChildElements(contentEntityId)

    local currentCustomField = self.Configuration["clientConfiguration"][self.curCustomFieldIndex]

    local selections = {}
    for childIndex = 1, numChilds do
        local checkBox = UiElementBus.Event.GetChild(contentEntityId, childIndex-1)
        -- If that checkbox is checked
        if UiCheckboxBus.Event.GetState(checkBox) == true then
            selections[#selections + 1] = "\""..self.Configuration["clientConfiguration"][self.curCustomFieldIndex]["predefines"][childIndex].."\""
        end
    end
    local serializedArray = "["..table.concat(selections, ",").."]"
    self.FieldValues[self.curReportIndex][currentCustomField["title"]] = serializedArray

    CloudGemDefectReporterRequestBus.Broadcast.AddCustomField(self.reportIDs[self.curReportIndex], currentCustomField["title"], serializedArray)
end

function defectreportmenu:RecordCustomRadioButtonField()
    local maskEntityId = UiElementBus.Event.FindChildByName(self.CustomRadioButtonField, 'Mask')
    local contentEntityId = UiElementBus.Event.FindChildByName(maskEntityId, 'Content')
    local numChilds = UiElementBus.Event.GetNumChildElements(contentEntityId)

    local currentCustomField = self.Configuration["clientConfiguration"][self.curCustomFieldIndex]

    local selection = ""
    for childIndex = 1, numChilds do
        local radioButton = UiElementBus.Event.GetChild(contentEntityId, childIndex - 1)
        -- If that radio button is checked 
        if UiRadioButtonBus.Event.GetState(radioButton) == true then
            selection = currentCustomField["predefines"][childIndex]
            break
        end
    end

    self.FieldValues[self.curReportIndex][currentCustomField["title"]] = selection

    CloudGemDefectReporterRequestBus.Broadcast.AddCustomField(self.reportIDs[self.curReportIndex], currentCustomField["title"], selection)
end

function defectreportmenu:RecordCustomTextField()
    local textInputElement = UiElementBus.Event.FindChildByName(self.CustomTextField, "Text Input")
    local inputText = UiTextInputBus.Event.GetText(textInputElement)
    if #inputText == 0 then
        inputText = ""
    end

    local currentCustomField = self.Configuration["clientConfiguration"][self.curCustomFieldIndex]

    self.FieldValues[self.curReportIndex][currentCustomField["title"]] = inputText

    CloudGemDefectReporterRequestBus.Broadcast.AddCustomField(self.reportIDs[self.curReportIndex], currentCustomField["title"], inputText)
end

return defectreportmenu
