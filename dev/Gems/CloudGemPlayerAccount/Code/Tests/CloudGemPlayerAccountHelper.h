/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/Component/TickBus.h>
#include <CloudGemFramework/AwsApiRequestJob.h>

#pragma warning(disable: 4355) // <future> includes ppltasks.h which throws a C4355 warning: 'this' used in base member initializer list
#include <aws/cognito-idp/CognitoIdentityProviderClient.h>
#include <aws/cognito-idp/model/AdminConfirmSignUpRequest.h>
#include <aws/cognito-idp/model/AdminConfirmSignUpResult.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#pragma warning(default: 4355)

#include <CloudGemPlayerAccount/CloudGemPlayerAccountBus.h>
#include <CloudCanvas/CloudCanvasMappingsBus.h>

using namespace CloudGemPlayerAccount;

// Event names from the CloudGemPlayerAccountRequests EBus.
enum class CloudGemPlayerAccountEventType
{
    ChangePassword,
    DeleteOwnAccount,
    DeleteUserAttributes,
    GetCurrentUser,
    GetPlayerAccount,
    GetUser,
    GlobalSignOut,
    InitiateAuth,
    SignOut,
    SignUp,
    UpdatePlayerAccount,
    UpdateUserAttributes,
};

// Used to capture an event that was broadcast to the CloudGemPlayerAccountNotifications EBus.
class CloudGemPlayerAccountEvent
{
public:
    CloudGemPlayerAccountEvent() {}

    CloudGemPlayerAccountEvent(CloudGemPlayerAccountEventType type, const BasicResultInfo& basicResultInfo)
        : type(type), basicResultInfo(basicResultInfo) {}

    CloudGemPlayerAccountEvent(CloudGemPlayerAccountEventType type, const AccountResultInfo& accountResultInfo)
        : type(type), accountResultInfo(accountResultInfo) {}

    CloudGemPlayerAccountEvent(CloudGemPlayerAccountEventType type, const BasicResultInfo& basicResultInfo, const UserAttributeValues& attributes, const UserAttributeList& mfaOptions)
        : type(type), basicResultInfo(basicResultInfo), attributes(attributes), mfaOptions(mfaOptions) {}

    CloudGemPlayerAccountEvent(CloudGemPlayerAccountEventType type, const AccountResultInfo& accountResultInfo, const PlayerAccount& playerAccount)
        : type(type), accountResultInfo(accountResultInfo), playerAccount(playerAccount) {}

    CloudGemPlayerAccountEventType type;
    AccountResultInfo accountResultInfo;
    BasicResultInfo basicResultInfo;
    PlayerAccount playerAccount;
    UserAttributeValues attributes;
    UserAttributeList mfaOptions;
};

class GetAccountResponse
{
public:
    GetAccountResponse() {}

    GetAccountResponse(CloudGemPlayerAccountEvent playerAccountEvent)
        : accountResultInfo(playerAccountEvent.accountResultInfo), playerAccount(playerAccountEvent.playerAccount)
    {}

    AccountResultInfo accountResultInfo;
    PlayerAccount playerAccount;
};

class GetUserResponse
{
public:
    GetUserResponse() {}

    GetUserResponse(CloudGemPlayerAccountEvent playerAccountEvent)
        : basicResultInfo(playerAccountEvent.basicResultInfo), attributes(playerAccountEvent.attributes), mfaOptions(playerAccountEvent.mfaOptions)
    {}

    BasicResultInfo basicResultInfo;
    UserAttributeValues attributes;
    UserAttributeList mfaOptions;
};

