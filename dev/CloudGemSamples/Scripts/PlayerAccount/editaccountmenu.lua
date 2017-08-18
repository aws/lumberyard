require "Scripts/PlayerAccount/menu"

editaccountmenu = menu:new{canvasName = "EditAccount", loadMainMenuOnSignOut = true}

function editaccountmenu:Show()
    Debug.Log("Getting account information...")
    self:ConnectToCloudGemPlayerAccountNotificationHandler()
    self.currentUserLoaded = false
    self.playerAccountLoaded = false
    CloudGemPlayerAccountRequestBus.Broadcast.GetPlayerAccount();
    CloudGemPlayerAccountRequestBus.Broadcast.GetUser(self.context.username);
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
        self.currentUserSaved = false
        self.playerAccountSaved = false
        self.saveSuccessful = true
        CloudGemPlayerAccountRequestBus.Broadcast.UpdatePlayerAccount(playerAccount);
        CloudGemPlayerAccountRequestBus.Broadcast.UpdateUserAttributes(self.context.username, attributes);
        return
    end

    menu.OnAction(self, entityId, actionName)
end

function editaccountmenu:OnGetUserComplete(result, attributes, mfaOptions)
    self:RunOnMainThread(function()
        if (result.wasSuccessful) then
            self.userAttributes = attributes
        else
            self.userAttributes = nil
            Debug.Log("Failed to get current user: " .. result.errorMessage)
        end
        self.currentUserLoaded = true
        self:ShowWhenLoadIsComplete()
    end)
end

function editaccountmenu:OnGetPlayerAccountComplete(result, playerAccount)
    self:RunOnMainThread(function()
        if (result.wasSuccessful) then
            self.playerAccount = playerAccount
        else
            self.playerAccount = nil
            if result.errorMessage:match(".*No account found.*") then
                Debug.Log("The account hasn't been created yet.")
            else
                Debug.Log("Failed to get current user: " .. result.errorMessage)
            end
        end
        self.playerAccountLoaded = true
        self:ShowWhenLoadIsComplete()
    end)
end

function editaccountmenu:ShowWhenLoadIsComplete()
    if self.currentUserLoaded and self.playerAccountLoaded then
        menu.Show(self)
        if self.playerAccount then
            self:SetText("PlayerNameText", self.playerAccount:GetPlayerName())
        end
        if self.userAttributes then
            self:SetText("EmailText", self.userAttributes:GetAttribute("email"))
        end
    end
end

function editaccountmenu:OnUpdatePlayerAccountComplete(result)
    self:RunOnMainThread(function()
        if not result.wasSuccessful then
            Debug.Log("Failed to update account: " .. result.errorMessage)
            self.saveSuccessful = false
        end
        self.playerAccountSaved = true
        self:OnSaveComplete()
    end)
end

function editaccountmenu:OnUpdateUserAttributesComplete(result, deliveryDetailsArray)
    self:RunOnMainThread(function()
        if not result.wasSuccessful then
            Debug.Log("Failed to update user pool: " .. result.errorMessage)
            self.saveSuccessful = false
        end
        self.currentUserSaved = true
        self:OnSaveComplete()
    end)
end

function editaccountmenu:OnSaveComplete()
    if self.currentUserSaved and self.playerAccountSaved then
        if self.saveSuccessful then
            self.menuManager:ShowMenu("MainMenu")
        end
    end
end

return editaccountmenu
