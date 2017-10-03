local menu = require "Scripts/PlayerAccount/menu"

local mainmenu = menu:new{canvasName = "MainMenu"}

function mainmenu:OnBeforeShow()
    Debug.Log("Getting current username...")
    local getCurrentUserAsyncResult = self.playerAccountBus:GetCurrentUser()
    local getPlayerAccountAsyncResult = self.playerAccountBus:GetPlayerAccount()
    local result = getCurrentUserAsyncResult:Chain(getPlayerAccountAsyncResult)
    result:OnComplete(function(currentUserParameters, playerAccountParameters)
        local currentUserResult = unpack(currentUserParameters)
        self.playerAccountResult, self.playerAccount = unpack(playerAccountParameters)

        if (currentUserResult.wasSuccessful) then
            Debug.Log("Current username: '" .. currentUserResult.username .. "'")
            self.context.username = currentUserResult.username
            self.canvasName = "MainMenuSignedIn"
            self.loadMainMenuOnSignOut = true
        else
            Debug.Log("Failed to get current user: " .. currentUserResult.errorMessage)
            selfcanvasName = "MainMenu"
            self.loadMainMenuOnSignOut = false
        end
    end)
    return result
end

function mainmenu:OnAfterShow()
    if self.playerAccountResult.wasSuccessful then
        self:SetText("PlayerNameText", self.playerAccount:GetPlayerName())
    else
        if self.playerAccountResult.errorMessage:match(".*No account found.*") then
            Debug.Log("The account hasn't been created yet.")
        else
            Debug.Log("Failed to get current user: " .. self.playerAccountResult.errorMessage)
        end
    end
end

function mainmenu:OnAction(entityId, actionName)
    if actionName == "SignOutClick" then
        Debug.Log("Signing out " .. self.context.username .. "...")
        self.loadMainMenuOnSignOut = false
        self.playerAccountBus:SignOut(self.context.username):OnComplete(function(result)
            Debug.Log("Signed out.");
            self.menuManager:ShowMenu("MainMenu")
        end)
        return
    end

    menu.OnAction(self, entityId, actionName)
end

return mainmenu
