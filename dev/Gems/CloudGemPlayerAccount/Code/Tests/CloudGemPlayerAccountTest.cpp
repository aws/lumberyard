#include "CloudGemPlayerAccount_precompiled.h"

#include <mutex>
#include <sstream>

#include <AzTest/AzTest.h>
#include <gmock/gmock.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <ISystem.h>

#include <CloudCanvas/CloudCanvasMappingsBus.h>

#include "CloudGemPlayerAccountHelper.h"

// Collects debugging information and adds it to a scoped trace that will get printed on test failure.
#define USE_SCOPED_TRACE                                                            \
    std::stringstream traceMessage;                                                 \
    traceMessage << "\nTest Username: " << username.c_str();                        \
    auto traceCurrentUser = helper.GetCurrentUser();                                \
    if (traceCurrentUser.wasSuccessful)                                             \
    {                                                                               \
        traceMessage << "\nSigned in as: " << traceCurrentUser.username.c_str();    \
    }                                                                               \
    else                                                                            \
    {                                                                               \
        traceMessage << "\nNot signed in.";                                         \
    }                                                                               \
    AZStd::string traceServiceApi;                                                  \
    EBUS_EVENT_RESULT(traceServiceApi, CloudGemFramework::CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, "CloudGemPlayerAccount.ServiceApi"); \
    traceMessage << "\nService API: " << traceServiceApi.c_str();                   \
    AZStd::string traceUserPool;                                                    \
    EBUS_EVENT_RESULT(traceUserPool, CloudGemFramework::CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, "CloudGemPlayerAccount.PlayerUserPool"); \
    traceMessage << "\nUser Pool: " << traceUserPool.c_str();                         \
    SCOPED_TRACE(traceMessage.str());

using namespace ::testing;

class CloudGemPlayerAccountTest
    : public ::testing::Test
{
protected:
    void SetUp() override
    {

    }

    void TearDown() override
    {

    }
};

TEST_F(CloudGemPlayerAccountTest, ExampleTest)
{
    ASSERT_TRUE(true);
}

class UnitTestEnvironment : public AZ::Test::ITestEnvironment
{

public:

    void SetupEnvironment() override
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
    }

    void TeardownEnvironment() override
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }

};

AZ_UNIT_TEST_HOOK(new UnitTestEnvironment());

class AwsIntegrationTestEnvionment : public AZ::Test::ITestEnvironment
{
public:
    void SetupEnvironment() override
    {
        {
            puts("");
            puts("---------------------------------------------------------------------------------");
            puts("-- ");
            puts("-- Use Gems\\CloudGemPlayerAccount\\AWS\\test\\runtests.cmd to execute these tests.");
            puts("--");
            puts("-- These tests depend on the following conditions to run successfully:");
            puts("-- ");
            puts("-- 1) sys_game_folder=CloudGemSamples must be specified in the bootstrap.cfg.");
            puts("-- ");
            puts("-- 2) Cloud Canvas project, deployment, and resource group stacks must be setup");
            puts("--    as done by Gems\\CloudGemPlayerAccount\\AWS\\test\\integration_test.py before it");
            puts("--    executes the test.");
            puts("-- ");
            puts("-- 3) There must be a \"default\" AWS credentials profile with cognito-idp:AdminConfirmSignUp");
            puts("      permissions.");
            puts("-- ");
            puts("---------------------------------------------------------------------------------");
            puts("");
        }
    }

    void TeardownEnvironment() override
    {
    }
};

AZ_INTEG_TEST_HOOK(new AwsIntegrationTestEnvionment());

const char* TEST_EMAIL = "test@example.com";
const char* TEST_PASSWORD = "Password_1";
const char* TEST_PASSWORD2 = "Password_2";

const char* TEST_PLAYER_NAME = "Test Player Name";

const char* TEST_ADDRESS = "test_address2";
const char* TEST_BIRTHDATE = "birthdate2";
const char* TEST_EMAIL2 = "test2@example.com";
const char* TEST_FAMILY_NAME = "test_family_name";
const char* TEST_GENDER = "test_gender";
const char* TEST_GIVEN_NAME = "test_given_name";
const char* TEST_LOCALE = "test_locale";
const char* TEST_MIDDLE_NAME = "test_middle_name";
const char* TEST_NICKNAME = "test_nickname";
const char* TEST_PHONE_NUMBER = "+12345678901";
const char* TEST_PICTURE = "test_picture";
const char* TEST_WEBSITE = "test_website";
const char* TEST_ZONEINFO = "test_zoneinfo";

