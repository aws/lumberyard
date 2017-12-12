local menu = require "Scripts/PlayerAccount/menu"

local signinmenu = menu:new{canvasName = "SignIn"}

function signinmenu:OnAfterShow()
    if self.context.username then
        self:SetText("UsernameText", self.context.username)
    end
end

function signinmenu:OnAction(entityId, actionName)
    if actionName == "SignInClick" then
        local username = self:GetText("UsernameText")
        local password = self:GetText("PasswordText")
        
        self.context.username = username
        
        Debug.Log("Signing in as " .. username .. "...")
        self.playerAccountBus:InitiateAuth(username, password):OnComplete(function(result)
            if (result.wasSuccessful) then
                Debug.Log("Sign in success")
                self.menuManager:ShowMenu("MainMenu")
            else
                if result.errorTypeName == "ACCOUNT_BLACKLISTED" then
                    Debug.Log("Unable to sign in: the account is blacklisted.")
                elseif result.errorTypeName == "FORCE_CHANGE_PASSWORD" then
                    Debug.Log("Unable to sign in: A password change is required.")
                    self.menuManager:ShowMenu("ForceChangePassword")
                elseif result.errorTypeName == "UserNotConfirmedException" then
                    Debug.Log("Unable to sign in: A confirmation code is required.")
                    self.menuManager:ShowMenu("ConfirmSignUp")
                else
                    Debug.Log("Sign in failed: " .. result.errorMessage)
                end
            end
        end)
        return
    end

    if actionName == "ForgotPasswordClick" then
        local username = self:GetText("UsernameText")
        self.context.username = username
        self.menuManager:ShowMenu("ForgotPassword")
        return
    end

    menu.OnAction(self, entityId, actionName)
end

return signinmenu
