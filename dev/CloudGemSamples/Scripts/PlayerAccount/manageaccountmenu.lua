local menu = require "Scripts/PlayerAccount/menu"

local manageaccountmenu = menu:new{canvasName = "ManageAccount", loadMainMenuOnSignOut = true}

function manageaccountmenu:OnAction(entityId, actionName)
    if actionName == "GlobalSignOutClick" then
        Debug.Log("Global sign out...")
        self.loadMainMenuOnSignOut = false
        self.playerAccountBus:GlobalSignOut(self.context.username):OnComplete(function(result)
            if (result.wasSuccessful) then
                self.menuManager:ShowMenu("MainMenu")
            else
                self.loadMainMenuOnSignOut = true
                Debug.Log("Failed to globally sign out: " .. result.errorMessage)
            end
        end)
        return
    end

    menu.OnAction(self, entityId, actionName)
end

return manageaccountmenu