class Integ_CloudGemPlayerAccountTest
    : public Test
{
protected:
    void SetUp() override
    {
        if (username.empty())
        {
            std::stringstream usernameBuilder;
            usernameBuilder << "test" << rand() % 10000;
            username = usernameBuilder.str().c_str();
        }
    }

    void TearDown() override
    {
    }

    static AZStd::string username;
    CloudGemPlayerAccountHelper helper;
};

AZStd::string Integ_CloudGemPlayerAccountTest::username;

TEST_F(Integ_CloudGemPlayerAccountTest, SignedOutOnStart)
{
    USE_SCOPED_TRACE;
    
    auto result = helper.GetCurrentUser();
    if (result.wasSuccessful)
    {
        helper.SignOut(result.username);
        result = helper.GetCurrentUser();
    }

    std::stringstream message;
    message << "Response username: " << result.username.c_str();
    SCOPED_TRACE(message.str());

    ASSERT_FALSE(result.wasSuccessful);
}

TEST_F(Integ_CloudGemPlayerAccountTest, HasCachedCredentialsWhenSignedOut)
{
    bool result;
    EBUS_EVENT_RESULT(result, CloudGemPlayerAccountRequestBus, HasCachedCredentials, username);
    ASSERT_FALSE(result);
}

TEST_F(Integ_CloudGemPlayerAccountTest, SignUp)
{
    USE_SCOPED_TRACE;

    UserAttributeValues attributes;
    attributes.SetAttribute("email", TEST_EMAIL);
    auto result = helper.SignUp(username, TEST_PASSWORD, attributes);
    ASSERT_TRUE(result.wasSuccessful);

    helper.AdminConfirmSignUp(username);
}

TEST_F(Integ_CloudGemPlayerAccountTest, InitiateAuth)
{
    USE_SCOPED_TRACE;

    auto result = helper.InitiateAuth(username, TEST_PASSWORD);
    ASSERT_TRUE(result.wasSuccessful);
}

TEST_F(Integ_CloudGemPlayerAccountTest, HasCachedCredentialsWhenSignedIn)
{
    bool result;
    EBUS_EVENT_RESULT(result, CloudGemPlayerAccountRequestBus, HasCachedCredentials, username);
    ASSERT_TRUE(result);
}

TEST_F(Integ_CloudGemPlayerAccountTest, GetUserWithMinimumAttributes)
{
    USE_SCOPED_TRACE;

    auto result = helper.GetUser(username);
    ASSERT_TRUE(result.basicResultInfo.wasSuccessful);

    ASSERT_TRUE(result.attributes.HasAttribute("email"));
    ASSERT_EQ(result.attributes.GetAttribute("email"), AZStd::string{ TEST_EMAIL });

    ASSERT_FALSE(result.attributes.HasAttribute("address"));
    ASSERT_FALSE(result.attributes.HasAttribute("birthdate"));
    ASSERT_FALSE(result.attributes.HasAttribute("family_name"));
    ASSERT_FALSE(result.attributes.HasAttribute("gender"));
    ASSERT_FALSE(result.attributes.HasAttribute("locale"));
    ASSERT_FALSE(result.attributes.HasAttribute("middle_name"));
    ASSERT_FALSE(result.attributes.HasAttribute("nickname"));
    ASSERT_FALSE(result.attributes.HasAttribute("phone_number"));
    ASSERT_FALSE(result.attributes.HasAttribute("picture"));
    ASSERT_FALSE(result.attributes.HasAttribute("website"));
    ASSERT_FALSE(result.attributes.HasAttribute("zoneinfo"));
}

