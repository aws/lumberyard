require "Scripts/PlayerAccount/menu"

editplayernamemenu = menu:new{canvasName = "EditPlayerName"}

function editplayernamemenu:Show()
    Debug.Log("Getting account information...")
    self:ConnectToCloudGemPlayerAccountNotificationHandler()
    CloudGemPlayerAccountRequestBus.Broadcast.GetPlayerAccount();
end

function editplayernamemenu:OnAction(entityId, actionName)
    if actionName == "SavePlayerNameClick" then
        local playerName = self:GetText("PlayerNameText")
    
        local playerAccount = PlayerAccount();
        playerAccount:SetPlayerName(playerName);

        Debug.Log("Updating account...")
        CloudGemPlayerAccountRequestBus.Broadcast.UpdatePlayerAccount(playerAccount);
        return
    end

    menu.OnAction(self, entityId, actionName)
end

function editplayernamemenu:OnGetPlayerAccountComplete(result, playerAccount)
    self:RunOnMainThread(function()
        if (result.wasSuccessful) then
            menu.Show(self)
            self:SetText("PlayerNameText", playerAccount:GetPlayerName())
        else
            if result.errorMessage:match(".*No account found.*") then
                Debug.Log("The account hasn't been created yet.")
                menu.Show(self)
            else
                Debug.Log("Failed to get current user: " .. result.errorMessage)
                self.menuManager:ShowMenu("MainMenu")
            end
        end
    end)
end

function editplayernamemenu:OnUpdatePlayerAccountComplete(result)
    self:RunOnMainThread(function()
        if (result.wasSuccessful) then
            self.menuManager:ShowMenu("MainMenu")
        else
            Debug.Log("Failed to update account: " .. result.errorMessage)
        end
    end)
end

return editplayernamemenu
