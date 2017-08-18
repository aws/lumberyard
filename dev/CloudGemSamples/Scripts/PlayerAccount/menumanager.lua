require "Scripts/PlayerAccount/changepasswordmenu"
require "Scripts/PlayerAccount/confirmsignupmenu"
require "Scripts/PlayerAccount/createaccountmenu"
require "Scripts/PlayerAccount/editaccountmenu"
require "Scripts/PlayerAccount/editplayernamemenu"
require "Scripts/PlayerAccount/forcechangepasswordmenu"
require "Scripts/PlayerAccount/forgotpasswordmenu"
require "Scripts/PlayerAccount/mainmenu"
require "Scripts/PlayerAccount/manageaccountmenu"
require "Scripts/PlayerAccount/signinmenu"

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

    self:ShowMenu("MainMenu")

    self.tickBusHandler = TickBus.Connect(self, self.entityId);
    self.clientManagerNotificationHandler = ClientManagerNotificationBus.Connect(self)
end

function menumanager:OnDeactivate()
    if self.tickBusHandler then
        self.tickBusHandler:Disconnect();
        self.tickBusHandler = nil
    end
    if self.clientManagerNotificationHandler then
        self.clientManagerNotificationHandler:Disconnect()
        self.clientManagerNotificationHandler = nil
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

function menumanager:OnAfterConfigurationChange()
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
        context = self.context
    }
    self.menu:Show(context)
end

return menumanager
