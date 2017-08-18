require "Scripts/PlayerAccount/menu"

changepasswordmenu = menu:new{canvasName = "ChangePassword", loadMainMenuOnSignOut = true}

function changepasswordmenu:Show()
    menu.Show(self)

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
        CloudGemPlayerAccountRequestBus.Broadcast.ChangePassword(username, oldPassword, newPassword);
        return
    end

    menu.OnAction(self, entityId, actionName)
end

function changepasswordmenu:OnChangePasswordComplete(result)
    self:RunOnMainThread(function()
        if (result.wasSuccessful) then
            Debug.Log("Change password success")
            self.menuManager:ShowMenu("MainMenu")
        else
            Debug.Log("Change password failed: " .. result.errorMessage)
        end
    end)
end

return changepasswordmenu
