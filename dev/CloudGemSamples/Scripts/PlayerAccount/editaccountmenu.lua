local menu = require "Scripts/PlayerAccount/menu"

local editaccountmenu = menu:new{canvasName = "EditAccount", loadMainMenuOnSignOut = true}

function editaccountmenu:OnBeforeShow()
    Debug.Log("Getting account information...")

    local getUserAsyncResult = self.playerAccountBus:GetUser(self.context.username)
    local getPlayerAccountAsyncResult = self.playerAccountBus:GetPlayerAccount()
    local result = getUserAsyncResult:Chain(getPlayerAccountAsyncResult)
    result:OnComplete(function(userParameters, playerAccountParameters)
        local userResult, attributes, mfaOptions = unpack(userParameters)
        local playerAccountResult, playerAccount = unpack(playerAccountParameters)
        
        if (userResult.wasSuccessful) then
            self.userAttributes = attributes
        else
            self.userAttributes = nil
            Debug.Log("Failed to get current user: " .. userResult.errorMessage)
        end

        if (playerAccountResult.wasSuccessful) then
            self.playerAccount = playerAccount
        else
            self.playerAccount = nil
            if playerAccountResult.errorMessage:match(".*No account found.*") then
                Debug.Log("The account hasn't been created yet.")
            else
                Debug.Log("Failed to get current user: " .. playerAccountResult.errorMessage)
            end
        end
    end)
    return result
end

function editaccountmenu:OnAfterShow()
    if self.playerAccount then
        self:SetText("PlayerNameText", self.playerAccount:GetPlayerName())
    end
    if self.userAttributes then
        self:SetText("EmailText", self.userAttributes:GetAttribute("email"))
    end
end

function editaccountmenu:OnAction(entityId, actionName)
    if actionName == "SaveAccountClick" then
        local playerName = self:GetText("PlayerNameText")
        local email = self:GetText("EmailText")

        local playerAccount = PlayerAccount();
        playerAccount:SetPlayerName(playerName);

        local attributes = UserAttributeValues();
        attributes:SetAttribute("email", email);

        Debug.Log("Updating account...")
        local updatePlayerAsyncResult = self.playerAccountBus:UpdatePlayerAccount(playerAccount)
        local updateUserAttributesAsyncResult = self.playerAccountBus:UpdateUserAttributes(self.context.username, attributes)
        updatePlayerAsyncResult:Chain(updateUserAttributesAsyncResult):OnComplete(function(updatePlayerParameters, updateUserParameters)
            local updatePlayerResult = unpack(updatePlayerParameters)
            local updateUserResult = unpack(updateUserParameters)

            if not updatePlayerResult.wasSuccessful then
                Debug.Log("Failed to update account: " .. updatePlayerResult.errorMessage)
            end

            if not updateUserResult.wasSuccessful then
                Debug.Log("Failed to update user pool: " .. updateUserResult.errorMessage)
            end

            if updatePlayerResult.wasSuccessful and updateUserResult.wasSuccessful then
                self.menuManager:ShowMenu("MainMenu")
            end
        end)
        return
    end

    menu.OnAction(self, entityId, actionName)
end

return editaccountmenu
