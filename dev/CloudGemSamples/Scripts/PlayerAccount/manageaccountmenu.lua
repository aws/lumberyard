require "Scripts/PlayerAccount/menu"

manageaccountmenu = menu:new{canvasName = "ManageAccount", loadMainMenuOnSignOut = true}

function manageaccountmenu:OnAction(entityId, actionName)
    if actionName == "GlobalSignOutClick" then
        Debug.Log("Global sign out...")
        CloudGemPlayerAccountRequestBus.Broadcast.GlobalSignOut(self.context.username);
        return
    end

    menu.OnAction(self, entityId, actionName)
end

function manageaccountmenu:OnGlobalSignOutComplete(result)
    self:RunOnMainThread(function()
        if (result.wasSuccessful) then
            self.menuManager:ShowMenu("MainMenu")
        else
            Debug.Log("Failed to globally sign out: " .. result.errorMessage)
        end
    end)
end

return manageaccountmenu