// Wrapper for CloudGemPlayerAccount to simplify calls from tests.
// Converts asynchronous calls into synchronous calls.
// Events queued for the TickBus are executed while waiting since responses are queued there for running on the main thread.
class CloudGemPlayerAccountHelper
    : protected CloudGemPlayerAccountNotificationBus::Handler
{
public:
    CloudGemPlayerAccountHelper()
    {
        CloudGemPlayerAccountNotificationBus::Handler::BusConnect();
    }

    ~CloudGemPlayerAccountHelper()
    {
        CloudGemPlayerAccountNotificationBus::Handler::BusDisconnect();
    }

    BasicResultInfo ChangePassword(const AZStd::string& username, const AZStd::string& previousPassword, const AZStd::string& proposedPassword)
    {
        EBUS_EVENT(CloudGemPlayerAccountRequestBus, ChangePassword, username, previousPassword, proposedPassword);
        CloudGemPlayerAccountEvent event;
        if (WaitForEvent(CloudGemPlayerAccountEventType::ChangePassword, event))
        {
            return event.basicResultInfo;
        }
        return BasicResultInfo();
    }

    BasicResultInfo DeleteOwnAccount(const AZStd::string& username)
    {
        EBUS_EVENT(CloudGemPlayerAccountRequestBus, DeleteOwnAccount, username);
        CloudGemPlayerAccountEvent event;
        if (WaitForEvent(CloudGemPlayerAccountEventType::DeleteOwnAccount, event))
        {
            return event.basicResultInfo;
        }
        return BasicResultInfo();
    }

    BasicResultInfo DeleteUserAttributes(const AZStd::string& username, const UserAttributeList& attributesToDelete)
    {
        EBUS_EVENT(CloudGemPlayerAccountRequestBus, DeleteUserAttributes, username, attributesToDelete);
        CloudGemPlayerAccountEvent event;
        if (WaitForEvent(CloudGemPlayerAccountEventType::DeleteUserAttributes, event))
        {
            return event.basicResultInfo;
        }
        return BasicResultInfo();
    }

    BasicResultInfo GetCurrentUser()
    {
        EBUS_EVENT(CloudGemPlayerAccountRequestBus, GetCurrentUser);
        CloudGemPlayerAccountEvent event;
        if (WaitForEvent(CloudGemPlayerAccountEventType::GetCurrentUser, event))
        {
            return event.basicResultInfo;
        }
        return BasicResultInfo();
    }

    GetAccountResponse GetPlayerAccount()
    {
        EBUS_EVENT(CloudGemPlayerAccountRequestBus, GetPlayerAccount);
        CloudGemPlayerAccountEvent event;
        if (WaitForEvent(CloudGemPlayerAccountEventType::GetPlayerAccount, event))
        {
            return GetAccountResponse(event);
        }
        return GetAccountResponse();
    }

    GetUserResponse GetUser(const AZStd::string& username)
    {
        EBUS_EVENT(CloudGemPlayerAccountRequestBus, GetUser, username);
        CloudGemPlayerAccountEvent event;
        if (WaitForEvent(CloudGemPlayerAccountEventType::GetUser, event))
        {
            return GetUserResponse(event);
        }
        return GetUserResponse();
    }

    BasicResultInfo GlobalSignOut(const AZStd::string& username)
    {
        EBUS_EVENT(CloudGemPlayerAccountRequestBus, GlobalSignOut, username);
        CloudGemPlayerAccountEvent event;
        if (WaitForEvent(CloudGemPlayerAccountEventType::GlobalSignOut, event))
        {
            return event.basicResultInfo;
        }
        return BasicResultInfo();
    }

    BasicResultInfo InitiateAuth(const AZStd::string& username, const AZStd::string& password)
    {
        EBUS_EVENT(CloudGemPlayerAccountRequestBus, InitiateAuth, username, password);
        CloudGemPlayerAccountEvent event;
        if (WaitForEvent(CloudGemPlayerAccountEventType::InitiateAuth, event))
        {
            return event.basicResultInfo;
        }
        return BasicResultInfo();
    }

    void SignOut(const AZStd::string& username)
    {
        EBUS_EVENT(CloudGemPlayerAccountRequestBus, SignOut, username);
        CloudGemPlayerAccountEvent event;
        WaitForEvent(CloudGemPlayerAccountEventType::SignOut, event);
    }

    BasicResultInfo SignUp(const AZStd::string& username, const AZStd::string& password, const UserAttributeValues& attributes)
    {
        EBUS_EVENT(CloudGemPlayerAccountRequestBus, SignUp, username, password, attributes);
        CloudGemPlayerAccountEvent event;
        if (WaitForEvent(CloudGemPlayerAccountEventType::SignUp, event))
        {
            return event.basicResultInfo;
        }
        return BasicResultInfo();
    }

    AccountResultInfo UpdatePlayerAccount(const PlayerAccount& playerAccount)
    {
        EBUS_EVENT(CloudGemPlayerAccountRequestBus, UpdatePlayerAccount, playerAccount);
        CloudGemPlayerAccountEvent event;
        if (WaitForEvent(CloudGemPlayerAccountEventType::UpdatePlayerAccount, event))
        {
            return event.accountResultInfo;
        }
        return AccountResultInfo();
    }

    BasicResultInfo UpdateUserAttributes(const AZStd::string& username, const UserAttributeValues& attributes)
    {
        EBUS_EVENT(CloudGemPlayerAccountRequestBus, UpdateUserAttributes, username, attributes);
        CloudGemPlayerAccountEvent event;
        if (WaitForEvent(CloudGemPlayerAccountEventType::UpdateUserAttributes, event))
        {
            return event.basicResultInfo;
        }
        return BasicResultInfo();
    }

    void AdminConfirmSignUp(const AZStd::string& username)
    {
        AZStd::string userPool;
        EBUS_EVENT_RESULT(userPool, CloudGemFramework::CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, "CloudGemPlayerAccount.PlayerUserPool");

        using ConfirmSignUpRequestJob = AWS_API_REQUEST_JOB(CognitoIdentityProvider, AdminConfirmSignUp);

        ConfirmSignUpRequestJob::Config config{ ConfirmSignUpRequestJob::GetDefaultConfig() };
        config.credentialsProvider = std::make_shared<Aws::Auth::ProfileConfigFileAWSCredentialsProvider>("default");

        auto job = ConfirmSignUpRequestJob::Create(
            [](ConfirmSignUpRequestJob* job) {},
            [](ConfirmSignUpRequestJob* job) {},
            &config
        );

        job->request.SetUsername(username.c_str());
        job->request.SetUserPoolId(userPool.c_str());
        job->StartAndAssistUntilComplete();
    }

protected:
    virtual void OnChangePasswordComplete(const BasicResultInfo& resultInfo) override
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_events.push_back({ CloudGemPlayerAccountEventType::ChangePassword, resultInfo });
    }

    virtual void OnDeleteOwnAccountComplete(const BasicResultInfo& resultInfo) override
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_events.push_back({ CloudGemPlayerAccountEventType::DeleteOwnAccount, resultInfo });
    }

    virtual void OnDeleteUserAttributesComplete(const BasicResultInfo& resultInfo) override
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_events.push_back({ CloudGemPlayerAccountEventType::DeleteUserAttributes, resultInfo });
    }

    virtual void OnGetCurrentUserComplete(const BasicResultInfo& resultInfo) override
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_events.push_back({ CloudGemPlayerAccountEventType::GetCurrentUser, resultInfo });
    }

    virtual void OnGetPlayerAccountComplete(const AccountResultInfo& resultInfo, const PlayerAccount& playerAccount)
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_events.push_back({ CloudGemPlayerAccountEventType::GetPlayerAccount, resultInfo, playerAccount });
    }

    virtual void OnInitiateAuthComplete(const BasicResultInfo& resultInfo) override
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_events.push_back({ CloudGemPlayerAccountEventType::InitiateAuth, resultInfo });
    }

    virtual void OnGetUserComplete(const BasicResultInfo& resultInfo, const UserAttributeValues& attributes, const UserAttributeList& mfaOptions) override
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_events.push_back({ CloudGemPlayerAccountEventType::GetUser, resultInfo, attributes, mfaOptions });
    }

    virtual void OnGlobalSignOutComplete(const BasicResultInfo& resultInfo) override
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_events.push_back({ CloudGemPlayerAccountEventType::GlobalSignOut, resultInfo });
    }

    virtual void OnSignOutComplete(const BasicResultInfo& resultInfo) override
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_events.push_back({ CloudGemPlayerAccountEventType::SignOut, resultInfo });
    }

    virtual void OnSignUpComplete(const BasicResultInfo& resultInfo, const DeliveryDetails& deliveryDetails, bool wasConfirmed) override
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_events.push_back({ CloudGemPlayerAccountEventType::SignUp, resultInfo });
    }

    virtual void OnUpdatePlayerAccountComplete(const AccountResultInfo& resultInfo) override
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_events.push_back({ CloudGemPlayerAccountEventType::UpdatePlayerAccount, resultInfo });
    }

    virtual void OnUpdateUserAttributesComplete(const BasicResultInfo& resultInfo, const DeliveryDetailsArray& details) override
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_events.push_back({ CloudGemPlayerAccountEventType::UpdateUserAttributes, resultInfo });
    }

private:
    bool WaitForEvent(CloudGemPlayerAccountEventType type, CloudGemPlayerAccountEvent& event)
    {
        for (int x = 0; x < 100; x++)
        {
            AZ::TickBus::ExecuteQueuedEvents();
            {
                std::lock_guard<std::mutex> lock(m_eventMutex);
                for (auto it = m_events.begin(); it != m_events.end(); it++)
                {
                    if (it->type == type)
                    {
                        event = *it;
                        m_events.erase(it);
                        return true;
                    }
                }
            }
            Sleep(100);
        }

        printf("Timed out waiting for event %d\n", type);
        return false;
    }

    std::list<CloudGemPlayerAccountEvent> m_events;
    std::mutex m_eventMutex;

    std::mutex m_waitMutex;
    std::condition_variable m_waitCV;
};
