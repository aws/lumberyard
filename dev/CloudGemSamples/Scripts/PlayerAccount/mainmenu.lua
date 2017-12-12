require "Scripts/PlayerAccount/menu"

mainmenu = menu:new{canvasName = "MainMenu", loadMainMenuOnSignOut = true}

function mainmenu:Show()
    Debug.Log("Getting current username...")
    self:ConnectToCloudGemPlayerAccountNotificationHandler()
    self.currentUserLoaded = false
    self.playerAccountLoaded = false
    self.isLoading = true

    CloudGemPlayerAccountRequestBus.Broadcast.GetCurrentUser()
    CloudGemPlayerAccountRequestBus.Broadcast.GetPlayerAccount();
end

function mainmenu:OnAction(entityId, actionName)
    if actionName == "SignOutClick" then
        Debug.Log("Signing out " .. self.context.username .. "...")
        CloudGemPlayerAccountRequestBus.Broadcast.SignOut(self.context.username);
        return
    end

    menu.OnAction(self, entityId, actionName)
end

function mainmenu:OnGetCurrentUserComplete(result)
    self:RunOnMainThread(function()
        if (result.wasSuccessful) then
            Debug.Log("Current username: '" .. result.username .. "'")
            self.context.username = result.username
            self.canvasName = "MainMenuSignedIn"
        else
            Debug.Log("Failed to get current user: " .. result.errorMessage)
            selfcanvasName = "MainMenu"
        end
        self.currentUserLoaded = true
        self:ShowWhenLoadIsComplete()
    end)
end

function mainmenu:OnGetPlayerAccountComplete(result, playerAccount)
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

function mainmenu:ShowWhenLoadIsComplete()
    if self.isLoading and self.currentUserLoaded and self.playerAccountLoaded then
        menu.Show(self)
        if self.playerAccount then
            self:SetText("PlayerNameText", self.playerAccount:GetPlayerName())
        end
        self.isLoading = false
    end
end

function mainmenu:OnSignOutComplete(username)
    self:RunOnMainThread(function()
        Debug.Log("Signed out.");
        self.menuManager:ShowMenu("MainMenu")
    end)
end

return mainmenu
