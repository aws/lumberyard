local menu = require "Scripts/PlayerAccount/menu"

local editplayernamemenu = menu:new{canvasName = "EditPlayerName"}

function editplayernamemenu:OnBeforeShow()
    Debug.Log("Getting account information...")
    local result = self.playerAccountBus:GetPlayerAccount()
    result:OnComplete(function(playerAccountResult, playerAccount)
        self.playerAccountResult = playerAccountResult
        self.playerAccount = playerAccount
    end)
    return result
end

function editplayernamemenu:OnAfterShow()
    if (self.playerAccountResult.wasSuccessful) then
        self:SetText("PlayerNameText", self.playerAccount:GetPlayerName())
    else
        if self.playerAccountResult.errorMessage:match(".*No account found.*") then
            Debug.Log("The account hasn't been created yet.")
        else
            Debug.Log("Failed to get current user: " .. self.playerAccountResult.errorMessage)
            self.menuManager:ShowMenu("MainMenu")
        end
    end
end

function editplayernamemenu:OnAction(entityId, actionName)
    if actionName == "SavePlayerNameClick" then
        local playerName = self:GetText("PlayerNameText")
    
        local playerAccount = PlayerAccount();
        playerAccount:SetPlayerName(playerName);

        Debug.Log("Updating account...")
        self.playerAccountBus:UpdatePlayerAccount(playerAccount):OnComplete(function(result)
            if (result.wasSuccessful) then
                self.menuManager:ShowMenu("MainMenu")
            else
                Debug.Log("Failed to update account: " .. result.errorMessage)
            end
        end)
        return
    end

    menu.OnAction(self, entityId, actionName)
end

return editplayernamemenu
