local menu = require "Scripts/PlayerAccount/menu"

local forgotpasswordmenu = menu:new{canvasName = "ForgotPassword"}

function forgotpasswordmenu:OnAfterShow()
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
        self.playerAccountBus:ConfirmForgotPassword(username, password, code:match("^%s*(.-)%s*$")):OnComplete(function(result)
            if (result.wasSuccessful) then
                Debug.Log("Change forgotten password success")
                self.menuManager:ShowMenu("SignIn")
            else
                Debug.Log("Change forgotten password failed: " .. result.errorMessage)
            end
        end)
        return
    end
    
    if actionName == "SendCodeClick" then
        local username = self:GetText("UsernameText")
        self.context.username = username
        self.playerAccountBus:ForgotPassword(username):OnComplete(function(result)
            if (result.wasSuccessful) then
                Debug.Log("Send code success")
            else
                Debug.Log("Send code failed: " .. result.errorMessage)
            end
        end)
        return
    end

    menu.OnAction(self, entityId, actionName)
end

return forgotpasswordmenu
