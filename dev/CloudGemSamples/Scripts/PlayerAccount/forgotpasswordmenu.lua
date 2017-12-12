require "Scripts/PlayerAccount/menu"

forgotpasswordmenu = menu:new{canvasName = "ForgotPassword"}

function forgotpasswordmenu:Show()
    menu.Show(self)

    if self.context.username then
        self:SetText("UsernameText", self.context.username)
    end
end

function forgotpasswordmenu:OnAction(entityId, actionName)
    if actionName == "ConfirmForgotPasswordClick" then
        local username = self:GetText("UsernameText")
        local password = self:GetText("PasswordText")
        local code = self:GetText("CodeText")
        
        self.context.username = username
        
        Debug.Log("Changing forgotten password for " .. username .. "...")
        CloudGemPlayerAccountRequestBus.Broadcast.ConfirmForgotPassword(username, password, code);
        return
    end
    
    if actionName == "SendCodeClick" then
        local username = self:GetText("UsernameText")
        self.context.username = username
        CloudGemPlayerAccountRequestBus.Broadcast.ForgotPassword(username);
        return
    end

    menu.OnAction(self, entityId, actionName)
end

function forgotpasswordmenu:OnConfirmForgotPasswordComplete(result)
    self:RunOnMainThread(function()
        if (result.wasSuccessful) then
            Debug.Log("Change forgotten password success")
            self.menuManager:ShowMenu("SignIn")
        else
            Debug.Log("Change forgotten password failed: " .. result.errorMessage)
        end
    end)
end

function forgotpasswordmenu:OnForgotPasswordComplete(result, deliveryDetails)
    self:RunOnMainThread(function()
        if (result.wasSuccessful) then
            Debug.Log("Send code success")
        else
            Debug.Log("Send code failed: " .. result.errorMessage)
        end
    end)
end

return forgotpasswordmenu
