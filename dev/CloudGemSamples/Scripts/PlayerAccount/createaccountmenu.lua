local menu = require "Scripts/PlayerAccount/menu"

local createaccountmenu = menu:new{canvasName = "CreateAccount"}

function createaccountmenu:OnAfterShow()
    if self.context.username then
        self:SetText("UsernameText", self.context.username)
    end
end

function createaccountmenu:OnAction(entityId, actionName)
    if actionName == "CreateAccountClick" then
        local username = self:GetText("UsernameText")
        local password = self:GetText("PasswordText")
        local email = self:GetText("EmailText")

        self.context.username = username

        local attributes = UserAttributeValues();
        attributes:SetAttribute("email", email);

        Debug.Log("Signing up as " .. username .. "...")
        self.playerAccountBus:SignUp(username, password, attributes):OnComplete(function(result)
            if (result.wasSuccessful) then
                Debug.Log("Sign up success")
                self.menuManager:ShowMenu("ConfirmSignUp")
            else
                Debug.Log("Sign up failed: " .. result.errorMessage)
            end
        end)
        return
    end

    menu.OnAction(self, entityId, actionName)
end

return createaccountmenu
