keyexplanation = {}

function keyexplanation:OnActivate()
    self.keyExplanation = UiCanvasManagerBus.Broadcast.LoadCanvas("Levels/DefectReporterSample/UI/DefectReporterKeyExplanation.uicanvas")
    local keyExplanationText = UiCanvasBus.Event.FindElementByName(self.keyExplanation, "Key")

    local reportDefectKey = CloudGemDefectReporterRequestBus.Broadcast.GetInputRecord("report_defect"):gsub("keyboard_key_function_", "")
    local openDefectReportUiKey = CloudGemDefectReporterRequestBus.Broadcast.GetInputRecord("open_defect_report_ui"):gsub("keyboard_key_function_", "")

    local existingKeyExplanation = ""
    if reportDefectKey ~= "" then
        existingKeyExplanation = existingKeyExplanation.."Press "..reportDefectKey.." to generate a report"
    end
    if openDefectReportUiKey ~= "" then
        existingKeyExplanation = existingKeyExplanation.."\nPress "..openDefectReportUiKey.." to open the report dialog"
    end
    UiTextBus.Event.SetText(keyExplanationText, existingKeyExplanation)
end

return keyexplanation