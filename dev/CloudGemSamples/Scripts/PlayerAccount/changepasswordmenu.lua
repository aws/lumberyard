local menu = require "Scripts/PlayerAccount/menu"

local changepasswordmenu = menu:new{canvasName = "ChangePassword", loadMainMenuOnSignOut = true}

function changepasswordmenu:OnAfterShow()
    if self.context.username then
        self:SetText("UsernameText", self.context.username)
    end
end

function changepasswordmenu:OnAction(entityId, actionName)
    if actionName == "ChangePasswordClick" then
        local username = self:GetText("UsernameText")
        local oldPassword = self:GetText("OldPasswordText")
        local newPassword = self:GetText("NewPasswordText")
        
        self.context.username = username
        
        Debug.Log("Changing password for " .. username .. "...")
        self.playerAccountBus:ChangePassword(username, oldPassword, newPassword):OnComplete(function(result)
            if (result.wasSuccessful) then
                Debug.Log("Change password success")
                self.menuManager:ShowMenu("MainMenu")
            else
                Debug.Log("Change password failed: " .. result.errorMessage)
            end
        end)
        return
    end

    menu.OnAction(self, entityId, actionName)
end

return changepasswordmenu
