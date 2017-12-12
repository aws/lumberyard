local menu = require "Scripts/PlayerAccount/menu"

local confirmsignupmenu = menu:new{canvasName = "ConfirmSignUp"}

function confirmsignupmenu:OnAfterShow()
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
        self.playerAccountBus:ConfirmSignUp(username, code:match("^%s*(.-)%s*$")):OnComplete(function(result)
            if (result.wasSuccessful) then
                Debug.Log("Confirm signup success")
                self.menuManager:ShowMenu("SignIn", {username = self.username})
            else
                Debug.Log("Confirm signup failed: " .. result.errorMessage)
            end
        end)
        return
    end

    menu.OnAction(self, entityId, actionName)
end

return confirmsignupmenu