TEST_F(Integ_CloudGemPlayerAccountTest, UpdateUserAttributes)
{
    USE_SCOPED_TRACE;

    UserAttributeValues attributes;
    attributes.SetAttribute("email", TEST_EMAIL2);
    attributes.SetAttribute("address", TEST_ADDRESS);
    attributes.SetAttribute("birthdate", TEST_BIRTHDATE);
    attributes.SetAttribute("family_name", TEST_FAMILY_NAME);
    attributes.SetAttribute("gender", TEST_GENDER);
    attributes.SetAttribute("locale", TEST_LOCALE);
    attributes.SetAttribute("middle_name", TEST_MIDDLE_NAME);
    attributes.SetAttribute("nickname", TEST_NICKNAME);
    attributes.SetAttribute("phone_number", TEST_PHONE_NUMBER);
    attributes.SetAttribute("picture", TEST_PICTURE);
    attributes.SetAttribute("website", TEST_WEBSITE);
    attributes.SetAttribute("zoneinfo", TEST_ZONEINFO);

    auto updateResult = helper.UpdateUserAttributes(username, attributes);
    ASSERT_TRUE(updateResult.wasSuccessful);

    auto result = helper.GetUser(username);
    ASSERT_TRUE(result.basicResultInfo.wasSuccessful);

    ASSERT_TRUE(result.attributes.HasAttribute("address"));
    ASSERT_EQ(result.attributes.GetAttribute("address"), AZStd::string{ TEST_ADDRESS });
    ASSERT_TRUE(result.attributes.HasAttribute("birthdate"));
    ASSERT_EQ(result.attributes.GetAttribute("birthdate"), AZStd::string{ TEST_BIRTHDATE });
    ASSERT_TRUE(result.attributes.HasAttribute("email"));
    ASSERT_EQ(result.attributes.GetAttribute("email"), AZStd::string{ TEST_EMAIL2 });
    ASSERT_TRUE(result.attributes.HasAttribute("family_name"));
    ASSERT_EQ(result.attributes.GetAttribute("family_name"), AZStd::string{ TEST_FAMILY_NAME });
    ASSERT_TRUE(result.attributes.HasAttribute("gender"));
    ASSERT_EQ(result.attributes.GetAttribute("gender"), AZStd::string{ TEST_GENDER });
    ASSERT_TRUE(result.attributes.HasAttribute("locale"));
    ASSERT_EQ(result.attributes.GetAttribute("locale"), AZStd::string{ TEST_LOCALE });
    ASSERT_TRUE(result.attributes.HasAttribute("middle_name"));
    ASSERT_EQ(result.attributes.GetAttribute("middle_name"), AZStd::string{ TEST_MIDDLE_NAME });
    ASSERT_TRUE(result.attributes.HasAttribute("nickname"));
    ASSERT_EQ(result.attributes.GetAttribute("nickname"), AZStd::string{ TEST_NICKNAME });
    ASSERT_TRUE(result.attributes.HasAttribute("phone_number"));
    ASSERT_EQ(result.attributes.GetAttribute("phone_number"), AZStd::string{ TEST_PHONE_NUMBER });
    ASSERT_TRUE(result.attributes.HasAttribute("picture"));
    ASSERT_EQ(result.attributes.GetAttribute("picture"), AZStd::string{ TEST_PICTURE });
    ASSERT_TRUE(result.attributes.HasAttribute("website"));
    ASSERT_EQ(result.attributes.GetAttribute("website"), AZStd::string{ TEST_WEBSITE });
    ASSERT_TRUE(result.attributes.HasAttribute("zoneinfo"));
    ASSERT_EQ(result.attributes.GetAttribute("zoneinfo"), AZStd::string{ TEST_ZONEINFO });
}

