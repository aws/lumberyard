local playeraccountbus = require "Scripts/PlayerAccount/playeraccountbus"

local lbaccountutils = {
}

function lbaccountutils:new(handler)
    self.playerAccountBus = playeraccountbus:new{
        threadHandler = handler
    }
    local instance = {}
    self.__index = self
    setmetatable(instance, self)
    return instance
end

function lbaccountutils:PlayerAccountsEnabled(callback_func)
    local serviceStatusAsyncResult = self.playerAccountBus:GetServiceStatus():OnComplete(callback_func)
    return serviceStatusAsyncResult
end

function lbaccountutils:GetUsername(callback_func)
    local getCurrentUserAsyncResult = self.playerAccountBus:GetCurrentUser()
    local getPlayerAccountAsyncResult = self.playerAccountBus:GetPlayerAccount()
    local result = getCurrentUserAsyncResult:Chain(getPlayerAccountAsyncResult)
    result:OnComplete(callback_func);
    return result
end


return lbaccountutils