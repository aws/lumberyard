local changepasswordmenu = require "Scripts/PlayerAccount/changepasswordmenu"
local confirmsignupmenu = require "Scripts/PlayerAccount/confirmsignupmenu"
local createaccountmenu = require "Scripts/PlayerAccount/createaccountmenu"
local editaccountmenu = require "Scripts/PlayerAccount/editaccountmenu"
local editplayernamemenu = require "Scripts/PlayerAccount/editplayernamemenu"
local forcechangepasswordmenu = require "Scripts/PlayerAccount/forcechangepasswordmenu"
local forgotpasswordmenu = require "Scripts/PlayerAccount/forgotpasswordmenu"
local mainmenu = require "Scripts/PlayerAccount/mainmenu"
local manageaccountmenu = require "Scripts/PlayerAccount/manageaccountmenu"
local playeraccountbus = require "Scripts/PlayerAccount/playeraccountbus"
local signinmenu = require "Scripts/PlayerAccount/signinmenu"

menumanager =
{
    Properties = {},
    tasks = {},
}

function menumanager:OnActivate()
    self.menus =
    {
        ChangePassword = changepasswordmenu,
        ConfirmSignUp = confirmsignupmenu,
        CreateAccount = createaccountmenu,
        EditAccount = editaccountmenu,
        EditPlayerName = editplayernamemenu,
        ForceChangePassword = forcechangepasswordmenu,
        ForgotPassword = forgotpasswordmenu,
        MainMenu = mainmenu,
        ManageAccount = manageaccountmenu,
        SignIn = signinmenu
    }
    
    self.context = {}

    local util = require("scripts.util")
    util.SetMouseCursorVisible(true)

    self.playerAccountBus = playeraccountbus:new{
        threadHandler = self
    }

    self:ShowMenu("MainMenu")

    self.tickBusHandler = TickBus.Connect(self, self.entityId);
    self.playerIdentityNotificationHandler = PlayerIdentityNotificationBus.Connect(self)
end

function menumanager:OnDeactivate()
    if self.playerAccountBus then
        self.playerAccountBus:OnDeactivate()
        self.playerAccountBus = nil
    end
    if self.tickBusHandler then
        self.tickBusHandler:Disconnect();
        self.tickBusHandler = nil
    end
    if self.clientManagerNotificationHandler then
        self.playerIdentityNotificationHandler:Disconnect()
        self.playerIdentityNotificationHandler = nil
    end
    if self.menu then
        self.menu:Hide()
        self.menu = nil
    end
end

function menumanager:OnTick(deltaTime, timePoint)
    local task = nil
    repeat
        task = table.remove(self.tasks)
        if task then
            task()
        end
    until not task
end

function menumanager:OnAfterIdentityUpdate()
    if self.menu then
        self.menu:OnAfterConfigurationChange()
    end
end

function menumanager:RunOnMainThread(task)
    table.insert(self.tasks, task)
end

function menumanager:ShowMenu(menuName, context)
    if not self.menus[menuName] then
        Debug.Log("Menu '" .. menuName .. "' is not configured")
        return
    end

    if self.menu then
        self.menu:Hide()
    end
    self.menu = self.menus[menuName]:new{
        menuManager = self,
        entityId = self.entityId,
        context = self.context,
        playerAccountBus = self.playerAccountBus
    }
    self.menu:Show(context)
end

return menumanager
