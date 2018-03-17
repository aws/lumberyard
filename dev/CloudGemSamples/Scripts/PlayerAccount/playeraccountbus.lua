local asyncresult = require "Scripts/asyncresult"

local playeraccountbus = {}

function playeraccountbus:new(instance)
    instance = instance or {}
    instance.handlersByRequestId = {}
    self.__index = self
    setmetatable(instance, self)
    instance:Init()
    return instance
end

function playeraccountbus:Init()
    self.tasks = {}
    self.cloudGemPlayerAccountNotificationHandler = CloudGemPlayerAccountNotificationBus.Connect(self, self.entityId)
end

function playeraccountbus:OnDeactivate()
    if self.cloudGemPlayerAccountNotificationHandler then
        self.cloudGemPlayerAccountNotificationHandler:Disconnect()
        self.cloudGemPlayerAccountNotificationHandler = nil
    end
end

--- API wrappers
function playeraccountbus:GetServiceStatus()
    local requestId = CloudGemPlayerAccountRequestBus.Broadcast.GetServiceStatus()
    return self:CreateAsyncResult(requestId)
end
function playeraccountbus:OnGetServiceStatusComplete(result)
    self:HandleEvent(result)
end


function playeraccountbus:GetCurrentUser()
    local requestId = CloudGemPlayerAccountRequestBus.Broadcast.GetCurrentUser()
    return self:CreateAsyncResult(requestId)
end
function playeraccountbus:OnGetCurrentUserComplete(result)
    self:HandleEvent(result)
end

function playeraccountbus:SignUp(username, password, attributes)
    local requestId = CloudGemPlayerAccountRequestBus.Broadcast.SignUp(username, password, attributes)
    return self:CreateAsyncResult(requestId)
end
function playeraccountbus:OnSignUpComplete(result)
    self:HandleEvent(result)
end

function playeraccountbus:ConfirmSignUp(username, code)
    local requestId = CloudGemPlayerAccountRequestBus.Broadcast.ConfirmSignUp(username, code)
    return self:CreateAsyncResult(requestId)
end
function playeraccountbus:OnConfirmSignUpComplete(result)
    self:HandleEvent(result)
end

function playeraccountbus:ForgotPassword(username)
    local requestId = CloudGemPlayerAccountRequestBus.Broadcast.ForgotPassword(username)
    return self:CreateAsyncResult(requestId)
end
function playeraccountbus:OnForgotPasswordComplete(result)
    self:HandleEvent(result)
end

function playeraccountbus:ConfirmForgotPassword(username, password, code)
    local requestId = CloudGemPlayerAccountRequestBus.Broadcast.ConfirmForgotPassword(username, password, code)
    return self:CreateAsyncResult(requestId)
end
function playeraccountbus:OnConfirmForgotPasswordComplete(result)
    self:HandleEvent(result)
end

function playeraccountbus:InitiateAuth(username, password)
    local requestId = CloudGemPlayerAccountRequestBus.Broadcast.InitiateAuth(username, password)
    return self:CreateAsyncResult(requestId)
end
function playeraccountbus:OnInitiateAuthComplete(result)
    self:HandleEvent(result)
end

function playeraccountbus:RespondToForceChangePasswordChallenge(username, oldPassword, newPassword)
    local requestId = CloudGemPlayerAccountRequestBus.Broadcast.RespondToForceChangePasswordChallenge(username, oldPassword, newPassword)
    return self:CreateAsyncResult(requestId)
end
function playeraccountbus:OnRespondToForceChangePasswordChallengeComplete(result)
    self:HandleEvent(result)
end

function playeraccountbus:SignOut(username)
    local requestId = CloudGemPlayerAccountRequestBus.Broadcast.SignOut(username)
    return self:CreateAsyncResult(requestId)
end
function playeraccountbus:OnSignOutComplete(result)
    self:HandleEvent(result)
end

function playeraccountbus:ChangePassword(username, oldPassword, newPassword)
    local requestId = CloudGemPlayerAccountRequestBus.Broadcast.ChangePassword(username, oldPassword, newPassword)
    return self:CreateAsyncResult(requestId)
end
function playeraccountbus:OnChangePasswordComplete(result)
    self:HandleEvent(result)
end

function playeraccountbus:GlobalSignOut(username)
    local requestId = CloudGemPlayerAccountRequestBus.Broadcast.GlobalSignOut(username)
    return self:CreateAsyncResult(requestId)
end
function playeraccountbus:OnGlobalSignOutComplete(result, attributes, mfaOptions)
    self:HandleEvent(result, attributes, mfaOptions)
end

function playeraccountbus:GetUser(username)
    local requestId = CloudGemPlayerAccountRequestBus.Broadcast.GetUser(username)
    return self:CreateAsyncResult(requestId)
end
function playeraccountbus:OnGetUserComplete(result, attributes, mfaOptions)
    self:HandleEvent(result, attributes, mfaOptions)
end

function playeraccountbus:UpdateUserAttributes(username, attributes)
    local requestId = CloudGemPlayerAccountRequestBus.Broadcast.UpdateUserAttributes(username, attributes)
    return self:CreateAsyncResult(requestId)
end
function playeraccountbus:OnUpdateUserAttributesComplete(result)
    self:HandleEvent(result)
end

function playeraccountbus:GetPlayerAccount()
    local requestId = CloudGemPlayerAccountRequestBus.Broadcast.GetPlayerAccount()
    return self:CreateAsyncResult(requestId)
end
function playeraccountbus:OnGetPlayerAccountComplete(result, account)
    self:HandleEvent(result, account)
end

function playeraccountbus:UpdatePlayerAccount(playerAccount)
    local requestId = CloudGemPlayerAccountRequestBus.Broadcast.UpdatePlayerAccount(playerAccount)
    return self:CreateAsyncResult(requestId)
end
function playeraccountbus:OnUpdatePlayerAccountComplete(result)
    self:HandleEvent(result)
end

--- Helpers

function playeraccountbus:CreateAsyncResult(requestId)
    local result = asyncresult:new{
        threadHandler = self.threadHandler
    }
    self.handlersByRequestId[requestId] = result
    return result
end

function playeraccountbus:HandleEvent(...)
    local requestId = arg[1].requestId
    local result = self.handlersByRequestId[requestId]
    if result then
        self.handlersByRequestId[requestId] = nil
        result:Complete(unpack(arg))
    end
end

return playeraccountbus