TEST_F(Integ_CloudGemPlayerAccountTest, DeleteUserAttributes)
{
    USE_SCOPED_TRACE;

    UserAttributeList attributesToDelete;
    attributesToDelete.AddAttribute("address");
    attributesToDelete.AddAttribute("birthdate");
    attributesToDelete.AddAttribute("family_name");
    attributesToDelete.AddAttribute("gender");
    attributesToDelete.AddAttribute("locale");
    attributesToDelete.AddAttribute("middle_name");
    attributesToDelete.AddAttribute("nickname");
    attributesToDelete.AddAttribute("phone_number");
    attributesToDelete.AddAttribute("picture");
    attributesToDelete.AddAttribute("website");
    attributesToDelete.AddAttribute("zoneinfo");

    auto deleteResult = helper.DeleteUserAttributes(username, attributesToDelete);
    ASSERT_TRUE(deleteResult.wasSuccessful);

    auto result = helper.GetUser(username);
    ASSERT_TRUE(result.basicResultInfo.wasSuccessful);

    ASSERT_TRUE(result.attributes.HasAttribute("email"));
    ASSERT_EQ(result.attributes.GetAttribute("email"), AZStd::string{ TEST_EMAIL2 });

    ASSERT_FALSE(result.attributes.HasAttribute("address"));
    ASSERT_FALSE(result.attributes.HasAttribute("birthdate"));
    ASSERT_FALSE(result.attributes.HasAttribute("family_name"));
    ASSERT_FALSE(result.attributes.HasAttribute("gender"));
    ASSERT_FALSE(result.attributes.HasAttribute("locale"));
    ASSERT_FALSE(result.attributes.HasAttribute("middle_name"));
    ASSERT_FALSE(result.attributes.HasAttribute("nickname"));
    ASSERT_FALSE(result.attributes.HasAttribute("phone_number"));
    ASSERT_FALSE(result.attributes.HasAttribute("picture"));
    ASSERT_FALSE(result.attributes.HasAttribute("website"));
    ASSERT_FALSE(result.attributes.HasAttribute("zoneinfo"));
}

TEST_F(Integ_CloudGemPlayerAccountTest, GetPlayerAccountWithNoPlayerName)
{
    USE_SCOPED_TRACE;

    auto result = helper.GetPlayerAccount();
    ASSERT_TRUE(result.accountResultInfo.wasSuccessful);
    ASSERT_EQ(result.playerAccount.GetPlayerName(), AZStd::string());
}

TEST_F(Integ_CloudGemPlayerAccountTest, UpdatePlayerAccount)
{
    USE_SCOPED_TRACE;

    PlayerAccount playerAccount;
    playerAccount.SetPlayerName(TEST_PLAYER_NAME);
    auto updateResult = helper.UpdatePlayerAccount(playerAccount);
    ASSERT_TRUE(updateResult.wasSuccessful);

    auto result = helper.GetPlayerAccount();
    ASSERT_TRUE(result.accountResultInfo.wasSuccessful);
    ASSERT_EQ(result.playerAccount.GetPlayerName(), AZStd::string{ TEST_PLAYER_NAME });
}

TEST_F(Integ_CloudGemPlayerAccountTest, ChangePassword)
{
    USE_SCOPED_TRACE;

    auto result = helper.ChangePassword(username, TEST_PASSWORD, TEST_PASSWORD2);
    ASSERT_TRUE(result.wasSuccessful);
}

TEST_F(Integ_CloudGemPlayerAccountTest, SignOut)
{
    USE_SCOPED_TRACE;

    ASSERT_TRUE(helper.GetCurrentUser().wasSuccessful);
    helper.SignOut(username);
    ASSERT_FALSE(helper.GetCurrentUser().wasSuccessful);
}

TEST_F(Integ_CloudGemPlayerAccountTest, InitiateAuthAfterPasswordChange)
{
    USE_SCOPED_TRACE;

    auto result = helper.InitiateAuth(username, TEST_PASSWORD2);
    ASSERT_TRUE(result.wasSuccessful);
}

TEST_F(Integ_CloudGemPlayerAccountTest, GlobalSignOut)
{
    USE_SCOPED_TRACE;

    ASSERT_TRUE(helper.GetCurrentUser().wasSuccessful);
    ASSERT_TRUE(helper.GlobalSignOut(username).wasSuccessful);
    ASSERT_FALSE(helper.GetCurrentUser().wasSuccessful);

    auto result = helper.InitiateAuth(username, TEST_PASSWORD2);
    ASSERT_TRUE(result.wasSuccessful);
}

TEST_F(Integ_CloudGemPlayerAccountTest, DeleteOwnAccount)
{
    USE_SCOPED_TRACE;

    ASSERT_TRUE(helper.GetCurrentUser().wasSuccessful);
    ASSERT_TRUE(helper.DeleteOwnAccount(username).wasSuccessful);
    ASSERT_FALSE(helper.GetCurrentUser().wasSuccessful);

    auto result = helper.InitiateAuth(username, TEST_PASSWORD2);
    ASSERT_FALSE(result.wasSuccessful);
}
