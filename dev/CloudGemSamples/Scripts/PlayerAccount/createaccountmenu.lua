require "Scripts/PlayerAccount/menu"

createaccountmenu = menu:new{canvasName = "CreateAccount"}

function createaccountmenu:Show()
    menu.Show(self)

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
        CloudGemPlayerAccountRequestBus.Broadcast.SignUp(username, password, attributes);
        return
    end

    menu.OnAction(self, entityId, actionName)
end

function createaccountmenu:OnSignUpComplete(basicResultInfo, deliveryDetails, wasConfirmed)
    self:RunOnMainThread(function()
        if (basicResultInfo.wasSuccessful) then
            Debug.Log("Sign up success")
            self.menuManager:ShowMenu("ConfirmSignUp")
        else
            Debug.Log("Sign up failed: " .. basicResultInfo.errorMessage)
        end
    end)
end

return createaccountmenu
