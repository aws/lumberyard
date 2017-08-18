require "Scripts/PlayerAccount/menu"

confirmsignupmenu = menu:new{canvasName = "ConfirmSignUp"}

function confirmsignupmenu:Show()
    menu.Show(self)

    if self.context.username then
        self:SetText("UsernameText", self.context.username)
    end
end

function confirmsignupmenu:OnAction(entityId, actionName)
    if actionName == "VerifyClick" then
        local username = self:GetText("UsernameText")
        local code = self:GetText("CodeText")

        self.context.username = username

        Debug.Log("Sending confirmation code for " .. username .. "...")
        CloudGemPlayerAccountRequestBus.Broadcast.ConfirmSignUp(username, code:match("^%s*(.-)%s*$"));
        return
    end

    menu.OnAction(self, entityId, actionName)
end

function confirmsignupmenu:OnConfirmSignUpComplete(basicResultInfo)
    self:RunOnMainThread(function()
        if (basicResultInfo.wasSuccessful) then
            Debug.Log("Confirm signup success")
            self.menuManager:ShowMenu("SignIn", {username = self.username})
        else
            Debug.Log("Confirm signup failed: " .. basicResultInfo.errorMessage)
        end
    end)
end

return confirmsignupmenu
